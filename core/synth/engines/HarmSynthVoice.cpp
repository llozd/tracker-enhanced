// HARM: additive synthesis after the Kawai K5 and K5m (openspec pocket-rack-engines).
// Ported from Pocket Rack's eng_k5.c (llozd). 32 sine harmonics a voice, shaped in three stages
// as on the original: the harmonic levels (each harmonic on one of four envelopes), a spectral
// filter (cutoff, slope, flat level) applied by frequency, and a formant EQ. A few macros draw
// what the original sets harmonic by harmonic.

#include "config.h"
#ifdef FEATURE_SYNTH

#include "HarmSynthVoice.h"
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
enum class HarmParamId : int
{
    kAngle = 0,
    kOddEven,
    kBloom,
    kHarmonicDecay,
    kCutoff,
    kSlope,
    kFlat,
    kVelocityFilter,
    kFormantDepth,
    kFormantFreq,
    kOctave,
    kFifth,
    kTwin,
    kHarmonicMod,
    kKeyHarmonics,
    kKeyScale,
    kEnvAmount,
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

constexpr int kHarmonics = 32;
constexpr float kTop = 0.46875f;   // a harmonic at this share of the sample rate is gone
constexpr float kFadeAt = 0.375f;  // ... and fades out from here

// log2(k + 1)
constexpr float kLog2[kHarmonics] = {
    0.0000000f, 1.0000000f, 1.5849625f, 2.0000000f, 2.3219281f, 2.5849625f, 2.8073549f, 3.0000000f,
    3.1699250f, 3.3219281f, 3.4594316f, 3.5849625f, 3.7004397f, 3.8073549f, 3.9068906f, 4.0000000f,
    4.0874628f, 4.1699250f, 4.2479275f, 4.3219281f, 4.3923174f, 4.4594316f, 4.5235620f, 4.5849625f,
    4.6438562f, 4.7004397f, 4.7548875f, 4.8073549f, 4.8579810f, 4.9068906f, 4.9541963f, 5.0000000f
};
// The envelope group of harmonic k + 1: 1-2, 3-6, 7-14, 15-32
constexpr uint8_t kGroup[kHarmonics] = { 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3,
                                         3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };

using namespace enginekit;

inline float softKnee(float x)
{
    const float a = std::fabs(x);
    if (a <= 0.5f)
        return x;
    const float y = 0.5f + 0.5f * softTanh((a - 0.5f) * 2.0f);
    return x < 0.0f ? -y : y;
}

class HarmSynthVoice : public sdk::CustomSynthVoice<HarmSynthVoice, HarmParamId>
{
  public:
    bool isActive() final { return amp.isActive(); }

    void onNoteOn(const InstrumentVoiceContext& /*ctx*/,
                  note_t note,
                  float velocity,
                  bool isLegatoStep) final
    {
        if (!amp.isActive())
        {
            for (int k = 0; k < kHarmonics; k++) // Schroeder's phases: bright without a spike
            {
                ph[k] = static_cast<float>(k * (k + 1)) / (2.0f * kHarmonics);
                ph[k] -= std::floor(ph[k]);
                cur[k] = 0.0f;
            }
        }
        if (!isLegatoStep)
            age = 0;
        noteVelocity = velocity;
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

        const float dt = static_cast<float>(n) / kSampleRate;
        const int nh = twin > 0.0f ? kHarmonics / 2 : kHarmonics;
        const float note = glide.advance(n) + tuneSemitones + fineCents * 0.01f + pitchBendSemitones;
        const float freq = 440.0f * sdk::fastPow2((note - 69.0f) / 12.0f);
        const float baseInc = freq / kSampleRate;
        const float key = note - 60.0f;

        // The harmonic levels (DHG), balanced for loudness.
        float base[kHarmonics];
        {
            const float octaveGain = sdk::fastPow2(octaveDb / 6.0206f);
            const float fifthGain = sdk::fastPow2(fifthDb / 6.0206f);
            float sum2 = 0.0f;
            for (int k = 0; k < nh; k++)
            {
                const int h = k + 1;
                float g = sdk::fastPow2(-angle * kLog2[k]);
                if (h & 1)
                {
                    if (oddEven < 0.0f && h > 1)
                        g *= 1.0f + oddEven;
                }
                else if (oddEven > 0.0f)
                {
                    g *= 1.0f - oddEven;
                }
                if (h > 1 && !(h & (h - 1))) // 2, 4, 8, 16, 32
                    g *= octaveGain;
                else if (h % 3 == 0 && !((h / 3) & (h / 3 - 1))) // 3, 6, 12, 24
                    g *= fifthGain;
                base[k] = g;
                sum2 += g * g;
            }
            const float norm = 0.3f / std::sqrt(sum2 * (nh < kHarmonics ? 2.0f : 1.0f) + 1.0e-9f);
            for (int k = 0; k < nh; k++)
                base[k] *= norm;
        }

        // The four group envelopes, the upper two swelling with the harmonic modulation.
        float env[4];
        const float seconds = static_cast<float>(age) / kSampleRate;
        for (int g = 0; g < 4; g++)
        {
            modPh[g] += (0.7f + 0.73f * static_cast<float>(g)) * dt;
            if (modPh[g] >= 1.0f)
                modPh[g] -= 1.0f;
            env[g] = groupEnvelope(g, seconds);
            if (harmonicMod > 0.0f && g >= 2)
                env[g] *= 1.0f - harmonicMod * (0.5f + 0.5f * sdk::fastSine(modPh[g]));
        }
        age += n;

        // The formant EQ (DFT): a peak over a dip, as loud as without.
        const float lf0 = std::log2(freq + 1.0e-3f);
        const float keyHarm = keyHarmonics * key / 24.0f;
        const float fdep = formantDepth * 2.0f;
        const float lff = 9.9658f + formantFreqOct;
        float gk[kHarmonics];
        float s0 = 0.0f, s1 = 0.0f;
        for (int k = 0; k < nh; k++)
        {
            float g = base[k] * env[kGroup[k]];
            if (keyHarm != 0.0f)
                g *= sdk::fastPow2(-std::max(keyHarm, -1.0f) * kLog2[k]);
            gk[k] = g;
            if (fdep > 0.0f)
            {
                const float d = lf0 + kLog2[k] - lff;
                s0 += g * g;
                gk[k] *= sdk::fastPow2(fdep * (std::exp(-d * d) - 0.35f));
                s1 += gk[k] * gk[k];
            }
        }
        if (s1 > 0.0f)
        {
            const float r = std::sqrt(s0 / s1);
            for (int k = 0; k < nh; k++)
                gk[k] *= r;
        }

        // The spectral filter (DDF): 50 Hz to 12.8 kHz, opened by velocity, the keyboard and the envelope.
        const float lfc = 5.64f + cutoff * 8.0f + velocityFilter * 4.0f * noteVelocity +
                          keyScale * key / 12.0f + envAmount * 8.0f * amp.value();
        const float slopeOct = (6.0f + slope * 42.0f) / 6.02f;
        const float twinRatio = sdk::fastPow2(twin * 50.0f / 1200.0f);

        for (int k = 0; k < kHarmonics; k++)
        {
            const int src = k % nh;
            float inc = baseInc * static_cast<float>(src + 1);
            if (k >= nh)
                inc *= twinRatio;
            const float o = lf0 + kLog2[src] - lfc;
            float tgt = gk[src] * (o > 0.0f ? sdk::fastPow2(-slopeOct * o)
                                            : flat + (1.0f - flat) * sdk::fastPow2(2.0f * o));
            if (inc >= kTop)
                tgt = 0.0f;
            else if (inc > kFadeAt)
                tgt *= (kTop - inc) / (kTop - kFadeAt);

            cur[k] += (tgt - cur[k]) * 0.25f;
            float p = ph[k];
            if (cur[k] < 1.0e-4f && tgt < 1.0e-4f) // silent: only the phase moves on
            {
                cur[k] = tgt;
                p += std::min(inc, kTop) * static_cast<float>(n);
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

        for (uint32_t i = 0; i < n; i++)
            out[i] = softKnee(out[i]) * amp.process() * volume;
    }

    static void createMetadata(const Patch& /*patch*/, Patch::Metadata& metadata);

  private:
    // Group g's envelope at s seconds: an attack (Bloom), then a fall towards a floor (Harmonic Decay).
    float groupEnvelope(int g, float s) const
    {
        const float attack = bloom * bloom * static_cast<float>(g);
        if (attack > 0.0f && s < attack)
            return s / attack;
        if (harmonicDecay <= 0.0f)
            return 1.0f;
        const float floorLevel = 1.0f - harmonicDecay * (0.25f + 0.25f * static_cast<float>(g));
        const float rate = harmonicDecay * (0.3f + 1.5f * static_cast<float>(g));
        return floorLevel + (1.0f - floorLevel) * std::exp(-rate * (s - attack));
    }

    // Parameters
    float angle = 1.0f, oddEven = 0.0f, bloom = 0.0f, harmonicDecay = 0.0f;
    float cutoff = 1.0f, slope = 0.504f, flat = 1.0f, velocityFilter = 0.0f;
    float formantDepth = 0.0f, formantFreqOct = 0.0f, octaveDb = 0.0f, fifthDb = 0.0f;
    float twin = 0.0f, harmonicMod = 0.0f, keyHarmonics = 0.0f, keyScale = 0.0f, envAmount = 0.0f;
    float attackSec = 0.003f, decaySec = 0.4f, sustainLevel = 0.85f, releaseSec = 0.3f;
    float tuneSemitones = 0.0f, fineCents = 0.0f, glideTimeSec = 0.0f, volume = 1.0f;

    // State
    Adsr amp;
    Glide glide;
    float ph[kHarmonics] = {}, cur[kHarmonics] = {}, modPh[4] = {};
    uint32_t age = 0;
    float noteVelocity = 1.0f;
};

void HarmSynthVoice::createMetadata(const Patch& /*patch*/, Patch::Metadata& metadata)
{
    using namespace polysynth::sdk;
    using Param = Patch::Metadata::Param;
    using V = HarmSynthVoice;
    using X = Patch::optional_paramvalue_t;

    metadata.params.resize(enumutil::EnumMax<HarmParamId>::value + 1);
    metadata.params.shrink_to_fit();
    if (paramHandlers.empty())
        paramHandlers.resize(enumutil::EnumMax<HarmParamId>::value + 1);
    ParamHelper<HarmSynthVoice> helper(metadata, paramHandlers);

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
    auto decibels = [](const char* name, int cc) {
        return Param{ .displayName = name,
                      .defaultValue = 0.0f,
                      .minValue = -12.0f,
                      .maxValue = 12.0f,
                      .midiCC = static_cast<uint8_t>(cc),
                      .valueFormatter = [](const Patch&, Patch::paramvalue_t value)
                      { return fmt::format("{:+.1f} dB", value); } };
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

    helper.addParam(*HarmParamId::kAngle,
                    { .displayName = "Angle",
                      .defaultValue = 1.0f,
                      .minValue = -2.0f,
                      .maxValue = 2.0f,
                      .midiCC = static_cast<uint8_t>(osc),
                      .valueFormatter = [](const Patch&, Patch::paramvalue_t value)
                      { return fmt::format("{:+.2f}", value); } },
                    [](V& v, X x) { v.angle = *x; });
    helper.addParam(*HarmParamId::kOddEven, bipolar("Odd / Even", osc + 1), [](V& v, X x) { v.oddEven = *x; });
    helper.addParam(*HarmParamId::kBloom, unit("Bloom", 0.0f, osc + 2), [](V& v, X x) { v.bloom = *x; });
    helper.addParam(*HarmParamId::kHarmonicDecay, unit("Harm Decay", 0.0f, osc + 3),
                    [](V& v, X x) { v.harmonicDecay = *x; });
    helper.addParam(*HarmParamId::kCutoff, unit("Cutoff", 1.0f, *MidiCCs::kFilterCutoff),
                    [](V& v, X x) { v.cutoff = *x; });
    helper.addParam(*HarmParamId::kSlope, unit("Slope", 0.504f, *MidiCCs::kFilterResonance),
                    [](V& v, X x) { v.slope = *x; });
    helper.addParam(*HarmParamId::kFlat, unit("Flat", 1.0f, osc + 4), [](V& v, X x) { v.flat = *x; });
    helper.addParam(*HarmParamId::kVelocityFilter, unit("Vel > Filter", 0.0f, *MidiCCs::kFilterEnvVelocity),
                    [](V& v, X x) { v.velocityFilter = *x; });
    helper.addParam(*HarmParamId::kFormantDepth, unit("Formant", 0.0f, osc + 5),
                    [](V& v, X x) { v.formantDepth = *x; });
    helper.addParam(*HarmParamId::kFormantFreq,
                    { .displayName = "Formant Freq",
                      .defaultValue = 0.0f,
                      .minValue = -4.0f,
                      .maxValue = 4.0f,
                      .midiCC = static_cast<uint8_t>(osc + 6),
                      .valueFormatter = [](const Patch&, Patch::paramvalue_t value)
                      { return fmt::format("{:.0f} Hz", 1000.0f * std::exp2(value)); } },
                    [](V& v, X x) { v.formantFreqOct = *x; });
    helper.addParam(*HarmParamId::kOctave, decibels("Octave", osc + 7), [](V& v, X x) { v.octaveDb = *x; });
    helper.addParam(*HarmParamId::kFifth, decibels("Fifth", osc + 8), [](V& v, X x) { v.fifthDb = *x; });
    helper.addParam(*HarmParamId::kTwin, unit("Twin", 0.0f, osc + 9), [](V& v, X x) { v.twin = *x; });
    helper.addParam(*HarmParamId::kHarmonicMod, unit("Harm Mod", 0.0f, osc + 10),
                    [](V& v, X x) { v.harmonicMod = *x; });
    helper.addParam(*HarmParamId::kKeyHarmonics, bipolar("Key > Harm", osc + 11),
                    [](V& v, X x) { v.keyHarmonics = *x; });
    helper.addParam(*HarmParamId::kKeyScale, bipolar("Key > Filter", *MidiCCs::kUserEnv1Attack),
                    [](V& v, X x) { v.keyScale = *x; });
    helper.addParam(*HarmParamId::kEnvAmount, bipolar("Env Amount", *MidiCCs::kFilterEnvAmount),
                    [](V& v, X x) { v.envAmount = *x; });

    helper.addParam(*HarmParamId::kAttack, time("Attack", 0.003f, *MidiCCs::kAmpEnvAttack),
                    [](V& v, X x) { v.attackSec = *x; });
    helper.addParam(*HarmParamId::kDecay, time("Decay", 0.4f, *MidiCCs::kAmpEnvDecay),
                    [](V& v, X x) { v.decaySec = *x; });
    helper.addParam(*HarmParamId::kSustain, unit("Sustain", 0.85f, *MidiCCs::kAmpEnvSustain),
                    [](V& v, X x) { v.sustainLevel = *x; });
    helper.addParam(*HarmParamId::kRelease, time("Release", 0.3f, *MidiCCs::kAmpEnvRelease),
                    [](V& v, X x) { v.releaseSec = *x; });

    helper.addParam(*HarmParamId::kTune,
                    { .displayName = "Tune",
                      .defaultValue = 0.0f,
                      .minValue = -24,
                      .maxValue = 24,
                      .valueFormatter = createSemitoneFormatter() },
                    [](V& v, X x) { v.tuneSemitones = std::round(*x); });
    helper.addParam(*HarmParamId::kFine,
                    { .displayName = "Fine",
                      .defaultValue = 0.0f,
                      .minValue = -100,
                      .maxValue = 100,
                      .valueFormatter = createCentsFormatter() },
                    [](V& v, X x) { v.fineCents = *x; });
    helper.addParam(*HarmParamId::kPlayMode,
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
    helper.addParam(*HarmParamId::kGlideTime,
                    { .displayName = "Glide Time",
                      .defaultValue = 0.0f,
                      .minValue = 0.0f,
                      .maxValue = 2.0f,
                      .editCurve = Param::Curve::kExponential,
                      .valueFormatter = createTimeFormatter() },
                    [](V& v, X x) { v.glideTimeSec = *x; });
    helper.addParam(*HarmParamId::kVolume,
                    { .displayName = "Volume",
                      .defaultValue = 1.0f,
                      .minValue = 0.0f,
                      .maxValue = 2.0f,
                      .macroOperation = Param::MacroOperation::kMult,
                      .valueFormatter = createPercentFormatter() },
                    [](V& v, X x) { v.volume = *x; });

    using Element = EditorElement;
    auto p = [](HarmParamId id) { return Element{ .type = Element::Type::kParam, .id = *id }; };
    metadata.layoutFlat = Patch::Metadata::EditorLayout{
        .sections = {
          { .name = "Harmonics",
            .shortName = "Harm",
            .elements = { p(HarmParamId::kAngle), p(HarmParamId::kOddEven), p(HarmParamId::kOctave),
                          p(HarmParamId::kFifth), p(HarmParamId::kBloom), p(HarmParamId::kHarmonicDecay) } },
          { .name = "Filter",
            .shortName = "Filter",
            .elements = { p(HarmParamId::kCutoff), p(HarmParamId::kSlope), p(HarmParamId::kFlat),
                          p(HarmParamId::kEnvAmount), p(HarmParamId::kVelocityFilter),
                          p(HarmParamId::kKeyScale) } },
          { .name = "Formant",
            .shortName = "Formant",
            .elements = { p(HarmParamId::kFormantDepth), p(HarmParamId::kFormantFreq), p(HarmParamId::kTwin),
                          p(HarmParamId::kHarmonicMod), p(HarmParamId::kKeyHarmonics) } },
          { .name = "Envelope",
            .shortName = "Env",
            .elements = { p(HarmParamId::kAttack), p(HarmParamId::kDecay), p(HarmParamId::kSustain),
                          p(HarmParamId::kRelease) } },
          { .name = "Voice",
            .shortName = "Voice",
            .elements = { p(HarmParamId::kTune), p(HarmParamId::kFine), p(HarmParamId::kPlayMode),
                          p(HarmParamId::kGlideTime), p(HarmParamId::kVolume) } },
        },
    };
    metadata.layout23 = metadata.layoutFlat;

    metadata.initDefaultMacros = [](auto& macros)
    {
        macros[0] = Patch::Metadata::Macro{ .displayName = "Cutoff",
                                            .modifiers = { { *HarmParamId::kCutoff, 1.0f } },
                                            .bipolar = true };
        macros[1] = Patch::Metadata::Macro{ .displayName = "Angle",
                                            .modifiers = { { *HarmParamId::kAngle, 1.0f } },
                                            .bipolar = true };
        macros[2] = Patch::Metadata::Macro{ .displayName = "Bloom / Decay",
                                            .modifiers = { { *HarmParamId::kBloom, 0.8f },
                                                           { *HarmParamId::kHarmonicDecay, 0.8f } },
                                            .bipolar = true };
        macros[3] = Patch::Metadata::Macro{ .displayName = "Formant",
                                            .modifiers = { { *HarmParamId::kFormantDepth, 1.0f },
                                                           { *HarmParamId::kFormantFreq, 0.5f } },
                                            .bipolar = true };
        macros[4] = Patch::Metadata::Macro{ .displayName = "Odd / Even",
                                            .modifiers = { { *HarmParamId::kOddEven, 1.0f } },
                                            .bipolar = true };
        macros[5] = Patch::Metadata::Macro{ .displayName = "Twin / Mod",
                                            .modifiers = { { *HarmParamId::kTwin, 0.8f },
                                                           { *HarmParamId::kHarmonicMod, 0.6f } },
                                            .bipolar = true };
        macros[*Patch::SystemMacroId::kVelocity].modifiers = { { *HarmParamId::kVolume, 1.0f } };
    };

    metadata.instrumentTypeId = static_cast<int>(InstrumentTypeId::kHarmSynth);
    metadata.outputChannels = Patch::Metadata::OutputChannels::kMono;
    metadata.maxVoices = 4;
}

} // namespace

std::unique_ptr<InstrumentType> createHarmSynth()
{
    return std::make_unique<InstrumentType>(
      InstrumentType{ .name = "HARM",
                      .createMetadata = &HarmSynthVoice::createMetadata,
                      .createVoice = [](int /*voiceIdx*/) -> std::unique_ptr<InstrumentVoice>
                      { return std::make_unique<HarmSynthVoice>(); } });
}

} // namespace polysynth

#endif
