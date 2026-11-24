# Bus channels

**Status: implemented**, untested on hardware. CPU cost to be measured with the inserts.

**Platforms:** Plus and Mini only, built on [realtime-effects](../realtime-effects/proposal.md) and wrapped in `#if FEATURE_REALTIME_FX`. The Original stores the settings and ignores them, so projects stay portable.

## Why
Shared processing on a group of channels (one EQ and one compressor on tracks 1 to 4, say) needs an effect on every track today, which costs CPU per track and does not compress the group as a whole.

## What changes
- **4 buses**, each with 3 insert slots (the same effects as every other channel) and a volume.
- **An output per sample track and synth part**: Master (default) or Bus 1 to 4. The channel is taken after its own inserts, volume and pan, and plays only through its bus.
- Buses go into the master mix, before the master inserts and the limiter.
- Delay and reverb sends stay on the channel (tapped before volume, as now), so the bus effects work on the dry signal.
- Stems and multi-channel USB audio still take each track before the bus. The master export includes the buses.

## UI
On the Master screen's Inserts page, the Channel picker gains Bus 1 to 4 (between the synth parts and the master), and Slot gains an **Out** slot after slot 3 on tracks, synth parts and buses:
- Track or synth part: F5 **Output** (Master, Bus 1 to 4).
- Bus: F5 **Volume** (silent to +24 dB, 0 dB default), F6 **Inputs** (how many channels go to it, read only).
- Delete on these restores Master or 0 dB.

## Out of scope
- Bus pan and mute, sending a bus to another bus, and sends from a bus.
- A bus stem in the export.
