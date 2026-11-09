#ifndef PATTERN_FILE_VALIDATION_H
#define PATTERN_FILE_VALIDATION_H

#include <stddef.h>
#include <stdint.h>

namespace PatternFileValidation
{
constexpr uint8_t kLastLegacyPatternVersion = 5;
constexpr uint8_t kLastNarrowPatternVersion = 6; // up to 128 rows per track
constexpr uint8_t kWidePatternVersion = 7;        // 1024 rows per track
constexpr uint16_t k8TracksPatternSizeV5 = 6184;
constexpr uint16_t k12TracksPatternSizeV5 = 9260;
constexpr uint16_t k16TracksPatternSizeV5 = 12336;
constexpr uint16_t k8TracksPatternSizeV6 = 8232;
constexpr uint16_t k12TracksPatternSizeV6 = 12332;
constexpr uint16_t k16TracksPatternSizeV6 = 16432;
// since version 7 the header size field is unused, only the file size is checked
constexpr size_t kPatternSizeV7 = 131136;
constexpr size_t kPatternSizeV8 = 65600;
constexpr size_t kCrcSize = sizeof(uint32_t);

struct Layout
{
    size_t crcDataSize;
    bool hasCrc;
};

inline bool isKnownPatternSize(uint8_t version, uint16_t size)
{
    if (version > kLastNarrowPatternVersion)
    {
        return true;
    }

    if (version <= kLastLegacyPatternVersion)
    {
        return size == k8TracksPatternSizeV5 || size == k12TracksPatternSizeV5 ||
               size == k16TracksPatternSizeV5;
    }

    return size == k8TracksPatternSizeV6 || size == k12TracksPatternSizeV6 ||
           size == k16TracksPatternSizeV6;
}

inline bool resolveLayout(uint8_t version,
                          uint16_t declaredSize,
                          size_t fileSize,
                          Layout& layout)
{
    if (!isKnownPatternSize(version, declaredSize))
    {
        return false;
    }

    if (version > kLastNarrowPatternVersion)
    {
        if (fileSize != (version == kWidePatternVersion ? kPatternSizeV7 : kPatternSizeV8))
        {
            return false;
        }

        layout = { fileSize - kCrcSize, true };
        return true;
    }

    if (fileSize == declaredSize)
    {
        layout = { fileSize - kCrcSize, true };
        return true;
    }

    if (version <= kLastLegacyPatternVersion && fileSize + kCrcSize == declaredSize)
    {
        layout = { fileSize, false };
        return true;
    }

    return false;
}
}

#endif
