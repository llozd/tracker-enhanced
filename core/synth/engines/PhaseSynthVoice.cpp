// PHASE: a phase distortion synth in the manner of the Casio CZ series (openspec pocket-rack-engines).
// Written for this firmware from the published description of phase distortion (the CZ manuals
// and the patent's principle: a cosine read through a bent phase, the bend set by DCW). It uses no
// code from Pocket Rack's PHASE engine or CrispyZebra.
//
// Each wave is a phase transfer function T(phase) for full distortion. The oscillator reads
// cos(2 pi (phase + DCW (T(phase) - phase))), so DCW 0 is always a pure sine and DCW 1 the full
// shape. The resonance waves are a windowed cosine whose frequency rises with DCW instead.
// Two lines, each with two waves that alternate cycle by cycle, a DCW envelope and its own level;
// the second line can be detuned, ring modulated or mixed.

#include "config.h"
#ifdef FEATURE_SYNTH

#include "PhaseSynthVoice.h"
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
enum class PhaseParamId : int
{
    kWaveA1 = 0,
    kWaveB1,
    kDcw1,
    kDcwEnv1,
    kDcwAttack1,
    kDcwDecay1,
    kDcwSustain1,
    kDcwRelease1,
    kWaveA2,
    kWaveB2,
    kDcw2,
    kDcwEnv2,
    kDcwAttack2,
    kDcwDecay2,
    kDcwSustain2,
    kDcwRelease2,
    kLineMode,
    kLevel2,
    kDetuneSemis,
    kDetuneCents,
    kNoise,
    kKeyFollow,
    kAttack,
    kDecay,
    kSustain,
    kRelease,
    kVibratoRate,
    kVibratoDepth,
    kTune,
    kFine,
    kPlayMode,
    kGlideTime,
    kVolume,
};

enum Wave : int
{
    kWaveOff = 0, // wave B only
    kSaw,
    kSquare,
    kPulse,
    kDoubleSine,
    kSawPulse,
    kResoSaw,
    kResoTriangle,
    kResoTrapezoid,
    kWaveCount
};
constexpr std::array<const char*, kWaveCount> kWaveNames = { "Off",      "Saw",        "Square",
                                                             "Pulse",    "Double Sine", "Saw-Pulse",
                                                             "Reso Saw", "Reso Tri",    "Reso Trap" };

enum LineMode : int
{
    kLine1,
    kLine2,
    kLine1And2,
    kRing,
};
constexpr std::array<const char*, 4> kLineModeNames = { "Line 1", "Line 2", "1 + 2", "1 x 2 Ring" };

using namespace enginekit;

constexpr float kKnee = 0.02f; // the steepest bend the full-distortion shapes use

// The bent phase at full distortion, for the non-resonant waves (phase and result in 0..1).
inline float transfer(int wave, float ph)
{
    switch (wave)
    {
        case kSaw: // the cosine's falling half squeezed into the start: a saw
            return ph < kKnee ? ph * (0.5f / kKnee) : 0.5f + (ph - kKnee) * (0.5f / (1.0f - kKnee));
        case kSquare: // each half rises fast and holds: cos holds at -1, then at +1
        {
            const float h = ph < 0.5f ? ph : ph - 0.5f;
            const float r = std::min(h * (0.5f / kKnee), 0.5f);
            return ph < 0.5f ? r : 0.5f + r;
        }
        case kPulse: // one fast cycle, then held at +1
            return std::min(ph * (1.0f / (4.0f * kKnee)), 1.0f);
        case kDoubleSine: // two cycles a period
            return ph < 0.5f ? 2.0f * ph : 2.0f * ph - 1.0f;
        case kSawPulse: // a saw in the first half, held in the second
            return ph < 0.5f ? transfer(kSaw, 2.0f * ph) : 1.0f;
        default: return ph;
    }
}

// One sample of a wave at phase ph (0..1) with distortion d (0..1).
inline float waveSample(int wave, float ph, float d)
{
    if (wave >= kResoSaw)
    {
        // A cosine at 1 + 15 d times the frequency, restarted each cycle under a window.
        const float r = 1.0f + 15.0f * d;
        float window;
        if (wave == kResoSaw)
            window = 1.0f - ph;
        else if (wave == kResoTriangle)
            window = 1.0f - std::fabs(2.0f * ph - 1.0f);
        else
            window = std::min(1.0f, 2.0f * (1.0f - ph));
        return window * sdk::fastSine(ph * r + 0.25f);
    }
    const float bent = ph + d * (transfer(wave, ph) - ph);
    return sdk::fastSine(bent + 0.25f); // cos
}

// One line: a phase, its two waves alternating by cycle, its DCW envelope.
struct Line
{
    float phase = 0.0f;
    bool second = false; // playing wave B this cycle
    Adsr dcwEnv;
};

class PhaseSynthVoice : public sdk::CustomSynthVoice<PhaseSynthVoice, PhaseParamId>
{
  public:
    bool isActive() final { return amp.isActive(); }

    void onNoteOn(const InstrumentVoiceContext& /*ctx*/,
                  note_t note,
                  float /*velocity*/,
                  bool isLegatoStep) final
    {
        if (!amp.isActive())
        {
            line[0].phase = line[1].phase = 0.0f;
            line[0].second = line[1].second = false;
            dc = 0.0f;
        }
        glide.noteOn(static_cast<float>(note), glideTimeSec,
                     playMode != PlayMode::kLegato || isLegatoStep);
        setEnvelopes();
        amp.gateOn(!isLegatoStep);
        line[0].dcwEnv.gateOn(!isLegatoStep);
        line[1].dcwEnv.gateOn(!isLegatoStep);
    }

    void onNoteOff(const InstrumentVoiceContext& /*ctx*/, note_t /*note*/) final
    {
        amp.gateOff();
        line[0].dcwEnv.gateOff();
        line[1].dcwEnv.gateOff();
    }

    void renderVoiceAudio(const InstrumentVoiceContext& /*ctx*/,
                          [[maybe_unused]] std::optional<AudioBufferStereoRef> inputBuffer,
                          AudioBufferStereoRef outputBuffer,
                          std::span<AudioBufferRef> /*auxOutBuffer*/) final
    {
        auto out = outputBuffer[0];
        const uint32_t n = static_cast<uint32_t>(out.size());
        if (!amp.isActive())
        {
            std::fill(out.begin(), out.end(), 0.0f);
            return;
        }
        setEnvelopes();

        const float vib = vibratoLfo.advance(vibratoRateHz, Lfo::Wave::kSine, n) * vibratoDepth;
        const float note = glide.advance(n) + tuneSemitones + fineCents * 0.01f + pitchBendSemitones + vib;
        const float inc1 = std::min(0.45f, 440.0f * sdk::fastPow2((note - 69.0f) / 12.0f) / kSampleRate);
        const float inc2 =
          std::min(0.45f, inc1 * sdk::fastPow2((detuneSemis + detuneCents * 0.01f) / 12.0f));
        // Key follow: less distortion for high notes, as on the CZ (keeps the top from aliasing).
        const float keyScale = std::clamp(1.0f - keyFollow * (note - 48.0f) / 48.0f, 0.0f, 1.0f);

        const bool use1 = lineMode != kLine2;
        const bool use2 = lineMode != kLine1;
        const float g1 = lineMode == kLine1And2 ? 0.6f : 1.0f;
        const float g2 = lineMode == kLine1And2 ? 0.6f * level2 : 1.0f;
        const float level = 0.5f * volume;

        for (uint32_t i = 0; i < n; i++)
        {
            const float d1 = std::clamp(dcw[0] + dcwEnvAmount[0] * line[0].dcwEnv.process(), 0.0f, 1.0f) * keyScale;
            const float d2 = std::clamp(dcw[1] + dcwEnvAmount[1] * line[1].dcwEnv.process(), 0.0f, 1.0f) * keyScale;
            const float s1 = use1 ? step(line[0], waveA[0], waveB[0], d1, inc1) : 0.0f;
            const float s2 = use2 ? step(line[1], waveA[1], waveB[1], d2, inc2) : 0.0f;

            float y;
            if (lineMode == kRing)
                y = s1 * s2;
            else
                y = g1 * s1 + g2 * s2;
            if (noiseAmount > 0.0f) // noise modulation: a rough, breathy edge
                y *= 1.0f - noiseAmount + noiseAmount * noise.next();

            dc += 0.001f * (y - dc); // the resonance waves carry an offset
            out[i] = (y - dc) * amp.process() * level;
        }
    }

    static void createMetadata(const Patch& /*patch*/, Patch::Metadata& metadata);

  private:
    static float step(Line& l, int a, int b, float d, float inc)
    {
        const int wave = (l.second && b != kWaveOff) ? b : a;
        const float s = waveSample(wave, l.phase, d);
        l.phase += inc;
        if (l.phase >= 1.0f)
        {
            l.phase -= 1.0f;
            l.second = !l.second;
        }
        return s;
    }

    void setEnvelopes()
    {
        amp.set(attackSec, decaySec, sustainLevel, releaseSec);
        for (int k = 0; k < 2; k++)
            line[k].dcwEnv.set(dcwAttack[k], dcwDecay[k], dcwSustain[k], dcwRelease[k]);
    }

    // Parameters
    int waveA[2] = { kSaw, kSquare }, waveB[2] = { kWaveOff, kWaveOff };
    float dcw[2] = { 0.2f, 0.2f }, dcwEnvAmount[2] = { 0.6f, 0.6f };
    float dcwAttack[2] = { 0.003f, 0.003f }, dcwDecay[2] = { 0.6f, 0.6f };
    float dcwSustain[2] = { 0.3f, 0.3f }, dcwRelease[2] = { 0.3f, 0.3f };
    LineMode lineMode = kLine1;
    float level2 = 1.0f, detuneSemis = 0.0f, detuneCents = 7.0f, noiseAmount = 0.0f, keyFollow = 0.3f;
    float attackSec = 0.003f, decaySec = 0.5f, sustainLevel = 0.8f, releaseSec = 0.3f;
    float vibratoRateHz = 5.0f, vibratoDepth = 0.0f;
    float tuneSemitones = 0.0f, fineCents = 0.0f, glideTimeSec = 0.0f, volume = 1.0f;

    // State
    Adsr amp;
    Line line[2];
    Lfo vibratoLfo;
    Glide glide;
    Noise noise;
    float dc = 0.0f;
};

void PhaseSynthVoice::createMetadata(const Patch& /*patch*/, Patch::Metadata& metadata)
{
    using namespace polysynth::sdk;
    using Param = Patch::Metadata::Param;
    using V = PhaseSynthVoice;
    using X = Patch::optional_paramvalue_t;

    metadata.params.resize(enumutil::EnumMax<PhaseParamId>::value + 1);
    metadata.params.shrink_to_fit();
    if (paramHandlers.empty())
        paramHandlers.resize(enumutil::EnumMax<PhaseParamId>::value + 1);
    ParamHelper<PhaseSynthVoice> helper(metadata, paramHandlers);

    auto unit = [](const char* name, float def, int cc) {
        return Param{ .displayName = name,
                      .defaultValue = def,
                      .minValue = 0.0f,
                      .maxValue = 1.0f,
                      .midiCC = static_cast<uint8_t>(cc),
                      .valueFormatter = createPercentFormatter() };
    };
    auto bipolar = [](const char* name, float def, int cc) {
        return Param{ .displayName = name,
                      .defaultValue = def,
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
    auto wave = [](const char* name, int def, int lowest, int cc) {
        return Param{ .displayName = name,
                      .defaultValue = def,
                      .minValue = static_cast<Patch::paramvalue_short_t>(lowest),
                      .maxValue = kWaveCount - 1,
                      .isEnum = true,
                      .midiCC = static_cast<uint8_t>(cc),
                      .valueFormatter = [](const Patch&, Patch::paramvalue_t value)
                      { return std::string(kWaveNames[std::clamp<int>(std::lround(value), 0, kWaveCount - 1)]); } };
    };
    const int osc = *MidiCCs::kOscParam0;

    helper.addParam(*PhaseParamId::kWaveA1, wave("L1 Wave A", kSaw, kSaw, osc), [](V& v, X x) { v.waveA[0] = std::clamp<int>(std::lround(*x), kSaw, kWaveCount - 1); });
    helper.addParam(*PhaseParamId::kWaveB1, wave("L1 Wave B", kWaveOff, kWaveOff, osc + 1), [](V& v, X x) { v.waveB[0] = std::clamp<int>(std::lround(*x), kWaveOff, kWaveCount - 1); });
    helper.addParam(*PhaseParamId::kDcw1, unit("L1 DCW", 0.2f, osc + 2), [](V& v, X x) { v.dcw[0] = *x; });
    helper.addParam(*PhaseParamId::kDcwEnv1, bipolar("L1 DCW Env", 0.6f, osc + 3), [](V& v, X x) { v.dcwEnvAmount[0] = *x; });
    helper.addParam(*PhaseParamId::kDcwAttack1, time("L1 DCW Att", 0.003f, *MidiCCs::kFilterEnvAttack), [](V& v, X x) { v.dcwAttack[0] = *x; });
    helper.addParam(*PhaseParamId::kDcwDecay1, time("L1 DCW Dec", 0.6f, *MidiCCs::kFilterEnvDecay), [](V& v, X x) { v.dcwDecay[0] = *x; });
    helper.addParam(*PhaseParamId::kDcwSustain1, unit("L1 DCW Sus", 0.3f, *MidiCCs::kFilterEnvSustain), [](V& v, X x) { v.dcwSustain[0] = *x; });
    helper.addParam(*PhaseParamId::kDcwRelease1, time("L1 DCW Rel", 0.3f, *MidiCCs::kFilterEnvRelease), [](V& v, X x) { v.dcwRelease[0] = *x; });
    helper.addParam(*PhaseParamId::kWaveA2, wave("L2 Wave A", kSquare, kSaw, osc + 4), [](V& v, X x) { v.waveA[1] = std::clamp<int>(std::lround(*x), kSaw, kWaveCount - 1); });
    helper.addParam(*PhaseParamId::kWaveB2, wave("L2 Wave B", kWaveOff, kWaveOff, osc + 5), [](V& v, X x) { v.waveB[1] = std::clamp<int>(std::lround(*x), kWaveOff, kWaveCount - 1); });
    helper.addParam(*PhaseParamId::kDcw2, unit("L2 DCW", 0.2f, osc + 6), [](V& v, X x) { v.dcw[1] = *x; });
    helper.addParam(*PhaseParamId::kDcwEnv2, bipolar("L2 DCW Env", 0.6f, osc + 7), [](V& v, X x) { v.dcwEnvAmount[1] = *x; });
    helper.addParam(*PhaseParamId::kDcwAttack2, time("L2 DCW Att", 0.003f, *MidiCCs::kUserEnv2Attack), [](V& v, X x) { v.dcwAttack[1] = *x; });
    helper.addParam(*PhaseParamId::kDcwDecay2, time("L2 DCW Dec", 0.6f, *MidiCCs::kUserEnv2Decay), [](V& v, X x) { v.dcwDecay[1] = *x; });
    helper.addParam(*PhaseParamId::kDcwSustain2, unit("L2 DCW Sus", 0.3f, *MidiCCs::kUserEnv2Sustain), [](V& v, X x) { v.dcwSustain[1] = *x; });
    helper.addParam(*PhaseParamId::kDcwRelease2, time("L2 DCW Rel", 0.3f, *MidiCCs::kUserEnv2Release), [](V& v, X x) { v.dcwRelease[1] = *x; });
    helper.addParam(*PhaseParamId::kLineMode,
                    { .displayName = "Line Mode",
                      .defaultValue = kLine1,
                      .minValue = 0,
                      .maxValue = 3,
                      .isEnum = true,
                      .midiCC = static_cast<uint8_t>(osc + 8),
                      .valueFormatter = [](const Patch&, Patch::paramvalue_t value)
                      { return std::string(kLineModeNames[std::clamp<int>(std::lround(value), 0, 3)]); } },
                    [](V& v, X x) { v.lineMode = static_cast<LineMode>(std::clamp<int>(std::lround(*x), 0, 3)); });
    helper.addParam(*PhaseParamId::kLevel2, unit("L2 Level", 1.0f, osc + 9), [](V& v, X x) { v.level2 = *x; });
    helper.addParam(*PhaseParamId::kDetuneSemis,
                    { .displayName = "L2 Detune",
                      .defaultValue = 0.0f,
                      .minValue = -24,
                      .maxValue = 24,
                      .midiCC = static_cast<uint8_t>(osc + 10),
                      .valueFormatter = createSemitoneFormatter() },
                    [](V& v, X x) { v.detuneSemis = std::round(*x); });
    helper.addParam(*PhaseParamId::kDetuneCents,
                    { .displayName = "L2 Fine",
                      .defaultValue = 7.0f,
                      .minValue = -100,
                      .maxValue = 100,
                      .midiCC = static_cast<uint8_t>(osc + 11),
                      .valueFormatter = createCentsFormatter() },
                    [](V& v, X x) { v.detuneCents = *x; });
    helper.addParam(*PhaseParamId::kNoise, unit("Noise", 0.0f, *MidiCCs::kUserEnv1Attack), [](V& v, X x) { v.noiseAmount = *x; });
    helper.addParam(*PhaseParamId::kKeyFollow, unit("Key Follow", 0.3f, *MidiCCs::kUserEnv1Decay), [](V& v, X x) { v.keyFollow = *x; });
    helper.addParam(*PhaseParamId::kAttack, time("Attack", 0.003f, *MidiCCs::kAmpEnvAttack), [](V& v, X x) { v.attackSec = *x; });
    helper.addParam(*PhaseParamId::kDecay, time("Decay", 0.5f, *MidiCCs::kAmpEnvDecay), [](V& v, X x) { v.decaySec = *x; });
    helper.addParam(*PhaseParamId::kSustain, unit("Sustain", 0.8f, *MidiCCs::kAmpEnvSustain), [](V& v, X x) { v.sustainLevel = *x; });
    helper.addParam(*PhaseParamId::kRelease, time("Release", 0.3f, *MidiCCs::kAmpEnvRelease), [](V& v, X x) { v.releaseSec = *x; });
    helper.addParam(*PhaseParamId::kVibratoRate,
                    { .displayName = "Vibrato Rate",
                      .defaultValue = 5.0f,
                      .minValue = 0.05f,
                      .maxValue = 20.0f,
                      .editCurve = Param::Curve::kExponential,
                      .midiCC = *MidiCCs::kLfo1Frequency,
                      .valueFormatter = createFrequencyFormatter() },
                    [](V& v, X x) { v.vibratoRateHz = *x; });
    helper.addParam(*PhaseParamId::kVibratoDepth,
                    { .displayName = "Vibrato Depth",
                      .defaultValue = 0.0f,
                      .minValue = 0.0f,
                      .maxValue = 2.0f,
                      .valueFormatter = [](const Patch&, Patch::paramvalue_t value)
                      { return fmt::format("{:.2f} st", value); } },
                    [](V& v, X x) { v.vibratoDepth = *x; });
    helper.addParam(*PhaseParamId::kTune,
                    { .displayName = "Tune",
                      .defaultValue = 0.0f,
                      .minValue = -24,
                      .maxValue = 24,
                      .valueFormatter = createSemitoneFormatter() },
                    [](V& v, X x) { v.tuneSemitones = std::round(*x); });
    helper.addParam(*PhaseParamId::kFine,
                    { .displayName = "Fine",
                      .defaultValue = 0.0f,
                      .minValue = -100,
                      .maxValue = 100,
                      .valueFormatter = createCentsFormatter() },
                    [](V& v, X x) { v.fineCents = *x; });
    helper.addParam(*PhaseParamId::kPlayMode,
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
    helper.addParam(*PhaseParamId::kGlideTime,
                    { .displayName = "Glide Time",
                      .defaultValue = 0.0f,
                      .minValue = 0.0f,
                      .maxValue = 2.0f,
                      .editCurve = Param::Curve::kExponential,
                      .valueFormatter = createTimeFormatter() },
                    [](V& v, X x) { v.glideTimeSec = *x; });
    helper.addParam(*PhaseParamId::kVolume,
                    { .displayName = "Volume",
                      .defaultValue = 1.0f,
                      .minValue = 0.0f,
                      .maxValue = 2.0f,
                      .macroOperation = Param::MacroOperation::kMult,
                      .valueFormatter = createPercentFormatter() },
                    [](V& v, X x) { v.volume = *x; });

    using Element = EditorElement;
    auto p = [](PhaseParamId id) { return Element{ .type = Element::Type::kParam, .id = *id }; };
    metadata.layoutFlat = Patch::Metadata::EditorLayout{
        .sections = {
          { .name = "Line 1",
            .shortName = "Line 1",
            .elements = { p(PhaseParamId::kWaveA1), p(PhaseParamId::kWaveB1), p(PhaseParamId::kDcw1),
                          p(PhaseParamId::kDcwEnv1), p(PhaseParamId::kDcwAttack1), p(PhaseParamId::kDcwDecay1),
                          p(PhaseParamId::kDcwSustain1), p(PhaseParamId::kDcwRelease1) } },
          { .name = "Line 2",
            .shortName = "Line 2",
            .elements = { p(PhaseParamId::kWaveA2), p(PhaseParamId::kWaveB2), p(PhaseParamId::kDcw2),
                          p(PhaseParamId::kDcwEnv2), p(PhaseParamId::kDcwAttack2), p(PhaseParamId::kDcwDecay2),
                          p(PhaseParamId::kDcwSustain2), p(PhaseParamId::kDcwRelease2) } },
          { .name = "Lines",
            .shortName = "Mix",
            .elements = { p(PhaseParamId::kLineMode), p(PhaseParamId::kLevel2), p(PhaseParamId::kDetuneSemis),
                          p(PhaseParamId::kDetuneCents), p(PhaseParamId::kNoise), p(PhaseParamId::kKeyFollow) } },
          { .name = "Amplifier",
            .shortName = "Amp",
            .elements = { p(PhaseParamId::kAttack), p(PhaseParamId::kDecay), p(PhaseParamId::kSustain),
                          p(PhaseParamId::kRelease), p(PhaseParamId::kVibratoRate),
                          p(PhaseParamId::kVibratoDepth) } },
          { .name = "Voice",
            .shortName = "Voice",
            .elements = { p(PhaseParamId::kTune), p(PhaseParamId::kFine), p(PhaseParamId::kPlayMode),
                          p(PhaseParamId::kGlideTime), p(PhaseParamId::kVolume) } },
        },
    };
    metadata.layout23 = metadata.layoutFlat;

    metadata.initDefaultMacros = [](auto& macros)
    {
        macros[0] = Patch::Metadata::Macro{ .displayName = "DCW",
                                            .modifiers = { { *PhaseParamId::kDcw1, 1.0f },
                                                           { *PhaseParamId::kDcw2, 1.0f } },
                                            .bipolar = true };
        macros[1] = Patch::Metadata::Macro{ .displayName = "DCW Env",
                                            .modifiers = { { *PhaseParamId::kDcwEnv1, 1.0f },
                                                           { *PhaseParamId::kDcwEnv2, 1.0f } },
                                            .bipolar = true };
        macros[2] = Patch::Metadata::Macro{ .displayName = "DCW Decay",
                                            .modifiers = { { *PhaseParamId::kDcwDecay1, 0.8f },
                                                           { *PhaseParamId::kDcwDecay2, 0.8f } },
                                            .bipolar = true };
        macros[3] = Patch::Metadata::Macro{ .displayName = "Detune",
                                            .modifiers = { { *PhaseParamId::kDetuneCents, 1.0f } },
                                            .bipolar = true };
        macros[4] = Patch::Metadata::Macro{ .displayName = "L2 Level",
                                            .modifiers = { { *PhaseParamId::kLevel2, 1.0f } },
                                            .bipolar = true };
        macros[5] = Patch::Metadata::Macro{ .displayName = "Noise",
                                            .modifiers = { { *PhaseParamId::kNoise, 1.0f } },
                                            .bipolar = false };
        macros[*Patch::SystemMacroId::kVelocity].modifiers = { { *PhaseParamId::kVolume, 1.0f } };
    };

    metadata.instrumentTypeId = static_cast<int>(InstrumentTypeId::kPhaseSynth);
    metadata.outputChannels = Patch::Metadata::OutputChannels::kMono;
    metadata.maxVoices = 8;
}

} // namespace

std::unique_ptr<InstrumentType> createPhaseSynth()
{
    return std::make_unique<InstrumentType>(
      InstrumentType{ .name = "PHASE",
                      .createMetadata = &PhaseSynthVoice::createMetadata,
                      .createVoice = [](int /*voiceIdx*/) -> std::unique_ptr<InstrumentVoice>
                      { return std::make_unique<PhaseSynthVoice>(); } });
}

} // namespace polysynth

#endif
