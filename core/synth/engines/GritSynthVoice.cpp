// GRIT: a digital wave module after the Cheetah MS800 (openspec synth-engines).
// No filter, as on the original: 8-bit companded
// single-cycle waves played without interpolation, so they alias and grit. A tone steps through
// the waves (each step a wave and a level, held for a time and crossfaded into the next), then
// ends, loops or turns back. A second tone can be layered, transposed and detuned.

#include "config.h"
#ifdef FEATURE_SYNTH

#include "GritSynthVoice.h"
#include "EngineKit.h"
#include "GritWaves.h"
#include "PolySynth/sdk/SynthVoiceSDK.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace polysynth
{
namespace
{

// Stored in patches by position: append only. core/synth/presets/gen_presets.py mirrors this order.
enum class GritParamId : int
{
    kWave = 0,
    kSteps,
    kStepTime,
    kCrossfade,
    kJump,
    kLoop,
    kLevelStep,
    kRandom,
    kLayer,
    kLayerWave,
    kLayerTranspose,
    kLayerDetune,
    kLayerTime,
    kVelocityWave,
    kGrit,
    kBits,
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

enum LoopMode : int
{
    kEnd,
    kLoop,
    kPing,
};
constexpr std::array<const char*, 3> kLoopNames = { "End", "Loop", "Ping-Pong" };
constexpr int kMaxSteps = 16;

using namespace enginekit;

struct Rng
{
    uint32_t state = 0x2545F491u;
    uint32_t next()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
};

// One tone's wave sequence.
struct Sequence
{
    uint32_t t = 0;       // samples into the step
    uint8_t step = 0;
    uint8_t dir = 0;      // ping-pong: 1 going back
    uint8_t wave = 0, next = 0;
    uint32_t phase = 0;   // Q32
};

class GritSynthVoice : public sdk::CustomSynthVoice<GritSynthVoice, GritParamId>
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
            held = 0;
            hold = 0;
            a.phase = b.phase = 0;
        }
        firstWave = wave + static_cast<int>(velocity * velocityWave * 10.0f);
        start(a, firstWave);
        start(b, firstWave + layerWave);
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

        const float vib = vibratoLfo.advance(vibratoRateHz, Lfo::Wave::kSine, n) * vibratoDepth;
        const float note =
          glide.advance(n) + tuneSemitones + fineCents * 0.01f + pitchBendSemitones + vib;
        const float freq = 440.0f * sdk::fastPow2((note - 69.0f) / 12.0f);
        const uint32_t inc = toInc(freq);
        const uint32_t len = static_cast<uint32_t>(stepTimeSec * kSampleRate) + 1u;

        int32_t y[128];
        const uint32_t count = std::min<uint32_t>(n, 128);
        std::fill(y, y + count, 0);
        const int32_t layerQ = static_cast<int32_t>(layer * 127.0f);
        tone(a, firstWave, len, inc, 32767 * 127 / (127 + layerQ), y, count);
        if (layerQ > 0)
        {
            const float ratio = sdk::fastPow2((layerTranspose * 100.0f + layerDetune) / 1200.0f);
            const uint32_t lenB = static_cast<uint32_t>(static_cast<float>(len) * sdk::fastPow2(layerTime * 2.0f)) + 1u;
            tone(b, firstWave + layerWave, lenB, toInc(freq * ratio), 32767 * layerQ / (127 + layerQ), y, count);
        }

        const uint32_t every = 1u + static_cast<uint32_t>(gritAmount * 127.0f) / 8u;
        const uint32_t mask = ~((1u << (static_cast<uint32_t>(bits * 127.0f) * 13u / 127u)) - 1u);
        const float level = volume * (0.5f / 32768.0f); // a full-scale wave at -6 dBFS, like the other engines
        for (uint32_t i = 0; i < count; i++)
        {
            int32_t s = y[i];
            if (every > 1u) // a sample held for a few
            {
                if (!hold)
                {
                    held = s;
                    hold = every;
                }
                hold--;
                s = held;
            }
            s = static_cast<int32_t>(static_cast<uint32_t>(std::clamp<int32_t>(s, -32768, 32767)) & mask);
            out[i] = static_cast<float>(static_cast<int16_t>(s)) * level * amp.process();
        }
    }

    static void createMetadata(const Patch& /*patch*/, Patch::Metadata& metadata);

  private:
    static uint32_t toInc(float freq)
    {
        return static_cast<uint32_t>(std::min(freq / kSampleRate, 0.49f) * 4294967296.0f);
    }

    // The step after k (dir: ping-pong's direction, updated).
    uint32_t after(uint32_t k, uint8_t& dir) const
    {
        const uint32_t n = static_cast<uint32_t>(steps);
        if (n < 2u)
            return 0;
        if (loopMode == kLoop)
            return (k + 1u) % n;
        if (loopMode == kEnd)
            return k + 1u < n ? k + 1u : k;
        if (dir ? k == 0u : k + 1u >= n)
            dir ^= 1u;
        return dir ? k - 1u : k + 1u;
    }

    // The wave of step k from the first wave w0, or a random one.
    uint8_t waveOf(int w0, uint32_t k)
    {
        const uint32_t r = rng.next();
        if (random > 0.0f && static_cast<float>(r >> 25) < random * 127.0f)
            return static_cast<uint8_t>((r >> 8) % grit::kWaveCount);
        int w = (w0 + static_cast<int>(k) * jump) % grit::kWaveCount;
        return static_cast<uint8_t>(w < 0 ? w + grit::kWaveCount : w);
    }

    void start(Sequence& q, int w0)
    {
        uint8_t dir = 0;
        q.t = 0;
        q.step = 0;
        q.dir = 0;
        q.wave = waveOf(w0, 0);
        q.next = waveOf(w0, after(0, dir));
    }

    // How far into the crossfade (the last xf samples of a step of len) sample t is, Q15.
    static int32_t fade(uint32_t t, uint32_t len, uint32_t xf)
    {
        t = std::min(t, len);
        return xf && t + xf > len ? static_cast<int32_t>((static_cast<uint64_t>(t + xf - len) << 15) / xf) : 0;
    }

    float stepLevel(uint32_t k) const
    {
        const float s = levelStep;
        return sdk::fastPow2(s * (static_cast<float>(k) - (s > 0.0f ? static_cast<float>(steps - 1) : 0.0f)));
    }

    void tone(Sequence& q, int w0, uint32_t len, uint32_t inc, int32_t gain, int32_t* y, uint32_t n)
    {
        uint32_t xf = static_cast<uint32_t>(crossfade * 127.0f) * len / 127u;
        if (q.t >= len) // the step moves on at its end
        {
            q.t = q.t - len < len ? q.t - len : 0;
            q.step = static_cast<uint8_t>(after(q.step, q.dir));
            uint8_t dir = q.dir;
            q.wave = q.next;
            q.next = waveOf(w0, after(q.step, dir));
        }
        if (q.next == q.wave && levelStep == 0.0f)
            xf = 0;
        const int32_t x0 = fade(q.t, len, xf);
        q.t += n;
        const int32_t x1 = fade(q.t, len, xf);

        uint8_t dir = q.dir;
        const int32_t la = static_cast<int32_t>(stepLevel(q.step) * static_cast<float>(gain));
        const int32_t lb = static_cast<int32_t>(stepLevel(after(q.step, dir)) * static_cast<float>(gain));
        const uint8_t* wa = grit::kWaves[q.wave];
        const uint8_t* wb = grit::kWaves[q.next];
        uint32_t f = q.phase;
        if (!x0 && !x1)
        {
            for (uint32_t i = 0; i < n; i++, f += inc)
                y[i] += (grit::kMuLaw[wa[f >> 24]] * la) >> 15;
        }
        else
        {
            const int32_t dx = (x1 - x0) / static_cast<int32_t>(n);
            for (uint32_t i = 0; i < n; i++, f += inc)
            {
                const int32_t x = x0 + dx * static_cast<int32_t>(i);
                const int32_t va = (grit::kMuLaw[wa[f >> 24]] * la) >> 15;
                const int32_t vb = (grit::kMuLaw[wb[f >> 24]] * lb) >> 15;
                y[i] += va + (((vb - va) * x) >> 15);
            }
        }
        q.phase = f;
    }

    // Parameters
    int wave = 2, steps = 1, jump = 1, layerWave = 0;
    LoopMode loopMode = kLoop;
    float stepTimeSec = 0.078f, crossfade = 0.5f, levelStep = 0.0f, random = 0.0f;
    float layer = 0.0f, layerTranspose = 0.0f, layerDetune = 0.0f, layerTime = 0.0f;
    float velocityWave = 0.0f, gritAmount = 0.0f, bits = 0.0f;
    float attackSec = 0.003f, decaySec = 0.4f, sustainLevel = 0.85f, releaseSec = 0.2f;
    float vibratoRateHz = 5.0f, vibratoDepth = 0.0f;
    float tuneSemitones = 0.0f, fineCents = 0.0f, glideTimeSec = 0.0f, volume = 1.0f;

    // State
    Adsr amp;
    Lfo vibratoLfo;
    Glide glide;
    Rng rng;
    Sequence a, b;
    int firstWave = 2;
    int32_t held = 0;
    uint32_t hold = 0;
};

void GritSynthVoice::createMetadata(const Patch& /*patch*/, Patch::Metadata& metadata)
{
    using namespace polysynth::sdk;
    using Param = Patch::Metadata::Param;
    using V = GritSynthVoice;
    using X = Patch::optional_paramvalue_t;

    metadata.params.resize(enumutil::EnumMax<GritParamId>::value + 1);
    metadata.params.shrink_to_fit();
    if (paramHandlers.empty())
        paramHandlers.resize(enumutil::EnumMax<GritParamId>::value + 1);
    ParamHelper<GritSynthVoice> helper(metadata, paramHandlers);

    auto unit = [](const char* name, float def, int cc) {
        return Param{ .displayName = name,
                      .defaultValue = def,
                      .minValue = 0.0f,
                      .maxValue = 1.0f,
                      .midiCC = static_cast<uint8_t>(cc),
                      .valueFormatter = createPercentFormatter() };
    };
    auto integer = [](const char* name, float def, float lo, float hi, int cc) {
        return Param{ .displayName = name,
                      .defaultValue = def,
                      .minValue = static_cast<Patch::paramvalue_short_t>(lo),
                      .maxValue = static_cast<Patch::paramvalue_short_t>(hi),
                      .midiCC = static_cast<uint8_t>(cc),
                      .valueFormatter = [](const Patch&, Patch::paramvalue_t value)
                      { return fmt::format("{:d}", static_cast<int>(std::lround(value))); } };
    };
    auto time = [](const char* name, float def, float lo, int cc) {
        return Param{ .displayName = name,
                      .defaultValue = def,
                      .minValue = static_cast<Patch::paramvalue_short_t>(lo),
                      .maxValue = 10.0f,
                      .editCurve = Param::Curve::kExponential,
                      .midiCC = static_cast<uint8_t>(cc),
                      .valueFormatter = createTimeFormatter() };
    };
    const int osc = *MidiCCs::kOscParam0;

    helper.addParam(*GritParamId::kWave,
                    { .displayName = "Wave",
                      .defaultValue = 2,
                      .minValue = 0,
                      .maxValue = grit::kWaveCount - 1,
                      .isEnum = true,
                      .midiCC = static_cast<uint8_t>(osc),
                      .valueFormatter = [](const Patch&, Patch::paramvalue_t value)
                      { return std::string(grit::kWaveNames[std::clamp<int>(std::lround(value), 0, grit::kWaveCount - 1)]); } },
                    [](V& v, X x) { v.wave = std::clamp<int>(std::lround(*x), 0, grit::kWaveCount - 1); });
    helper.addParam(*GritParamId::kSteps, integer("Steps", 1, 1, kMaxSteps, osc + 1),
                    [](V& v, X x) { v.steps = std::clamp<int>(std::lround(*x), 1, kMaxSteps); });
    helper.addParam(*GritParamId::kStepTime, time("Step Time", 0.078f, 0.001f, osc + 2),
                    [](V& v, X x) { v.stepTimeSec = *x; });
    helper.addParam(*GritParamId::kCrossfade, unit("Crossfade", 0.5f, osc + 3),
                    [](V& v, X x) { v.crossfade = *x; });
    helper.addParam(*GritParamId::kJump, integer("Jump", 1, -10, 10, osc + 4),
                    [](V& v, X x) { v.jump = std::clamp<int>(std::lround(*x), -10, 10); });
    helper.addParam(*GritParamId::kLoop,
                    { .displayName = "Loop",
                      .defaultValue = kLoop,
                      .minValue = 0,
                      .maxValue = 2,
                      .isEnum = true,
                      .midiCC = static_cast<uint8_t>(osc + 5),
                      .valueFormatter = [](const Patch&, Patch::paramvalue_t value)
                      { return std::string(kLoopNames[std::clamp<int>(std::lround(value), 0, 2)]); } },
                    [](V& v, X x) { v.loopMode = static_cast<LoopMode>(std::clamp<int>(std::lround(*x), 0, 2)); });
    helper.addParam(*GritParamId::kLevelStep,
                    { .displayName = "Level Step",
                      .defaultValue = 0.0f,
                      .minValue = -1.0f,
                      .maxValue = 1.0f,
                      .midiCC = static_cast<uint8_t>(osc + 6),
                      .valueFormatter = createPercentFormatter() },
                    [](V& v, X x) { v.levelStep = *x; });
    helper.addParam(*GritParamId::kRandom, unit("Random", 0.0f, osc + 7), [](V& v, X x) { v.random = *x; });

    helper.addParam(*GritParamId::kLayer, unit("Layer", 0.0f, osc + 8), [](V& v, X x) { v.layer = *x; });
    helper.addParam(*GritParamId::kLayerWave, integer("Layer Wave", 0, -20, 20, osc + 9),
                    [](V& v, X x) { v.layerWave = std::clamp<int>(std::lround(*x), -20, 20); });
    helper.addParam(*GritParamId::kLayerTranspose,
                    { .displayName = "Layer Trans",
                      .defaultValue = 0.0f,
                      .minValue = -24,
                      .maxValue = 24,
                      .midiCC = static_cast<uint8_t>(osc + 10),
                      .valueFormatter = createSemitoneFormatter() },
                    [](V& v, X x) { v.layerTranspose = std::round(*x); });
    helper.addParam(*GritParamId::kLayerDetune,
                    { .displayName = "Layer Detune",
                      .defaultValue = 0.0f,
                      .minValue = -50,
                      .maxValue = 50,
                      .midiCC = static_cast<uint8_t>(osc + 11),
                      .valueFormatter = createCentsFormatter() },
                    [](V& v, X x) { v.layerDetune = *x; });
    helper.addParam(*GritParamId::kLayerTime,
                    { .displayName = "Layer Time",
                      .defaultValue = 0.0f,
                      .minValue = -1.0f,
                      .maxValue = 1.0f,
                      .midiCC = *MidiCCs::kUserEnv1Attack,
                      .valueFormatter = [](const Patch&, Patch::paramvalue_t value)
                      { return fmt::format("x{:.2f}", std::exp2(value * 2.0f)); } },
                    [](V& v, X x) { v.layerTime = *x; });
    helper.addParam(*GritParamId::kVelocityWave, unit("Vel > Wave", 0.0f, *MidiCCs::kUserEnv1Decay),
                    [](V& v, X x) { v.velocityWave = *x; });
    helper.addParam(*GritParamId::kGrit, unit("Grit", 0.0f, *MidiCCs::kUserEnv1Sustain),
                    [](V& v, X x) { v.gritAmount = *x; });
    helper.addParam(*GritParamId::kBits, unit("Bits", 0.0f, *MidiCCs::kUserEnv1Release),
                    [](V& v, X x) { v.bits = *x; });

    helper.addParam(*GritParamId::kAttack, time("Attack", 0.003f, 0.0f, *MidiCCs::kAmpEnvAttack),
                    [](V& v, X x) { v.attackSec = *x; });
    helper.addParam(*GritParamId::kDecay, time("Decay", 0.4f, 0.0f, *MidiCCs::kAmpEnvDecay),
                    [](V& v, X x) { v.decaySec = *x; });
    helper.addParam(*GritParamId::kSustain, unit("Sustain", 0.85f, *MidiCCs::kAmpEnvSustain),
                    [](V& v, X x) { v.sustainLevel = *x; });
    helper.addParam(*GritParamId::kRelease, time("Release", 0.2f, 0.0f, *MidiCCs::kAmpEnvRelease),
                    [](V& v, X x) { v.releaseSec = *x; });
    helper.addParam(*GritParamId::kVibratoRate,
                    { .displayName = "Vibrato Rate",
                      .defaultValue = 5.0f,
                      .minValue = 0.05f,
                      .maxValue = 20.0f,
                      .editCurve = Param::Curve::kExponential,
                      .midiCC = *MidiCCs::kLfo1Frequency,
                      .valueFormatter = createFrequencyFormatter() },
                    [](V& v, X x) { v.vibratoRateHz = *x; });
    helper.addParam(*GritParamId::kVibratoDepth,
                    { .displayName = "Vibrato Depth",
                      .defaultValue = 0.0f,
                      .minValue = 0.0f,
                      .maxValue = 2.0f,
                      .valueFormatter = [](const Patch&, Patch::paramvalue_t value)
                      { return fmt::format("{:.2f} st", value); } },
                    [](V& v, X x) { v.vibratoDepth = *x; });

    helper.addParam(*GritParamId::kTune,
                    { .displayName = "Tune",
                      .defaultValue = 0.0f,
                      .minValue = -24,
                      .maxValue = 24,
                      .valueFormatter = createSemitoneFormatter() },
                    [](V& v, X x) { v.tuneSemitones = std::round(*x); });
    helper.addParam(*GritParamId::kFine,
                    { .displayName = "Fine",
                      .defaultValue = 0.0f,
                      .minValue = -100,
                      .maxValue = 100,
                      .valueFormatter = createCentsFormatter() },
                    [](V& v, X x) { v.fineCents = *x; });
    helper.addParam(*GritParamId::kPlayMode,
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
    helper.addParam(*GritParamId::kGlideTime,
                    { .displayName = "Glide Time",
                      .defaultValue = 0.0f,
                      .minValue = 0.0f,
                      .maxValue = 2.0f,
                      .editCurve = Param::Curve::kExponential,
                      .valueFormatter = createTimeFormatter() },
                    [](V& v, X x) { v.glideTimeSec = *x; });
    helper.addParam(*GritParamId::kVolume,
                    { .displayName = "Volume",
                      .defaultValue = 1.0f,
                      .minValue = 0.0f,
                      .maxValue = 2.0f,
                      .macroOperation = Param::MacroOperation::kMult,
                      .valueFormatter = createPercentFormatter() },
                    [](V& v, X x) { v.volume = *x; });

    using Element = EditorElement;
    auto p = [](GritParamId id) { return Element{ .type = Element::Type::kParam, .id = *id }; };
    metadata.layoutFlat = Patch::Metadata::EditorLayout{
        .sections = {
          { .name = "Wave",
            .shortName = "Wave",
            .elements = { p(GritParamId::kWave), p(GritParamId::kSteps), p(GritParamId::kStepTime),
                          p(GritParamId::kCrossfade), p(GritParamId::kJump), p(GritParamId::kLoop) } },
          { .name = "Sequence",
            .shortName = "Seq",
            .elements = { p(GritParamId::kLevelStep), p(GritParamId::kRandom),
                          p(GritParamId::kVelocityWave), p(GritParamId::kGrit), p(GritParamId::kBits) } },
          { .name = "Layer",
            .shortName = "Layer",
            .elements = { p(GritParamId::kLayer), p(GritParamId::kLayerWave),
                          p(GritParamId::kLayerTranspose), p(GritParamId::kLayerDetune),
                          p(GritParamId::kLayerTime) } },
          { .name = "Envelope",
            .shortName = "Env",
            .elements = { p(GritParamId::kAttack), p(GritParamId::kDecay), p(GritParamId::kSustain),
                          p(GritParamId::kRelease), p(GritParamId::kVibratoRate),
                          p(GritParamId::kVibratoDepth) } },
          { .name = "Voice",
            .shortName = "Voice",
            .elements = { p(GritParamId::kTune), p(GritParamId::kFine), p(GritParamId::kPlayMode),
                          p(GritParamId::kGlideTime), p(GritParamId::kVolume) } },
        },
    };
    metadata.layout23 = metadata.layoutFlat;

    metadata.initDefaultMacros = [](auto& macros)
    {
        macros[0] = Patch::Metadata::Macro{ .displayName = "Wave",
                                            .modifiers = { { *GritParamId::kWave, 1.0f } },
                                            .bipolar = true };
        macros[1] = Patch::Metadata::Macro{ .displayName = "Step Time",
                                            .modifiers = { { *GritParamId::kStepTime, 1.0f } },
                                            .bipolar = true };
        macros[2] = Patch::Metadata::Macro{ .displayName = "Crossfade",
                                            .modifiers = { { *GritParamId::kCrossfade, 1.0f } },
                                            .bipolar = true };
        macros[3] = Patch::Metadata::Macro{ .displayName = "Jump",
                                            .modifiers = { { *GritParamId::kJump, 1.0f } },
                                            .bipolar = true };
        macros[4] = Patch::Metadata::Macro{ .displayName = "Layer",
                                            .modifiers = { { *GritParamId::kLayer, 1.0f } },
                                            .bipolar = true };
        macros[5] = Patch::Metadata::Macro{ .displayName = "Grit / Bits",
                                            .modifiers = { { *GritParamId::kGrit, 0.8f },
                                                           { *GritParamId::kBits, 0.6f } },
                                            .bipolar = false };
        macros[*Patch::SystemMacroId::kVelocity].modifiers = { { *GritParamId::kVolume, 1.0f } };
    };

    metadata.instrumentTypeId = static_cast<int>(InstrumentTypeId::kGritSynth);
    metadata.outputChannels = Patch::Metadata::OutputChannels::kMono;
    metadata.maxVoices = 8;
}

} // namespace

std::unique_ptr<InstrumentType> createGritSynth()
{
    return std::make_unique<InstrumentType>(
      InstrumentType{ .name = "GRIT",
                      .createMetadata = &GritSynthVoice::createMetadata,
                      .createVoice = [](int /*voiceIdx*/) -> std::unique_ptr<InstrumentVoice>
                      { return std::make_unique<GritSynthVoice>(); } });
}

} // namespace polysynth

#endif
