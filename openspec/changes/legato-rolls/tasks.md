# Tasks

- [ ] Reproduce and capture the gap (envelope release vs gate length)
- [x] Gate for the full roll period (`rollGateLength`: period + 1 tick, so hits overlap)
- [x] Pre-retrigger note-off: kept. It now lands in the same tick as the next note-on, and MIDI needs it
- [x] Declick the restart: already handled. A note-on on a sounding voice fades it out over 1 ms (`envelopePhaseForced`, `RELEASE_NOTE_ON_VAL`) before restarting
- [x] Check dense consecutive notes: a new note already sends note-off and note-on in the same tick
- [ ] Hardware listening test
