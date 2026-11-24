# Design

## Audio graph

```
track / synth part panning L/R --+--> tracks mixer (input 0-10, gain 1 or 0)
                                 +--> bus mixer 1-4 (input 0-10, gain 1 or 0) -> bus insert 1-4 --+
                                                                                                  |
tracks mixer (inputs 11-14: bus returns, gain = bus volume) <-------------------------------------+
  -> tracks and sends mixer -> master insert -> limiter
```

- `MixerAndFxs` owns `busMixer[4][L/R]` (`AudioMixer<11>`) and `busInsert[4]` (`AudioEffectInsert` on insert channels 12-15). They are declared before the tracks mixer so they update first in each block, with no extra latency.
- The tracks mixer grows from 11 to 15 inputs on the Plus and Mini (`kTracksMixerInputs`). On the Original `kBusCount` is 0, so nothing changes there.
- `audioEngine::connectToBuses` feeds each track's and synth part's panning amplifiers to every bus mixer, at the same input index as the tracks mixer.

## Routing by gain
- `setChannelOutput` sets the source's gain to 1 in the one mixer it goes to and 0 in the other four, with the audio interrupt off so all of them change in the same block. `AudioMixer` ramps each gain over one block, so a change does not click.
- `AudioMixer::update` now drops an input whose gain is 0 (and was 0 in the previous block) without mixing it. A mixer whose inputs are all silenced sends no block, so an unused bus mixer costs a loop over its inputs, and its insert sleeps. Before this, a 0-gain input still produced a block of zeros, which would have kept every bus insert awake.
- `AudioMixer::gainImmediate` sets a gain without the ramp. The bus mixers start with every input at 0 through it, so the first block does not fade a track into every bus.
- Bus volume is the bus return's gain in the tracks mixer, through `ampLogValues` (0-100, 50 is 0 dB, 0.48 dB a step).

## Storage
- Appended to `strMtValues`: `uint8_t channelOutput[11]` (0 master, 1-4 bus), `uint8_t busVolume[4]`, `strInsertSlot busInsertSlot[4][3]`. `strMtProjectRemote` grows from 3528 to 3952 B.
- The existing `insertSlot[12][3]` is not reshaped. `strMtValues::insertSlotOf(channel, slot)` maps insert channels 0-11 to it and 12-15 to `busInsertSlot`.
- `PROJECT_FILE_VERSION` 19 to 20. `recoveryProjectVersion19` and `getDefaultValues` call `strMtValues::setDefaultBuses` (everything to the master, buses at 0 dB and empty). `loadProjectValues` sets an out-of-range output to the master and an out-of-range volume to 0 dB.
- `audioEngine::refreshInserts` loads the bus slots with the rest, then every output and bus volume.

## Memory (Tracker+)
- `EXTERNAL_RAM`: 12 more insert slots (delay lines and state), about 96 KB.
- `SRAM_DTC`: 8 mixers, 4 inserts and 4 more tracks mixer inputs.
- See the PR for the measured linker table.

## CPU
- No input routed to a bus: the bus mixers send nothing and the bus inserts do nothing.
- A bus with no effects: one mixer pass per side and a pass-through insert.
- A bus's effects cost the same as on any other channel, and share the pool of 4 reverbs.
