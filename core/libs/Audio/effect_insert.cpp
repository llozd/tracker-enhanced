#include "effect_insert.h"

#if FEATURE_REALTIME_FX

#include <cr_section_macros.h>

namespace
{
__NOINIT(EXTERNAL_RAM) float bufL[AUDIO_BLOCK_SAMPLES];
__NOINIT(EXTERNAL_RAM) float bufR[AUDIO_BLOCK_SAMPLES];

constexpr float kToFloat = 1.0f / 32768.0f;

void toFloat(const audio_block_t* block, float* out)
{
    if (!block)
    {
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++)
            out[i] = 0.0f;
        return;
    }
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++)
        out[i] = block->data[i] * kToFloat;
}

// Returns true if any sample is not zero.
bool toInt16(const float* in, audio_block_t* block)
{
    int32_t any = 0;
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++)
    {
        float v = in[i] * 32768.0f;
        v = v > 32767.0f ? 32767.0f : v < -32768.0f ? -32768.0f : v;
        const int16_t s = static_cast<int16_t>(v);
        block->data[i] = s;
        any |= s;
    }
    return any != 0;
}
} // namespace

void AudioEffectInsert::update(void)
{
    insertfx::Channel& ch = insertfx::channel(channelId);

    if (!insertfx::hasActiveSlot(ch))
    {
        ch.sleeping = false;
        int32_t peak = 0;
        for (int c = 0; c < 2; c++)
        {
            audio_block_t* block = receiveReadOnly(c);
            if (block)
            {
                for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++)
                {
                    const int32_t v = block->data[i] < 0 ? -block->data[i] : block->data[i];
                    peak = v > peak ? v : peak;
                }
                transmit(block, c);
                release(block);
            }
        }
        // the meters still show the channel's level with every slot empty
        const float level = peak * kToFloat;
        ch.inPeak = level > ch.inPeak ? level : ch.inPeak;
        ch.outPeak = ch.inPeak;
        return;
    }

    audio_block_t* inL = receiveReadOnly(0);
    audio_block_t* inR = receiveReadOnly(1);
    const bool silentInput = !inL && !inR;

    if (silentInput && ch.sleeping)
    {
        return;
    }
    ch.sleeping = false;

    toFloat(inL, bufL);
    toFloat(inR, bufR);
    if (inL)
        release(inL);
    if (inR)
        release(inR);

    insertfx::processChannel(ch, bufL, bufR, AUDIO_BLOCK_SAMPLES);

    audio_block_t* outL = allocate();
    audio_block_t* outR = allocate();
    if (!outL || !outR)
    {
        if (outL)
            release(outL);
        if (outR)
            release(outR);
        return;
    }

    const bool soundL = toInt16(bufL, outL);
    const bool soundR = toInt16(bufR, outR);

    if (silentInput && !soundL && !soundR)
    {
        // The tails are done: stay quiet until the input has sound again.
        ch.sleeping = true;
    }
    else
    {
        transmit(outL, 0);
        transmit(outR, 1);
    }
    release(outL);
    release(outR);
}

#endif // FEATURE_REALTIME_FX
