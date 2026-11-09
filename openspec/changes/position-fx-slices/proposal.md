# Position effect on slices

**Status: planned**

**Platforms:** all. No feature gate.

## Why
Position currently scrubs the whole sample. With slices, the user wants it to scrub within the played slice.

## What changes
When a slice is played, the Position effect maps its value across that slice's start to end instead of the whole sample. Non-sliced playback is unchanged.

## Open questions
- Behaviour when slice end is before the mapped position, and with reverse playback.
- Interaction with other playback-start effects.
