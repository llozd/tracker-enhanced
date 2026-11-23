# Tasks

## Engine (PR 1)
- [x] Host check: every effect stable and silent after its tail
- [x] Effect kernels in `core/libs/Audio/fx/` (EQ, dynamics, drive, colour, motion), with descriptor tables
- [x] `AudioEffectInsert`: 3 slots, float conversion, pass-through when empty, idle when silent, slot updates with the audio interrupt off
- [x] Wire inserts into the 8 track chains and the master, and into `PolySynth::update` for the 3 synth parts
- [x] Delay lines in `EXTERNAL_RAM` (one per slot)
- [x] Dev console command to set an insert, for testing before the UI

## Measure (on a Tracker+)
- [ ] Baseline audio CPU on a busy project
- [ ] Cost per block of each effect type, then the CPU budget and the per-type cost table

## Follow-ups
- [x] Mono synth parts: stereo output when a chorus is in a slot

## Reverb (PR 2)
- [x] Reverb insert from the firmware's `Reverb`, a pool of 4 with refusal when full (host check: pool limit, stereo, tail ends in silence)

## Storage and UI (PR 3)
- [x] Insert slots in the project (`strMtValues::insertSlot`), file version 19 and upgrade step, loaded on project open
- [x] Inserts page on the Master screen: channel, slot, effect picker, paged parameters, on desktop and Mini (Mini menu mode 4)
- [x] Popup when a reverb is refused
- [x] Meters on the Inserts page: input and output level, gain reduction for the dynamics, parameter position bars, and an EQ response graph (`cEqCurve`, `insertfx::responseDb`, host-checked against measured response within 0.004 dB)
- [ ] CPU budget check with a popup, after measuring

## Finish
- [ ] Hardware listening tests, CPU checks with full projects, export with inserts
- [x] README, CHANGELOG and user docs (`docs/realtime_effects.md`)
