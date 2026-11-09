# Position effect on slices

**Status: implemented**, untested on hardware

**Platforms:** all. No feature gate.

## Why
With slices, the user wants Position to scrub within the played slice.

## Findings
- Position was ignored in Slice and Beat Slice play modes: `playerEngine::fxPosition` skipped them, so it did nothing there rather than scrubbing the whole sample.

## What changes
In Slice and Beat Slice modes, Position maps its value across the played slice, from its start to its end, in the playback direction. 0% plays the whole slice, 50% starts halfway through it. Other play modes are unchanged.

## Decisions
- **Reverse playback:** the offset is measured from where playback begins, so a reversed slice at 50% plays from its middle back to its start.
- **Range:** the offset is capped one point short of the slice end, so a hit never has zero length.
- **Other effects:** works with the Slice effect (the forced slice is resolved first, then Position applies within it).
- **Streamed samples:** a start outside the slice pre-cache streams straight from the SD card, so a hit deep into a long streamed slice may start slightly later.
