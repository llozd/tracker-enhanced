#include "AudioVoice.h"

#ifndef PLATFORM_IMX
AudioVoice::AudioVoice()
{
    connections.push_back(new AudioConnection(&memoryPlayer, &bitDepth));
    connections.push_back(new AudioConnection(&bitDepth, &filter));
    connections.push_back(new AudioConnection(&filter, &amplifier));
    connections.push_back(new AudioConnection(&amplifier, &monoToStereo));
    connections.push_back(new AudioConnection(&amplifier, &rms));
}
#else
AudioVoice::AudioVoice()
{
    for (int channel = 0; channel < VoiceChannels::kChannelsCount; channel++)
    {
        connections.push_back(new AudioConnection(&memoryPlayer[channel], &bitDepth[channel]));
        connections.push_back(new AudioConnection(&bitDepth[channel], &filter[channel]));
#if FEATURE_REALTIME_FX
        connections.push_back(new AudioConnection(&filter[channel], 0, &insert, channel));
        connections.push_back(new AudioConnection(&insert, channel, &amplifier[channel], 0));
#else
        connections.push_back(new AudioConnection(&filter[channel], &amplifier[channel]));
#endif
        connections.push_back(new AudioConnection(&amplifier[channel], &panningAmplifier[channel]));
        connections.push_back(new AudioConnection(&amplifier[channel], &rms[channel]));
    }
}

AudioStream* AudioVoice::getPostFilter([[maybe_unused]] VoiceChannels channel)
{
#if FEATURE_REALTIME_FX
    return &insert;
#else
    return &filter[channel];
#endif
}

uint8_t AudioVoice::getPostFilterIndex(VoiceChannels channel)
{
#if FEATURE_REALTIME_FX
    return channel;
#else
    return 0;
#endif
}

#if FEATURE_REALTIME_FX
AudioEffectInsert* AudioVoice::getInsert()
{
    return &insert;
}
#endif
#endif

#ifndef PLATFORM_IMX
AudioPlayMemory* AudioVoice::getMemoryPlayer()
{
    return &memoryPlayer;
}
#else
AudioPlayMemory* AudioVoice::getMemoryPlayer(VoiceChannels channel)
{
    return &memoryPlayer[channel];
}
#endif

#ifndef PLATFORM_IMX
AudioBitDepth* AudioVoice::getBitDepth()
{
    return &bitDepth;
}
#else
AudioBitDepth* AudioVoice::getBitDepth(VoiceChannels channel)
{
    return &bitDepth[channel];
}
#endif

#ifndef PLATFORM_IMX
AudioFilterStateVariable* AudioVoice::getFilter()
{
    return &filter;
}
#else
AudioFilterStateVariable* AudioVoice::getFilter(VoiceChannels channel)
{
    return &filter[channel];
}
#endif

#ifndef PLATFORM_IMX
AudioAmplifier* AudioVoice::getAmplifier()
{
    return &amplifier;
}
#else
AudioAmplifier* AudioVoice::getAmplifier(VoiceChannels channel)
{
    return &amplifier[channel];
}
#endif

#ifndef PLATFORM_IMX
MonoToStereo* AudioVoice::getMonoToStereo()
{
    return &monoToStereo;
}
#else
AudioAmplifier* AudioVoice::getPanningAmplifier(VoiceChannels channel)
{
    return &panningAmplifier[channel];
}
#endif

#ifndef PLATFORM_IMX
AudioAnalyzeRMS* AudioVoice::getRMS()
{
    return &rms;
}
#else
AudioAnalyzeRMS* AudioVoice::getRMS(VoiceChannels channel)
{
    return &rms[channel];
}
#endif

envelopeGenerator* AudioVoice::getEnvelopeGenerator(EnvelopeGeneratorType type)
{
    return &envelope[type];
}
