# Tasks

## Shared
- [x] Answer the open questions: neutral device names, presets ship as `.patch` files
- [x] `.patch` generator (`core/synth/presets/gen_presets.py`, `make patches`) and release packaging (`Patches/` in the effects zip)
- [ ] Convert the Pocket Rack presets of each port (MONO, PRISM, GRIT done)
- [x] Reserve the six `InstrumentTypeId` values (9 PRISM, 10 MONO, 11 GRIT, 12 HARM, 13 PHASE, 14 POLY) and badges for all six (`gen_synth_badge.py`, RAM_G slots from 129000)
- [ ] Register each engine as it lands (`registry.cpp`, `Synth::kSynthTypes`, model names, badge tables)
- [x] Shared ADSR, LFO, glide, noise and polyBLEP (`core/synth/engines/EngineKit.h`)

## Ports (one PR each)
- [x] PRISM: port (per-voice spectrum and scans), ADSR, glide, 40 factory patches (host check: stable, level matches MONO)
- [ ] PRISM: CPU at 4 voices on hardware
- [x] SH-101 (MONO): port, ladder filter, ADSR, LFO, glide, 8 factory patches (host check: stable, idle after release)
- [x] MS800 (GRIT): wave generator ported (`gen_grit_waves.py`, tables identical to Pocket Rack's), step sequence, second tone, grit and bits, 12 factory patches (host check: stable, level matched)
- [ ] K5: port, harmonic envelopes, DDF and DFT, init patch, CPU at 4 voices

## New engines (one PR each, written without Pocket Rack's or CrispyZebra's code)
- [ ] PHASE: phase-distortion oscillators, two lines, envelopes, ring mod and detune, init patch
- [ ] PROPHET: two oscillators, Poly-Mod, ladder filter, envelopes, LFO, init patch

## Checks
- [ ] Project and `.patch` save and load for every new engine, and projects with existing engines unchanged
- [ ] CPU and memory checked on Plus (and Mini)
- [ ] README, CHANGELOG and `docs/synth_engines.md` for each engine (MONO, PRISM, GRIT done)
