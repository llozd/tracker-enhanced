## MODIFIED Requirements

### Requirement: Performance effects keep their musical length
Pattern Length and Step Stutter SHALL last the same musical time at every lines per beat as at 4 lines per beat.

#### Scenario: Pattern Length at 16 LPB
- **WHEN** the project is at 16 LPB and Pattern Length is set to 16
- **THEN** the pattern loops every 64 rows (one bar)

#### Scenario: Pattern Length at 1 LPB
- **WHEN** the project is at 1 LPB and Pattern Length is set to 1
- **THEN** the pattern loops every row (the shortest possible loop)

#### Scenario: Step Stutter at 8 LPB
- **WHEN** the project is at 8 LPB and Step Stutter is set to 1
- **THEN** the note retriggers every 2 rows (one 16th note)
