# Lines per beat

**Status: complete** (PRs #3 to #7). Built for Tracker+ and Mini; not yet tested on hardware.

## Why
Fixed 4 lines per beat limits rhythmic detail. Renoise lets the user choose the resolution per song.

## What changes
- Selectable LPB of 1, 2, 4, 8 or 16, stored per project, with a global default for new projects.
- Patterns grow from 128 to 1024 rows.
- Changing LPB scales pattern lengths so duration is kept.
- Project and pattern formats change and are not backwards compatible with official firmware. Old pattern files (v1 to v6) still load.
- Version reset to 0.1.0.

## Out of scope
32 LPB (too ambitious for now), stretching note content on LPB change.
