# Tasks

## Engine (PR 1)
- [x] Host check: every effect stable, silent after its tail, and within -42 dB of Pocket Rack's output
- [x] Port the `fxdsp.c` kernels to `core/libs/Audio/fx/` (EQ, dynamics, drive, colour, motion), with descriptor tables
- [x] `AudioEffectInsert`: 3 slots, float conversion, pass-through when empty, idle when silent, slot updates with the audio interrupt off
- [x] Wire inserts into the 8 track chains and the master, and into `PolySynth::update` for the 3 synth parts
- [x] Delay lines in `EXTERNAL_RAM` (one per slot)
- [x] Dev console command to set an insert, for testing before the UI

## Measure (on a Tracker+)
- [ ] Baseline audio CPU on a busy project
- [ ] Cost per block of each effect type, then the CPU budget and the per-type cost table

## Follow-ups
- [ ] Mono synth parts: stereo output when an insert widens (chorus, flanger)

## Reverb (PR 2)
- [ ] `AudioEffectPolyverb` takes its buffers from the caller; reverb pool and limit

## Storage and UI (PR 3)
- [ ] `strInsertSettings` in the project, file version bump and upgrade step
- [ ] Inserts page: slot list, effect picker, parameters, on desktop and Mini
- [ ] Budget check with a popup when refused

## Finish
- [ ] Hardware listening tests, CPU checks with full projects, export with inserts
- [ ] README, CHANGELOG and user docs
