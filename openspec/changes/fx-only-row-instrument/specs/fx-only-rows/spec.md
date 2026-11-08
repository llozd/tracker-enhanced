## MODIFIED Requirements

### Requirement: Effects on note-less rows reach the playing instrument
On a row with effects but no note, MIDI and synth effects SHALL target the track's last played instrument.

#### Scenario: Bend a synth note later
- **WHEN** a synth note is played and Microtune is set on an empty row three rows later
- **THEN** pitchbend is sent to that synth part on that row

#### Scenario: Bend during release
- **WHEN** the note's gate has ended and the release is still sounding
- **THEN** the pitchbend still affects the release tail

#### Scenario: No note played yet
- **WHEN** no note has played on the track since playback started
- **THEN** the effect is ignored as before
