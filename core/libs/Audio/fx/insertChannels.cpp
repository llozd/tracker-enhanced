#include "insertChannels.h"

#if FEATURE_REALTIME_FX

#include <cr_section_macros.h>
#include <cstring>

#include "interruptsControl.h"

namespace insertfx
{
namespace
{
__NOINIT(EXTERNAL_RAM) Channel channels[kChannelCount];
__NOINIT(EXTERNAL_RAM) float delayLines[kChannelCount][kSlots][2 * kDelayLength];
} // namespace

void initChannels()
{
    for (int c = 0; c < kChannelCount; c++)
    {
        Channel& ch = channels[c];
        std::memset(&ch, 0, sizeof(ch));
        for (int s = 0; s < kSlots; s++)
        {
            setDefaults(ch.slot[s], kOff);
            ch.state[s].delay = delayLines[c][s];
            reset(ch.state[s]);
        }
    }
}

Channel& channel(ChannelId id)
{
    return channels[id < kChannelCount ? id : kMaster];
}

void setSlot(ChannelId id, int slotIdx, const Slot& slot)
{
    if (id >= kChannelCount || slotIdx < 0 || slotIdx >= kSlots)
        return;
    DISABLE_AUDIO_INTERRUPTS();
    channels[id].slot[slotIdx] = slot;
    ENABLE_AUDIO_INTERRUPTS();
}

const Slot& getSlot(ChannelId id, int slotIdx)
{
    return channel(id).slot[slotIdx < 0 || slotIdx >= kSlots ? 0 : slotIdx];
}

bool hasActiveSlot(const Channel& ch)
{
    for (const Slot& s : ch.slot)
    {
        if (s.type != kOff && s.type < kTypeCount)
            return true;
    }
    return false;
}

bool widensStereo(const Channel& ch)
{
    for (const Slot& s : ch.slot)
    {
        if (s.type == kChorus)
            return true;
    }
    return false;
}

void processChannel(Channel& ch, float* l, float* r, uint32_t n)
{
    for (int s = 0; s < kSlots; s++)
    {
        run(ch.state[s], ch.slot[s], l, r, n);
    }
}

} // namespace insertfx

#endif // FEATURE_REALTIME_FX
