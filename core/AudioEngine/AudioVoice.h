#ifndef AUDIOENGINE_AUDIOVOICE_H_
#define AUDIOENGINE_AUDIOVOICE_H_
#include "Audio.h"
#include "effect_insert.h"

class AudioVoice
{
  public:
    enum EnvelopeGeneratorType
    {
        kFilter,
        kWavetablePosition,
        kGranularLength = kWavetablePosition,
        kGranularPosition,
        kPanning,
        kFinetune,
        kEnvelopeGeneratorCount
    };

#ifdef PLATFORM_IMX
    enum VoiceChannels
    {
        kLeft,
        kRight,
        kChannelsCount
    };
#endif

    AudioVoice();
#ifndef PLATFORM_IMX
    AudioPlayMemory* getMemoryPlayer();
    AudioBitDepth* getBitDepth();
    AudioFilterStateVariable* getFilter();
    AudioAmplifier* getAmplifier();
    MonoToStereo* getMonoToStereo();
    AudioAnalyzeRMS* getRMS();
#else
    AudioPlayMemory* getMemoryPlayer(VoiceChannels channel);
    AudioBitDepth* getBitDepth(VoiceChannels channel);
    AudioFilterStateVariable* getFilter(VoiceChannels channel);
    AudioAmplifier* getAmplifier(VoiceChannels channel);
    AudioAmplifier* getPanningAmplifier(VoiceChannels channel);
    AudioAnalyzeRMS* getRMS(VoiceChannels channel);
    // What follows the filter: the insert if there is one. Sends tap from here.
    AudioStream* getPostFilter(VoiceChannels channel);
    uint8_t getPostFilterIndex(VoiceChannels channel);
#if FEATURE_REALTIME_FX
    AudioEffectInsert* getInsert();
#endif
#endif
    envelopeGenerator* getEnvelopeGenerator(EnvelopeGeneratorType type);

  private:
#ifndef PLATFORM_IMX
    AudioPlayMemory memoryPlayer;
    AudioBitDepth bitDepth;
    AudioFilterStateVariable filter;
    AudioAmplifier amplifier;
    MonoToStereo monoToStereo;
    AudioAnalyzeRMS rms;
#else
    AudioPlayMemory memoryPlayer[VoiceChannels::kChannelsCount];
    AudioBitDepth bitDepth[VoiceChannels::kChannelsCount];
    AudioFilterStateVariable filter[VoiceChannels::kChannelsCount];
#if FEATURE_REALTIME_FX
    AudioEffectInsert insert; // declared here so it updates between the filter and the amplifier
#endif
    AudioAmplifier amplifier[VoiceChannels::kChannelsCount];
    AudioAmplifier panningAmplifier[VoiceChannels::kChannelsCount];
    AudioAnalyzeRMS rms[VoiceChannels::kChannelsCount];
#endif

    envelopeGenerator envelope[kEnvelopeGeneratorCount];
    std::vector<AudioConnection*> connections;
};

#endif /* AUDIOENGINE_AUDIOVOICE_H_ */
