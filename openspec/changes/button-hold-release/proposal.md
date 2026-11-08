# Held button releases while turning the knob

**Status: investigating**

## Problem
While holding a button such as FX1 and turning the knob to pick an effect, the button sometimes acts as released. The knob then changes the effect's value instead. Also seen on older firmware, so not caused by the LPB work.

## Findings
- The pattern editor checks `isButtonPressed(FX1/FX2)` on every knob tick to choose between changing the effect type and its value.
- `platforms/imx/source/Drivers/Keys.cpp`: an I2C read/write error calls `refreshConfig()`. If the key controller config differs, it releases every pressed button in software and re-initialises the matrix while the button is still down.
- This is a lead, not confirmed.

## What changes
Log when `refreshConfig()` runs and whether it matches the symptom. If so, stop clearing held buttons on a recoverable error, or restore them from a re-read of the key state.
