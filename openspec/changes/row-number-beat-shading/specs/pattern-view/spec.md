## ADDED Requirements

### Requirement: Row numbers show position in the beat
Row numbers SHALL be shaded by their position within the beat.

#### Scenario: Beat row
- **WHEN** a row is the first row of a beat
- **THEN** its number is drawn at full brightness

#### Scenario: Half-beat row
- **WHEN** LPB is even and a row is halfway through a beat
- **THEN** its number is dimmer than a beat row and brighter than other rows

#### Scenario: Other rows
- **WHEN** a row is neither a beat nor a half-beat row
- **THEN** its number is drawn at the dimmest shade

#### Scenario: One line per beat
- **WHEN** LPB is 1
- **THEN** all row numbers are drawn at full brightness
