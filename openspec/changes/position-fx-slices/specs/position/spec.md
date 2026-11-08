## MODIFIED Requirements

### Requirement: Position is slice-relative
Position SHALL set the start offset within the played slice.

#### Scenario: Slice playback
- **WHEN** a slice is played with Position at 50%
- **THEN** playback starts halfway between that slice's start and end

#### Scenario: No slices
- **WHEN** a sample without slices is played
- **THEN** Position behaves as before
