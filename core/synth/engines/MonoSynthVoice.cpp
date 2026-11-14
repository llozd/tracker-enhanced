// MONO: a one-oscillator monosynth voice after the Roland SH-101 (openspec pocket-rack-engines).
// Ported from Pocket Rack's eng_sh101.c (llozd). A saw, a pulse with its width, a sub oscillator
// and noise into a resonant four-pole low-pass. One ADSR drives the filter and the amplifier, as
// on the original; an LFO gives vibrato, filter wobble and pulse-width modulation.

#include "config.h"
#ifdef FEATURE_SYNTH

#include "MonoSynthVoice.h"
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
enum class MonoParamId : int
{
    kSaw = 0,
    kPulse,
    kSub,
    kNoise,
    kSubMode,
    kPulseWidth,
    kCutoff,
    kResonance,
    kEnvAmount,
    kKeyTrack,
    kLfoWave,
    kLfoRate,
    kLfoPitch,
    kLfoFilter,
    kLfoPw,
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

constexpr std::array<const char*, 3> kSubModeNames = { "1 Oct Square", "2 Oct Square", "2 Oct Pulse" };
constexpr std::array<const char*, 5> kLfoWaveNames = { "Sine", "Triangle", "Saw", "Square", "S&H" };

using namespace enginekit;

inline float wrap01(float x)
{
    return x - std::floor(x);
}

// Band-limited pulse, high for phase < duty.
inline float pulse(float ph, float inc, float duty)
{
    return (ph < duty ? 1.0f : -1.0f) + polyBlep(ph, inc) - polyBlep(wrap01(ph - duty + 1.0f), inc);
}

class MonoSynthVoice : public sdk::CustomSynthVoice<MonoSynthVoice, MonoParamId>
{
  public:
    MonoSynthVoice() = default;

    bool isActive() final { return amp.isActive(); }

    void onNoteOn(const InstrumentVoiceContext& /*ctx*/,
                  note_t note,
                  float /*velocity*/,
                  bool isLegatoStep) final
    {
        if (!amp.isActive())
        {
            s0 = s1 = s2 = s3 = y4 = dc = 0.0f;
            gPrev = 0.1f;
            subPhase = 0.0f;
        }
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
        if (!amp.isActive())
        {
            std::fill(out.begin(), out.end(), 0.0f);
            return;
        }

        amp.set(attackSec, decaySec, sustainLevel, releaseSec);
        const float lfoValue = lfo.advance(lfoRateHz, lfoWave, n);
        const float note = glide.advance(n) + tuneSemitones + fineCents * 0.01f +
                           pitchBendSemitones + lfoValue * lfoPitch * 2.0f;
        const float inc = std::min(0.45f, 440.0f * sdk::fastPow2((note - 69.0f) / 12.0f) / kSampleRate);

        const float pw = std::clamp(pulseWidth + lfoValue * lfoPw * 0.5f, 0.0f, 1.0f);
        const float duty = 0.5f - 0.45f * pw;

        // Filter: 30 Hz to about 8.7 kHz over the cutoff range, plus keyboard, envelope and LFO.
        float oct = cutoff * 8.2f + keyTrack * (note - 60.0f) / 12.0f +
                    envAmount * 8.0f * amp.value() + lfoValue * lfoFilter * 4.0f;
        const float fc = std::clamp(30.0f * sdk::fastPow2(oct), 20.0f, 18000.0f);
        const float g = std::tan(3.14159265f * fc / kSampleRate);
        const float G = g / (1.0f + g);
        const float dG = (G - gPrev) / static_cast<float>(n);
        const float k = resonance * 4.05f;
        const float inGain = 1.0f + 0.3f * k; // the bass the resonance takes away
        const float level = 0.5f * volume;

        const float subInc = subMode == 0 ? inc * 0.5f : inc * 0.25f;
        const float subDuty = subMode == 2 ? 0.25f : 0.5f;

        float Gc = gPrev;
        for (uint32_t i = 0; i < n; i++)
        {
            const float saw = 2.0f * phase - 1.0f - polyBlep(phase, inc);
            const float pul = pulse(phase, inc, duty);
            const float sub = pulse(subPhase, subInc, subDuty);
            const float x =
              (sawLevel * saw + pulseLevel * pul + subLevel * sub + noiseLevel * noise.next()) * 0.75f;

            const float u = softTanh(x * inGain - k * y4);
            Gc += dG;
            float v = (u - s0) * Gc;
            const float y1 = v + s0;
            s0 = y1 + v;
            v = (y1 - s1) * Gc;
            const float y2 = v + s1;
            s1 = y2 + v;
            v = (y2 - s2) * Gc;
            const float y3 = v + s2;
            s2 = y3 + v;
            v = (y3 - s3) * Gc;
            y4 = v + s3;
            s3 = y4 + v;

            dc += 0.0014f * (y4 - dc); // output coupling
            float y = (y4 - dc) * 2.5f; // make-up gain, a soft knee over the peaks
            const float ya = std::fabs(y);
            if (ya > 1.0f)
                y = (y < 0.0f ? -1.0f : 1.0f) * (1.0f + 0.6f * softTanh((ya - 1.0f) * (1.0f / 0.6f)));

            out[i] = y * amp.process() * level;

            phase += inc;
            if (phase >= 1.0f)
                phase -= 1.0f;
            subPhase += subInc;
            if (subPhase >= 1.0f)
                subPhase -= 1.0f;
        }
        gPrev = Gc;
    }

    static void createMetadata(const Patch& /*patch*/, Patch::Metadata& metadata)
    {
        using namespace polysynth::sdk;
        using Param = Patch::Metadata::Param;

        metadata.params.resize(enumutil::EnumMax<MonoParamId>::value + 1);
        metadata.params.shrink_to_fit();
        if (paramHandlers.empty())
            paramHandlers.resize(enumutil::EnumMax<MonoParamId>::value + 1);
        ParamHelper<MonoSynthVoice> helper(metadata, paramHandlers);

        auto level = [](const char* name, float def, int cc) {
            return Param{ .displayName = name,
                          .defaultValue = def,
                          .minValue = 0.0f,
                          .maxValue = 1.0f,
                          .midiCC = static_cast<uint8_t>(cc),
                          .valueFormatter = createPercentFormatter() };
        };
        auto time = [](const char* name, float def, int cc) {
            return Param{ .displayName = name,
                          .defaultValue = def,
                          .minValue = 0.0f,
                          .maxValue = 5.0f,
                          .editCurve = Param::Curve::kExponential,
                          .midiCC = static_cast<uint8_t>(cc),
                          .valueFormatter = createTimeFormatter() };
        };
        const int osc = *MidiCCs::kOscParam0;

        helper.addParam(*MonoParamId::kSaw, level("Saw", 0.79f, osc),
                        [](auto& v, Patch::optional_paramvalue_t x) { v.sawLevel = *x; });
        helper.addParam(*MonoParamId::kPulse, level("Pulse", 0.0f, osc + 1),
                        [](auto& v, Patch::optional_paramvalue_t x) { v.pulseLevel = *x; });
        helper.addParam(*MonoParamId::kSub, level("Sub", 0.63f, osc + 2),
                        [](auto& v, Patch::optional_paramvalue_t x) { v.subLevel = *x; });
        helper.addParam(*MonoParamId::kNoise, level("Noise", 0.0f, osc + 3),
                        [](auto& v, Patch::optional_paramvalue_t x) { v.noiseLevel = *x; });
        helper.addParam(
          *MonoParamId::kSubMode,
          { .displayName = "Sub Mode",
            .defaultValue = 0,
            .minValue = 0,
            .maxValue = static_cast<Patch::paramvalue_short_t>(kSubModeNames.size() - 1),
            .isEnum = true,
            .midiCC = static_cast<uint8_t>(osc + 4),
            .valueFormatter = [](const Patch&, Patch::paramvalue_t value)
            { return std::string(kSubModeNames[std::clamp<int>(std::lround(value), 0, 2)]); } },
          [](auto& v, Patch::optional_paramvalue_t x) { v.subMode = std::clamp<int>(std::lround(*x), 0, 2); });
        helper.addParam(*MonoParamId::kPulseWidth, level("Pulse Width", 0.0f, osc + 5),
                        [](auto& v, Patch::optional_paramvalue_t x) { v.pulseWidth = *x; });

        helper.addParam(*MonoParamId::kCutoff, level("Cutoff", 0.32f, *MidiCCs::kFilterCutoff),
                        [](auto& v, Patch::optional_paramvalue_t x) { v.cutoff = *x; });
        helper.addParam(*MonoParamId::kResonance, level("Resonance", 0.24f, *MidiCCs::kFilterResonance),
                        [](auto& v, Patch::optional_paramvalue_t x) { v.resonance = *x; });
        helper.addParam(*MonoParamId::kEnvAmount,
                        { .displayName = "Env Amount",
                          .defaultValue = 0.3f,
                          .minValue = -1.0f,
                          .maxValue = 1.0f,
                          .midiCC = *MidiCCs::kFilterEnvAmount,
                          .valueFormatter = createPercentFormatter() },
                        [](auto& v, Patch::optional_paramvalue_t x) { v.envAmount = *x; });
        helper.addParam(*MonoParamId::kKeyTrack, level("Key Track", 0.5f, osc + 6),
                        [](auto& v, Patch::optional_paramvalue_t x) { v.keyTrack = *x; });

        helper.addParam(
          *MonoParamId::kLfoWave,
          { .displayName = "LFO Wave",
            .defaultValue = 1,
            .minValue = 0,
            .maxValue = static_cast<Patch::paramvalue_short_t>(kLfoWaveNames.size() - 1),
            .isEnum = true,
            .midiCC = static_cast<uint8_t>(osc + 7),
            .valueFormatter = [](const Patch&, Patch::paramvalue_t value)
            { return std::string(kLfoWaveNames[std::clamp<int>(std::lround(value), 0, 4)]); } },
          [](auto& v, Patch::optional_paramvalue_t x)
          { v.lfoWave = static_cast<Lfo::Wave>(std::clamp<int>(std::lround(*x), 0, 4)); });
        helper.addParam(*MonoParamId::kLfoRate,
                        { .displayName = "LFO Rate",
                          .defaultValue = 4.0f,
                          .minValue = 0.05f,
                          .maxValue = 20.0f,
                          .editCurve = Param::Curve::kExponential,
                          .midiCC = *MidiCCs::kLfo1Frequency,
                          .valueFormatter = createFrequencyFormatter() },
                        [](auto& v, Patch::optional_paramvalue_t x) { v.lfoRateHz = *x; });
        helper.addParam(*MonoParamId::kLfoPitch, level("LFO > Pitch", 0.0f, osc + 8),
                        [](auto& v, Patch::optional_paramvalue_t x) { v.lfoPitch = *x; });
        helper.addParam(*MonoParamId::kLfoFilter, level("LFO > Filter", 0.0f, *MidiCCs::kFilterLfoAmount),
                        [](auto& v, Patch::optional_paramvalue_t x) { v.lfoFilter = *x; });
        helper.addParam(*MonoParamId::kLfoPw, level("LFO > PW", 0.0f, osc + 9),
                        [](auto& v, Patch::optional_paramvalue_t x) { v.lfoPw = *x; });

        helper.addParam(*MonoParamId::kAttack, time("Attack", 0.003f, *MidiCCs::kAmpEnvAttack),
                        [](auto& v, Patch::optional_paramvalue_t x) { v.attackSec = *x; });
        helper.addParam(*MonoParamId::kDecay, time("Decay", 0.4f, *MidiCCs::kAmpEnvDecay),
                        [](auto& v, Patch::optional_paramvalue_t x) { v.decaySec = *x; });
        helper.addParam(*MonoParamId::kSustain, level("Sustain", 0.5f, *MidiCCs::kAmpEnvSustain),
                        [](auto& v, Patch::optional_paramvalue_t x) { v.sustainLevel = *x; });
        helper.addParam(*MonoParamId::kRelease, time("Release", 0.15f, *MidiCCs::kAmpEnvRelease),
                        [](auto& v, Patch::optional_paramvalue_t x) { v.releaseSec = *x; });

        helper.addParam(*MonoParamId::kTune,
                        { .displayName = "Tune",
                          .defaultValue = 0.0f,
                          .minValue = -24,
                          .maxValue = 24,
                          .midiCC = static_cast<uint8_t>(osc + 10),
                          .valueFormatter = createSemitoneFormatter() },
                        [](auto& v, Patch::optional_paramvalue_t x) { v.tuneSemitones = std::round(*x); });
        helper.addParam(*MonoParamId::kFine,
                        { .displayName = "Fine",
                          .defaultValue = 0.0f,
                          .minValue = -100,
                          .maxValue = 100,
                          .midiCC = static_cast<uint8_t>(osc + 11),
                          .valueFormatter = createCentsFormatter() },
                        [](auto& v, Patch::optional_paramvalue_t x) { v.fineCents = *x; });
        helper.addParam(
          *MonoParamId::kPlayMode,
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
          [](auto& v, Patch::optional_paramvalue_t x)
          { v.playMode = static_cast<PlayMode>(std::clamp<int>(std::lround(*x), 0, 2)); });
        helper.addParam(*MonoParamId::kGlideTime,
                        { .displayName = "Glide Time",
                          .defaultValue = 0.0f,
                          .minValue = 0.0f,
                          .maxValue = 2.0f,
                          .editCurve = Param::Curve::kExponential,
                          .valueFormatter = createTimeFormatter() },
                        [](auto& v, Patch::optional_paramvalue_t x) { v.glideTimeSec = *x; });
        helper.addParam(*MonoParamId::kVolume,
                        { .displayName = "Volume",
                          .defaultValue = 1.0f,
                          .minValue = 0.0f,
                          .maxValue = 2.0f,
                          .macroOperation = Param::MacroOperation::kMult,
                          .valueFormatter = createPercentFormatter() },
                        [](auto& v, Patch::optional_paramvalue_t x) { v.volume = *x; });

        using Element = EditorElement;
        auto p = [](MonoParamId id) { return Element{ .type = Element::Type::kParam, .id = *id }; };
        metadata.layoutFlat = Patch::Metadata::EditorLayout{
            .sections = {
              { .name = "Oscillator",
                .shortName = "Osc",
                .elements = { p(MonoParamId::kSaw), p(MonoParamId::kPulse), p(MonoParamId::kSub),
                              p(MonoParamId::kNoise), p(MonoParamId::kSubMode),
                              p(MonoParamId::kPulseWidth) } },
              { .name = "Filter",
                .shortName = "Filter",
                .elements = { p(MonoParamId::kCutoff), p(MonoParamId::kResonance),
                              p(MonoParamId::kEnvAmount), p(MonoParamId::kKeyTrack) } },
              { .name = "LFO",
                .shortName = "LFO",
                .elements = { p(MonoParamId::kLfoWave), p(MonoParamId::kLfoRate),
                              p(MonoParamId::kLfoPitch), p(MonoParamId::kLfoFilter),
                              p(MonoParamId::kLfoPw) } },
              { .name = "Envelope",
                .shortName = "Env",
                .elements = { p(MonoParamId::kAttack), p(MonoParamId::kDecay),
                              p(MonoParamId::kSustain), p(MonoParamId::kRelease) } },
              { .name = "Voice",
                .shortName = "Voice",
                .elements = { p(MonoParamId::kTune), p(MonoParamId::kFine),
                              p(MonoParamId::kPlayMode), p(MonoParamId::kGlideTime),
                              p(MonoParamId::kVolume) } },
            },
        };
        metadata.layout23 = metadata.layoutFlat;

        metadata.initDefaultMacros = [](auto& macros)
        {
            macros[0] = Patch::Metadata::Macro{ .displayName = "Cutoff",
                                                .modifiers = { { *MonoParamId::kCutoff, 1.0f } },
                                                .bipolar = true };
            macros[1] = Patch::Metadata::Macro{ .displayName = "Resonance",
                                                .modifiers = { { *MonoParamId::kResonance, 1.0f } },
                                                .bipolar = true };
            macros[2] = Patch::Metadata::Macro{ .displayName = "Env Amount",
                                                .modifiers = { { *MonoParamId::kEnvAmount, 1.0f } },
                                                .bipolar = true };
            macros[3] = Patch::Metadata::Macro{ .displayName = "Pulse Width",
                                                .modifiers = { { *MonoParamId::kPulseWidth, 1.0f } },
                                                .bipolar = true };
            macros[4] = Patch::Metadata::Macro{ .displayName = "Sub / Noise",
                                                .modifiers = { { *MonoParamId::kSub, 0.8f },
                                                               { *MonoParamId::kNoise, 0.5f } },
                                                .bipolar = true };
            macros[5] = Patch::Metadata::Macro{ .displayName = "Decay / Release",
                                                .modifiers = { { *MonoParamId::kDecay, 0.6f },
                                                               { *MonoParamId::kRelease, 0.6f } },
                                                .bipolar = true };
            macros[*Patch::SystemMacroId::kVelocity].modifiers = { { *MonoParamId::kVolume, 1.0f } };
        };

        metadata.instrumentTypeId = static_cast<int>(InstrumentTypeId::kMonoSynth);
        metadata.outputChannels = Patch::Metadata::OutputChannels::kMono;
        metadata.maxVoices = 4;
    }

  private:
    // Parameters
    float sawLevel = 0.79f, pulseLevel = 0.0f, subLevel = 0.63f, noiseLevel = 0.0f;
    int subMode = 0;
    float pulseWidth = 0.0f;
    float cutoff = 0.32f, resonance = 0.24f, envAmount = 0.3f, keyTrack = 0.5f;
    Lfo::Wave lfoWave = Lfo::Wave::kTriangle;
    float lfoRateHz = 4.0f, lfoPitch = 0.0f, lfoFilter = 0.0f, lfoPw = 0.0f;
    float attackSec = 0.003f, decaySec = 0.4f, sustainLevel = 0.5f, releaseSec = 0.15f;
    float tuneSemitones = 0.0f, fineCents = 0.0f, glideTimeSec = 0.0f, volume = 1.0f;

    // State
    Adsr amp;
    Lfo lfo;
    Glide glide;
    Noise noise;
    float phase = 0.0f, subPhase = 0.0f;
    float s0 = 0.0f, s1 = 0.0f, s2 = 0.0f, s3 = 0.0f, y4 = 0.0f, dc = 0.0f, gPrev = 0.1f;
};

} // namespace

std::unique_ptr<InstrumentType> createMonoSynth()
{
    return std::make_unique<InstrumentType>(
      InstrumentType{ .name = "MONO",
                      .createMetadata = &MonoSynthVoice::createMetadata,
                      .createVoice = [](int /*voiceIdx*/) -> std::unique_ptr<InstrumentVoice>
                      { return std::make_unique<MonoSynthVoice>(); } });
}

} // namespace polysynth

#endif
