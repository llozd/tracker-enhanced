#ifndef LIBS_AUDIO_FX_INSERTFX_H_
#define LIBS_AUDIO_FX_INSERTFX_H_

// Insert effect kernels (openspec realtime-effects).

#include "platformFeatures.h"

#if FEATURE_REALTIME_FX

#include <cstdint>

namespace insertfx
{

// Stored in project files: append only, never reorder.
enum Type : uint8_t
{
    kOff,
    kEq3,
    kPq,
    kComp,
    kLimit,
    kGlue,
    kFet,
    kOverdrive,
    kDigital,
    kCrush,
    kTape,
    kTube,
    kTransformer,
    kConsole,
    kFlanger,
    kPhaser,
    kChorus,
    kSoftClip,
    kSaturate,
    kDiode,
    kExciter,
    kReverb, // runs in insertChannels (pooled), not in run()
    kTypeCount
};

enum Family : uint8_t
{
    kFamilyOff,
    kFamilyEq,
    kFamilyDynamics,
    kFamilyDrive,
    kFamilyColour,
    kFamilyMotion,
    kFamilySpace,
    kFamilyCount
};

enum ParamFormat : uint8_t
{
    kFormatPercent, // 0..127 shown as a percentage
    kFormatInt,
    kFormatEnum,
};

constexpr int kParams = 16;
constexpr int kSlots = 3;
constexpr int kBiquads = 6;
constexpr int kDelayLength = 1024; // per channel, a power of two (23 ms)
constexpr int kFlangerLength = 512;

struct ParamDesc
{
    const char* label;
    ParamFormat format;
    int16_t min, max, def;
    const char* const* names; // kFormatEnum
    const char* unit;
};

struct Desc
{
    const char* name;
    const char* abbr; // 4 characters at most
    Family family;
    uint8_t count;
    ParamDesc p[kParams];
};

const Desc& describe(Type type);

struct Slot
{
    Type type;
    int16_t params[kParams];
};

struct Biquad
{
    float b0, b1, b2, a1, a2;
};

struct State
{
    int16_t p[kParams]; // the parameters the coefficients were prepared for
    Type type;
    bool ready;
    Biquad q[kBiquads];
    float z[2][kBiquads][2];
    float env, env2, gain, lp[2], hp[2], ap[2][8], dc[2], hold[2], lfo, fb[2];
    float gainReductionDb; // for a meter
    uint32_t count, writeIdx;
    float* delay; // 2 x kDelayLength floats, owned by the caller
    float param[kParams];
    uint8_t reverb; // pool entry + 1, 0 for none (managed by insertChannels)
};

// Sets the slot to the type's default parameters.
void setDefaults(Slot& slot, Type type);

// Clears the state for a fresh start. delay keeps pointing at the caller's line.
void reset(State& state);

// One block through one effect, in place (r may be null for mono).
void run(State& state, const Slot& slot, float* l, float* r, uint32_t n);

// For the UI: an EQ slot's frequency response in dB at hz (computed from the slot's parameters,
// so it is right while the channel sleeps). 0 for other types.
float responseDb(const Slot& slot, float hz);
inline bool isEq(Type type) { return type == kEq3 || type == kPq; }
inline bool isDynamics(Type type) { return type == kComp || type == kLimit || type == kGlue || type == kFet; }

} // namespace insertfx

#endif // FEATURE_REALTIME_FX
#endif // LIBS_AUDIO_FX_INSERTFX_H_
