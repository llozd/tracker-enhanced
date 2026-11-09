# Extended MIDI CC control

**Status: planned**

**Platforms:** Plus and Mini; Original if the memory fits. Slot and column counts go in `PlatformLimits` (`core/include/platformFeatures.h`) so the Original can use lower values. Project files keep the full layout on every device.

## Why
Each MIDI instrument has 6 CC slots and each step has 2 FX slots, which limits expressive control of external gear.

## What changes
- More CC assignments per MIDI instrument.
- More than 2 CC messages on a single row.
- Project format change (already not backwards compatible).

## Open questions
- Number of CC slots and extra FX columns, given tight SRAM_DTC (about 16 KB free) and 64 KB pattern size.
- How extra columns are shown and edited in the pattern view.
