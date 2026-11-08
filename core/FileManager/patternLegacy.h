#ifndef FILEMANAGER_PATTERNLEGACY_H_
#define FILEMANAGER_PATTERNLEGACY_H_

#include "mtSequencer.h"

// Pattern layout used by file versions up to 6: 128 rows per track, 8 bit track length.
// Old files are translated in this layout and then expanded in place to Sequencer::strPattern.
struct strPatternLegacy
{
    static constexpr int kRows = 128;

    Sequencer::strPattern::strFileHeader header;

    float tempoRezerwa;
    float swingRez;

    uint8_t rezerwa1;
    uint8_t rezerwa2;
    uint8_t rezerwa3;
    uint8_t rezerwa4;

    struct strTrack
    {
        uint8_t length;
        Sequencer::strPattern::strTrack::strStep step[kRows];
    } track[kAllTracks];

    uint32_t crc;
};

static_assert(sizeof(strPatternLegacy) == k16TracksPatternSize_v6);

#endif /* FILEMANAGER_PATTERNLEGACY_H_ */
