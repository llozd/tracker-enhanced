# Original Tracker support (reduced feature set)

**Status: planned**

## Why
Original Tracker (MK66) owners should get the sequencer improvements, without the features its hardware cannot run.

## What changes
- Bring the MK66 platform back from git history (removed in `e6e43cb`) and restore its build pipeline: `Makefile` target, `version.mk` entries, CI build and artifacts, issue/PR templates.
- Add per-platform feature switches and limits so whole features can be left out of the Original build.
- **Included on the Original:** sequencer improvements, with no extra audio overhead.
  - Lines per beat, up to 16, with 1024-row patterns and length scaling.
  - Legato rolls (`legato-rolls`).
  - Effects on rows without a note, MIDI part only (`fx-only-row-instrument`).
  - Position on slices (`position-fx-slices`).
  - Extended MIDI CC control (`extended-midi-cc`), if its memory cost fits.
  - Future sequencer-only improvements.
- **Excluded from the Original:** realtime track and master effects (`realtime-effects`) and anything else that adds DSP or significant CPU and memory overhead. Code and UI are compiled out.
- README: the Original is supported with a reduced feature set.

## Open questions
- Is 16 LPB usable on a 180 MHz M4? The display already takes about 20 to 35 ms per step, and a row at 140 BPM / 16 LPB lasts about 27 ms. Test on hardware, and if it can't keep up, lower the cap or reduce redraws.
- Where the about 4 MB for 1024-row patterns comes from: shrink the resident sample pool (about 7.5 MB to 4 MB), reduce undo depth, or both.
- The button-hold fix (`button-hold-release`) is in the IMX key driver. The MK66 driver needs checking separately.
- Hardware tester for the Original.
