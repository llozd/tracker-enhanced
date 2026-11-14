# All tracks view

**Status: planned**

**Platforms:** all. UI only, no feature gate.

## Why
The default pattern view shows 4 tracks and the compact view 8. There is no way to see what all 16 tracks are doing at once.

## What changes
A new pattern view that shows all 16 tracks on one screen, with a dot for each filled cell instead of its value:
- Each track has three narrow cells per row: **note**, **FX1** and **FX2**.
  - Note cell: a filled dot for a note, a short dash for note off, cut or fade.
  - FX cells: a dot when the step has that effect.
  - Colours come from the theme (note and FX colours), and muted tracks are dimmed as in the other views.
- The header shows track numbers 1 to 16. Rows, scrolling, the playhead and row number beat shading work as in the other views.
- The cursor is a frame on one cell. Left/Right move between tracks (keeping the column), Up/Down between rows. Pads, the encoder, delete, copy/paste and Shift selection work as in the other views. The values of the step under the cursor show in the bottom bar.
- Layout: 800 px less the row numbers leaves about 48 px a track, so about 16 px a cell.

## Access
- **Hold Pattern and press Shift** to open it. Tap **Pattern** to go back to the default view, as with the compact view. Pattern + Note, Instr, FX1 or FX2 still opens the compact view.
- On the Mini, Pattern is reached through its function key. Press that key first, then Shift: pressing Shift first selects the function key's second binding.

## How to reach the existing compact view (for reference)
Hold **Pattern** and press **Note**, **Instr**, **FX1** or **FX2** (one, or two of them). The pattern shows 8 tracks with only those columns. Tap Pattern again to return to the default 4-track view.

## Out of scope
- Volume and micro-move columns.
- More rows than the other views.
