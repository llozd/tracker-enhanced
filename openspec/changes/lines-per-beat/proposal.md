# Lines per beat

**Status: complete** (PRs #3 to #7). Built for Tracker+ and Mini; not yet tested on hardware.

**Platforms:** all, with the same 512-row pattern format everywhere. On the Original the pattern buffers are paid for by 1 MB less sample memory. The 16 LPB cap may need lowering there after hardware tests.

## Why
Fixed 4 lines per beat limits rhythmic detail. Renoise lets the user choose the resolution per song.

## What changes
- Selectable LPB of 1, 2, 4, 8, 12 or 16, stored per project, with a global default for new projects.
- Patterns grow from 128 to 512 rows (1024 in pattern file v7, reduced to 512 in v8 so the Original fits).
- Changing LPB scales pattern lengths so duration is kept.
- Project and pattern formats change and are not backwards compatible with official firmware. Old pattern files (v1 to v7) still load; v7 patterns longer than 512 rows are cut to 512.
- Version reset to 0.1.0.

## Out of scope
32 LPB (too ambitious for now), stretching note content on LPB change.
