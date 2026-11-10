# Tasks

- [x] LPB timing engine, 1 to 16 (#3)
- [x] Reset version to 0.1.0 (#3)
- [x] Global default LPB in config (#4)
- [x] Per-project LPB setting and project file version 18 (#5)
- [x] Widen patterns to 1024 rows, load legacy pattern files (#6)
- [x] Reduce patterns to 512 rows (pattern file v8), cut v7 files on load
- [x] Scale pattern lengths on LPB change (#7)
- [x] Update README and memory docs (#7)
- [x] Add 12 LPB for triplet subdivisions
- [ ] Hardware test: load old projects, save and reload, undo, copy/paste at long lengths
- [ ] Hardware test: external clock sync and CPU load at 16 LPB
- [x] Metronome follows LPB: its settings count 16th notes and are converted to rows (`isMetronomeTickRow`, `getMetronomeBarRows`), including the count-in length
- [ ] MOD export and time-stretch FX still assume 4 steps per beat
- [ ] Pads backlight length indicator above 191 rows; performance-mode length values
- [ ] Optional: stretch note content when LPB changes
