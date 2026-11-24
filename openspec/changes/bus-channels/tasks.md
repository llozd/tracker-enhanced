# Tasks

- [x] Skip 0-gain inputs in `AudioMixer` so an unused mixer sends nothing
- [x] Insert channels 12-15 for the buses (`insertfx::kBus0`), dev console `fx b1`-`b4`
- [x] Bus mixers and bus inserts in `MixerAndFxs`, returns into the tracks mixer
- [x] Feed tracks and synth parts to the bus mixers, routing by gain (`setChannelOutput`, `setBusVolume`)
- [x] Store outputs, volumes and bus slots (project version 20, upgrade step, defaults, load checks)
- [x] Inserts page: Bus 1-4 channels, Out slot with Output, Volume and Inputs
- [x] Docs: `docs/realtime_effects.md`, README, CHANGELOG
- [ ] Hardware: route tracks to a bus, check the effect, level and no clicks on changing output
- [ ] Hardware: measure the CPU cost of an idle bus and a bus with effects
- [ ] Hardware: open a version 19 project and check every channel plays through the master
