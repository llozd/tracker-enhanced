// PRISM: an additive spectral synth where the spectrum itself is the instrument
// (openspec pocket-rack-engines). Ported from Pocket Rack's eng_prism.c (llozd).
// 24 sine partials a voice. Their frequencies and amplitudes are worked out once a block from
// the spectrum controls; the voice runs their phases, drift, freeze and decay.
// In Pocket Rack the spectrum and its slow scans (comb, sweep, smear) are shared by a part's
// voices; here each voice computes its own, so the scans of two voices are not in step.

#include "config.h"
#ifdef FEATURE_SYNTH

#include "PrismSynthVoice.h"
#include "EngineKit.h"
#include "PolySynth/sdk/SynthVoiceSDK.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace polysynth
{
namespace
{

// Stored in patches by position: append only. core/synth/presets/gen_presets.py mirrors this order.
enum class PrismParamId : int
{
    kStructure = 0,
    kTilt,
    kComb,
    kSweep,
    kSmear,
    kDrift,
    kWarp,
    kFreeze,
    kOddEven,
    kShift,
    kScramble,
    kGate,
    kSpectralDecay,
    kUnfold,
    kPhase,
    kFold,
    kAttack,
    kDecay,
    kSustain,
    kRelease,
    kTune,
    kFine,
    kPlayMode,
    kGlideTime,
    kVolume,
};

constexpr int kPartials = 24;

// log2(k + 1)
constexpr float kLog2[kPartials] = {
    0.0000000f, 1.0000000f, 1.5849625f, 2.0000000f, 2.3219281f, 2.5849625f, 2.8073549f, 3.0000000f,
    3.1699250f, 3.3219281f, 3.4594316f, 3.5849625f, 3.7004397f, 3.8073549f, 3.9068906f, 4.0000000f,
    4.0874628f, 4.1699250f, 4.2479275f, 4.3219281f, 4.3923174f, 4.4594316f, 4.5235620f, 4.5849625f
};
// WARP's fixed set: an irregular, glassy, gamelan-like series of ratios
constexpr float kAlien[kPartials] = { 1.000f,  1.482f,  2.093f,  2.756f,  3.351f,  4.127f,
                                      4.862f,  5.538f,  6.245f,  7.091f,  7.804f,  8.617f,
                                      9.372f,  10.218f, 11.036f, 11.847f, 12.699f, 13.512f,
                                      14.368f, 15.231f, 16.083f, 16.944f, 17.806f, 18.671f };
// SCRAMBLE's fixed shuffle (no partial on itself)
constexpr uint8_t kPerm[kPartials] = { 13, 4, 21, 8,  17, 0,  11, 22, 6, 15, 2,  19,
                                       9,  23, 5, 12, 20, 3, 16, 10, 1, 18, 7, 14 };

using namespace enginekit;

// Linear below 0.5, then a soft shoulder towards 1.
inline float softKnee(float x)
{
    const float a = std::fabs(x);
    if (a <= 0.5f)
        return x;
    const float y = 0.5f + 0.5f * softTanh((a - 0.5f) * 2.0f);
    return x < 0.0f ? -y : y;
}

class PrismSynthVoice : public sdk::CustomSynthVoice<PrismSynthVoice, PrismParamId>
{
  public:
    PrismSynthVoice()
    {
        for (int k = 0; k < kPartials; k++)
            smearPh[k] = static_cast<float>(k) / kPartials; // so the partials do not wander in step
    }

    bool isActive() final { return amp.isActive(); }

    void onNoteOn(const InstrumentVoiceContext& /*ctx*/,
                  note_t note,
                  float /*velocity*/,
                  bool isLegatoStep) final
    {
        fresh = amp.isActive() ? 1 : 2; // 2: from rest
        glide.noteOn(static_cast<float>(note), glideTimeSec,
                     playMode != PlayMode::kLegato || isLegatoStep);
        amp.set(attackSec, decaySec, sustainLevel, releaseSec);
        amp.gateOn(!isLegatoStep);
    }

    void onNoteOff(const InstrumentVoiceContext& /*ctx*/, note_t /*note*/) final { amp.gateOff(); }

    void renderVoiceAudio(const InstrumentVoiceContext& /*ctx*/,
                          [[maybe_unused]] std::optional<AudioBufferStereoRef> inputBuffer,
                          AudioBufferStereoRef outputBuffer,
                          std::span<AudioBufferRef> /*auxOutBuffer*/) final
    {
        auto out = outputBuffer[0];
        const uint32_t n = static_cast<uint32_t>(out.size());
        std::fill(out.begin(), out.end(), 0.0f);
        if (!amp.isActive())
            return;

        amp.set(attackSec, decaySec, sustainLevel, releaseSec);
        computeSpectrum(n);

        const float note = glide.advance(n) + tuneSemitones + fineCents * 0.01f + pitchBendSemitones;
        const float baseInc = 440.0f * sdk::fastPow2((note - 69.0f) / 12.0f) / kSampleRate;

        if (fresh)
        {
            for (int k = 0; k < kPartials; k++)
            {
                snap[k] = tab[k];
                life[k] = spectralDecay < 0.0f ? 0.0f : 1.0f; // a bloom begins with nothing
                if (fresh == 2)
                {
                    cur[k] = spectralDecay < 0.0f ? 0.0f : tab[k];
                    ph[k] = phaseSpread > 0.0f ? noise.next() * 0.5f * phaseSpread + 0.5f * phaseSpread : 0.0f;
                    drift[k] = 0.0f;
                }
            }
            age = 0;
            fresh = 0;
        }

        float unfoldAmount = 1.0f;
        if (unfold > 0.0f)
        {
            float u = static_cast<float>(age) / (unfold * 4.0f * kSampleRate);
            u = std::min(u, 1.0f);
            unfoldAmount = u * u * (3.0f - 2.0f * u);
            age += n;
        }

        for (int k = 0; k < kPartials; k++)
        {
            float ratio = ratios[k];
            if (unfoldAmount < 1.0f) // from a tight cluster towards the partial's place
            {
                const float start = 1.0f + 0.03f * static_cast<float>(k);
                ratio = start + (ratio - start) * unfoldAmount;
            }
            float inc = baseInc * ratio;
            if (driftAmount > 0.0f) // a leaky random walk, up to about 25 cents
            {
                drift[k] += noise.next() * 128.0f - drift[k] * (1.0f / 64.0f);
                drift[k] = std::clamp(drift[k], -2047.0f, 2047.0f);
                inc *= 1.0f + drift[k] * driftAmount * (127.0f / 256.0f / 65536.0f);
            }

            float tgt = tab[k] + (snap[k] - tab[k]) * freeze;
            if (spectralDecay > 0.0f)
            {
                life[k] *= decayFactor[k];
                tgt *= life[k];
            }
            else if (spectralDecay < 0.0f)
            {
                life[k] = 1.0f - (1.0f - life[k]) * decayFactor[k];
                tgt *= life[k];
            }
            if (inc >= 0.46875f) // at the top of the band: gone
                tgt = 0.0f;
            else if (inc > 0.375f) // fading out towards it
                tgt *= (0.46875f - inc) / 0.09375f;

            cur[k] += (tgt - cur[k]) * 0.25f;
            float p = ph[k];
            if (cur[k] < 1.0e-4f && tgt < 1.0e-4f) // silent: only the phase moves on
            {
                cur[k] = tgt;
                p += inc * static_cast<float>(n);
            }
            else
            {
                const float a = cur[k];
                for (uint32_t i = 0; i < n; i++)
                {
                    out[i] += sdk::fastSine(p) * a;
                    p += inc;
                    if (p >= 1.0f)
                        p -= 1.0f;
                }
            }
            ph[k] = p - std::floor(p);
        }

        const float level = volume;
        for (uint32_t i = 0; i < n; i++)
            out[i] = softKnee(out[i]) * amp.process() * level;
    }

    static void createMetadata(const Patch& /*patch*/, Patch::Metadata& metadata);

  private:
    // The spectrum for this block: ratios, amplitudes (normalised for equal power) and decay factors.
    void computeSpectrum(uint32_t n)
    {
        const float dt = static_cast<float>(n) / kSampleRate;
        const float sigma = (structure - 0.5039f) * 0.8f;
        const float tau = (1.0f - tilt) * 3.0f;
        const float combN = 1.0f + comb * 7.0f;
        const float combD = std::min(1.0f, comb * (127.0f / 31.0f));
        const float foldAt = 24.0f - 21.0f * fold;
        float a[kPartials], b[kPartials], amax = 0.0f, sum2 = 0.0f;

        sweepPh = wrap(sweepPh + 0.35f * dt);
        combPh = wrap(combPh + 0.2f * dt);
        const float centre = 11.5f + 11.5f * sdk::fastSine(sweepPh);

        for (int k = 0; k < kPartials; k++)
        {
            float r = sdk::fastPow2((1.0f + sigma) * kLog2[k]);
            float g = sdk::fastPow2(-tau * kLog2[k]);
            r += (kAlien[k] - r) * warp;
            r += shift;
            r = std::fabs(r); // a partial below 0 turns round
            if (fold > 0.0f && r > foldAt)
            {
                const float m2 = 2.0f * foldAt, q = r - m2 * std::floor(r / m2);
                r = q > foldAt ? m2 - q : q;
            }
            ratios[k] = std::max(r, 0.02f);

            if (combD > 0.0f) // a smooth comb, its teeth sliding along
            {
                const float w = 0.5f + 0.5f * sdk::fastSine(static_cast<float>(k) / combN - combPh + 0.25f);
                g *= 1.0f - combD + combD * w * w;
            }
            if (sweep > 0.0f) // a window about 3 partials wide scanning the spectrum
            {
                const float d = (static_cast<float>(k) - centre) * 0.4f;
                g *= 1.0f - sweep + sweep * sdk::fastPow2(-1.4427f * d * d);
            }
            if (smear > 0.0f) // each partial its own slow swell
            {
                smearPh[k] = wrap(smearPh[k] + (0.17f + 0.11f * static_cast<float>(k)) * dt);
                g *= 1.0f - smear * (0.5f + 0.5f * sdk::fastSine(smearPh[k]));
            }
            if (k & 1) // the even harmonics (k odd) ...
            {
                if (oddEven > 0.0f)
                    g *= 1.0f - oddEven;
            }
            else if (oddEven < 0.0f) // ... or the odd ones
            {
                g *= 1.0f + oddEven;
            }
            a[k] = g;
        }
        for (int k = 0; k < kPartials; k++) // SCRAMBLE, then GATE (a share of the loudest)
        {
            b[k] = a[k] + (a[kPerm[k]] - a[k]) * scramble;
            amax = std::max(amax, b[k]);
        }
        const float threshold = gate * 0.9f * amax;
        for (int k = 0; k < kPartials; k++)
        {
            a[k] = b[k] > threshold ? b[k] - threshold : 0.0f;
            sum2 += a[k] * a[k];
        }
        const float norm = 0.4f / std::sqrt(sum2 + 1.0e-9f);
        for (int k = 0; k < kPartials; k++)
        {
            tab[k] = a[k] * norm;
            const float dcay = spectralDecay * 64.0f;
            const float rate = dcay > 0.0f   ? dcay * (0.9f / 63.0f) * static_cast<float>(k + 1)
                               : dcay < 0.0f ? -dcay * (4.0f / 64.0f) / static_cast<float>(k + 1)
                                             : 0.0f;
            decayFactor[k] = std::exp(-rate * dt);
        }
    }

    static float wrap(float x) { return x >= 1.0f ? x - 1.0f : x; }

    // Parameters
    float structure = 0.5039f, tilt = 0.551f, comb = 0.0f, sweep = 0.0f, smear = 0.0f;
    float driftAmount = 0.0f, warp = 0.0f, freeze = 0.0f;
    float oddEven = 0.0f, shift = 0.0f, scramble = 0.0f, gate = 0.0f, spectralDecay = 0.0f;
    float unfold = 0.0f, phaseSpread = 0.0f, fold = 0.0f;
    float attackSec = 0.003f, decaySec = 0.4f, sustainLevel = 0.8f, releaseSec = 0.4f;
    float tuneSemitones = 0.0f, fineCents = 0.0f, glideTimeSec = 0.0f, volume = 1.0f;

    // Spectrum
    float ratios[kPartials] = {}, tab[kPartials] = {}, decayFactor[kPartials] = {};
    float sweepPh = 0.0f, combPh = 0.0f, smearPh[kPartials] = {};

    // Voice
    Adsr amp;
    Glide glide;
    Noise noise;
    float ph[kPartials] = {}, cur[kPartials] = {}, snap[kPartials] = {}, drift[kPartials] = {},
          life[kPartials] = {};
    uint32_t age = 0;
    uint8_t fresh = 0;

};

void PrismSynthVoice::createMetadata(const Patch& /*patch*/, Patch::Metadata& metadata)
{
    using namespace polysynth::sdk;
    using Param = Patch::Metadata::Param;
    using V = PrismSynthVoice;
    using X = Patch::optional_paramvalue_t;

    metadata.params.resize(enumutil::EnumMax<PrismParamId>::value + 1);
    metadata.params.shrink_to_fit();
    if (paramHandlers.empty())
        paramHandlers.resize(enumutil::EnumMax<PrismParamId>::value + 1);
    ParamHelper<PrismSynthVoice> helper(metadata, paramHandlers);

    auto unit = [](const char* name, float def, int cc) {
        return Param{ .displayName = name,
                      .defaultValue = def,
                      .minValue = 0.0f,
                      .maxValue = 1.0f,
                      .midiCC = static_cast<uint8_t>(cc),
                      .valueFormatter = createPercentFormatter() };
    };
    auto bipolar = [](const char* name, int cc) {
        return Param{ .displayName = name,
                      .defaultValue = 0.0f,
                      .minValue = -1.0f,
                      .maxValue = 1.0f,
                      .midiCC = static_cast<uint8_t>(cc),
                      .valueFormatter = createPercentFormatter() };
    };
    auto time = [](const char* name, float def, int cc) {
        return Param{ .displayName = name,
                      .defaultValue = def,
                      .minValue = 0.0f,
                      .maxValue = 10.0f,
                      .editCurve = Param::Curve::kExponential,
                      .midiCC = static_cast<uint8_t>(cc),
                      .valueFormatter = createTimeFormatter() };
    };
    const int osc = *MidiCCs::kOscParam0;

    helper.addParam(*PrismParamId::kStructure, unit("Structure", 0.5039f, osc),
                    [](V& v, X x) { v.structure = *x; });
    helper.addParam(*PrismParamId::kTilt, unit("Tilt", 0.551f, osc + 1), [](V& v, X x) { v.tilt = *x; });
    helper.addParam(*PrismParamId::kComb, unit("Comb", 0.0f, osc + 2), [](V& v, X x) { v.comb = *x; });
    helper.addParam(*PrismParamId::kSweep, unit("Sweep", 0.0f, osc + 3), [](V& v, X x) { v.sweep = *x; });
    helper.addParam(*PrismParamId::kSmear, unit("Smear", 0.0f, osc + 4), [](V& v, X x) { v.smear = *x; });
    helper.addParam(*PrismParamId::kDrift, unit("Drift", 0.0f, osc + 5),
                    [](V& v, X x) { v.driftAmount = *x; });
    helper.addParam(*PrismParamId::kWarp, unit("Warp", 0.0f, osc + 6), [](V& v, X x) { v.warp = *x; });
    helper.addParam(*PrismParamId::kFreeze, unit("Freeze", 0.0f, osc + 7),
                    [](V& v, X x) { v.freeze = *x; });
    helper.addParam(*PrismParamId::kOddEven, bipolar("Odd / Even", osc + 8),
                    [](V& v, X x) { v.oddEven = *x; });
    helper.addParam(*PrismParamId::kShift,
                    { .displayName = "Shift",
                      .defaultValue = 0.0f,
                      .minValue = -2.0f,
                      .maxValue = 2.0f,
                      .midiCC = static_cast<uint8_t>(osc + 9),
                      .valueFormatter = [](const Patch&, Patch::paramvalue_t value)
                      { return fmt::format("{:+.2f}", value); } },
                    [](V& v, X x) { v.shift = *x; });
    helper.addParam(*PrismParamId::kScramble, unit("Scramble", 0.0f, osc + 10),
                    [](V& v, X x) { v.scramble = *x; });
    helper.addParam(*PrismParamId::kGate, unit("Gate", 0.0f, osc + 11), [](V& v, X x) { v.gate = *x; });
    helper.addParam(*PrismParamId::kSpectralDecay, bipolar("Spectral Decay", *MidiCCs::kUserEnv1Decay),
                    [](V& v, X x) { v.spectralDecay = *x; });
    helper.addParam(*PrismParamId::kUnfold, unit("Unfold", 0.0f, *MidiCCs::kUserEnv1Attack),
                    [](V& v, X x) { v.unfold = *x; });
    helper.addParam(*PrismParamId::kPhase, unit("Phase", 0.0f, *MidiCCs::kUserEnv1Sustain),
                    [](V& v, X x) { v.phaseSpread = *x; });
    helper.addParam(*PrismParamId::kFold, unit("Fold", 0.0f, *MidiCCs::kUserEnv1Release),
                    [](V& v, X x) { v.fold = *x; });

    helper.addParam(*PrismParamId::kAttack, time("Attack", 0.003f, *MidiCCs::kAmpEnvAttack),
                    [](V& v, X x) { v.attackSec = *x; });
    helper.addParam(*PrismParamId::kDecay, time("Decay", 0.4f, *MidiCCs::kAmpEnvDecay),
                    [](V& v, X x) { v.decaySec = *x; });
    helper.addParam(*PrismParamId::kSustain, unit("Sustain", 0.8f, *MidiCCs::kAmpEnvSustain),
                    [](V& v, X x) { v.sustainLevel = *x; });
    helper.addParam(*PrismParamId::kRelease, time("Release", 0.4f, *MidiCCs::kAmpEnvRelease),
                    [](V& v, X x) { v.releaseSec = *x; });

    helper.addParam(*PrismParamId::kTune,
                    { .displayName = "Tune",
                      .defaultValue = 0.0f,
                      .minValue = -24,
                      .maxValue = 24,
                      .valueFormatter = createSemitoneFormatter() },
                    [](V& v, X x) { v.tuneSemitones = std::round(*x); });
    helper.addParam(*PrismParamId::kFine,
                    { .displayName = "Fine",
                      .defaultValue = 0.0f,
                      .minValue = -100,
                      .maxValue = 100,
                      .valueFormatter = createCentsFormatter() },
                    [](V& v, X x) { v.fineCents = *x; });
    helper.addParam(*PrismParamId::kPlayMode,
                    { .displayName = "Glide Mode",
                      .defaultValue = 0,
                      .minValue = 0,
                      .maxValue = 2,
                      .isEnum = true,
                      .valueFormatter = [](const Patch&, Patch::paramvalue_t value)
                      {
                          constexpr std::array kModes = { "Always", "Overlap", "Legato" };
                          return std::string(kModes.at(std::clamp<int>(std::lround(value), 0, 2)));
                      } },
                    [](V& v, X x) { v.playMode = static_cast<PlayMode>(std::clamp<int>(std::lround(*x), 0, 2)); });
    helper.addParam(*PrismParamId::kGlideTime,
                    { .displayName = "Glide Time",
                      .defaultValue = 0.0f,
                      .minValue = 0.0f,
                      .maxValue = 2.0f,
                      .editCurve = Param::Curve::kExponential,
                      .valueFormatter = createTimeFormatter() },
                    [](V& v, X x) { v.glideTimeSec = *x; });
    helper.addParam(*PrismParamId::kVolume,
                    { .displayName = "Volume",
                      .defaultValue = 1.0f,
                      .minValue = 0.0f,
                      .maxValue = 2.0f,
                      .macroOperation = Param::MacroOperation::kMult,
                      .valueFormatter = createPercentFormatter() },
                    [](V& v, X x) { v.volume = *x; });

    using Element = EditorElement;
    auto p = [](PrismParamId id) { return Element{ .type = Element::Type::kParam, .id = *id }; };
    metadata.layoutFlat = Patch::Metadata::EditorLayout{
        .sections = {
          { .name = "Spectrum",
            .shortName = "Spec",
            .elements = { p(PrismParamId::kStructure), p(PrismParamId::kTilt), p(PrismParamId::kWarp),
                          p(PrismParamId::kOddEven), p(PrismParamId::kShift), p(PrismParamId::kFold) } },
          { .name = "Motion",
            .shortName = "Motion",
            .elements = { p(PrismParamId::kComb), p(PrismParamId::kSweep), p(PrismParamId::kSmear),
                          p(PrismParamId::kDrift), p(PrismParamId::kFreeze) } },
          { .name = "Shape",
            .shortName = "Shape",
            .elements = { p(PrismParamId::kScramble), p(PrismParamId::kGate),
                          p(PrismParamId::kSpectralDecay), p(PrismParamId::kUnfold),
                          p(PrismParamId::kPhase) } },
          { .name = "Envelope",
            .shortName = "Env",
            .elements = { p(PrismParamId::kAttack), p(PrismParamId::kDecay), p(PrismParamId::kSustain),
                          p(PrismParamId::kRelease) } },
          { .name = "Voice",
            .shortName = "Voice",
            .elements = { p(PrismParamId::kTune), p(PrismParamId::kFine), p(PrismParamId::kPlayMode),
                          p(PrismParamId::kGlideTime), p(PrismParamId::kVolume) } },
        },
    };
    metadata.layout23 = metadata.layoutFlat;

    metadata.initDefaultMacros = [](auto& macros)
    {
        macros[0] = Patch::Metadata::Macro{ .displayName = "Tilt",
                                            .modifiers = { { *PrismParamId::kTilt, 1.0f } },
                                            .bipolar = true };
        macros[1] = Patch::Metadata::Macro{ .displayName = "Comb",
                                            .modifiers = { { *PrismParamId::kComb, 1.0f } },
                                            .bipolar = true };
        macros[2] = Patch::Metadata::Macro{ .displayName = "Sweep",
                                            .modifiers = { { *PrismParamId::kSweep, 1.0f } },
                                            .bipolar = true };
        macros[3] = Patch::Metadata::Macro{ .displayName = "Warp",
                                            .modifiers = { { *PrismParamId::kWarp, 1.0f } },
                                            .bipolar = true };
        macros[4] = Patch::Metadata::Macro{ .displayName = "Smear / Drift",
                                            .modifiers = { { *PrismParamId::kSmear, 0.8f },
                                                           { *PrismParamId::kDrift, 0.6f } },
                                            .bipolar = true };
        macros[5] = Patch::Metadata::Macro{ .displayName = "Structure",
                                            .modifiers = { { *PrismParamId::kStructure, 1.0f } },
                                            .bipolar = true };
        macros[*Patch::SystemMacroId::kVelocity].modifiers = { { *PrismParamId::kVolume, 1.0f } };
    };

    metadata.instrumentTypeId = static_cast<int>(InstrumentTypeId::kPrismSynth);
    metadata.outputChannels = Patch::Metadata::OutputChannels::kMono;
    metadata.maxVoices = 4;
}

} // namespace

std::unique_ptr<InstrumentType> createPrismSynth()
{
    return std::make_unique<InstrumentType>(
      InstrumentType{ .name = "PRISM",
                      .createMetadata = &PrismSynthVoice::createMetadata,
                      .createVoice = [](int /*voiceIdx*/) -> std::unique_ptr<InstrumentVoice>
                      { return std::make_unique<PrismSynthVoice>(); } });
}

} // namespace polysynth

#endif
