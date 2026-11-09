# Legato rolls

**Status: implemented**, untested on hardware

**Platforms:** all. Sequencer-only, so no feature gate. The declick must stay cheap enough for the Original.

## Why
Rolls and fast runs of notes sound gappy, with a tiny stop between hits. This exists in older firmware too.

## Findings
In `core/source/mtSequencer.cpp` (roll handling, about lines 1085 to 1155):
- Roll note length is set to half the roll period, so the gate closes at 50% of each hit.
- `stopOpenNoteOnTrack` sends a note-off before each retrigger, starting the envelope release.
- The next note-on restarts the voice from the sample start.

## What changes
Make roll hits legato: gate each hit for the full period, so the next hit's note-off and note-on land in the same tick. The sample engine already fades a sounding voice out over 1 ms before restarting it, so the restart is click-free without a new crossfade. Consecutive notes already behave this way.
