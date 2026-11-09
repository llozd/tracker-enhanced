# Design

## Feature switches
- `core/include/platformFeatures.h` defines per-platform switches, chosen by `PLATFORM_IMX`. Features test these switches, never the platform directly.

| Gate | Plus / Mini | Original | Used by |
| :--- | :---: | :---: | :--- |
| `FEATURE_REALTIME_FX` | 1 | 0 | `realtime-effects` |
| `FEATURE_SYNTH` (core makefile, existing) | on | off | synth engines; `fx-only-row-instrument` is MIDI-only on the Original |
| `SAMPLE_MEMORY_SIZE` (`mtStructs.h`, existing) | 55 MB | 10.25 MB | pays for the pattern buffers |

- Limits are added to the header when a feature needs a per-platform value (for example `extended-midi-cc` slot counts, or a lower LPB cap if 16 LPB is too heavy on the Original).
- Excluded features are wrapped in `#if FEATURE_...` (code and UI), or their source files are skipped in the MK66 build. This follows the existing `FEATURE_SYNTH` / `GUI_HANDLE_SYNTH` pattern.
- Menus only offer what the platform supports.

## Project portability
- One project and pattern format for all devices, so projects move between Original and Plus/Mini.
- Settings for excluded features (for example effect parameters) are still stored in the file. The Original keeps them unchanged and ignores them, so a project saved on the Original still has them on a Plus.

## Memory (MK66: 16 MB SDRAM)
- Patterns are 512 rows on every device, so the pattern format stays the same everywhere. With 1024 rows the Original was about 2.9 MB over; with 512 it was about 0.58 MB over.
- `SAMPLE_MEMORY_SIZE` on the Original drops from 11.25 MB to 10.25 MB (resident pool about 7.5 MB to 6.5 MB). Undo depth is unchanged (20 pattern, 10 song).
- Keep `SAMPLE_MEMORY_SIZE > 2 * SAMPLE_CHANNELS * EDIT_BUFF_SIZE` (3.75 MB on MK66).

## Build
- Revert the platform and build parts of `e6e43cb`. Keep the newer README and docs changes and update them.
- `make all` builds Plus, Mini and Original. CI builds and uploads all three.
