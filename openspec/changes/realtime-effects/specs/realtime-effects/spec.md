## ADDED Requirements

### Requirement: Effects on tracks and master
Each new effect SHALL be usable on an individual track and on the master channel.

#### Scenario: Insert on a track
- **WHEN** the user enables an effect on a track
- **THEN** only that track's audio is processed

### Requirement: Bounded CPU use
The firmware SHALL not drop audio when effects are enabled.

#### Scenario: Limit reached
- **WHEN** enabling another effect would exceed the CPU budget
- **THEN** it is refused or the user is told, and audio continues cleanly
