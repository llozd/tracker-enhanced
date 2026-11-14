# Tasks

## Shared
- [ ] Answer the open questions (display names, shipped presets)
- [ ] Append the six `InstrumentTypeId` values and register them (`registry.cpp`, `Synth::kSynthTypes`, model names, badges)
- [ ] Reusable amp envelope, filter envelope, LFO and glide helpers for the ports (or per engine, if they differ)

## Ports (one PR each)
- [ ] PRISM: port, rescale rates, init patch, CPU at 4 voices
- [ ] SH-101: port, ladder filter and envelopes, init patch
- [ ] MS800: port the wave table (generator or generated table), step sequence and second tone, init patch
- [ ] K5: port, harmonic envelopes, DDF and DFT, init patch, CPU at 4 voices

## New engines (one PR each, written without Pocket Rack's or CrispyZebra's code)
- [ ] PHASE: phase-distortion oscillators, two lines, envelopes, ring mod and detune, init patch
- [ ] PROPHET: two oscillators, Poly-Mod, ladder filter, envelopes, LFO, init patch

## Checks
- [ ] Project and `.patch` save and load for every new engine, and projects with existing engines unchanged
- [ ] CPU and memory checked on Plus (and Mini)
- [ ] README, CHANGELOG and `docs/` user notes
