#pragma once

// Building blocks for the engines ported from Pocket Rack (openspec pocket-rack-engines):
// an ADSR, an LFO, glide and noise. Pocket Rack takes these from the part; on the Tracker each
// engine carries its own. Everything runs per sample or per block, with no allocation.

#include "PolySynth/sdk/SynthVoiceSDK.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace polysynth::enginekit
{

constexpr float kSampleRate = sdk::kSampleRate;

// Linear attack, exponential decay and release (one-pole towards the target).
class Adsr
{
  public:
    enum class Stage : uint8_t
    {
        kIdle,
        kAttack,
        kDecay,
        kRelease,
    };

    void set(float attackSec, float decaySec, float sustain, float releaseSec)
    {
        attackInc = 1.0f / std::max(1.0f, attackSec * kSampleRate);
        decayCoef = coefFor(decaySec);
        sustainLevel = std::clamp(sustain, 0.0f, 1.0f);
        releaseCoef = coefFor(releaseSec);
    }

    void gateOn(bool retrigger)
    {
        if (retrigger || stage == Stage::kIdle || stage == Stage::kRelease)
            stage = Stage::kAttack;
    }
    void gateOff()
    {
        if (stage != Stage::kIdle)
            stage = Stage::kRelease;
    }

    bool isActive() const { return stage != Stage::kIdle; }
    float value() const { return level; }

    float process()
    {
        switch (stage)
        {
            case Stage::kAttack:
                level += attackInc;
                if (level >= 1.0f)
                {
                    level = 1.0f;
                    stage = Stage::kDecay;
                }
                break;
            case Stage::kDecay: level = sustainLevel + (level - sustainLevel) * decayCoef; break;
            case Stage::kRelease:
                level *= releaseCoef;
                if (level < 1.0e-4f)
                {
                    level = 0.0f;
                    stage = Stage::kIdle;
                }
                break;
            case Stage::kIdle: break;
        }
        return level;
    }

  private:
    // Falls to 1/1000 (-60 dB) in timeSec.
    static float coefFor(float timeSec)
    {
        const float samples = std::max(1.0f, timeSec * kSampleRate);
        return std::exp(-6.9077553f / samples);
    }

    Stage stage = Stage::kIdle;
    float level = 0.0f;
    float attackInc = 1.0f;
    float decayCoef = 0.0f;
    float sustainLevel = 1.0f;
    float releaseCoef = 0.0f;
};

// Free-running LFO, advanced once per block (output in -1..1).
class Lfo
{
  public:
    enum class Wave : uint8_t
    {
        kSine,
        kTriangle,
        kSaw,
        kSquare,
        kSampleHold,
        kCount
    };

    float advance(float rateHz, Wave wave, uint32_t samples)
    {
        phase += rateHz * static_cast<float>(samples) / kSampleRate;
        if (phase >= 1.0f)
        {
            phase -= std::floor(phase);
            held = random.nextBipolar();
        }
        switch (wave)
        {
            case Wave::kTriangle: return phase < 0.5f ? phase * 4.0f - 1.0f : 3.0f - phase * 4.0f;
            case Wave::kSaw: return phase * 2.0f - 1.0f;
            case Wave::kSquare: return phase < 0.5f ? 1.0f : -1.0f;
            case Wave::kSampleHold: return held;
            default: return sdk::fastSine(phase);
        }
    }

    void reset() { phase = 0.0f; }

  private:
    struct Rng
    {
        uint32_t state = 0x9E3779B9u;
        float nextBipolar()
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return static_cast<float>(static_cast<int32_t>(state)) * (1.0f / 2147483648.0f);
        }
    } random;
    float phase = 0.0f;
    float held = 0.0f;
};

// White noise, -1..1.
struct Noise
{
    uint32_t state = 0x12345679u;
    float next()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float>(static_cast<int32_t>(state)) * (1.0f / 2147483648.0f);
    }
};

// Portamento in notes: moves towards the target at a constant rate (timeSec per 12 semitones).
class Glide
{
  public:
    void noteOn(float note, float timeSec, bool glide)
    {
        target = note;
        if (!glide || timeSec <= 0.001f || !started)
            current = note;
        started = true;
        rate = timeSec > 0.001f ? 12.0f / (timeSec * kSampleRate) : 0.0f;
    }

    // Advances by samples and returns the note.
    float advance(uint32_t samples)
    {
        if (rate <= 0.0f)
        {
            current = target;
            return current;
        }
        const float step = rate * static_cast<float>(samples);
        if (current < target)
            current = std::min(target, current + step);
        else
            current = std::max(target, current - step);
        return current;
    }

  private:
    float current = 60.0f;
    float target = 60.0f;
    float rate = 0.0f;
    bool started = false;
};

// polyBLEP residual for a discontinuity at phase 0 (t and dt in cycles).
inline float polyBlep(float t, float dt)
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

// Bounded soft clip shaped like tanh near 0, exact at 0 and +-3.
inline float softTanh(float x)
{
    x = std::clamp(x, -3.0f, 3.0f);
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

} // namespace polysynth::enginekit
