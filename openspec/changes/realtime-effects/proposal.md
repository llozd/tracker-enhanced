# Realtime track and master effects

**Status: planned**

**Platforms:** Plus and Mini only. Code and UI are wrapped in `#if FEATURE_REALTIME_FX` (`core/include/platformFeatures.h`), which is 0 on the Original. Effect settings are still stored in project files there.

## Why
The firmware lacks console-style tone shaping and character effects.

## What changes
New realtime effects on tracks and on the master channel:
- Parametric EQ inspired by the Calrec PQ1549 (inspired by only; no copied branding or code).
- Saturation, exciter, noise / colouring.
- Console / sound desk style processing.
- More compressors: vintage, FET and glue.

CPU is measured on hardware; the number of simultaneous per-track effects may be limited.

## Out of scope
Offline `.ptfx` Sample Editor effects, which are separate.
