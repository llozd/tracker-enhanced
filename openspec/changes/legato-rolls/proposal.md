# Legato rolls

**Status: planned**

## Why
Rolls and fast runs of notes sound gappy, with a tiny stop between hits. This exists in older firmware too.

## Findings
In `core/source/mtSequencer.cpp` (roll handling, about lines 1085 to 1155):
- Roll note length is set to half the roll period, so the gate closes at 50% of each hit.
- `stopOpenNoteOnTrack` sends a note-off before each retrigger, starting the envelope release.
- The next note-on restarts the voice from the sample start.

## What changes
Make roll hits legato: gate for the full period, no note-off before each retrigger, and avoid a click on restart (for example a short crossfade). Check the same restart behaviour for consecutive notes.
