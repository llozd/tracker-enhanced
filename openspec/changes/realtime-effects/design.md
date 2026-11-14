# Design

## Where the inserts sit

**Sample tracks.** Each track's chain is `AudioPlayMemory -> AudioBitDepth -> AudioFilterStateVariable -> AudioAmplifier (volume) -> panning -> tracks mixer`. The delay and reverb sends are tapped from the filter output. The insert goes straight after the filter, so the effect is heard on the dry signal and in the sends (the usual insert position), before volume and pan:

```
filter L/R -> [insert: slot 1 -> 2 -> 3] -> amplifier -> panning -> tracks mixer
                                          -> delay and reverb pre-FX mixers
```

**Synth parts.** `PolySynth::update` (`core/libs/Audio/synth_PolySynth.cpp`, open source) renders each part to float buffers, then splits them into the dry output and the two sends. The part's inserts run on those float buffers before the split, so the sends hear them too.

**Master.** Between the tracks-and-sends mixer and the limiter (`connectMixerToOutput`):

```
tracks and sends mixer L/R -> [master insert: slot 1 -> 2 -> 3] -> limiter -> final mixer -> output
```

## The insert object
- `AudioEffectInsert` (`core/libs/Audio/effect_insert.{h,cpp}`): an `AudioStream` with 2 inputs and 2 outputs. It converts the int16 blocks to float (+-1.0), runs its slots in order, and converts back with saturation.
- All three slots empty: the blocks pass through untouched (no copy, no conversion).
- Silent input: when both inputs are NULL and every slot's tail is done (reverb, delay lines, compressor release), it transmits nothing, so the rest of the graph keeps its idle path.
- Offline render (`AudioStream::renderMode`) runs the same code, so exports and stems include the inserts.
- Parameter changes come from the UI thread as a pending set of values. The insert picks them up at the start of its next block, and recalculates coefficients there. Nothing is allocated in `update()`.

## Effect kernels
- `core/libs/Audio/fx/` holds the kernels ported from `fxdsp.c`: float functions over a block of stereo samples, each with a type, 16 parameters (int16, with a descriptor table: name, range, format) and a state.
- `x0x/fastmath.h` is replaced with this firmware's maths (`powf` and `expf` only in coefficient updates, never per sample).
- The chorus (2 x 1024 floats) and flanger (2 x 512 floats) delay lines come from a pool in `EXTERNAL_RAM` (`__NOINIT(EXTERNAL_RAM)`), not from `SRAM_DTC` (about 17 KB free).
- Reverb: `AudioEffectPolyverb` keeps its buffers in static arrays today. It is changed to take its buffers from the caller, so the send reverb keeps its own and each reverb insert takes 64 KB from a small reverb pool in `EXTERNAL_RAM`. The pool size is the reverb limit.

## Memory
- Per slot: about 200 B of state (6 biquads, envelope followers, LFOs) for 36 slots (11 channels and the master, 3 each), in `EXTERNAL_RAM`.
- Delay pool: one chorus or flanger delay line a slot would be 36 x 8 KB = 288 KB, so the pool is sized for the slots that can run at once.
- Reverb pool: 64 KB a reverb.
- Plus/Mini `EXTERNAL_RAM` is about 95 % used (about 3.3 MB free at 0.1.0b1), so the pools are sized from the measured free space.

## CPU
- The Plus has 600 MHz, about 1.74 M cycles per 128-sample block. Each effect type gets a measured cost per block (cycle counter, on hardware, in a dev console build).
- Enabling an effect checks the sum of the active slots' costs against a budget that leaves headroom for voices and synths. Over budget: refused, with a popup.
- First measurement step: a dev console command (`ENABLE_DEV_CONSOLE`) to set an insert from the host, so costs are measured before the UI exists.

## Storage
- A new `strInsertSettings` per channel (12: 8 tracks, 3 synth parts, master), with 3 slots of `{type, 16 x int16 params}`, about 1.2 KB in all, appended to `strMtValues` (`mtStructs.h`).
- `PROJECT_FILE_VERSION` goes up by one, with an upgrade step in `mtFileManagerOldVersionRecovery.cpp` that sets every slot to Off.
- Effect type IDs are append-only, because they are stored.
- The Original reads and writes the same settings and ignores them (`FEATURE_REALTIME_FX` 0).

## UI (to be detailed before that PR)
- An inserts page per channel, reached from the track and synth part screens and from the Master screen: three slot rows, an effect picker by family, and the effect's parameters on the encoder, with the same layout on the Mini.
