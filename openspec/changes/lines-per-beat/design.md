# Design

- **Timing:** base step period is scaled by 4/LPB (`getCurrentStepTempo()` = tempo * LPB / 4). FX are step-relative, so Roll, Arp, Micromove, Swing, Slide and Tempo speed up with LPB. Envelope/LFO sync follows the step; short delay sync stays beat-relative.
- **MIDI:** clock out every `2 * LPB` microsteps; external clock pulse is `576 * LPB / 24` nano ticks; Song Position unit (16th note) maps to `beats * LPB / 4` rows.
- **Storage:** LPB is an index into `{1,2,4,8,16}` (`linesPerBeatIndex`) because the menu list type stores an index. Project file version 18; config gets `defaultLinesPerBeatIndex`.
- **Pattern format:** `MAXSTEP` 511, track length uint16, pattern file version 8, size 65600 bytes, validated by file size. Legacy patterns (v6 and older) are read through `strPatternLegacy` and expanded in place. Version 7 files (1024 rows, 131136 bytes) are read track by track, keeping the first 512 rows, and lengths are clamped.
- **Scaling:** `fitPatternToLinesPerBeat` rescales track lengths with rounding, clamped to 1..512. Content is not moved. `onLinesPerBeatChanged` stops playback and clears undo.
- **Memory:** 36 pattern-sized buffers (sequencer, file manager, song copy, 20 pattern undo, 10 song undo) cost about 2.3 MB. Plus/Mini sample memory stays at 55 MB (60 MB before LPB). The Original drops from 11.25 MB to 10.25 MB.
