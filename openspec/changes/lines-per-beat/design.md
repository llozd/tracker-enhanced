# Design

- **Timing:** base step period is scaled by 4/LPB (`getCurrentStepTempo()` = tempo * LPB / 4). FX are step-relative, so Roll, Arp, Micromove, Swing, Slide and Tempo speed up with LPB. Envelope/LFO sync follows the step; short delay sync stays beat-relative.
- **MIDI:** clock out every `2 * LPB` microsteps; external clock pulse is `576 * LPB / 24` nano ticks; Song Position unit (16th note) maps to `beats * LPB / 4` rows.
- **Storage:** LPB is an index into `{1,2,4,8,16}` (`linesPerBeatIndex`) because the menu list type stores an index. Project file version 18; config gets `defaultLinesPerBeatIndex`.
- **Pattern format:** `MAXSTEP` 1023, track length uint16, pattern file version 7, size 131136 bytes, validated by file size. Legacy patterns are read through `strPatternLegacy` and expanded in place.
- **Scaling:** `fitPatternToLinesPerBeat` rescales track lengths with rounding, clamped to 1..1024. Content is not moved. `onLinesPerBeatChanged` stops playback and clears undo.
- **Memory:** pattern-sized buffers cost about 4.5 MB, so sample memory drops from 60 MB to 55 MB. External RAM 98.5% used, SRAM_DTC 95.3% (about 16 KB free).
