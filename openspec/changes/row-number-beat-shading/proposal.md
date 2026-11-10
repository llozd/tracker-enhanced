# Row number beat shading

**Status: implemented**, untested on hardware

**Platforms:** all. No feature gate.

## Why
At up to 16 lines per beat it is hard to see where a row sits within the beat. The full-width beat highlight only marks the beat itself, and highlighting more rows across the pattern would be confusing.

## What changes
The row numbers on both sides of the pattern view are shaded by their position in the beat:
- Beat rows: full brightness (unchanged).
- Half-beat rows (even LPB only): slightly dimmer.
- All other rows: dimmest.

The background beat highlight is unchanged. At 1 LPB every row is a beat row, so nothing changes.

## Design
- Shades are mixes of the theme's row number color (`patternTrackerColors[16]`) with the background (`[1]`): off-beat 50%, half-beat 75%, so they follow custom themes.
- Shading uses the project's lines per beat, not the pattern divider, so it always shows position within the beat.
