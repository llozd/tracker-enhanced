#include "SynthRandomiser.h"

#ifdef FEATURE_SYNTH

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <vector>

#include "Synth.h"
#include "interruptsControl.h"

namespace synthrandom
{
namespace
{

struct Snapshot
{
    bool valid = false;
    int typeIdx = -1;
    std::string patchName;
    std::vector<polysynth::Patch::optional_paramvalue_t> values;
};

Snapshot previous[kSynthParts];

struct Rng
{
    uint32_t state = 1;
    uint32_t next()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    float unit() { return static_cast<float>(next() >> 8) * (1.0f / 16777216.0f); } // 0..1
};

bool contains(const std::string& haystack, const char* needle)
{
    return haystack.find(needle) != std::string::npos;
}

// Mode parameters (CHIP's model, a line mode) change other parameters' metadata: set them first.
template<typename Fn>
void forEachParam(const polysynth::Patch& patch, bool modesFirst, Fn fn)
{
    for (int pass = 0; pass < (modesFirst ? 2 : 1); pass++)
    {
        const auto& params = patch.getMetadata().params;
        for (int i = 0; i < static_cast<int>(params.size()); i++)
        {
            const bool isMode = params[i].invalidatesOtherMetadata;
            if (!modesFirst || isMode == (pass == 0))
                fn(i);
        }
    }
}

} // namespace

Rule ruleFor(std::string_view displayName)
{
    std::string name(displayName);
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });

    for (const char* keep : { "volume", "tune", "fine", "glide", "pan", "send", "voice", "poly" })
    {
        if (contains(name, keep))
            return Rule::kKeep;
    }
    if (contains(name, "cutoff"))
        return Rule::kOpenCutoff;
    if (contains(name, "attack") || contains(name, " att"))
        return Rule::kShortAttack;
    if (contains(name, "sustain") || contains(name, " sus"))
        return Rule::kSustain;
    if (contains(name, "release") || contains(name, " rel") || contains(name, "decay") || contains(name, " dec"))
        return Rule::kShortTime;
    return Rule::kRandom;
}

void randomise(Synth& synth, int part, uint32_t seed)
{
    if (part < 0 || part >= kSynthParts)
        return;
    auto& patch = synth.getPatch(part);

    Snapshot& snap = previous[part];
    const auto& params = patch.getMetadata().params;
    snap.values.resize(params.size());
    for (size_t i = 0; i < params.size(); i++)
        snap.values[i] = patch.getParamValue(static_cast<int>(i));
    snap.typeIdx = synth.getInstrumentTypeIdx(part);
    snap.patchName = synth.getPatchName(part);
    snap.valid = true;

    Rng rng;
    rng.state = seed | 1u;
    for (int i = 0; i < 4; i++)
        rng.next();

    forEachParam(patch, true, [&](int i) {
        const auto& p = patch.getMetadata().params[i];
        if (p.displayName.empty() || p.minValue >= p.maxValue)
            return;
        const Rule rule = ruleFor(p.displayName);
        if (rule == Rule::kKeep)
            return;

        if (p.isEnum)
        {
            const int lo = static_cast<int>(p.minValue), hi = static_cast<int>(p.maxValue);
            const int value = lo + static_cast<int>(rng.next() % static_cast<uint32_t>(hi - lo + 1));
            DISABLE_AUDIO_INTERRUPTS();
            patch.setParamValue(i, static_cast<float>(value), polysynth::Patch::ParamChangeSource::kKnob);
            ENABLE_AUDIO_INTERRUPTS();
            return;
        }

        float u = rng.unit();
        switch (rule)
        {
            case Rule::kShortAttack: u *= 0.4f; break;
            case Rule::kSustain: u = 0.4f + 0.6f * u; break;
            case Rule::kShortTime: u *= 0.7f; break;
            case Rule::kOpenCutoff: u = 0.3f + 0.7f * u; break;
            default: break;
        }
        DISABLE_AUDIO_INTERRUPTS();
        patch.setParamValueEditMapped(i, u, polysynth::Patch::ParamChangeSource::kKnob);
        ENABLE_AUDIO_INTERRUPTS();
    });

    synth.setPatchDirty(part);
}

bool hasPrevious(Synth& synth, int part)
{
    if (part < 0 || part >= kSynthParts)
        return false;
    const Snapshot& snap = previous[part];
    return snap.valid && snap.typeIdx == synth.getInstrumentTypeIdx(part) &&
           snap.patchName == synth.getPatchName(part);
}

bool restorePrevious(Synth& synth, int part)
{
    if (!hasPrevious(synth, part))
        return false;
    Snapshot& snap = previous[part];
    auto& patch = synth.getPatch(part);
    forEachParam(patch, true, [&](int i) {
        if (i < static_cast<int>(snap.values.size()) && snap.values[i].has_value())
        {
            DISABLE_AUDIO_INTERRUPTS();
            patch.setParamValue(i, snap.values[i], polysynth::Patch::ParamChangeSource::kKnob);
            ENABLE_AUDIO_INTERRUPTS();
        }
    });
    snap.valid = false;
    synth.setPatchDirty(part);
    return true;
}

} // namespace synthrandom

#endif
