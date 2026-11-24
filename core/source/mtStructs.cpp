#include "mtStructs.h"

void strMtValues::setDefaultTrackName(int trackIdx)
{
    assert(trackIdx < kMaxAllTracksInTrackers);

    if (trackIdx < kSampleTracks)
    {
        sprintf(TrackNames[trackIdx], "Track %d", trackIdx + 1);
    }
    else
    {
        sprintf(midiTrackNames[trackIdx - kSampleTracks], "Midi %d", trackIdx + 1);
    }
}

void strMtValues::setDefaultBuses()
{
    for (uint8_t& output : channelOutput)
    {
        output = kOutputMaster;
    }
    for (int bus = 0; bus < kBusCount; bus++)
    {
        busVolume[bus] = kBusVolumeDefault;
        for (strInsertSlot& slot : busInsertSlot[bus])
        {
            slot = strInsertSlot{};
        }
    }
}
