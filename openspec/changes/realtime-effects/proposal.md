# Realtime track and master effects

**Status: implemented**, untested on hardware. CPU limits to be measured.

**Platforms:** Plus and Mini only. Code and UI are wrapped in `#if FEATURE_REALTIME_FX` (`core/include/platformFeatures.h`), which is 0 on the Original. Effect settings are still stored in project files there.

## Why
The firmware lacks console-style tone shaping, character effects and modulation effects on individual channels.

## What changes
Insert effect slots on every audio channel and on the master:
- **Channels:** the 8 sample tracks and the 3 synth parts. MIDI tracks have no audio, so they have no inserts.
- **Master:** the summed mix (tracks, sends and inputs), before the existing limiter.
- **3 slots per channel and 3 on the master.** Any effect can go in any slot, in any order.

Effects:

| Family | Effects |
| :--- | :--- |
| EQ | Parametric EQ after the Calrec PQ1549 (4 bands, low and high cut), 3-band EQ |
| Dynamics | Compressor, limiter, glue (bus) compressor, FET compressor |
| Drive | Overdrive, soft clip, saturation, diode drive, digital clip, bit crush |
| Colour | Tape, tube, transformer, console, exciter |
| Motion | Chorus, flanger, phaser |
| Space | Reverb |

"After the Calrec PQ1549" and the other model names describe what an effect is modelled on. No copied branding or code.

Reverb is the most expensive in memory (about 64 KB a reverb) and CPU, so only a limited number of reverb inserts can run at once (the number is set after measuring). The existing delay and reverb sends stay as they are.

CPU is measured on hardware. When an effect would exceed the budget, it is refused and the user is told.

## Sources
- The reverb insert reuses this firmware's own reverb (`AudioEffectPolyverb`).

## Out of scope
Offline `.ptfx` Sample Editor effects, which are separate.
