#include "insertFx.h"

#if FEATURE_REALTIME_FX

#include <algorithm>
#include <cmath>
#include <cstring>

#include "PolySynth/sdk/SynthVoiceSDK.h"

namespace insertfx
{
namespace
{
constexpr float kFs = 44117.64706f; // AUDIO_SAMPLE_RATE_EXACT
constexpr float kTwoPi = 6.2831853f;

const char* const kGlueRatio[] = { "2:1", "4:1", "10:1" };
const char* const kGlueAttack[] = { "0.1ms", "0.3ms", "1ms", "3ms", "10ms", "30ms" };
const char* const kGlueRelease[] = { "0.1s", "0.2s", "0.4s", "0.8s", "1.2s", "AUTO" };
const char* const kFetRatio[] = { "4:1", "8:1", "12:1", "20:1", "ALL" };
const char* const kPhaserStages[] = { "4", "6", "8" };

constexpr ParamDesc pct(const char* l, int16_t d)
{
    return { l, kFormatPercent, 0, 127, d, nullptr, nullptr };
}
constexpr ParamDesc num(const char* l, int16_t lo, int16_t hi, int16_t d, const char* unit = nullptr)
{
    return { l, kFormatInt, lo, hi, d, nullptr, unit };
}
constexpr ParamDesc list(const char* l, int16_t hi, int16_t d, const char* const* names)
{
    return { l, kFormatEnum, 0, hi, d, names, nullptr };
}

// Same order as Type.
const Desc kDescs[kTypeCount] = {
    { "Off", "OFF", kFamilyOff, 0, {} },
    { "EQ 3 Band", "EQ3", kFamilyEq, 7,
      { pct("LF FRQ", 40), num("LF GAIN", -15, 15, 0, "dB"), pct("MF FRQ", 64),
        num("MF GAIN", -15, 15, 0, "dB"), pct("MF Q", 50), pct("HF FRQ", 70),
        num("HF GAIN", -15, 15, 0, "dB") } },
    { "EQ PQ 4 Band", "PQ", kFamilyEq, 15,
      { pct("LF FRQ", 30), num("LF GAIN", -15, 15, 0, "dB"), pct("LF Q", 50), pct("LM FRQ", 55),
        num("LM GAIN", -15, 15, 0, "dB"), pct("LM Q", 50), pct("HM FRQ", 80),
        num("HM GAIN", -15, 15, 0, "dB"), pct("HM Q", 50), pct("HF FRQ", 90),
        num("HF GAIN", -15, 15, 0, "dB"), pct("HF Q", 50), pct("LOW CUT", 0), pct("HIGH CUT", 127),
        num("OUT", -12, 12, 0, "dB") } },
    { "Compressor", "COMP", kFamilyDynamics, 7,
      { num("THRESH", -60, 0, -18, "dB"), pct("RATIO", 40), pct("ATTACK", 40), pct("RELEASE", 60),
        num("KNEE", 0, 24, 6, "dB"), num("MAKEUP", 0, 24, 0, "dB"), pct("MIX", 127) } },
    { "Limiter", "LIM", kFamilyDynamics, 3,
      { num("CEILING", -24, 0, -1, "dB"), pct("RELEASE", 50), num("GAIN", 0, 24, 0, "dB") } },
    { "Glue Comp", "GLUE", kFamilyDynamics, 6,
      { num("THRESH", -30, 0, -12, "dB"), list("RATIO", 2, 1, kGlueRatio),
        list("ATTACK", 5, 3, kGlueAttack), list("RELEASE", 5, 5, kGlueRelease),
        num("MAKEUP", 0, 20, 0, "dB"), pct("MIX", 127) } },
    { "FET Comp", "FET", kFamilyDynamics, 5,
      { pct("INPUT", 50), num("OUTPUT", -24, 12, 0, "dB"), num("ATTACK", 1, 7, 3),
        num("RELEASE", 1, 7, 4), list("RATIO", 4, 0, kFetRatio) } },
    { "Overdrive", "OD", kFamilyDrive, 4,
      { pct("DRIVE", 50), pct("TONE", 80), num("LEVEL", -12, 12, 0, "dB"), pct("MIX", 127) } },
    { "Digital Clip", "DIG", kFamilyDrive, 4,
      { pct("DRIVE", 50), pct("FOLD", 0), num("LEVEL", -12, 12, 0, "dB"), pct("MIX", 127) } },
    { "Bit Crush", "CRSH", kFamilyDrive, 3,
      { num("BITS", 1, 16, 8), num("RATE", 1, 64, 4), pct("MIX", 127) } },
    { "Tape", "TAPE", kFamilyColour, 3,
      { pct("AMOUNT", 50), pct("TONE", 80), num("LEVEL", -12, 12, 0, "dB") } },
    { "Tube", "TUBE", kFamilyColour, 3,
      { pct("AMOUNT", 50), pct("BIAS", 40), num("LEVEL", -12, 12, 0, "dB") } },
    { "Transformer", "XFMR", kFamilyColour, 3,
      { pct("AMOUNT", 50), pct("TONE", 64), num("LEVEL", -12, 12, 0, "dB") } },
    { "Console", "CONS", kFamilyColour, 3,
      { pct("AMOUNT", 50), pct("AIR", 40), num("LEVEL", -12, 12, 0, "dB") } },
    { "Flanger", "FLNG", kFamilyMotion, 5,
      { pct("RATE", 30), pct("DEPTH", 70), num("FEEDBK", -100, 100, 40), pct("DELAY", 40),
        pct("MIX", 64) } },
    { "Phaser", "PHAS", kFamilyMotion, 5,
      { pct("RATE", 30), pct("DEPTH", 80), num("FEEDBK", -100, 100, 40),
        list("STAGES", 2, 0, kPhaserStages), pct("MIX", 64) } },
    { "Chorus", "CHOR", kFamilyMotion, 7,
      { pct("RATE", 40), pct("DEPTH", 60), pct("DELAY", 64), num("VOICES", 2, 4, 3),
        pct("SPREAD", 90), pct("TONE", 100), pct("MIX", 64) } },
    { "Soft Clip", "SCLP", kFamilyDrive, 4,
      { pct("DRIVE", 40), pct("KNEE", 64), pct("TONE", 100), num("LEVEL", -12, 12, 0, "dB") } },
    { "Saturator", "SATU", kFamilyColour, 4,
      { pct("DRIVE", 45), pct("BIAS", 40), pct("TONE", 100), pct("MIX", 100) } },
    { "Diode Drive", "DIOD", kFamilyDrive, 4,
      { pct("DRIVE", 40), pct("TONE", 90), num("LEVEL", -12, 12, 0, "dB"), pct("MIX", 127) } },
    { "Exciter", "EXCT", kFamilyColour, 3, { pct("FREQ", 70), pct("AMOUNT", 50), pct("MIX", 64) } },
    { "Reverb", "VERB", kFamilySpace, 5,
      { pct("SIZE", 90), pct("DAMP", 40), pct("PREDLY", 0), pct("DIFFUS", 80), pct("MIX", 40) } },
};

using polysynth::sdk::fastPow2;
using polysynth::sdk::fastSine;

inline float clampf(float x, float a, float b)
{
    return x < a ? a : x > b ? b : x;
}

// For coefficient updates only (std maths).
inline float dbToGain(float db)
{
    return std::pow(10.0f, db * 0.05f);
}
inline float lerpLog(float lo, float hi, float v)
{
    return lo * std::pow(hi / lo, v * (1.0f / 127.0f));
}
inline float coefFromMs(float ms)
{
    return ms <= 0.0f ? 0.0f : std::exp(-1.0f / (0.001f * ms * kFs));
}

// Per sample.
inline float fastDbToGain(float db)
{
    return fastPow2(db * 0.16609640f);
}
// log2 to about 0.005 (0.03 dB): enough for a level detector.
inline float fastLog2(float x)
{
    union
    {
        float f;
        uint32_t i;
    } u = { x };
    float y = static_cast<float>(static_cast<int>((u.i >> 23) & 255) - 128);
    u.i = (u.i & 0x007FFFFF) | 0x3F800000;
    return y + (-0.34484843f * u.f + 2.02466578f) * u.f - 0.65871759f;
}

// Bounded soft clip shaped like tanh near 0 (slope 1), never above 1.
inline float softTanh(float x)
{
    x = clampf(x, -4.0f, 4.0f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float fastAsinh(float x)
{
    const float a = std::fabs(x);
    if (a < 0.02f)
    {
        return x * (1.0f - a * a * (1.0f / 6.0f));
    }
    const float y = 0.69314718f * fastLog2(a + std::sqrt(a * a + 1.0f));
    return x < 0.0f ? -y : y;
}

// Triangle fold: x up to +-1, then reflected back.
inline float fold(float x)
{
    float t = (clampf(x, -64.0f, 64.0f) + 1.0f) * 0.25f;
    t -= std::floor(t);
    return std::fabs(t * 4.0f - 2.0f) - 1.0f;
}

// Parabolic sine of one turn, for LFOs.
inline float parabolicSine(float ph)
{
    const float u = (ph - std::floor(ph)) * 2.0f - 1.0f;
    return 4.0f * u * (1.0f - std::fabs(u));
}

// RBJ cookbook biquads, normalised to a0 = 1.
void setPeak(Biquad& q, float f, float gainDb, float Q)
{
    const float A = dbToGain(gainDb * 0.5f);
    const float w = kTwoPi * clampf(f, 10.0f, kFs * 0.45f) / kFs;
    const float s = std::sin(w), c = std::cos(w);
    const float al = s / (2.0f * Q), a0 = 1.0f + al / A;
    q.b0 = (1.0f + al * A) / a0;
    q.b1 = -2.0f * c / a0;
    q.b2 = (1.0f - al * A) / a0;
    q.a1 = -2.0f * c / a0;
    q.a2 = (1.0f - al / A) / a0;
}

void setShelf(Biquad& q, float f, float gainDb, float S, bool high)
{
    const float A = dbToGain(gainDb * 0.5f);
    const float w = kTwoPi * clampf(f, 10.0f, kFs * 0.45f) / kFs;
    const float s = std::sin(w), c = std::cos(w);
    const float al = s * 0.5f * std::sqrt((A + 1.0f / A) * (1.0f / S - 1.0f) + 2.0f);
    const float sa = 2.0f * std::sqrt(A) * al;
    float a0;
    if (!high)
    {
        a0 = (A + 1.0f) + (A - 1.0f) * c + sa;
        q.b0 = A * ((A + 1.0f) - (A - 1.0f) * c + sa) / a0;
        q.b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * c) / a0;
        q.b2 = A * ((A + 1.0f) - (A - 1.0f) * c - sa) / a0;
        q.a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * c) / a0;
        q.a2 = ((A + 1.0f) + (A - 1.0f) * c - sa) / a0;
    }
    else
    {
        a0 = (A + 1.0f) - (A - 1.0f) * c + sa;
        q.b0 = A * ((A + 1.0f) + (A - 1.0f) * c + sa) / a0;
        q.b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * c) / a0;
        q.b2 = A * ((A + 1.0f) + (A - 1.0f) * c - sa) / a0;
        q.a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * c) / a0;
        q.a2 = ((A + 1.0f) - (A - 1.0f) * c - sa) / a0;
    }
}

void setPass(Biquad& q, float f, float Q, bool high)
{
    const float w = kTwoPi * clampf(f, 10.0f, kFs * 0.45f) / kFs;
    const float s = std::sin(w), c = std::cos(w), al = s / (2.0f * Q), a0 = 1.0f + al;
    if (high)
    {
        q.b0 = (1.0f + c) * 0.5f / a0;
        q.b1 = -(1.0f + c) / a0;
    }
    else
    {
        q.b0 = (1.0f - c) * 0.5f / a0;
        q.b1 = (1.0f - c) / a0;
    }
    q.b2 = q.b0;
    q.a1 = -2.0f * c / a0;
    q.a2 = (1.0f - al) / a0;
}

// Transposed direct form II.
inline float biquad(const Biquad& q, float z[2], float x)
{
    const float y = q.b0 * x + z[0];
    z[0] = q.b1 * x - q.a1 * y + z[1];
    z[1] = q.b2 * x - q.a2 * y;
    return y;
}

void prepare(State& e, const Slot& slot)
{
    const Desc& d = describe(slot.type);
    if (slot.type != e.type)
    {
        reset(e);
        e.type = slot.type;
    }
    for (int i = 0; i < kParams; i++)
    {
        const int16_t v =
          i < d.count ? std::clamp<int16_t>(slot.params[i], d.p[i].min, d.p[i].max) : 0;
        e.p[i] = slot.params[i];
        e.param[i] = static_cast<float>(v);
    }
    const float* p = e.param;
    switch (slot.type)
    {
        case kEq3:
            setShelf(e.q[0], lerpLog(30.0f, 500.0f, p[0]), p[1], 0.8f, false);
            setPeak(e.q[1], lerpLog(150.0f, 9000.0f, p[2]), p[3],
                    0.3f * std::pow(26.0f, p[4] / 127.0f));
            setShelf(e.q[2], lerpLog(1500.0f, 16000.0f, p[5]), p[6], 0.8f, true);
            break;
        case kPq:
            setShelf(e.q[0], lerpLog(30.0f, 450.0f, p[0]), p[1], 0.4f + 1.1f * p[2] / 127.0f, false);
            setPeak(e.q[1], lerpLog(100.0f, 3000.0f, p[3]), p[4],
                    0.4f * std::pow(25.0f, p[5] / 127.0f));
            setPeak(e.q[2], lerpLog(400.0f, 10000.0f, p[6]), p[7],
                    0.4f * std::pow(25.0f, p[8] / 127.0f));
            setShelf(e.q[3], lerpLog(2000.0f, 18000.0f, p[9]), p[10], 0.4f + 1.1f * p[11] / 127.0f,
                     true);
            setPass(e.q[4], lerpLog(20.0f, 400.0f, p[12]), 0.707f, true);
            setPass(e.q[5], lerpLog(3000.0f, 20000.0f, p[13]), 0.707f, false);
            break;
        case kOverdrive:
        case kDiode: setPass(e.q[0], lerpLog(900.0f, 14000.0f, p[1]), 0.707f, false); break;
        case kSoftClip:
        case kSaturate: setPass(e.q[0], lerpLog(900.0f, 14000.0f, p[2]), 0.707f, false); break;
        case kExciter: setPass(e.q[0], lerpLog(1500.0f, 12000.0f, p[0]), 0.707f, true); break;
        case kTape:
            setPass(e.q[0], lerpLog(3500.0f, 18000.0f, p[1]), 0.6f, false);
            setShelf(e.q[1], 90.0f, 2.5f * p[0] / 127.0f, 0.7f, false); // head bump
            break;
        case kTransformer: setPass(e.q[0], lerpLog(2500.0f, 16000.0f, p[1]), 0.5f, false); break;
        case kConsole: setShelf(e.q[0], 9000.0f, 5.0f * p[1] / 127.0f, 0.7f, true); break;
        case kChorus: // TONE: the wet one-pole low pass
            e.gain = 1.0f - std::exp(-kTwoPi * lerpLog(1500.0f, 18000.0f, p[5]) / kFs);
            break;
        default: break;
    }
    e.ready = true;
}

void runEq(State& e, int bands, float* l, float* r, uint32_t n, float out)
{
    uint32_t used = (1u << bands) - 1u;
    if (bands == 6) // PQ: a cut at the end of its range is off
    {
        if (e.param[12] <= 0.0f)
            used &= ~(1u << 4);
        if (e.param[13] >= 127.0f)
            used &= ~(1u << 5);
    }
    for (int c = 0; c < (r ? 2 : 1); c++)
    {
        float* x = c ? r : l;
        for (uint32_t i = 0; i < n; i++)
        {
            float v = x[i];
            for (int b = 0; b < bands; b++)
            {
                if ((used >> b) & 1u)
                    v = biquad(e.q[b], e.z[c][b], v);
            }
            x[i] = v * out;
        }
    }
}

// Gain reduction in dB (>= 0) for a level in dB: threshold, ratio, soft knee.
inline float gainReduction(float levelDb, float thr, float ratio, float knee)
{
    const float over = levelDb - thr, slope = 1.0f - 1.0f / ratio;
    if (knee > 0.01f && over > -knee * 0.5f && over < knee * 0.5f)
    {
        const float t = over + knee * 0.5f;
        return slope * t * t / (2.0f * knee);
    }
    return over > 0.0f ? over * slope : 0.0f;
}

void runDynamics(State& e, Type type, float* l, float* r, uint32_t n)
{
    const float* p = e.param;
    float thr, ratio, knee = 0.0f, att, rel, makeup, mix = 1.0f, scHp = 0.0f;
    bool limit = false, fet = false, autoRelease = false;
    switch (type)
    {
        case kComp:
            thr = p[0];
            ratio = 1.0f + 19.0f * std::pow(p[1] / 127.0f, 1.5f);
            knee = p[4];
            att = lerpLog(0.1f, 100.0f, p[2]);
            rel = lerpLog(10.0f, 2000.0f, p[3]);
            makeup = p[5];
            mix = p[6] / 127.0f;
            break;
        case kLimit:
            thr = p[0];
            ratio = 1000.0f;
            att = 0.02f;
            rel = lerpLog(5.0f, 500.0f, p[1]);
            makeup = p[2];
            limit = true;
            break;
        case kGlue:
        {
            static const float R[3] = { 2.0f, 4.0f, 10.0f };
            static const float A[6] = { 0.1f, 0.3f, 1.0f, 3.0f, 10.0f, 30.0f };
            static const float RL[5] = { 100.0f, 200.0f, 400.0f, 800.0f, 1200.0f };
            thr = p[0];
            ratio = R[static_cast<int>(p[1]) % 3];
            knee = 6.0f;
            att = A[static_cast<int>(p[2]) % 6];
            autoRelease = static_cast<int>(p[3]) >= 5;
            rel = autoRelease ? 300.0f : RL[static_cast<int>(p[3]) % 5];
            makeup = p[4];
            mix = p[5] / 127.0f;
            scHp = 60.0f;
            break;
        }
        default: // FET: the input drives the signal into a fixed threshold
        {
            static const float R[5] = { 4.0f, 8.0f, 12.0f, 20.0f, 100.0f };
            static const float A[7] = { 0.8f, 0.4f, 0.2f, 0.1f, 0.05f, 0.03f, 0.02f };
            static const float RL[7] = { 1100.0f, 600.0f, 300.0f, 150.0f, 100.0f, 70.0f, 50.0f };
            thr = -20.0f;
            ratio = R[static_cast<int>(p[4]) % 5];
            att = A[static_cast<int>(p[2]) - 1];
            rel = RL[static_cast<int>(p[3]) - 1];
            knee = 2.0f;
            makeup = p[1];
            fet = true;
            break;
        }
    }

    const float inputDb = fet ? p[0] * (30.0f / 127.0f) : limit ? p[2] : 0.0f;
    const float gIn = dbToGain(inputDb), gOut = dbToGain(fet || !limit ? makeup : 0.0f);
    const float ca = coefFromMs(att), cr = coefFromMs(rel);
    const float slowCoef = coefFromMs(2000.0f), fastRel = coefFromMs(120.0f),
                slowRel = coefFromMs(900.0f);
    const float hpA = scHp > 0.0f ? std::exp(-kTwoPi * scHp / kFs) : 0.0f;
    const float ceiling = limit ? dbToGain(thr) : 1.0f;
    float grPeak = 0.0f;

    for (uint32_t i = 0; i < n; i++)
    {
        const float xl = l[i] * gIn, xr = r ? r[i] * gIn : xl;
        float dl = xl, dr = xr, g, grDb;
        if (hpA > 0.0f) // sidechain high pass
        {
            e.hp[0] = hpA * e.hp[0] + (1.0f - hpA) * dl;
            dl -= e.hp[0];
            e.hp[1] = hpA * e.hp[1] + (1.0f - hpA) * dr;
            dr -= e.hp[1];
        }
        const float level = std::max(std::fabs(dl), std::fabs(dr));
        if (limit) // instant attack, then a release
        {
            const float want = level > ceiling ? level : 0.0f;
            e.env = want > e.env ? want : cr * e.env + (1.0f - cr) * want;
            g = e.env > ceiling ? ceiling / e.env : 1.0f;
            grDb = -6.0206f * fastLog2(g);
        }
        else
        {
            float c = level > e.env ? ca : cr;
            if (autoRelease && level <= e.env) // a fast release for peaks, a slow one for the body
            {
                e.env2 = e.env2 * slowCoef + (1.0f - slowCoef) * level;
                c = e.env > 2.0f * e.env2 ? fastRel : slowRel;
            }
            e.env = c * e.env + (1.0f - c) * level;
            grDb = gainReduction(6.0206f * fastLog2(e.env + 1e-9f), thr, ratio, knee);
            g = fastDbToGain(-grDb);
        }
        grPeak = std::max(grPeak, grDb);

        float yl = xl * g * gOut, yr = xr * g * gOut;
        if (fet)
        {
            yl = softTanh(yl * 1.1f) * (1.0f / 1.1f);
            yr = softTanh(yr * 1.1f) * (1.0f / 1.1f);
        }
        if (limit)
        {
            yl = clampf(yl, -ceiling, ceiling);
            yr = clampf(yr, -ceiling, ceiling);
        }
        if (mix < 0.999f)
        {
            yl = mix * yl + (1.0f - mix) * l[i];
            yr = mix * yr + (1.0f - mix) * (r ? r[i] : l[i]);
        }
        l[i] = yl;
        if (r)
            r[i] = yr;
    }
    e.gainReductionDb = e.gainReductionDb * 0.8f + grPeak * 0.2f;
}

// Every drive and colour is unity gain for a small signal.
void runDrive(State& e, Type type, float* l, float* r, uint32_t n)
{
    const float* p = e.param;
    for (int c = 0; c < (r ? 2 : 1); c++)
    {
        float* x = c ? r : l;
        float* z = e.z[c][0];
        switch (type)
        {
            case kOverdrive:
            {
                const float g = 1.0f + 39.0f * p[0] / 127.0f, lvl = dbToGain(p[2]), mix = p[3] / 127.0f;
                for (uint32_t i = 0; i < n; i++)
                {
                    const float in = x[i];
                    const float y = biquad(e.q[0], z, softTanh(in * g) / g) * lvl;
                    x[i] = mix * y + (1.0f - mix) * in;
                }
                break;
            }
            case kDigital: // a hard clip, folded back more and more
            {
                const float g = 1.0f + 39.0f * p[0] / 127.0f, fo = p[1] / 127.0f, lvl = dbToGain(p[2]),
                            mix = p[3] / 127.0f;
                for (uint32_t i = 0; i < n; i++)
                {
                    const float in = x[i], s = in * g;
                    const float y = (clampf(s, -1.0f, 1.0f) * (1.0f - fo) + fold(s) * fo) / g * lvl;
                    x[i] = mix * y + (1.0f - mix) * in;
                }
                break;
            }
            case kCrush:
            {
                const uint32_t hold = static_cast<uint32_t>(p[1]);
                const float mix = p[2] / 127.0f, steps = std::pow(2.0f, p[0] - 1.0f);
                for (uint32_t i = 0; i < n; i++)
                {
                    const float in = x[i];
                    if (((e.count + i) % hold) == 0u)
                        e.hold[c] = std::floor(clampf(in, -1.0f, 1.0f) * steps + 0.5f) / steps;
                    x[i] = mix * e.hold[c] + (1.0f - mix) * in;
                }
                break;
            }
            case kTape: // soft saturation, a rolled-off top, a head bump
            {
                const float a = 0.3f + 4.0f * p[0] / 127.0f, lvl = dbToGain(p[2]);
                for (uint32_t i = 0; i < n; i++)
                {
                    float y = softTanh(x[i] * a) / a;
                    y = biquad(e.q[0], e.z[c][0], y);
                    y = biquad(e.q[1], e.z[c][1], y);
                    x[i] = y * lvl;
                }
                break;
            }
            case kTube: // asymmetric: even harmonics, the bias taken out
            {
                const float a = 0.3f + 5.0f * p[0] / 127.0f, tb = softTanh(0.35f * p[1] / 127.0f),
                            bias = 0.35f * p[1] / 127.0f, lvl = dbToGain(p[2]);
                const float norm = 1.0f / (a * (1.0f - tb * tb));
                for (uint32_t i = 0; i < n; i++)
                {
                    const float y = (softTanh(x[i] * a + bias) - tb) * norm;
                    e.dc[c] += 0.0005f * (y - e.dc[c]);
                    x[i] = (y - e.dc[c]) * lvl;
                }
                break;
            }
            case kTransformer: // the lows saturate first, the top rolls off
            {
                const float a = 0.3f + 4.0f * p[0] / 127.0f, lvl = dbToGain(p[2]);
                for (uint32_t i = 0; i < n; i++)
                {
                    const float v = x[i];
                    e.lp[c] += 0.02f * (v - e.lp[c]);
                    float y = softTanh(e.lp[c] * a * 1.8f) / (a * 1.8f) + (v - e.lp[c]);
                    x[i] = biquad(e.q[0], z, y) * lvl;
                }
                break;
            }
            case kSoftClip: // linear, then a tanh shoulder: KNEE sets where
            {
                const float g = 1.0f + 23.0f * p[0] / 127.0f, th = 0.95f - 0.9f * p[1] / 127.0f,
                            lvl = dbToGain(p[3]);
                for (uint32_t i = 0; i < n; i++)
                {
                    const float v = x[i], a = std::fabs(v) * g;
                    float y = a <= th ? a : th + (1.0f - th) * softTanh((a - th) / (1.0f - th));
                    y = (v < 0.0f ? -y : y) / g;
                    x[i] = biquad(e.q[0], z, y) * lvl;
                }
                break;
            }
            case kSaturate: // asymmetric: even harmonics, the offset taken out
            {
                const float a = 0.3f + 5.0f * p[0] / 127.0f, bias = 0.4f * p[1] / 127.0f,
                            mix = p[3] / 127.0f, tb = softTanh(bias);
                const float norm = 1.0f / (a * (1.0f - tb * tb));
                for (uint32_t i = 0; i < n; i++)
                {
                    const float in = x[i];
                    const float y = (softTanh(in * a + bias) - tb) * norm;
                    e.dc[c] += 0.0005f * (y - e.dc[c]);
                    x[i] = mix * biquad(e.q[0], z, y - e.dc[c]) + (1.0f - mix) * in;
                }
                break;
            }
            case kDiode: // a diode pair: logarithmic, never flat
            {
                const float g = 1.0f + 19.0f * p[0] / 127.0f, lvl = dbToGain(p[2]), mix = p[3] / 127.0f;
                for (uint32_t i = 0; i < n; i++)
                {
                    const float in = x[i];
                    const float y = biquad(e.q[0], z, fastAsinh(in * g) / g) * lvl;
                    x[i] = mix * y + (1.0f - mix) * in;
                }
                break;
            }
            case kExciter: // only the harmonics the top makes are added
            {
                const float g = 1.0f + 11.0f * p[1] / 127.0f, mix = p[2] / 127.0f;
                for (uint32_t i = 0; i < n; i++)
                {
                    const float in = x[i], hp = biquad(e.q[0], z, in), s = hp * g;
                    const float y = softTanh(s + 0.25f * s * std::fabs(s)) / g - hp;
                    e.dc[c] += 0.0005f * (y - e.dc[c]);
                    x[i] = in + mix * (y - e.dc[c]);
                }
                break;
            }
            default: // console: a gentle asymmetric clip and some air
            {
                const float a = 0.2f + 2.5f * p[0] / 127.0f, lvl = dbToGain(p[2]);
                const float tb = softTanh(0.05f * a), norm = 1.0f / (a * (1.0f - tb * tb));
                for (uint32_t i = 0; i < n; i++)
                {
                    const float y = (softTanh(x[i] * a + 0.05f * a) - tb) * norm;
                    x[i] = biquad(e.q[0], z, y) * lvl;
                }
                break;
            }
        }
    }
    e.count += n;
}

void runFlangerPhaser(State& e, Type type, float* l, float* r, uint32_t n)
{
    const float* p = e.param;
    const float rate = 0.05f * std::pow(200.0f, p[0] / 127.0f), depth = p[1] / 127.0f;
    const float fb = clampf(p[2] / 100.0f, -0.95f, 0.95f), mix = p[4] / 127.0f, inc = rate / kFs;
    const float flangerBase = 0.0004f * kFs * (0.3f + 1.7f * p[3] / 127.0f);
    static const uint8_t kStages[3] = { 4, 6, 8 };
    const uint32_t stages = kStages[static_cast<int>(p[3]) % 3];

    for (uint32_t i = 0; i < n; i++)
    {
        const float lfo = 0.5f - 0.5f * fastSine(e.lfo + 0.25f);
        e.lfo += inc;
        if (e.lfo >= 1.0f)
            e.lfo -= 1.0f;

        if (type == kFlanger)
        {
            if (!e.delay)
                return;
            const float d = clampf(flangerBase * (1.0f - 0.9f * depth * lfo), 1.0f,
                                   static_cast<float>(kFlangerLength) - 2.0f);
            const uint32_t w = e.writeIdx & (kFlangerLength - 1);
            float rp = static_cast<float>(w) - d;
            if (rp < 0.0f)
                rp += static_cast<float>(kFlangerLength);
            const uint32_t i0 = static_cast<uint32_t>(rp), i1 = (i0 + 1u) & (kFlangerLength - 1);
            const float fr = rp - static_cast<float>(i0);
            for (int c = 0; c < (r ? 2 : 1); c++)
            {
                float* x = c ? r : l;
                float* line = e.delay + c * kFlangerLength;
                const float in = x[i], v = line[i0] * (1.0f - fr) + line[i1] * fr;
                line[w] = clampf(in + fb * v, -2.0f, 2.0f);
                x[i] = in + (v - in) * 0.5f * mix;
            }
        }
        else
        {
            const float f = 200.0f * fastPow2(4.3219281f * depth * lfo); // 200 Hz * 20^(depth lfo)
            const float w = 3.14159265f * f / kFs, a = (1.0f - w) / (1.0f + w);
            for (int c = 0; c < (r ? 2 : 1); c++)
            {
                float* x = c ? r : l;
                const float in = x[i];
                float v = in + fb * e.fb[c];
                for (uint32_t k = 0; k < stages; k++)
                {
                    const float y = -a * v + e.ap[c][k];
                    e.ap[c][k] = v + a * y;
                    v = y;
                }
                e.fb[c] = clampf(v, -2.0f, 2.0f);
                x[i] = in + (v - in) * 0.5f * mix;
            }
        }
        e.writeIdx++;
    }
}

// Per channel the line is written once and read at VOICES taps whose delay swings round DELAY,
// evenly spread in LFO phase. The right channel's taps are SPREAD * half a cycle behind.
void runChorus(State& e, float* l, float* r, uint32_t n)
{
    if (!e.delay)
        return;
    const float* p = e.param;
    const uint32_t voices = static_cast<uint32_t>(clampf(p[3], 2.0f, 4.0f));
    const float inc = 0.1f * std::pow(50.0f, p[0] / 127.0f) / kFs;
    const float base = 0.001f * kFs * (4.0f + 10.0f * p[2] / 127.0f);
    const float swing = 0.5f * base * p[1] / 127.0f, mix = p[6] / 127.0f, off = 0.5f * p[4] / 127.0f;
    const float tone = e.gain, inv = 1.0f / static_cast<float>(voices);
    const float maxd = static_cast<float>(kDelayLength) - 2.0f;

    for (uint32_t i = 0; i < n; i++)
    {
        const uint32_t w = e.writeIdx & (kDelayLength - 1);
        for (int c = 0; c < (r ? 2 : 1); c++)
        {
            float* x = c ? r : l;
            float* line = e.delay + c * kDelayLength;
            const float in = x[i];
            float wet = 0.0f;
            line[w] = in;
            for (uint32_t k = 0; k < voices; k++)
            {
                const float ph = e.lfo + static_cast<float>(k) * inv + (c ? off : 0.0f);
                const float d = clampf(base + swing * parabolicSine(ph), 1.0f, maxd);
                float rp = static_cast<float>(w) - d;
                if (rp < 0.0f)
                    rp += static_cast<float>(kDelayLength);
                const uint32_t i0 = static_cast<uint32_t>(rp), i1 = (i0 + 1u) & (kDelayLength - 1);
                const float fr = rp - static_cast<float>(i0);
                wet += line[i0] * (1.0f - fr) + line[i1] * fr;
            }
            e.lp[c] += tone * (wet * inv - e.lp[c]);
            x[i] = in + (e.lp[c] - in) * mix;
        }
        e.lfo += inc;
        if (e.lfo >= 1.0f)
            e.lfo -= 1.0f;
        e.writeIdx++;
    }
}

} // namespace

const Desc& describe(Type type)
{
    return kDescs[type < kTypeCount ? type : kOff];
}

void setDefaults(Slot& slot, Type type)
{
    const Desc& d = describe(type);
    slot.type = type < kTypeCount ? type : kOff;
    for (int i = 0; i < kParams; i++)
    {
        slot.params[i] = i < d.count ? d.p[i].def : 0;
    }
}

void reset(State& e)
{
    float* delay = e.delay;
    const uint8_t reverb = e.reverb;
    std::memset(&e, 0, sizeof(e));
    e.delay = delay;
    e.reverb = reverb;
    e.gain = 1.0f;
    if (delay)
    {
        std::memset(delay, 0, 2 * kDelayLength * sizeof(float));
    }
}

float responseDb(const Slot& slot, float hz)
{
    if (!isEq(slot.type))
        return 0.0f;
    State e{};
    prepare(e, slot);
    const int bands = slot.type == kPq ? 6 : 3;
    uint32_t used = (1u << bands) - 1u;
    if (bands == 6)
    {
        if (e.param[12] <= 0.0f)
            used &= ~(1u << 4);
        if (e.param[13] >= 127.0f)
            used &= ~(1u << 5);
    }
    // |H(e^jw)| of the cascade: (b0 + b1 z^-1 + b2 z^-2) / (1 + a1 z^-1 + a2 z^-2)
    const float w = kTwoPi * clampf(hz, 1.0f, kFs * 0.49f) / kFs;
    const float c1 = std::cos(w), s1 = std::sin(w), c2 = std::cos(2.0f * w), s2 = std::sin(2.0f * w);
    float db = slot.type == kPq ? e.param[14] : 0.0f;
    for (int b = 0; b < bands; b++)
    {
        if (!((used >> b) & 1u))
            continue;
        const Biquad& q = e.q[b];
        const float nr = q.b0 + q.b1 * c1 + q.b2 * c2, ni = -(q.b1 * s1 + q.b2 * s2);
        const float dr = 1.0f + q.a1 * c1 + q.a2 * c2, di = -(q.a1 * s1 + q.a2 * s2);
        const float mag2 = (nr * nr + ni * ni) / std::max(dr * dr + di * di, 1.0e-12f);
        db += 10.0f * std::log10(std::max(mag2, 1.0e-12f));
    }
    return db;
}

void run(State& e, const Slot& slot, float* l, float* r, uint32_t n)
{
    if (slot.type == kOff || slot.type == kReverb || slot.type >= kTypeCount)
        return;
    if (!e.ready || e.type != slot.type ||
        std::memcmp(e.p, slot.params, sizeof(int16_t) * describe(slot.type).count) != 0)
        prepare(e, slot);
    switch (slot.type)
    {
        case kEq3: runEq(e, 3, l, r, n, 1.0f); break;
        case kPq: runEq(e, 6, l, r, n, dbToGain(e.param[14])); break;
        case kComp:
        case kLimit:
        case kGlue:
        case kFet: runDynamics(e, slot.type, l, r, n); break;
        case kFlanger:
        case kPhaser: runFlangerPhaser(e, slot.type, l, r, n); break;
        case kChorus: runChorus(e, l, r, n); break;
        default: runDrive(e, slot.type, l, r, n); break;
    }
}

} // namespace insertfx

#endif // FEATURE_REALTIME_FX
