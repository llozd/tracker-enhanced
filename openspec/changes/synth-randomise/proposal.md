# Synth randomise

**Status: planned**

**Platforms:** Plus and Mini only. Synth engines need `FEATURE_SYNTH`, which is off on the Original.

## Why
Finding a starting sound by hand is slow. Pocket Rack has a Randomise action on every engine. The Tracker's synths should have one too.

## What changes
- A randomise action in the synth patch editor, for every engine: the factory engines (ACD, FAT, VAP, WTFM, PERC, DIRT), CHIP and the `pocket-rack-engines` engines.
- It sets each sound parameter of the part's patch to a random value in its range.
- Level, pan, sends, tuning, voice count and play mode are kept. Macro assignments and sample or wavetable slots are kept.
- The result is always playable. Parameters that could make a patch silent or stuck (for example a near-zero amp level or sustain, or a very long attack) are kept in a safe range.

## Design
- Generic, through the `Patch` API: walk `getMetadata().params`, take `minValue` and `maxValue`, and pick a random step for `isEnum` parameters. Apply inside `startBatchUpdate()` / `finishBatchUpdate()`.
- Parameters with `invalidatesOtherMetadata` (engine modes such as CHIP's model) are set first, then the metadata is read again before the rest are set.
- The factory engines are closed source, so their kept and clamped parameters are listed per `InstrumentTypeId` by `displayName`. The list comes from a dump of each engine's metadata (names, ranges) on a device. Open engines can mark them in their own metadata.
- Runs on the UI thread, not the audio interrupt.

## Open questions
- Which control triggers it on the desktop units and on the Mini. It needs a free button or menu entry in `SynthPatchEditor` that is not taken by the naming keyboard's random name (`interfaceButton1`).
- Undo: randomise again or reload the patch (as Pocket Rack does), or keep one level of "previous patch".
