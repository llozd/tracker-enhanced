# Design

## Feature switches
- One header (for example `core/include/platformFeatures.h`) defines per-platform switches and limits, chosen by `PLATFORM_IMX`:
  - `FEATURE_REALTIME_FX`: 1 on IMX, 0 on MK66.
  - `kMaxLinesPerBeatIndex`, undo depth, sample memory size.
- Features test these switches, never the platform directly. Excluded features are wrapped in `#if FEATURE_...` (code and UI), or their source files are skipped in the MK66 build. This follows the existing `FEATURE_SYNTH` / `GUI_HANDLE_SYNTH` pattern.
- Menus only offer what the platform supports.

## Project portability
- One project and pattern format for all devices, so projects move between Original and Plus/Mini.
- Settings for excluded features (for example effect parameters) are still stored in the file. The Original keeps them unchanged and ignores them, so a project saved on the Original still has them on a Plus.

## Memory (MK66: 16 MB SDRAM, about 1.1 MB free)
- 1024-row patterns need about 4 MB more for pattern-sized buffers.
- Options: resident sample pool from about 7.5 MB to about 4 MB (frees about 3.5 MB), undo depth from 20 to about 5 (frees about 2 MB).
- Keep `SAMPLE_MEMORY_SIZE > 2 * SAMPLE_CHANNELS * EDIT_BUFF_SIZE` (3.75 MB on MK66).

## Build
- Revert the platform and build parts of `e6e43cb`. Keep the newer README and docs changes and update them.
- `make all` builds Plus, Mini and Original. CI builds and uploads all three.
