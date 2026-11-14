#include "insertChannels.h"

#if FEATURE_REALTIME_FX

#include <cr_section_macros.h>
#include <cstring>
#include <new>

#include "interruptsControl.h"
#include "reverb.h"

namespace insertfx
{
namespace
{
constexpr int kReverbLineLength = 16384; // Reverb's FxEngine size, per buffer

__NOINIT(EXTERNAL_RAM) Channel channels[kChannelCount];
__NOINIT(EXTERNAL_RAM) float delayLines[kChannelCount][kSlots][2 * kDelayLength];

__NOINIT(EXTERNAL_RAM) alignas(Reverb) uint8_t reverbStorage[kReverbPool][sizeof(Reverb)];
__NOINIT(EXTERNAL_RAM) uint16_t reverbBuffers[kReverbPool][2][kReverbLineLength];
__NOINIT(EXTERNAL_RAM) int16_t reverbScratch[2][128];
bool reverbInUse[kReverbPool];

Reverb& reverbAt(int idx)
{
    return *reinterpret_cast<Reverb*>(reverbStorage[idx]);
}

// Audio interrupt must be off.
uint8_t acquireReverb()
{
    for (int i = 0; i < kReverbPool; i++)
    {
        if (!reverbInUse[i])
        {
            reverbInUse[i] = true;
            reverbAt(i).Reset();
            return static_cast<uint8_t>(i + 1);
        }
    }
    return 0;
}

void releaseReverb(uint8_t entry)
{
    if (entry > 0 && entry <= kReverbPool)
        reverbInUse[entry - 1] = false;
}

void runReverb(State& st, const Slot& slot, float* l, float* r, uint32_t n)
{
    if (st.reverb == 0 || n > 128)
        return;
    Reverb& rv = reverbAt(st.reverb - 1);
    const Desc& d = describe(kReverb);
    float p[5];
    for (int i = 0; i < 5; i++)
    {
        const int16_t v = slot.params[i];
        p[i] = (v < d.p[i].min ? d.p[i].min : v > d.p[i].max ? d.p[i].max : v) / 127.0f;
    }
    rv.SetTime(0.97f * p[0]);
    rv.SetLP(1.0f - p[1]);
    rv.SetPredelayLength(p[2]);
    rv.SetDiffusion(p[3]);
    rv.SetAmount(p[4]);

    int16_t* sl = reverbScratch[0];
    int16_t* sr = reverbScratch[1];
    for (uint32_t i = 0; i < n; i++)
    {
        const float vl = l[i] * 32767.0f, vr = (r ? r[i] : l[i]) * 32767.0f;
        sl[i] = static_cast<int16_t>(vl > 32767.0f ? 32767.0f : vl < -32767.0f ? -32767.0f : vl);
        sr[i] = static_cast<int16_t>(vr > 32767.0f ? 32767.0f : vr < -32767.0f ? -32767.0f : vr);
    }
    rv.Process(sl, sr, n);
    for (uint32_t i = 0; i < n; i++)
    {
        l[i] = sl[i] * (1.0f / 32767.0f);
        if (r)
            r[i] = sr[i] * (1.0f / 32767.0f);
    }
}
} // namespace

void initChannels()
{
    for (int i = 0; i < kReverbPool; i++)
    {
        Reverb* rv = new (reverbStorage[i]) Reverb();
        rv->Init(reverbBuffers[i][0], reverbBuffers[i][1]);
        rv->SetInputGain(0.2f);
        rv->Reset();
        reverbInUse[i] = false;
    }
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

bool setSlot(ChannelId id, int slotIdx, const Slot& slot)
{
    if (id >= kChannelCount || slotIdx < 0 || slotIdx >= kSlots)
        return false;
    bool ok = true;
    DISABLE_AUDIO_INTERRUPTS();
    Channel& ch = channels[id];
    State& st = ch.state[slotIdx];
    const bool wasReverb = ch.slot[slotIdx].type == kReverb, isReverb = slot.type == kReverb;
    if (isReverb && !wasReverb)
    {
        st.reverb = acquireReverb();
        ok = st.reverb != 0;
    }
    else if (!isReverb && wasReverb)
    {
        releaseReverb(st.reverb);
        st.reverb = 0;
    }
    if (ok)
        ch.slot[slotIdx] = slot;
    ENABLE_AUDIO_INTERRUPTS();
    return ok;
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
        if (s.type == kChorus || s.type == kReverb)
            return true;
    }
    return false;
}

void processChannel(Channel& ch, float* l, float* r, uint32_t n)
{
    for (int s = 0; s < kSlots; s++)
    {
        if (ch.slot[s].type == kReverb)
            runReverb(ch.state[s], ch.slot[s], l, r, n);
        else
            run(ch.state[s], ch.slot[s], l, r, n);
    }
}

} // namespace insertfx

#endif // FEATURE_REALTIME_FX
