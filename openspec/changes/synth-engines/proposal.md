# Synth engines

**Status: implemented**, untested on hardware. CPU at full polyphony to be measured.

**Platforms:** Plus and Mini only. Synth engines need `FEATURE_SYNTH`, which is off on the Original.

## Why
The Tracker's synth engines lack additive, spectral, a monosynth voice, a gritty digital wave module, phase distortion and an analog polysynth. Adding them gives the Tracker's synth parts more range.

## What changes
Six new engines, added alongside the existing ones (ACD, FAT, VAP, WTFM, PERC, DIRT, CHIP), none removed:

| Engine | What it is |
| :--- | :--- |
| PRISM | Additive spectral synth: 24 partials a voice, shaped by structure, tilt, comb, sweep and shimmer controls |
| SH-101 | Monosynth: saw, pulse, sub and noise into a resonant 4-pole low-pass |
| MS800 | Digital wave module: sequences of 8-bit single-cycle waves, unfiltered and aliasing, with an optional second tone |
| K5 | Additive synthesis after the Kawai K5: 32 harmonics a voice, harmonic envelopes, spectral filter and formant EQ |
| PHASE | CZ-style phase distortion synth |
| PROPHET | Prophet-5 style analog polysynth |

Each engine is a `CustomSynthVoice` (`docs/custom_synth_sdk.md`) in `core/synth/engines/`, with its own patch parameters, macros, badges and init patch.

## Licensing
- PHASE and PROPHET are written from the instruments' published architecture. Sequential's factory programs are not shipped.

## Decisions
- **No other makers' trademarks on the device.** Model names, badges and patch folders use neutral names. Docs may still describe an engine as "after the Kawai K5", as the `realtime-effects` record does. Working names, which can be changed before release:

  | Engine | Device name |
  | :--- | :--- |
  | PRISM | `PRISM` |
  | SH-101 | `MONO` |
  | MS800 | `GRIT` |
  | K5 | `HARM` |
  | PHASE | `PHASE` |
  | PROPHET | `POLY` |

- **Presets ship.** Every engine ships factory `.patch` files under `/Patches/` on the SD card, with the factory patches' layout and the device names above.

## Out of scope
- The Original Tracker.
