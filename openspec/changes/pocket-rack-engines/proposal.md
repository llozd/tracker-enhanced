# Pocket Rack synth engines

**Status: in progress**: MONO, PRISM, GRIT and HARM implemented, untested on hardware

**Platforms:** Plus and Mini only. Synth engines need `FEATURE_SYNTH`, which is off on the Original.

## Why
Pocket Rack (`../pocket-rack`, llozd's firmware for the M-VAVE FM-1) has synth engines the Tracker lacks: additive, spectral, a monosynth voice and a gritty digital wave module. Porting them gives the Tracker's synth parts more range.

## What changes
Six new engines, added alongside the existing ones (ACD, FAT, VAP, WTFM, PERC, DIRT, CHIP), none removed:

| Engine | Source | What it is |
| :--- | :--- | :--- |
| PRISM | Port of `eng_prism.c` | Additive spectral synth: 24 partials a voice, shaped by structure, tilt, comb, sweep and shimmer controls |
| SH-101 | Port of `eng_sh101.c` | Monosynth: saw, pulse, sub and noise into a resonant 4-pole low-pass |
| MS800 | Port of `eng_ms800.c` | Digital wave module: sequences of 8-bit single-cycle waves, unfiltered and aliasing, with an optional second tone |
| K5 | Port of `eng_k5.c` | Additive synthesis after the Kawai K5: 32 harmonics a voice, harmonic envelopes, spectral filter and formant EQ |
| PHASE | New code | CZ-style phase distortion synth |
| PROPHET | New code | Prophet-5 style analog polysynth |

Each engine is a `CustomSynthVoice` (`docs/custom_synth_sdk.md`) in `core/synth/engines/`, with its own patch parameters, macros, badges and init patch.

## Licensing
- PRISM, SH-101, MS800 and K5 were written only by llozd (checked in git history and file headers), so llozd relicenses them under this repository's `LICENSE` for the port. Pocket Rack's shared helpers are not by llozd and are not ported: the ports use the SDK's maths (`sdk::fastSine`, `sdk::fastPow2`) instead of `x0x/fastmath.h`, and their own envelopes, LFO and glide instead of Pocket Rack's `voice.c`.
- PHASE and PROPHET in Pocket Rack are GPL-3.0 code by other authors (Kerem Kilic; Leo Kuroshita's CrispyZebra for PHASE) and cannot be combined with Polyend's closed synth libraries. The Tracker versions are written new, from the instruments' published architecture. Nobody working on them reads Pocket Rack's `eng_phase.c`, `eng_prophet_test.c`, `prophet_*.c` or CrispyZebra. Sequential's factory programs are not shipped.

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

- **Presets ship.** Pocket Rack's presets for the four ported engines (llozd's own) are converted to factory `.patch` files under `/Patches/` on the SD card, with the factory patches' layout and the device names above. PHASE and PROPHET get new presets.

## Out of scope
- The Original Tracker.
- Pocket Rack's other engines (FM6, CZ-1, DRUM, TB-303, VOICE, TRIO, PHYS).
