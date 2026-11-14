# Jump between track halves

**Status: planned**

**Platforms:** all. UI only, no feature gate.

## Why
Getting from tracks 1 to 8 over to tracks 9 to 16 means moving the cursor across every track in between.

## What changes
- **Pattern + Right** moves the cursor from track n to track n + 8 (tracks 1 to 8 go to 9 to 16). **Pattern + Left** moves it back. The row and the selected column (note, instrument, FX1, FX2) stay the same. Nothing happens if the cursor is already in that half.
- The view follows the cursor:
  - Compact view (8 tracks): it shows exactly tracks 1 to 8 or 9 to 16.
  - Default view (4 tracks): the visible tracks shift by 8 as well (for example 2 to 5 becomes 10 to 13), so the cursor keeps its place on screen.
- Holding Pattern for the jump does not leave the compact view when Pattern is released. This works the same way as Pattern + Note today.

## Behaviour change
Pattern + Left/Right currently changes the pattern, but Pattern + Up/Down and Pattern + the jog wheel do that too. Left/Right are repurposed for this jump. Pattern + Up/Down takes over what only Left/Right did: while playing, they queue the next pattern (`sequencialSwitch_changeNextPattern`) instead of switching at once. The README and CHANGELOG note the change.

## Out of scope
- Shift + Left/Right (selection) is unchanged.
