## ADDED Requirements

### Requirement: Jump to the other half of the tracks
Pattern + Shift + Right SHALL move the cursor 8 tracks right from tracks 1 to 8, and Pattern + Shift + Left SHALL move it 8 tracks left from tracks 9 to 16, keeping the row and the selected column.

#### Scenario: Compact view
- **WHEN** the compact view shows tracks 1 to 8 with the cursor on track 3, and the user holds Pattern and Shift and presses Right
- **THEN** the view shows tracks 9 to 16 and the cursor is on track 11, same row and column

#### Scenario: Default view
- **WHEN** the default view shows tracks 2 to 5 with the cursor on track 4, and the user holds Pattern and Shift and presses Right
- **THEN** the view shows tracks 10 to 13 and the cursor is on track 12

#### Scenario: Already there
- **WHEN** the cursor is on tracks 9 to 16 and the user holds Pattern and Shift and presses Right
- **THEN** nothing changes

### Requirement: Existing shortcuts kept
Pattern + Left/Right and Pattern + Up/Down SHALL still change the pattern, and Shift + arrows SHALL still select.

#### Scenario: Change pattern
- **WHEN** the user holds Pattern (without Shift) and presses Right
- **THEN** the next pattern is selected, or queued while playing, as before
