# Effects on rows without a note

**Status: implemented**, untested on hardware

**Platforms:** all. On the Original only the MIDI part applies, because synth tracks are already compiled out there (`FEATURE_SYNTH`). No new gate.

## Problem
Microtune (pitchbend) and synth effects placed on a row with no note do nothing on MIDI and synth tracks. A note cannot be bent a few rows after it starts, for example on its release.

## Findings
- The sequencer routes effects by the row's own instrument (`stepToSend.instrument`). An empty step stores instrument `0`, which is sample instrument 1.
- `FX_TYPE_MICROTUNING` (`core/source/mtSequencer.cpp`, about line 653) only sends pitchbend when that instrument is MIDI or synth, so empty rows send nothing.
- The synth effects path (`engine.isSynthFx`, about line 502) has the same check.
- `FX_TYPE_OFF` already uses the track's last played instrument (`stepSent.instrument`).
- Sample tracks send empty-row effects straight to the track's sample player, so they likely already work. Behaviour during the release tail is unchecked.

## What changes
On rows without a note, MIDI and synth effects use the track's last played instrument. Pitchbend is channel-wide, so it also affects a note's release tail.
