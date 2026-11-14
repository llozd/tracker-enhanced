#ifndef LIBS_AUDIO_EFFECT_INSERT_H_
#define LIBS_AUDIO_EFFECT_INSERT_H_

// Stereo insert point: runs one channel's insert slots (openspec realtime-effects).

#include "AudioStream.h"
#include "fx/insertChannels.h"

#if FEATURE_REALTIME_FX

class AudioEffectInsert : public AudioStream
{
  public:
    AudioEffectInsert()
      : AudioStream(2, inputQueueArray)
    {
    }

    void attach(insertfx::ChannelId id)
    {
        channelId = id;
    }

    virtual void update(void);

  private:
    audio_block_t* inputQueueArray[2];
    insertfx::ChannelId channelId = insertfx::kMaster;
};

#endif // FEATURE_REALTIME_FX
#endif // LIBS_AUDIO_EFFECT_INSERT_H_
