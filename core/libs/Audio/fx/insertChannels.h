#ifndef LIBS_AUDIO_FX_INSERTCHANNELS_H_
#define LIBS_AUDIO_FX_INSERTCHANNELS_H_

// Insert slots of every audio channel: settings, state and delay lines (EXTERNAL_RAM).

#include "insertFx.h"

#if FEATURE_REALTIME_FX

namespace insertfx
{

enum ChannelId : uint8_t
{
    kTrack0 = 0, // 8 sample tracks
    kSynth0 = 8, // 3 synth parts
    kMaster = 11,
    kChannelCount
};

struct Channel
{
    Slot slot[kSlots];
    State state[kSlots];
    bool sleeping; // input silent and the tails have died out
};

// Clears every channel and hands out the delay lines. Call once before audio starts.
void initChannels();

Channel& channel(ChannelId id);

constexpr int kReverbPool = 4; // reverbs that can run at once (64 KB each)

// UI side: replaces one slot (atomic with respect to the audio interrupt).
// Returns false, leaving the slot as it was, when a reverb is asked for and the pool is empty.
bool setSlot(ChannelId id, int slotIdx, const Slot& slot);
const Slot& getSlot(ChannelId id, int slotIdx);

bool hasActiveSlot(const Channel& ch);

// True when a slot makes a mono input stereo (chorus, reverb), so the caller should feed it stereo.
bool widensStereo(const Channel& ch);

// Audio side: runs the channel's slots in place. r may be null for mono.
void processChannel(Channel& ch, float* l, float* r, uint32_t n);

} // namespace insertfx

#endif // FEATURE_REALTIME_FX
#endif // LIBS_AUDIO_FX_INSERTCHANNELS_H_
