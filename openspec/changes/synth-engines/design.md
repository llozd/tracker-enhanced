# Design

## Engine shape
- One `CustomSynthVoice` subclass per engine: `core/synth/engines/<Name>SynthVoice.{h,cpp}`, following `ChiptuneSynthVoice` (CHIP).
- New `InstrumentTypeId` values are appended after `kChiptuneSynth` (`synth/include/PolySynth/config.h`), and `kLastSynth` moves to the last one. Existing values never change, because they are stored in projects and `.patch` files.
- Register in `core/synth/registry.cpp`, add to `Synth::kSynthTypes`, `synthModelNames` (`SynthEditor.cpp`) and the badge lists (`SynthEditor.cpp`, `SynthPatchEditor.cpp`), with mini (80x28) and medium (108x40) L2 badges in `core/libs/images/`.

## Engine building blocks
Each engine carries its own envelopes, LFO and glide (`core/synth/engines/EngineKit.h`):

- Amp envelope, plus a filter envelope where the engine has a filter, as engine parameters.
- Engine LFO parameters. Tracker macros cover modulation routing.
- A glide parameter. `CustomSynthVoice` already handles the held-note stack and legato.
- Named `Patch::Metadata::Param`s with real ranges and formatters.
- `float` output at about -6 dBFS a voice, with the engine's own soft clip.
- Per-part block work (PRISM's spectrum, K5's harmonic envelopes): `renderPostProcessAudio` cannot do this, because it runs after the voices. Compute it in the first rendered voice of a block, or per voice if the cost is fine.
- Maths: `sdk::fastSine`, `sdk::fastPow2`, `sdk::noteToFreq`.

- Rate-dependent constants (filter coefficients, LFO and drift rates, envelope times) are set for the Tracker's rate (128-sample blocks, about 44.1 kHz).
- MS800's 21 waves come from `gen_grit_waves.py`, checked in as a `const` table in flash.
- The per-voice state stays in the voice object (heap). No new `.bss` beyond the SDK's parameter handler table, because `SRAM_DTC` has about 17 KB free.

## Voices
- The Tracker has 8 voices shared by 3 synth parts. `metadata.maxVoices`: SH-101 1 (mono), PRISM and K5 4, MS800, PHASE and PROPHET up to 8 if the CPU allows.

## New engines (PHASE, PROPHET)
- PHASE: two lines of phase-distortion oscillators (the CZ wave set, including resonance waves), DCW and DCA envelopes per line, ring mod and detune between the lines. Written from the Casio CZ manuals and published descriptions.
- PROPHET: two oscillators per voice (saw, pulse, triangle), Poly-Mod (filter envelope and osc B into osc A pitch and PW, and the filter), noise, a 4-pole resonant low-pass, filter and amp envelopes, LFO. Written from published architecture (manuals, service notes, published filter analyses).
- Original patches only. No Sequential factory programs.

## CPU
- K5 (32 sines a voice) and PRISM (24 partials a voice) are the heaviest. Measure each engine at its maximum voices with the Profiler (the CPU line in the title bar). Lower `maxVoices` or the partial count if a full part plus sample tracks drops audio.
