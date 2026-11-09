## ADDED Requirements

### Requirement: Selectable lines per beat
The project SHALL have a lines per beat setting of 1, 2, 4, 8 or 16, defaulting to the global default.

#### Scenario: New project
- **WHEN** a project is created
- **THEN** its LPB equals General > Default LPB

#### Scenario: Change LPB
- **WHEN** the user changes Project Settings > Lines per beat
- **THEN** playback stops, undo history is cleared and the pattern divider becomes one beat

### Requirement: Step-relative timing
Step-based effects and MIDI sync SHALL follow the LPB.

#### Scenario: Roll at higher LPB
- **WHEN** LPB goes from 4 to 8 at the same tempo
- **THEN** a step lasts half as long and a Roll period shortens with it

#### Scenario: MIDI clock
- **WHEN** clock is sent at any LPB
- **THEN** 24 pulses per beat are emitted

### Requirement: Pattern length scales with LPB
Pattern lengths SHALL keep their duration when LPB changes, up to 512 rows.

#### Scenario: Raise LPB
- **WHEN** a 64 row pattern at 4 LPB is changed to 16 LPB
- **THEN** its length becomes 256 rows and notes stay in their rows

#### Scenario: Lower LPB
- **WHEN** the scaled length is not a whole number of rows
- **THEN** it is rounded to the nearest row, minimum 1

### Requirement: Old pattern files load
Pattern files of version 7 or older SHALL load into the 512 row layout.

#### Scenario: Legacy pattern
- **WHEN** a 128 row pattern file is opened
- **THEN** all steps are preserved and its LPB is 4

#### Scenario: 1024 row pattern
- **WHEN** a version 7 pattern file longer than 512 rows is opened
- **THEN** its first 512 rows are kept and its length is clamped to 512
