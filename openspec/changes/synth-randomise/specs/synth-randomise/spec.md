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
