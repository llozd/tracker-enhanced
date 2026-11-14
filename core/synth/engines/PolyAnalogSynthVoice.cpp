// POLY: an analog polysynth voice in the manner of the Sequential Prophet-5 (openspec pocket-rack-engines).
// Written for this firmware from the instrument's published architecture (its manuals and the
// widely documented signal flow): two oscillators with sync, a mixer with noise, Poly-Mod (the
// filter envelope and oscillator B modulating oscillator A's pitch and width and the filter), a
// four-pole low-pass, filter and amplifier envelopes, and an LFO. It uses no code from Pocket
// Rack's Prophet engine, and no factory programs.

#include "config.h"
#ifdef FEATURE_SYNTH

#include "PolyAnalogSynthVoice.h"
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
enum class PolyParamId : int
{
    kShapeA = 0,
    kPulseWidthA,
    kTuneA,
    kSync,
    kShapeB,
    kPulseWidthB,
    kTuneB,
    kFineB,
    kLowFreqB,
    kKeyTrackB,
    kLevelA,
    kLevelB,
    kNoise,
    kPolyModEnv,
    kPolyModOscB,
    kPolyModFreqA,
    kPolyModPwA,
    kPolyModFilter,
    kCutoff,
    kResonance,
    kEnvAmount,
    kKeyTrack,
    kFilterAttack,
    kFilterDecay,
    kFilterSustain,
    kFilterRelease,
    kAttack,
    kDecay,
    kSustain,
    kRelease,
    kLfoRate,
    kLfoWave,
    kLfoPitch,
    kLfoPw,
    kLfoFilter,
    kDrift,
    kTune,
    kFine,
    kPlayMode,
    kGlideTime,
    kVolume,
};

enum ShapeA : int
{
    kASaw,
    kAPulse,
    kASawPulse,
};
constexpr std::array<const char*, 3> kShapeANames = { "Saw", "Pulse", "Saw + Pulse" };

enum ShapeB : int
{
    kBSaw,
    kBTriangle,
    kBPulse,
    kBSawPulse,
    kBTrianglePulse,
};
constexpr std::array<const char*, 5> kShapeBNames = { "Saw", "Triangle", "Pulse", "Saw + Pulse", "Tri + Pulse" };
constexpr std::array<const char*, 3> kLfoWaveNames = { "Triangle", "Saw", "Square" };
constexpr std::array<const char*, 3> kKeyTrackNames = { "Off", "Half", "Full" };
constexpr std::array<const char*, 2> kOffOn = { "Off", "On" };

using namespace enginekit;

inline float wrap01(float x)
{
    return x - std::floor(x);
}

inline float bandLimitedPulse(float ph, float inc, float duty)
{
    return (ph < duty ? 1.0f : -1.0f) + polyBlep(ph, inc) - polyBlep(wrap01(ph - duty + 1.0f), inc);
}

class PolyAnalogSynthVoice : public sdk::CustomSynthVoice<PolyAnalogSynthVoice, PolyParamId>
{
  public:
    explicit PolyAnalogSynthVoice(int voiceIdx)
    {
        // Each voice its own small, fixed tuning error (scaled by Drift), like a real voice card.
        noise.state = 0x9E3779B9u * static_cast<uint32_t>(voiceIdx + 1);
        driftA = noise.next();
        driftB = noise.next();
        driftCut = noise.next();
    }

    bool isActive() final { return amp.isActive(); }

    void onNoteOn(const InstrumentVoiceContext& /*ctx*/,
                  note_t note,
                  float /*velocity*/,
                  bool isLegatoStep) final
    {
        if (!amp.isActive())
        {
            s0 = s1 = s2 = s3 = y4 = dc = 0.0f;
            phaseA = phaseB = 0.0f;
        }
        glide.noteOn(static_cast<float>(note), glideTimeSec,
                     playMode != PlayMode::kLegato || isLegatoStep);
        setEnvelopes();
        amp.gateOn(!isLegatoStep);
        filterEnv.gateOn(!isLegatoStep);
    }

    void onNoteOff(const InstrumentVoiceContext& /*ctx*/, note_t /*note*/) final
    {
        amp.gateOff();
        filterEnv.gateOff();
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

        const float lfo = lfoOsc.advance(lfoRateHz, lfoWave, n);
        const float note = glide.advance(n) + tuneSemitones + fineCents * 0.01f + pitchBendSemitones;
        const float driftCents = drift * 12.0f;

        // Oscillator B (also a Poly-Mod source): low-frequency mode, keyboard tracking.
        float noteB = (keyTrackB ? note : 48.0f) + tuneB + fineB * 0.01f + driftB * driftCents * 0.01f;
        noteB += lfo * lfoPitch * 2.0f;
        float incB = lowFreqB ? 2.0f * sdk::fastPow2(tuneB / 12.0f) / kSampleRate
                              : 440.0f * sdk::fastPow2((noteB - 69.0f) / 12.0f) / kSampleRate;
        incB = std::min(incB, 0.45f);
        const float dutyB = 0.5f - 0.45f * std::clamp(pulseWidthB + lfo * lfoPw * 0.5f, 0.0f, 1.0f);

        const float noteA = note + tuneA + driftA * driftCents * 0.01f + lfo * lfoPitch * 2.0f;
        const float baseIncA = 440.0f * sdk::fastPow2((noteA - 69.0f) / 12.0f) / kSampleRate;
        const float pwA = pulseWidthA + lfo * lfoPw * 0.5f;

        // Filter: 20 Hz to about 20 kHz over the cutoff range, plus keyboard, envelope, LFO and drift.
        const float keyAmount = keyTrack == 0 ? 0.0f : keyTrack == 1 ? 0.5f : 1.0f;
        const float octBase = cutoff * 10.0f + keyAmount * (note - 60.0f) / 12.0f + lfo * lfoFilter * 4.0f +
                              driftCut * drift * 0.15f;
        const float k = resonance * 4.1f;
        const float levelA = mixA, levelB = mixB, levelNoise = mixNoise;
        const float level = 0.5f * volume;

        for (uint32_t i = 0; i < n; i++)
        {
            const float fenv = filterEnv.process();

            // Oscillator B
            float b = 0.0f;
            const bool bSaw = shapeB == kBSaw || shapeB == kBSawPulse;
            const bool bTri = shapeB == kBTriangle || shapeB == kBTrianglePulse;
            const bool bPulse = shapeB == kBPulse || shapeB == kBSawPulse || shapeB == kBTrianglePulse;
            if (bSaw)
                b += 2.0f * phaseB - 1.0f - polyBlep(phaseB, incB);
            if (bTri)
                b += phaseB < 0.5f ? 4.0f * phaseB - 1.0f : 3.0f - 4.0f * phaseB;
            if (bPulse)
                b += bandLimitedPulse(phaseB, incB, dutyB);

            // Poly-Mod: the filter envelope and oscillator B into A's pitch and width and the filter.
            const float pm = polyModEnv * fenv + polyModOscB * b * 0.5f;
            float incA = baseIncA;
            if (pmFreqA && pm != 0.0f)
                incA *= sdk::fastPow2(pm * 3.0f); // up to 3 octaves
            incA = std::min(incA, 0.45f);
            float duty = 0.5f - 0.45f * std::clamp(pwA + (pmPwA ? pm * 0.5f : 0.0f), 0.0f, 1.0f);

            float a = 0.0f;
            if (shapeA != kAPulse)
                a += 2.0f * phaseA - 1.0f - polyBlep(phaseA, incA);
            if (shapeA != kASaw)
                a += bandLimitedPulse(phaseA, incA, duty);

            const float x = (levelA * a + levelB * b + levelNoise * noise.next()) * 0.5f;

            // Four-pole low-pass (zero-delay one-poles in a resonant loop)
            float oct = octBase + envAmount * 8.0f * fenv + (pmFilter ? pm * 4.0f : 0.0f);
            const float fc = std::clamp(20.0f * sdk::fastPow2(oct), 20.0f, 18000.0f);
            const float w = 3.14159265f * fc / kSampleRate; // tan(w), Pade: within 1 % up to 18 kHz
            const float g = w * (15.0f - w * w) / (15.0f - 6.0f * w * w);
            const float G = g / (1.0f + g);
            const float u = softTanh(x * (1.0f + 0.3f * k) - k * y4);
            float v = (u - s0) * G;
            const float y1 = v + s0;
            s0 = y1 + v;
            v = (y1 - s1) * G;
            const float y2 = v + s1;
            s1 = y2 + v;
            v = (y2 - s2) * G;
            const float y3 = v + s2;
            s2 = y3 + v;
            v = (y3 - s3) * G;
            y4 = v + s3;
            s3 = y4 + v;
            dc += 0.0014f * (y4 - dc);

            float y = (y4 - dc) * 1.6f; // make-up gain, a soft knee over the peaks
            const float ya = std::fabs(y);
            if (ya > 1.0f)
                y = (y < 0.0f ? -1.0f : 1.0f) * (1.0f + 0.6f * softTanh((ya - 1.0f) * (1.0f / 0.6f)));
            out[i] = y * amp.process() * level;

            // Advance; A restarts with B in sync mode.
            phaseB += incB;
            bool wrappedB = false;
            if (phaseB >= 1.0f)
            {
                phaseB -= 1.0f;
                wrappedB = true;
            }
            phaseA += incA;
            if (sync && wrappedB)
                phaseA = phaseB * (incA / incB);
            if (phaseA >= 1.0f)
                phaseA -= std::floor(phaseA);
        }
    }

    static void createMetadata(const Patch& /*patch*/, Patch::Metadata& metadata);

  private:
    void setEnvelopes()
    {
        amp.set(attackSec, decaySec, sustainLevel, releaseSec);
        filterEnv.set(filterAttackSec, filterDecaySec, filterSustain, filterReleaseSec);
    }

    // Parameters
    int shapeA = kASaw, shapeB = kBSaw, keyTrack = 2;
    float pulseWidthA = 0.0f, tuneA = 0.0f, pulseWidthB = 0.0f, tuneB = 0.0f, fineB = 6.0f;
    bool sync = false, lowFreqB = false, keyTrackB = true;
    float mixA = 1.0f, mixB = 0.7f, mixNoise = 0.0f;
    float polyModEnv = 0.0f, polyModOscB = 0.0f;
    bool pmFreqA = false, pmPwA = false, pmFilter = false;
    float cutoff = 0.65f, resonance = 0.15f, envAmount = 0.35f;
    float filterAttackSec = 0.003f, filterDecaySec = 0.6f, filterSustain = 0.3f, filterReleaseSec = 0.4f;
    float attackSec = 0.003f, decaySec = 0.6f, sustainLevel = 0.8f, releaseSec = 0.4f;
    float lfoRateHz = 5.0f;
    Lfo::Wave lfoWave = Lfo::Wave::kTriangle;
    float lfoPitch = 0.0f, lfoPw = 0.0f, lfoFilter = 0.0f, drift = 0.3f;
    float tuneSemitones = 0.0f, fineCents = 0.0f, glideTimeSec = 0.0f, volume = 1.0f;

    // State
    Adsr amp, filterEnv;
    Lfo lfoOsc;
    Glide glide;
    Noise noise;
    float phaseA = 0.0f, phaseB = 0.0f;
    float s0 = 0.0f, s1 = 0.0f, s2 = 0.0f, s3 = 0.0f, y4 = 0.0f, dc = 0.0f;
    float driftA = 0.0f, driftB = 0.0f, driftCut = 0.0f;
};

void PolyAnalogSynthVoice::createMetadata(const Patch& /*patch*/, Patch::Metadata& metadata)
{
    using namespace polysynth::sdk;
    using Param = Patch::Metadata::Param;
    using V = PolyAnalogSynthVoice;
    using X = Patch::optional_paramvalue_t;

    metadata.params.resize(enumutil::EnumMax<PolyParamId>::value + 1);
    metadata.params.shrink_to_fit();
    if (paramHandlers.empty())
        paramHandlers.resize(enumutil::EnumMax<PolyParamId>::value + 1);
    ParamHelper<PolyAnalogSynthVoice> helper(metadata, paramHandlers);

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
    auto semis = [](const char* name, int cc) {
        return Param{ .displayName = name,
                      .defaultValue = 0.0f,
                      .minValue = -24,
                      .maxValue = 24,
                      .midiCC = static_cast<uint8_t>(cc),
                      .valueFormatter = createSemitoneFormatter() };
    };
    auto choice = [](const char* name, int def, int count, int cc) {
        return Param{ .displayName = name,
                      .defaultValue = def,
                      .minValue = 0,
                      .maxValue = static_cast<Patch::paramvalue_short_t>(count - 1),
                      .isEnum = true,
                      .midiCC = static_cast<uint8_t>(cc) };
    };
    auto withNames = [](Param p, auto names) {
        p.valueFormatter = [names](const Patch&, Patch::paramvalue_t value)
        { return std::string(names[std::clamp<int>(std::lround(value), 0, static_cast<int>(names.size()) - 1)]); };
        return p;
    };
    const int osc = *MidiCCs::kOscParam0;

    helper.addParam(*PolyParamId::kShapeA, withNames(choice("A Shape", kASaw, 3, osc), kShapeANames),
                    [](V& v, X x) { v.shapeA = std::clamp<int>(std::lround(*x), 0, 2); });
    helper.addParam(*PolyParamId::kPulseWidthA, unit("A Pulse Width", 0.0f, osc + 1), [](V& v, X x) { v.pulseWidthA = *x; });
    helper.addParam(*PolyParamId::kTuneA, semis("A Tune", osc + 2), [](V& v, X x) { v.tuneA = std::round(*x); });
    helper.addParam(*PolyParamId::kSync, withNames(choice("Sync", 0, 2, osc + 3), kOffOn),
                    [](V& v, X x) { v.sync = std::lround(*x) != 0; });
    helper.addParam(*PolyParamId::kShapeB, withNames(choice("B Shape", kBSaw, 5, osc + 4), kShapeBNames),
                    [](V& v, X x) { v.shapeB = std::clamp<int>(std::lround(*x), 0, 4); });
    helper.addParam(*PolyParamId::kPulseWidthB, unit("B Pulse Width", 0.0f, osc + 5), [](V& v, X x) { v.pulseWidthB = *x; });
    helper.addParam(*PolyParamId::kTuneB, semis("B Tune", osc + 6), [](V& v, X x) { v.tuneB = std::round(*x); });
    helper.addParam(*PolyParamId::kFineB,
                    { .displayName = "B Fine",
                      .defaultValue = 6.0f,
                      .minValue = -100,
                      .maxValue = 100,
                      .midiCC = static_cast<uint8_t>(osc + 7),
                      .valueFormatter = createCentsFormatter() },
                    [](V& v, X x) { v.fineB = *x; });
    helper.addParam(*PolyParamId::kLowFreqB, withNames(choice("B Low Freq", 0, 2, osc + 8), kOffOn),
                    [](V& v, X x) { v.lowFreqB = std::lround(*x) != 0; });
    helper.addParam(*PolyParamId::kKeyTrackB, withNames(choice("B Keyboard", 1, 2, osc + 9), kOffOn),
                    [](V& v, X x) { v.keyTrackB = std::lround(*x) != 0; });
    helper.addParam(*PolyParamId::kLevelA, unit("A Level", 1.0f, osc + 10), [](V& v, X x) { v.mixA = *x; });
    helper.addParam(*PolyParamId::kLevelB, unit("B Level", 0.7f, osc + 11), [](V& v, X x) { v.mixB = *x; });
    helper.addParam(*PolyParamId::kNoise, unit("Noise", 0.0f, *MidiCCs::kUserEnv1Attack), [](V& v, X x) { v.mixNoise = *x; });
    helper.addParam(*PolyParamId::kPolyModEnv, bipolar("PM Filt Env", 0.0f, *MidiCCs::kUserEnv1Decay), [](V& v, X x) { v.polyModEnv = *x; });
    helper.addParam(*PolyParamId::kPolyModOscB, unit("PM Osc B", 0.0f, *MidiCCs::kUserEnv1Sustain), [](V& v, X x) { v.polyModOscB = *x; });
    helper.addParam(*PolyParamId::kPolyModFreqA, withNames(choice("PM > Freq A", 0, 2, *MidiCCs::kUserEnv1Release), kOffOn),
                    [](V& v, X x) { v.pmFreqA = std::lround(*x) != 0; });
    helper.addParam(*PolyParamId::kPolyModPwA, withNames(choice("PM > PW A", 0, 2, *MidiCCs::kUserEnv2Attack), kOffOn),
                    [](V& v, X x) { v.pmPwA = std::lround(*x) != 0; });
    helper.addParam(*PolyParamId::kPolyModFilter, withNames(choice("PM > Filter", 0, 2, *MidiCCs::kUserEnv2Decay), kOffOn),
                    [](V& v, X x) { v.pmFilter = std::lround(*x) != 0; });
    helper.addParam(*PolyParamId::kCutoff, unit("Cutoff", 0.65f, *MidiCCs::kFilterCutoff), [](V& v, X x) { v.cutoff = *x; });
    helper.addParam(*PolyParamId::kResonance, unit("Resonance", 0.15f, *MidiCCs::kFilterResonance), [](V& v, X x) { v.resonance = *x; });
    helper.addParam(*PolyParamId::kEnvAmount, bipolar("Env Amount", 0.35f, *MidiCCs::kFilterEnvAmount), [](V& v, X x) { v.envAmount = *x; });
    helper.addParam(*PolyParamId::kKeyTrack, withNames(choice("Key Track", 2, 3, *MidiCCs::kUserEnv2Sustain), kKeyTrackNames),
                    [](V& v, X x) { v.keyTrack = std::clamp<int>(std::lround(*x), 0, 2); });
    helper.addParam(*PolyParamId::kFilterAttack, time("Filt Attack", 0.003f, *MidiCCs::kFilterEnvAttack), [](V& v, X x) { v.filterAttackSec = *x; });
    helper.addParam(*PolyParamId::kFilterDecay, time("Filt Decay", 0.6f, *MidiCCs::kFilterEnvDecay), [](V& v, X x) { v.filterDecaySec = *x; });
    helper.addParam(*PolyParamId::kFilterSustain, unit("Filt Sustain", 0.3f, *MidiCCs::kFilterEnvSustain), [](V& v, X x) { v.filterSustain = *x; });
    helper.addParam(*PolyParamId::kFilterRelease, time("Filt Release", 0.4f, *MidiCCs::kFilterEnvRelease), [](V& v, X x) { v.filterReleaseSec = *x; });
    helper.addParam(*PolyParamId::kAttack, time("Attack", 0.003f, *MidiCCs::kAmpEnvAttack), [](V& v, X x) { v.attackSec = *x; });
    helper.addParam(*PolyParamId::kDecay, time("Decay", 0.6f, *MidiCCs::kAmpEnvDecay), [](V& v, X x) { v.decaySec = *x; });
    helper.addParam(*PolyParamId::kSustain, unit("Sustain", 0.8f, *MidiCCs::kAmpEnvSustain), [](V& v, X x) { v.sustainLevel = *x; });
    helper.addParam(*PolyParamId::kRelease, time("Release", 0.4f, *MidiCCs::kAmpEnvRelease), [](V& v, X x) { v.releaseSec = *x; });
    helper.addParam(*PolyParamId::kLfoRate,
                    { .displayName = "LFO Rate",
                      .defaultValue = 5.0f,
                      .minValue = 0.05f,
                      .maxValue = 20.0f,
                      .editCurve = Param::Curve::kExponential,
                      .midiCC = *MidiCCs::kLfo1Frequency,
                      .valueFormatter = createFrequencyFormatter() },
                    [](V& v, X x) { v.lfoRateHz = *x; });
    helper.addParam(*PolyParamId::kLfoWave, withNames(choice("LFO Wave", 0, 3, *MidiCCs::kUserEnv2Release), kLfoWaveNames),
                    [](V& v, X x)
                    {
                        constexpr Lfo::Wave kWaves[3] = { Lfo::Wave::kTriangle, Lfo::Wave::kSaw, Lfo::Wave::kSquare };
                        v.lfoWave = kWaves[std::clamp<int>(std::lround(*x), 0, 2)];
                    });
    helper.addParam(*PolyParamId::kLfoPitch, unit("LFO > Pitch", 0.0f, *MidiCCs::kLfo2Frequency), [](V& v, X x) { v.lfoPitch = *x; });
    helper.addParam(*PolyParamId::kLfoPw, unit("LFO > PW", 0.0f, *MidiCCs::kFilterEnvVelocity), [](V& v, X x) { v.lfoPw = *x; });
    helper.addParam(*PolyParamId::kLfoFilter, unit("LFO > Filter", 0.0f, *MidiCCs::kFilterLfoAmount), [](V& v, X x) { v.lfoFilter = *x; });
    helper.addParam(*PolyParamId::kDrift, unit("Drift", 0.3f, *MidiCCs::kFilterType), [](V& v, X x) { v.drift = *x; });
    helper.addParam(*PolyParamId::kTune, semis("Tune", *MidiCCs::kBankSelectLSB), [](V& v, X x) { v.tuneSemitones = std::round(*x); });
    helper.addParam(*PolyParamId::kFine,
                    { .displayName = "Fine",
                      .defaultValue = 0.0f,
                      .minValue = -100,
                      .maxValue = 100,
                      .valueFormatter = createCentsFormatter() },
                    [](V& v, X x) { v.fineCents = *x; });
    helper.addParam(*PolyParamId::kPlayMode,
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
    helper.addParam(*PolyParamId::kGlideTime,
                    { .displayName = "Glide Time",
                      .defaultValue = 0.0f,
                      .minValue = 0.0f,
                      .maxValue = 2.0f,
                      .editCurve = Param::Curve::kExponential,
                      .valueFormatter = createTimeFormatter() },
                    [](V& v, X x) { v.glideTimeSec = *x; });
    helper.addParam(*PolyParamId::kVolume,
                    { .displayName = "Volume",
                      .defaultValue = 1.0f,
                      .minValue = 0.0f,
                      .maxValue = 2.0f,
                      .macroOperation = Param::MacroOperation::kMult,
                      .valueFormatter = createPercentFormatter() },
                    [](V& v, X x) { v.volume = *x; });

    using Element = EditorElement;
    auto p = [](PolyParamId id) { return Element{ .type = Element::Type::kParam, .id = *id }; };
    metadata.layoutFlat = Patch::Metadata::EditorLayout{
        .sections = {
          { .name = "Oscillators",
            .shortName = "Osc",
            .elements = { p(PolyParamId::kShapeA), p(PolyParamId::kPulseWidthA), p(PolyParamId::kTuneA),
                          p(PolyParamId::kSync), p(PolyParamId::kShapeB), p(PolyParamId::kPulseWidthB),
                          p(PolyParamId::kTuneB), p(PolyParamId::kFineB), p(PolyParamId::kLowFreqB),
                          p(PolyParamId::kKeyTrackB) } },
          { .name = "Mixer & Poly-Mod",
            .shortName = "Mod",
            .elements = { p(PolyParamId::kLevelA), p(PolyParamId::kLevelB), p(PolyParamId::kNoise),
                          p(PolyParamId::kPolyModEnv), p(PolyParamId::kPolyModOscB), p(PolyParamId::kPolyModFreqA),
                          p(PolyParamId::kPolyModPwA), p(PolyParamId::kPolyModFilter) } },
          { .name = "Filter",
            .shortName = "Filter",
            .elements = { p(PolyParamId::kCutoff), p(PolyParamId::kResonance), p(PolyParamId::kEnvAmount),
                          p(PolyParamId::kKeyTrack), p(PolyParamId::kFilterAttack), p(PolyParamId::kFilterDecay),
                          p(PolyParamId::kFilterSustain), p(PolyParamId::kFilterRelease) } },
          { .name = "Amp & LFO",
            .shortName = "Amp",
            .elements = { p(PolyParamId::kAttack), p(PolyParamId::kDecay), p(PolyParamId::kSustain),
                          p(PolyParamId::kRelease), p(PolyParamId::kLfoRate), p(PolyParamId::kLfoWave),
                          p(PolyParamId::kLfoPitch), p(PolyParamId::kLfoPw), p(PolyParamId::kLfoFilter) } },
          { .name = "Voice",
            .shortName = "Voice",
            .elements = { p(PolyParamId::kDrift), p(PolyParamId::kTune), p(PolyParamId::kFine),
                          p(PolyParamId::kPlayMode), p(PolyParamId::kGlideTime), p(PolyParamId::kVolume) } },
        },
    };
    metadata.layout23 = metadata.layoutFlat;

    metadata.initDefaultMacros = [](auto& macros)
    {
        macros[0] = Patch::Metadata::Macro{ .displayName = "Cutoff",
                                            .modifiers = { { *PolyParamId::kCutoff, 1.0f } },
                                            .bipolar = true };
        macros[1] = Patch::Metadata::Macro{ .displayName = "Resonance",
                                            .modifiers = { { *PolyParamId::kResonance, 1.0f } },
                                            .bipolar = true };
        macros[2] = Patch::Metadata::Macro{ .displayName = "Env Amount",
                                            .modifiers = { { *PolyParamId::kEnvAmount, 1.0f } },
                                            .bipolar = true };
        macros[3] = Patch::Metadata::Macro{ .displayName = "Poly-Mod",
                                            .modifiers = { { *PolyParamId::kPolyModEnv, 0.8f },
                                                           { *PolyParamId::kPolyModOscB, 0.8f } },
                                            .bipolar = true };
        macros[4] = Patch::Metadata::Macro{ .displayName = "Mod Wheel",
                                            .modifiers = { { *PolyParamId::kLfoPitch, 0.3f },
                                                           { *PolyParamId::kLfoFilter, 0.5f } },
                                            .bipolar = false };
        macros[5] = Patch::Metadata::Macro{ .displayName = "Release",
                                            .modifiers = { { *PolyParamId::kRelease, 0.8f },
                                                           { *PolyParamId::kFilterRelease, 0.8f } },
                                            .bipolar = true };
        macros[*Patch::SystemMacroId::kVelocity].modifiers = { { *PolyParamId::kVolume, 1.0f } };
    };

    metadata.instrumentTypeId = static_cast<int>(InstrumentTypeId::kPolyAnalogSynth);
    metadata.outputChannels = Patch::Metadata::OutputChannels::kMono;
    metadata.maxVoices = 8;
}

} // namespace

std::unique_ptr<InstrumentType> createPolyAnalogSynth()
{
    return std::make_unique<InstrumentType>(
      InstrumentType{ .name = "POLY",
                      .createMetadata = &PolyAnalogSynthVoice::createMetadata,
                      .createVoice = [](int voiceIdx) -> std::unique_ptr<InstrumentVoice>
                      { return std::make_unique<PolyAnalogSynthVoice>(voiceIdx); } });
}

} // namespace polysynth

#endif
