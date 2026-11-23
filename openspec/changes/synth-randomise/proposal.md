# Synth randomise

**Status: implemented**, untested on hardware

**Platforms:** Plus and Mini only. Synth engines need `FEATURE_SYNTH`, which is off on the Original.

## Why
Finding a starting sound by hand is slow. A Randomise action on every engine gives a quick starting point.

## What changes
- A randomise action in the synth patch editor, for every engine: the factory engines (ACD, FAT, VAP, WTFM, PERC, DIRT), CHIP and the `synth-engines` engines.
- It sets each sound parameter of the part's patch to a random value in its range.
- Level, pan, sends, tuning, voice count and play mode are kept. Macro assignments and sample or wavetable slots are kept.
- The result is always playable. Parameters that could make a patch silent or stuck (for example a near-zero amp level or sustain, or a very long attack) are kept in a safe range.

## Design
- Generic, through the `Patch` API: walk `getMetadata().params`, take `minValue` and `maxValue`, and pick a random step for `isEnum` parameters. Apply inside `startBatchUpdate()` / `finishBatchUpdate()`.
- Parameters with `invalidatesOtherMetadata` (engine modes such as CHIP's model) are set first, then the metadata is read again before the rest are set.
- Kept and limited parameters are chosen by display name, the same rules for every engine (the factory engines are closed source): names with Volume, Tune, Fine, Glide, Pan, Send, Voice or Poly are kept; Attack stays in the lower 40 % of its range, Sustain in the upper 60 %, Decay and Release in the lower 70 %, Cutoff in the upper 70 %. Values are set through the edit curve, so times spread evenly over their exponential range.
- Runs on the UI thread, not the audio interrupt.

## Decisions
- **Control: Shift + F7 / Shift + F8 on the synth instrument's parameter screen** (`SynthEditor.cpp`, the screen with Model, Polyphony, Gain and so on). All eight function keys are taken there (F1 to F6 the columns, F7 Load Patch, F8 Edit Patch), but Shift is free apart from the note keyboard popup. While Shift is held, the F7 and F8 labels read **Randomise** and **Previous**, the way `fMasterParams.cpp` swaps labels on Shift. That screen has no Mini-specific input code, so the same combination works on the Mini. The note keyboard popup's Shift behaviour stays as is.
- **Undo: one previous patch.** Before randomising, the part's parameter values are copied. **Previous** restores that copy and the sound follows. Randomising again replaces the copy, so Previous is one level deep. With no copy, Previous shows "No previous patch". The copy only applies while the part keeps the engine and patch name it was taken from. Restoring uses the copy up. The copy lives in RAM only (not `SRAM_DTC`), one per synth part, and is not saved with the project.
