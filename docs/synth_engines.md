# Tracker Enhanced Synth Engines (Tracker+ and Tracker Mini)

Tracker Enhanced adds synth engines alongside the stock ones (ACD, FAT, VAP, WTFM, PERC, DIRT, CHIP). Pick one as the **Model** of a synth part on the instrument's parameter screen. Like the stock engines, they are not available on the Original Tracker.

Factory patches come in the release's `Patches/` folder: copy it to the root of the microSD card, next to `Effects/`.

---

## MONO

A one-oscillator monosynth after the Roland SH-101, ported from Pocket Rack. A saw, a pulse with variable width, a sub oscillator and noise are mixed into a resonant four-pole low-pass. One envelope drives both the filter and the amplifier, as on the original. Up to 4 voices: set the part's Polyphony to 1 for the classic mono behaviour, with Glide Mode Legato for slides.

| Section | Parameter | What it does |
| :--- | :--- | :--- |
| **Oscillator** | Saw, Pulse, Sub, Noise | The level of each source |
| | Sub Mode | 1 Oct Square, 2 Oct Square or 2 Oct Pulse (25 %) |
| | Pulse Width | 0 % is a square, 100 % a thin pulse |
| **Filter** | Cutoff | 30 Hz to about 8.7 kHz |
| | Resonance | At the top the filter rings on its own; high settings take some bass away, as on the original |
| | Env Amount | How far the envelope opens (positive) or closes (negative) the filter |
| | Key Track | How much the cutoff follows the keyboard (50 % is half a semitone per semitone) |
| **LFO** | LFO Wave, LFO Rate | Sine, triangle, saw, square or sample & hold, 0.05 to 20 Hz |
| | LFO > Pitch, LFO > Filter, LFO > PW | Vibrato, filter wobble and pulse-width modulation |
| **Envelope** | Attack, Decay, Sustain, Release | The envelope for the filter and the amplifier |
| **Voice** | Tune, Fine, Glide Mode, Glide Time, Volume | Glide Mode: Always, Overlap or Legato |

Macros: Cutoff, Resonance, Env Amount, Pulse Width, Sub / Noise and Decay / Release. Velocity controls the volume.

Factory patches: Mono Bass, Sub Thump, Pulse Lead, Fat Saw Lead, Reso Zap, Noise Hit, String Pad and Hollow PWM.

---

## PRISM

An additive spectral synth, ported from Pocket Rack: the spectrum itself is the instrument. Each voice has 24 sine partials whose spacing and loudness come from the controls below, recomputed every few milliseconds, so the sound moves on its own. Up to 4 voices. Partials near the top of the audio band fade out, so nothing aliases, and the level stays the same however many partials sound.

| Section | Parameter | What it does |
| :--- | :--- | :--- |
| **Spectrum** | Structure | How the partials are spaced: the middle is harmonic, lower bunches them towards the fundamental, higher spreads them like a bell or a bar |
| | Tilt | Dark to bright: at 0 the partials fall off fast (nearly a sine), at 100 % all are equally loud (a buzz); a saw at about 67 % |
| | Warp | Moves the partials to a fixed glassy, gamelan-like set |
| | Odd / Even | Below 0 thins the odd harmonics (hollow), above 0 the even ones (a reed, a square) |
| | Shift | Moves every partial by the same amount (up to 2 fundamentals): harmonic becomes an inharmonic ring |
| | Fold | Folds partials that pass a ceiling back under it: a dense cluster |
| **Motion** | Comb | A comb that keeps some partials and dips others, sliding slowly along the spectrum |
| | Sweep | A spotlight on a few partials scanning up and down (a moving formant) |
| | Smear | Every partial's loudness wanders at its own rate (a choir's shimmer) |
| | Drift | Every partial's pitch drifts on its own slow random walk (up to about 25 cents) |
| | Freeze | Holds the spectrum a note started with: at 100 % Comb, Sweep and Smear stop moving it |
| **Shape** | Scramble | Shuffles which partial gets which loudness |
| | Gate | Silences partials below a share of the loudest; with motion, partials pop in and out |
| | Spectral Decay | Above 0 the high partials die first (a pluck); below 0 they arrive last (a bloom) |
| | Unfold | The partials start close together and fan out to their places, over up to 4 s |
| | Phase | The partials' starting phases: 0 together (a sharp start), 100 % random (soft) |
| **Envelope** | Attack, Decay, Sustain, Release | The amplitude envelope |
| **Voice** | Tune, Fine, Glide Mode, Glide Time, Volume | |

Macros: Tilt, Comb, Sweep, Warp, Smear / Drift and Structure. Velocity controls the volume.

Each voice runs its own slow scans (Comb, Sweep, Smear), so chords move a little more freely than on Pocket Rack, where a part's voices share them.

Factory patches: 40, from Glass Choir and Alien Bell to Time Stretch and Cluster Pluck.

---

## GRIT

A digital wave module after the Cheetah MS800, ported from Pocket Rack. There is no filter, as on the original: the sound is a sequence of 8-bit companded single-cycle waves played without interpolation, so it aliases and grits as that ROM did. A tone steps through the waves (each step a wave and a level, held for a time and crossfaded into the next), then ends, loops or turns back. A second tone can be layered, transposed and detuned. Up to 8 voices.

The 21 waves: SINE, TRI, SAW, SQR, P25, P12, ORGN, OCTV, FIFT, HOLW, BUZZ, AH, EE, BELL, SYNC, RESO, FOLD, RHRM, NOIS, DUST and STEP. They are drawn for this engine, not read from the original's ROM.

| Section | Parameter | What it does |
| :--- | :--- | :--- |
| **Wave** | Wave | The first step's wave |
| | Steps | How many steps the sequence has: 1 is a plain wave, up to 16 |
| | Step Time | How long a step lasts, 1 ms to 10 s |
| | Crossfade | The share of a step spent fading into the next: 0 is a hard switch |
| | Jump | How many waves each step moves on, -10 to 10 |
| | Loop | After the last step: End holds it, Loop starts again, Ping-Pong turns back |
| **Sequence** | Level Step | Below 0 each step is quieter (6 dB a step at -100 %), above 0 each louder, up to the last at full level |
| | Random | The chance that a step takes a random wave |
| | Vel > Wave | Velocity moves the first wave on (up to 10 waves) |
| | Grit | Holds samples, down to about 2.8 kHz |
| | Bits | Drops bits below the original's 8 |
| **Layer** | Layer | The second tone's level |
| | Layer Wave, Layer Trans, Layer Detune | Its first wave against Wave, its transposition (±24) and detune (±50 cents) |
| | Layer Time | Its step time, ×0.25 to ×4 |
| **Envelope** | Attack, Decay, Sustain, Release, Vibrato Rate, Vibrato Depth | The amplitude envelope and a vibrato |
| **Voice** | Tune, Fine, Glide Mode, Glide Time, Volume | |

Macros: Wave, Step Time, Crossfade, Jump, Layer and Grit / Bits. Velocity controls the volume.

Factory patches: Digi Saw, Grit Bass, Wave Choir, Seq Organ, Chip Arp, Dust Keys, Ping Pad, Alias Lead, Noise Sweep, Step Rhythm, Glass Scan and Random Bits.
