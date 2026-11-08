## ADDED Requirements

### Requirement: More CC slots per MIDI instrument
A MIDI instrument SHALL support more CC assignments than the current 6.

#### Scenario: Assign extra CC
- **WHEN** the user assigns a CC to a slot beyond the sixth
- **THEN** it is saved with the instrument and available as a step FX

### Requirement: More than two CCs per row
A step SHALL be able to send more than two CC messages.

#### Scenario: Three CCs on one row
- **WHEN** three CC values are set on one step
- **THEN** all three are sent when the step plays
