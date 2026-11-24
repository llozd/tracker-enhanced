# Realtime Insert Effects (Tracker+ and Tracker Mini)

Every audio channel has **3 insert effect slots**: the 8 sample tracks, the 3 synth parts, the 4 buses and the master. Effects run live while you play and are saved with the project. MIDI tracks have no audio, so they have no inserts. The Original Tracker does not have inserts.

---

## Opening the Inserts page

Press **Master** to cycle through the Master screen's pages: **Master**, **Mixer**, **Mixer (delay and reverb)**, then **Inserts**. On the Tracker Mini, pick **Master > Inserts** from the menu.

| Field | Button | What it does |
| :--- | :---: | :--- |
| **Channel** | F1 | Track 1 to 8, Synth 1 to 3, Bus 1 to 4, or Master |
| **Slot** | F2 | Slot 1, 2 or 3 of that channel. Slots run in order: slot 1 feeds slot 2, which feeds slot 3. Tracks, synth parts and buses also have an **Out** slot after slot 3 (see [Buses](#buses)) |
| **Effect** | F3 | The effect in the slot, or Off |
| **Page** | F4 | Which group of 4 parameters the last four fields show |
| **Parameters** | F5 to F8 | The effect's parameters, 4 per page |

Select a field with its button (or **Left** and **Right**), then turn the **encoder** or press **Up** and **Down** to change it. **Delete** on the Effect field clears the slot. On a parameter it restores that parameter's default.

### Meters and graphs

The bars above the fields show what the channel is doing:

* **Channel** and **Slot** columns: the channel's input and output level (green, -48 dBFS to full scale). With every slot empty they show the channel's level.
* **Effect** column, for the compressors and the limiter: the gain reduction (cyan, hanging down from the top, up to 24 dB).
* **Parameter** columns: each parameter's position in its range.
* **EQ 3 Band** and **EQ PQ 4 Band**: a graph of the EQ's frequency response (20 Hz to 20 kHz, ±18 dB, grid lines at 0, ±6 and ±12 dB) in place of the parameter bars. It follows the parameters as you turn them.

---

## Buses

The 4 buses group channels, so they can share effects. For example, to put one EQ and one compressor on tracks 1 to 4, send those tracks to Bus 1 and put the effects on Bus 1.

* **Send a channel to a bus**: pick the track or synth part as the Channel, turn Slot past 3 to **Out**, then set **Output** (F5) to Master or Bus 1 to 4. Delete on Output sends it back to Master.
* **Bus level**: pick the bus as the Channel and go to its **Out** slot. **Volume** (F5) runs from silent to +24 dB, 0 dB by default (Delete resets it). **Inputs** (F6) shows how many channels go to the bus.
* A channel sent to a bus is taken after its own inserts, volume and pan, and plays only through the bus. The bus then goes to the master mix, before the master inserts.
* Delay and reverb sends stay on the channel, so they are not affected by the bus's effects.
* Stems and multi-channel USB audio take each track from before the bus. A song export of the master includes the buses.

---

## Where the inserts sit

* **Sample tracks**: after the track's filter, before its volume and pan. The track's delay and reverb sends hear the effect.
* **Synth parts**: before the part's sends, so the sends hear the effect. A chorus or reverb makes a mono synth part stereo.
* **Buses**: on the sum of the channels sent to the bus, before the bus volume.
* **Master**: on the whole mix (tracks, buses, sends and inputs), before the limiter.

---

## Effects

| Effect | What it is | Parameters |
| :--- | :--- | :--- |
| **EQ 3 Band** | Low shelf, mid bell, high shelf | LF FRQ, LF GAIN, MF FRQ, MF GAIN, MF Q, HF FRQ, HF GAIN |
| **EQ PQ 4 Band** | Four-band parametric EQ after the Calrec PQ1549, with low and high cut | Frequency, gain and Q per band, LOW CUT, HIGH CUT (off at the ends of their range), OUT |
| **Compressor** | Utility compressor with a soft knee and a parallel mix | THRESH, RATIO, ATTACK, RELEASE, KNEE, MAKEUP, MIX |
| **Limiter** | Ceiling limiter | CEILING, RELEASE, GAIN |
| **Glue Comp** | Bus compressor with stepped ratio, attack and release (AUTO release), sidechain high pass | THRESH, RATIO, ATTACK, RELEASE, MAKEUP, MIX |
| **FET Comp** | Fast compressor driven by its input, with stepped ratio (ALL is the all-buttons mode) | INPUT, OUTPUT, ATTACK, RELEASE, RATIO |
| **Overdrive** | Soft drive with a tone filter | DRIVE, TONE, LEVEL, MIX |
| **Digital Clip** | Hard clip that folds back more and more | DRIVE, FOLD, LEVEL, MIX |
| **Bit Crush** | Bit depth and sample rate reduction | BITS, RATE, MIX |
| **Soft Clip** | Linear, then a soft shoulder | DRIVE, KNEE, TONE, LEVEL |
| **Diode Drive** | Mild diode-pair overdrive | DRIVE, TONE, LEVEL, MIX |
| **Tape** | Soft saturation, rolled-off top, low head bump | AMOUNT, TONE, LEVEL |
| **Tube** | Asymmetric saturation (even harmonics) | AMOUNT, BIAS, LEVEL |
| **Transformer** | Lows saturate first, top rolls off | AMOUNT, TONE, LEVEL |
| **Console** | Gentle desk-style saturation and air | AMOUNT, AIR, LEVEL |
| **Saturator** | Asymmetric saturation with a parallel mix | DRIVE, BIAS, TONE, MIX |
| **Exciter** | Adds harmonics to the top end only | FREQ, AMOUNT, MIX |
| **Chorus** | 2 to 4 modulated voices, spread across the stereo field | RATE, DEPTH, DELAY, VOICES, SPREAD, TONE, MIX |
| **Flanger** | Short modulated delay with feedback | RATE, DEPTH, FEEDBK, DELAY, MIX |
| **Phaser** | 4, 6 or 8 allpass stages with feedback | RATE, DEPTH, FEEDBK, STAGES, MIX |
| **Reverb** | The Tracker's reverb as an insert | SIZE, DAMP, PREDLY, DIFFUS, MIX |

The drives and colour effects keep quiet signals at the same level and add character on loud ones, so a high DRIVE can make the sound quieter. Use LEVEL to make it up.

---

## Limits

* **Reverb**: up to 4 reverb inserts at once (each needs 64 KB of memory). Picking a fifth shows **No free reverb**. The master reverb send is separate and always available.
* **CPU**: many effects on every channel can use a lot of processing power. If audio starts to break up, remove some effects.
* Empty slots cost nothing, and a channel's effects stop working once its input is silent and their tails have died out. A bus with nothing sent to it costs nothing.

---

## Projects

Insert settings, bus outputs and bus volumes are saved with the project. Projects made before inserts existed open with every slot empty, and projects made before buses existed open with every channel sent to the master. Projects with inserts open on the Original Tracker too, which keeps the settings but does not play the effects.
