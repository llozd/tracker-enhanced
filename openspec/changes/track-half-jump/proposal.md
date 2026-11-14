# Jump between track halves

**Status: planned**

**Platforms:** all. UI only, no feature gate.

## Why
Getting from tracks 1 to 8 over to tracks 9 to 16 means moving the cursor across every track in between.

## What changes
- **Hold Pattern and Shift, then press Right** to move the cursor from track n to track n + 8 (tracks 1 to 8 go to 9 to 16). **Left** moves it back. The row and the selected column (note, instrument, FX1, FX2) stay the same. Nothing happens if the cursor is already in that half.
- The view follows the cursor:
  - Compact view (8 tracks): it shows exactly tracks 1 to 8 or 9 to 16.
  - Default view (4 tracks): the visible tracks shift by 8 as well (for example 2 to 5 becomes 10 to 13), so the cursor keeps its place on screen.
  - All tracks view (`all-tracks-view`): only the cursor moves.
- Holding Pattern for the jump does not leave the compact view when Pattern is released. This works the same way as Pattern + Note today.

## Why this combination
Every other button acts as soon as it is pressed (Rec toggles edit mode, Note/Instr/FX change the column, Insert/Copy/Delete act, the screen buttons switch screens), so only Pattern and Shift can be held as modifiers. Pattern + Left/Right changes the pattern and Shift + Left/Right extends the selection, and both stay as they are. Pattern + Shift + Left/Right currently does the same as Pattern + Left/Right (Shift is ignored), so nothing is lost.

## Shared with the all tracks view
Pattern + Shift is one layer: with arrows it jumps, and on its own it opens the all tracks view. The view therefore toggles when Shift is released, and only if no arrow was pressed while it was held.

## Out of scope
- Pattern + Left/Right and Pattern + Up/Down (change pattern) and Shift + arrows (selection) are unchanged.
