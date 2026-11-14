#pragma once

// Synth randomise (openspec synth-randomise): random sound parameters for any engine, through
// the Patch metadata, and one level of Previous.

#include "config.h"
#ifdef FEATURE_SYNTH

#include <cstdint>
#include <string_view>

class Synth;

namespace synthrandom
{

enum class Rule : uint8_t
{
    kRandom,      // anywhere in its range
    kKeep,        // mix and setup: volume, tuning, glide, voices, pan, sends
    kShortAttack, // the lower 40 % of its range
    kSustain,     // the upper 60 %
    kShortTime,   // decay and release: the lower 70 %
    kOpenCutoff,  // the upper 70 %
};

// How a parameter is treated, by its display name.
Rule ruleFor(std::string_view displayName);

// Randomises part's patch and keeps its values for Previous. seed: any changing number.
void randomise(Synth& synth, int part, uint32_t seed);

// True when Previous can restore: a randomise happened and the part kept its engine and patch.
bool hasPrevious(Synth& synth, int part);

// Restores the values from before the last randomise. False when there is nothing to restore.
bool restorePrevious(Synth& synth, int part);

} // namespace synthrandom

#endif
