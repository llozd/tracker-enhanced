# Performance mode LPB scaling

**Status: implemented**, untested on hardware

**Platforms:** all. Sequencer-only, so no feature gate.

## Why
Performance mode was written for a fixed 4 lines per beat. Since lines per beat, two of its effects count rows instead of musical time:
- **Pattern Length** (1 to 128) sets the loop in rows, so at 16 LPB the longest loop is 2 bars instead of 8, and every setting is shorter.
- **Step Stutter** periods are in sequencer ticks per row, so at 8 LPB every division retriggers twice as fast.

## What changes
Both effects keep their 4 LPB meaning: the values are 16th notes, converted to rows (or ticks) with `Sequencer::scaleToLinesPerBeat`. Labels are unchanged. Pattern Length is clamped to 1..512 rows, and the stutter period to at least 2 ticks (the retrigger check is `stepTimer % period == 1`).

## Out of scope
- LFO speed effects: LFO sync follows the step for the whole engine, by the lines-per-beat design.
- Changing LPB while a Pattern Length effect is held keeps the old row count until it is set again. Changing LPB already stops playback.
