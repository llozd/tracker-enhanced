## ADDED Requirements

### Requirement: Randomise every engine
Every synth engine SHALL offer a randomise action that sets each of its sound parameters to a random value inside its range.

#### Scenario: Randomise
- **WHEN** the user randomises a synth part
- **THEN** its sound parameters change to random in-range values and the sound follows

### Requirement: Mix and setup kept
Randomise SHALL NOT change level, pan, sends, tuning, voice count, play mode, macro assignments or sample slots.

#### Scenario: Level kept
- **WHEN** the user randomises a part
- **THEN** its level, pan and sends are unchanged

### Requirement: Playable results
Randomise SHALL NOT produce a silent or stuck patch.

#### Scenario: Audible
- **WHEN** the user randomises and plays a note
- **THEN** the note is audible and ends after release

#### Scenario: Recover
- **WHEN** the user dislikes the result
- **THEN** randomising again, or loading a patch, replaces it

### Requirement: Previous patch
The parameter screen SHALL offer Previous, which restores the patch from before the last randomise.

#### Scenario: Go back
- **WHEN** the user randomises and then presses Shift + F8 (Previous)
- **THEN** the part's parameters return to their values before that randomise

#### Scenario: Nothing to restore
- **WHEN** no randomise happened since the part's patch was loaded or its engine changed
- **THEN** Previous changes nothing and shows "No previous patch"
