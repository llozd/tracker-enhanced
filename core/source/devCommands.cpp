// Dev console commands handled by the core (see docs/developer_guide.md, section 8).

#ifdef ENABLE_DEV_CONSOLE

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Profiler.h"
#include "fx/insertChannels.h"

namespace
{
#if FEATURE_REALTIME_FX
// t1..t8, s1..s3 or m
bool parseChannel(const char* s, insertfx::ChannelId& id)
{
    const int n = atoi(s + 1);
    switch (tolower(s[0]))
    {
        case 't':
            if (n < 1 || n > 8)
                return false;
            id = static_cast<insertfx::ChannelId>(insertfx::kTrack0 + n - 1);
            return true;
        case 's':
            if (n < 1 || n > 3)
                return false;
            id = static_cast<insertfx::ChannelId>(insertfx::kSynth0 + n - 1);
            return true;
        case 'm': id = insertfx::kMaster; return true;
        default: return false;
    }
}

bool parseType(const char* s, insertfx::Type& type)
{
    if (isdigit(static_cast<unsigned char>(s[0])))
    {
        const int n = atoi(s);
        if (n < 0 || n >= insertfx::kTypeCount)
            return false;
        type = static_cast<insertfx::Type>(n);
        return true;
    }
    for (int t = 0; t < insertfx::kTypeCount; t++)
    {
        if (strcasecmp(s, insertfx::describe(static_cast<insertfx::Type>(t)).abbr) == 0)
        {
            type = static_cast<insertfx::Type>(t);
            return true;
        }
    }
    return false;
}

// fx list | fx <channel> <slot> <type> [params...]
void handleFx(char* argument, char* reply, size_t size)
{
    char* tokens[2 + insertfx::kParams + 1];
    int count = 0;
    for (char* t = strtok(argument, " "); t && count < (int)(sizeof(tokens) / sizeof(*tokens));
         t = strtok(nullptr, " "))
    {
        tokens[count++] = t;
    }

    if (count == 1 && strcmp(tokens[0], "list") == 0)
    {
        size_t used = snprintf(reply, size, "ok");
        for (int t = 0; t < insertfx::kTypeCount && used < size; t++)
        {
            used += snprintf(reply + used, size - used, " %s",
                             insertfx::describe(static_cast<insertfx::Type>(t)).abbr);
        }
        return;
    }

    insertfx::ChannelId id;
    insertfx::Type type;
    const int slotIdx = count >= 2 ? atoi(tokens[1]) - 1 : -1;
    if (count < 3 || !parseChannel(tokens[0], id) || slotIdx < 0 ||
        slotIdx >= insertfx::kSlots || !parseType(tokens[2], type))
    {
        snprintf(reply, size, "err usage: fx list | fx t1-t8|s1-s3|m <slot 1-3> <type> [params]");
        return;
    }

    insertfx::Slot slot;
    insertfx::setDefaults(slot, type);
    for (int i = 3; i < count && i - 3 < insertfx::kParams; i++)
    {
        slot.params[i - 3] = static_cast<int16_t>(atoi(tokens[i]));
    }
    if (!insertfx::setSlot(id, slotIdx, slot))
    {
        snprintf(reply, size, "err no free reverb (%d at most)", insertfx::kReverbPool);
        return;
    }
    snprintf(reply, size, "ok fx %s %d %s", tokens[0], slotIdx + 1, insertfx::describe(type).abbr);
}
#endif

void handleCpu(char* reply, size_t size)
{
    Profiler::AudioProcessorUsage usage;
    Profiler::getInstance()->getAudioProcessingUsagePercent(usage, true);
    snprintf(reply, size, "ok cpu avg %d max %d min %d", usage.average, usage.tempMaximum,
             usage.minimum);
}
} // namespace

// Called by the platform console for commands it does not know. Returns false if unknown.
bool onDevCommand(const char* command, char* argument, char* reply, size_t size)
{
#if FEATURE_REALTIME_FX
    if (strcmp(command, "fx") == 0)
    {
        handleFx(argument, reply, size);
        return true;
    }
#endif
    if (strcmp(command, "cpu") == 0)
    {
        handleCpu(reply, size);
        return true;
    }
    return false;
}

#endif // ENABLE_DEV_CONSOLE
