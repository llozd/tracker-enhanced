## ADDED Requirements

### Requirement: All 16 tracks on one screen
The pattern editor SHALL offer a view that shows all 16 tracks at once, with note, FX1 and FX2 cells per track shown as activity marks instead of values.

#### Scenario: Open and close
- **WHEN** the user holds Pattern and presses and releases Shift, without pressing an arrow
- **THEN** the pattern shows all 16 tracks
- **AND WHEN** the user then taps Pattern
- **THEN** the default view returns, with the cursor on the same track and row

#### Scenario: Activity marks
- **WHEN** a step has a note and an FX1 but no FX2
- **THEN** its note cell and FX1 cell show a dot and its FX2 cell is empty

#### Scenario: Note off
- **WHEN** a step has a note off, cut or fade
- **THEN** its note cell shows a dash, not a dot

### Requirement: Editing still works
Cursor movement, pad note entry, the encoder, delete, copy/paste and selection SHALL work in the all tracks view, with the values of the step under the cursor shown in the bottom bar.

#### Scenario: Enter a note
- **WHEN** the cursor is on a note cell in edit mode and the user plays a pad
- **THEN** the note is entered and its cell shows a dot
