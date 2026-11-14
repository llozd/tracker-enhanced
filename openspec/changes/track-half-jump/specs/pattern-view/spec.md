## ADDED Requirements

### Requirement: Jump to the other half of the tracks
Pattern + Right SHALL move the cursor 8 tracks right from tracks 1 to 8, and Pattern + Left SHALL move it 8 tracks left from tracks 9 to 16, keeping the row and the selected column.

#### Scenario: Compact view
- **WHEN** the compact view shows tracks 1 to 8 with the cursor on track 3, and the user presses Pattern + Right
- **THEN** the view shows tracks 9 to 16 and the cursor is on track 11, same row and column

#### Scenario: Default view
- **WHEN** the default view shows tracks 2 to 5 with the cursor on track 4, and the user presses Pattern + Right
- **THEN** the view shows tracks 10 to 13 and the cursor is on track 12

#### Scenario: Already there
- **WHEN** the cursor is on tracks 9 to 16 and the user presses Pattern + Right
- **THEN** nothing changes

## MODIFIED Requirements

### Requirement: Changing pattern from the pattern editor
Pattern + Up/Down and Pattern + the jog wheel SHALL change the pattern, and while playing Pattern + Up/Down SHALL queue the next pattern. Pattern + Left/Right SHALL no longer change the pattern.

#### Scenario: Queue while playing
- **WHEN** a pattern is playing and the user presses Pattern + Up
- **THEN** the next pattern is queued to play after the current one
