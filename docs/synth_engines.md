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

---

## HARM

Additive synthesis after the Kawai K5 and K5m, ported from Pocket Rack. Each voice has 32 sine harmonics, shaped in three stages as on the original: the harmonic levels (each harmonic on one of four envelopes), a spectral filter applied by frequency, and a formant EQ. The original sets 63 levels and four six-stage envelopes by hand; here a few controls draw them. Up to 4 voices.

The four harmonic groups are harmonics 1-2, 3-6, 7-14 and 15-32. The harmonic levels and the formant EQ are balanced for loudness; the filter is not, so it takes level away as it closes. Harmonics near the top of the audio band fade out, so nothing aliases.

| Section | Parameter | What it does |
| :--- | :--- | :--- |
| **Harmonics** | Angle | The slope of the harmonic levels: +1 is about a saw, higher is darker, below 0 brighter |
| | Odd / Even | Below 0 thins the odd harmonics, above 0 the even ones (hollow, a clarinet) |
| | Octave, Fifth | The harmonics an octave apart (2, 4, 8, 16, 32) and a fifth above those (3, 6, 12, 24), ±12 dB |
| | Bloom | The harmonic groups' attacks: the higher a group, the later it comes in (up to 3 s) |
| | Harm Decay | The harmonic groups' decays: the higher a group, the faster and further it falls (a pluck, a bell) |
| **Filter** | Cutoff | About 50 Hz to 12.8 kHz |
| | Slope | How steeply it falls above the cutoff, 6 to 48 dB an octave |
| | Flat | The harmonics under the cutoff against the peak at it: 100 % a low-pass, 0 a band-pass |
| | Env Amount | How far the envelope opens (positive) or closes (negative) the filter |
| | Vel > Filter | Velocity opens the filter, up to 4 octaves |
| | Key > Filter | How far the cutoff follows the keyboard |
| **Formant** | Formant, Formant Freq | A formant peak up to 12 dB over the dip around it, centred 62 Hz to 16 kHz |
| | Twin | Two sources of 16 harmonics, the second detuned up to 50 cents (0: one source of 32) |
| | Harm Mod | The two upper groups swell and fade, each at its own rate |
| | Key > Harm | Above 0 high notes lose their upper harmonics, below 0 low notes do |
| **Envelope** | Attack, Decay, Sustain, Release | The amplitude envelope, which also drives Env Amount |
| **Voice** | Tune, Fine, Glide Mode, Glide Time, Volume | |

Macros: Cutoff, Angle, Bloom / Decay, Formant, Odd / Even and Twin / Mod. Velocity controls the volume.

Factory patches: Harm Brass, Digi Bell, Glass Pad, Hollow Reed, Vox Formant, Drawbar Organ, Pluck Harp, Band Sweep, Twin Strings, Additive Bass, Shimmer Pad and E Piano.

---

## PHASE

A phase distortion synth in the manner of Casio's CZ series. Written new for Tracker Enhanced from the published description of phase distortion, not ported. Each oscillator reads a cosine through a bent phase: **DCW** sets how far it bends, from a pure sine at 0 to the full wave shape at 100 %. Two lines, each with two waves that alternate cycle by cycle, their own DCW envelope and a level; the second line can be detuned, mixed or ring modulated. Up to 8 voices.

Waves: Saw, Square, Pulse, Double Sine, Saw-Pulse and the three resonance waves (Reso Saw, Reso Tri, Reso Trap), whose resonant peak rises with DCW. Wave B can be Off (Wave A every cycle).

| Section | Parameter | What it does |
| :--- | :--- | :--- |
| **Line 1, Line 2** | Wave A, Wave B | The two waves the line alternates between, cycle by cycle |
| | DCW | The distortion before the envelope: 0 is a sine |
| | DCW Env | How far the DCW envelope adds to (or takes from) DCW |
| | DCW Att, Dec, Sus, Rel | The line's DCW envelope, which sweeps the timbre like a filter envelope |
| **Lines** | Line Mode | Line 1, Line 2, 1 + 2 (mixed) or 1 × 2 Ring |
| | L2 Level, L2 Detune, L2 Fine | Line 2's level in 1 + 2, its transposition (±24) and fine detune |
| | Noise | Noise modulation for a rough, breathy edge |
| | Key Follow | Less distortion for high notes, as on the original (keeps the top end clean) |
| **Amplifier** | Attack, Decay, Sustain, Release, Vibrato Rate, Vibrato Depth | The amplitude envelope and a vibrato |
| **Voice** | Tune, Fine, Glide Mode, Glide Time, Volume | |

Macros: DCW, DCW Env, DCW Decay, Detune, L2 Level and Noise. Velocity controls the volume.

Factory patches: Phase Brass, Phase Bass, Reso Sweep, Glass Piano, Square Organ, Ring Bell, Wire Pad and Pulse Lead.
