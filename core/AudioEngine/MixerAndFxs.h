#ifndef AUDIOENGINE_MIXERANDFXS_H_
#define AUDIOENGINE_MIXERANDFXS_H_
#include "Audio.h"
#include "effect_insert.h"

enum struct MixerAndFxChannelType
{
    kLeft,
    kRight,
    kChannelCount
};

template<int voicesCount>
class MixerAndFxs
{
  public:
    enum FxType
    {
        kDelay,
        kReverb,
        kFxCount
    };

    enum TrackAndFxsChannels
    {
        kDryMixChannel,
        kDelayChannel,
        kReverbChannel,
        kInputChannel
    };

    static constexpr int kExternalInputsCount = 1;
#ifndef PLATFORM_IMX
    static constexpr int kSendsInputsCount = 2 * kExternalInputsCount + voicesCount;
#else
    static constexpr int kSendsInputsCount =
      2 * kExternalInputsCount + 2 * voicesCount + kSynthParts;
#endif
    static constexpr int kTrackMixerOutputsCount = 1;
    static constexpr int kTrackAndSendsMixerInputs =
      kTrackMixerOutputsCount + kExternalInputsCount + FxType::kFxCount;
    static constexpr int kTracksAndSynthVoices = voicesCount + kSynthParts;
#if FEATURE_REALTIME_FX
    static constexpr int kBusCount = insertfx::kBusCount;
#else
    static constexpr int kBusCount = 0;
#endif
    // The tracks mixer takes the tracks and synth parts, then the buses' returns.
    static constexpr int kTracksMixerInputs = kTracksAndSynthVoices + kBusCount;

    MixerAndFxs();
    AudioMixer<kSendsInputsCount>* getPreFxMixer(FxType type);
    AudioMixer<kTracksMixerInputs>* getTracksMixer(MixerAndFxChannelType channel);
#if FEATURE_REALTIME_FX
    // bus-channels: each bus sums the same inputs as the tracks mixer, then runs its insert.
    AudioMixer<kTracksAndSynthVoices>* getBusMixer(int bus, MixerAndFxChannelType channel);
    AudioEffectInsert* getBusInsert(int bus);
#endif
    AudioMixer<kTrackAndSendsMixerInputs>* getTracksAndSendsMixer(MixerAndFxChannelType channel);
    AudioEffectShortDelay* getDelay();
    AudioEffectPolyverb* getPolyverb();
    AudioAnalyzeRMS* getDelayRMS(MixerAndFxChannelType channel);
    AudioAnalyzeRMS* getPolyverbRMS(MixerAndFxChannelType channel);
    AudioAnalyzeRMS* getDryMixRMS(MixerAndFxChannelType channel);
#ifdef FEATURE_SYNTH
    AudioAnalyzeRMS* getSynthRMS(int synthIdx, MixerAndFxChannelType channel);
#endif

  private:
    AudioMixer<kSendsInputsCount> preFxMixer[FxType::kFxCount];
#if FEATURE_REALTIME_FX
    // declared before the tracks mixer so they update first in each block
    AudioMixer<kTracksAndSynthVoices>
      busMixer[kBusCount][(int)MixerAndFxChannelType::kChannelCount];
    AudioEffectInsert busInsert[kBusCount];
#endif
    AudioMixer<kTracksMixerInputs> tracksAndSynthMixer[(int)MixerAndFxChannelType::kChannelCount];
    AudioMixer<kTrackAndSendsMixerInputs>
      tracksAndSendsMixer[(int)MixerAndFxChannelType::kChannelCount];

    AudioAnalyzeRMS delayRMS[(int)MixerAndFxChannelType::kChannelCount];
    AudioAnalyzeRMS polyverbRMS[(int)MixerAndFxChannelType::kChannelCount];
    AudioAnalyzeRMS dryMixRMS[(int)MixerAndFxChannelType::kChannelCount];
#ifdef FEATURE_SYNTH
    AudioAnalyzeRMS synthRMS[kSynthParts][(int)MixerAndFxChannelType::kChannelCount];
#endif

    AudioEffectShortDelay shortDelay;
    AudioEffectPolyverb polyverb;

    std::vector<AudioConnection*> connections;
};

#endif /* AUDIOENGINE_MIXERANDFXS_H_ */
