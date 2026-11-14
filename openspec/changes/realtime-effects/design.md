# Design

## Where the inserts sit

**Sample tracks.** Each track's chain is `AudioPlayMemory -> AudioBitDepth -> AudioFilterStateVariable -> AudioAmplifier (volume) -> panning -> tracks mixer`. The delay and reverb sends are tapped from the filter output. The insert goes straight after the filter, so the effect is heard on the dry signal and in the sends (the usual insert position), before volume and pan:

```
filter L/R -> [insert: slot 1 -> 2 -> 3] -> amplifier -> panning -> tracks mixer
                                          -> delay and reverb pre-FX mixers
```

**Synth parts.** `PolySynth::update` (`core/libs/Audio/synth_PolySynth.cpp`, open source) renders each part to float buffers, then splits them into the dry output and the two sends. The part's inserts run on those float buffers before the split, so the sends hear them too.
- Every engine except PERC renders mono. When a part's slots include a chorus, the part is made stereo before its inserts (left copied to right) and takes the stereo output path, so the chorus widens it. Without a chorus it stays mono and costs nothing extra.
- PERC renders its own per-voice sends, which bypass the inserts.

**Master.** Between the tracks-and-sends mixer and the limiter (`connectMixerToOutput`):

```
tracks and sends mixer L/R -> [master insert: slot 1 -> 2 -> 3] -> limiter -> final mixer -> output
```

## The insert object
- `AudioEffectInsert` (`core/libs/Audio/effect_insert.{h,cpp}`): an `AudioStream` with 2 inputs and 2 outputs. It converts the int16 blocks to float (+-1.0), runs its slots in order, and converts back with saturation.
- All three slots empty: the blocks pass through untouched (no copy, no conversion).
- Silent input: the slots keep running on silence until the output reaches digital zero (every effect's tail has died out), then the insert sleeps and transmits nothing, so the rest of the graph keeps its idle path. Sound on the input wakes it.
- Offline render (`AudioStream::renderMode`) runs the same code, so exports and stems include the inserts.
- Settings: `insertfx::setSlot` (UI thread) writes a slot with the audio interrupt disabled. The kernels recompute coefficients in the audio block after a parameter changes. Nothing is allocated in `update()`.
- Storage: `insertfx::Channel` (`core/libs/Audio/fx/insertChannels.{h,cpp}`) holds the 12 channels' slots and states in `EXTERNAL_RAM`. The objects in the graph (`AudioEffectInsert`, 8 voices and the master) only keep a channel id.

## Effect kernels
- `core/libs/Audio/fx/` holds the kernels ported from `fxdsp.c`: float functions over a block of stereo samples, each with a type, 16 parameters (int16, with a descriptor table: name, range, format) and a state.
- `x0x/fastmath.h` is replaced with this firmware's maths (`powf` and `expf` only in coefficient updates, never per sample).
- `x0x/fastmath.h` is replaced. Coefficient updates use `std::` maths. Per-sample `powf`, `log2f` and `cosf` use `sdk::fastPow2`, `sdk::fastSine` and a fast `log2` (dynamics gain and asinh differ from Pocket Rack by about -42 dB, everything else by less than -63 dB).
- Every slot owns a delay line of 2 x 1024 floats (8 KB) in `EXTERNAL_RAM`, used by the chorus and the flanger: 36 slots, 288 KB.
- Reverb: the insert reverb is the firmware's own `Reverb` (`Polyverb/reverb.h`, MIT, Emilie Gillet), which already takes its buffers through `Init()`. A pool of 4 (`kReverbPool`) lives in `EXTERNAL_RAM` with 2 x 32 KB buffers each. Picking reverb for a slot takes an entry and `setSlot` refuses when none is free. Controls match the master reverb (size, damp, predelay, diffusion) plus a mix. A chorus or a reverb makes a mono synth part stereo.

## Memory (measured on Tracker+)
- `EXTERNAL_RAM`: +329 KB (delay lines and states). About 3 MB left.
- `SRAM_DTC`: +288 B (the insert objects).
- Flash: +19.5 KB.
- Reverb pool: +263 KB of `EXTERNAL_RAM` for 4 reverbs.

## CPU
- The Plus has 600 MHz, about 1.74 M cycles per 128-sample block. Each effect type gets a measured cost per block (cycle counter, on hardware, in a dev console build).
- Enabling an effect checks the sum of the active slots' costs against a budget that leaves headroom for voices and synths. Over budget: refused, with a popup.
- First measurement step: the dev console's `fx` and `cpu` commands (`ENABLE_DEV_CONSOLE`) set inserts and read the audio load from the host, so costs are measured before the UI exists.

## Storage
- `strInsertSlot insertSlot[12][3]` (`{type, reserved, 16 x int16 params}`, 1224 B) is appended to `strMtValues` (`mtStructs.h`), so `strMtProjectRemote` grows from 2304 to 3528 B.
- `PROJECT_FILE_VERSION` 18 to 19. `recoveryProjectVersion18` sets every slot of an older project to Off.
- `audioEngine::setInsertSlot` writes the project's slot and the engine's (refused, unchanged, when no reverb is free). `audioEngine::refreshInserts` loads every slot after a project load (`cProjectEditor::loadProjectValues`), freeing all reverbs first so slot order does not matter. A slot that cannot get a reverb loads empty.
- Effect type IDs are append-only, because they are stored.
- The Original reads and writes the same settings and ignores them (`FEATURE_REALTIME_FX` 0).

## UI (to be detailed before that PR)
- An inserts page per channel, reached from the track and synth part screens and from the Master screen: three slot rows, an effect picker by family, and the effect's parameters on the encoder, with the same layout on the Mini.
