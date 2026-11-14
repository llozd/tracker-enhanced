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
