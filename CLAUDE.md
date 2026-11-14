# Tracker Enhanced -- agent orientation

Community firmware (C++20, bare-metal ARM) for the Polyend Tracker family, forked from Polyend Tracker CE.
Three targets share one `core/`:

| Target | Make goal | `PLATFORM` / `PRODUCT` | CPU | Output |
| :--- | :--- | :--- | :--- | :--- |
| Original Tracker | `make original` | `MK66` / `ORIGINAL` | Cortex-M4F 180 MHz, 16 MB SDRAM, mono | `build/PolyendTracker_ENH_<ver>.ptf` |
| Tracker+ | `make plus` | `IMX` / `PLUS` | Cortex-M7 600 MHz, 64 MB SDRAM, stereo | `build/TrackerPlus_ENH_<ver>.ptf` |
| Tracker Mini | `make mini` | `IMX` / `MINI` | same as Plus, handheld controls | `build/TrackerMini_ENH_<ver>.ptf` |

## Setup (one time)

The only toolchain that is supported is GNU Arm Embedded **10.3-2021.10**. The Makefile picks up `./agt/10.3/` automatically (it is gitignored, and CI uses the same path):

```bash
mkdir -p agt/10.3
wget -q https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -O /tmp/agt.tar.bz2
tar -xjf /tmp/agt.tar.bz2 -C agt/10.3 --strip-components=1
agt/10.3/bin/arm-none-eabi-gcc --version   # 10.3.1 20210824
```

Also needs `make` and `python3` (standard library only). `clang-format` is optional.

## Build

```bash
make all        # all three targets (this is the "does it work" check)
make plus       # one target -- fastest loop while iterating
make ptfx       # Sample Editor plugins (editor-fx-sdk/effects/*.c) -> editor-fx-sdk/build/Effects/
make version    # print versions and output filenames
make clean      # REQUIRED after changing CUSTOM_DEFINES (flags are not tracked)
make plus CUSTOM_DEFINES="-DENABLE_LOG -DENABLE_DEV_CONSOLE"   # dev console + logging (IMX only)
DEBUG=1 make plus                                              # -Og + DEBUG (also changes boot behaviour)
```

- Build flow: each goal first builds `core/` into `libTracker.a` (`core/build-<PLATFORM>-<PRODUCT>/`), then links it in `platforms/mk66` or `platforms/imx` and copies the `.ptf` to `build/`.
- Measured on a 4-core VM (2026-11-14), clean builds: original ~2.5 min, plus ~2.7 min, mini ~2.8 min (`make all` is about 8 min). A no-op rebuild takes ~5 s and `make ptfx` ~1 s. Run long builds in the background and send output to a log, for example `make all > build/all.log 2>&1` (`build/` is gitignored).
- There are many existing warnings (roughly 2700 on original, 370 on plus, 280 on mini), mostly from vendor and 3rd party code. Don't try to get to zero -- just make sure your change adds none (`grep 'warning:' build/plus.log | grep <your file>`).
- Baseline memory usage at 0.1.0b1: Plus/Mini `SRAM_DTC` ~95% (~17 KB free), `SRAM_ITC` ~87%; Original `FLASH` ~86%, `RAM` ~60%, `EXTERNAL_RAM` ~97%.
- **An interrupted build leaves empty `.o` files** that make treats as up to date. The symptom is a link failure with `undefined reference` to symbols that obviously exist in `core/` (for example `AudioStream::transmit`, `sampleRecorder`). Fix: `make clean` and rebuild. Check with `find core/build-* -name '*.o' -empty`.
- Builds are incremental. Each goal deletes its `PlatformImpl.o`, so the build ID gets re-stamped every time.
- Check the linker `Memory region` table at the end of each target (CI posts it in the job summary). Headroom is tight -- see `docs/developer_guide.md` section 1.
- The Original's `.ptf` name must be 31 characters or fewer (bootloader buffer). `make original` enforces this.

## Tests

**There are no unit or host tests.** `core/source/mtTest.*` is the factory hardware self-test, not a test suite. To verify a change:
1. `make all` must compile cleanly for **all three** targets, even for single-platform changes. Do not add new warnings.
2. Check the memory regions did not grow unexpectedly.
3. Behaviour can only be checked on hardware. Write clear "How to Test" steps in the PR and mark it untested on hardware, as the OpenSpec statuses do.

CI (`.github/workflows/build.yml`) runs `make version`, `make all` and `make ptfx` on every PR and push to `main`. It **fails if any commit has an AI `Co-authored-by:` trailer** (Claude, Copilot, etc.). Never add one.

## Repo map

| Path | What |
| :--- | :--- |
| `core/source/` | Sequencer (`mtSequencer*.cpp`), MIDI, config, structs and file formats (`mtStructs.h`), logging (`log.h`) |
| `core/AudioEngine/` | Voices, mixer and FX, sequencer to audio glue (`mtAudioEngine`, `mtAudioNoteOnHandle.cpp`) |
| `core/interface/<screen>/` | UI screens (patternEditor, songEditor, configEditor, ...). `core/interface/core/` has the module table and `interfaceDefs.h` |
| `core/libs/NewDisplay/` | FT812 display rendering (for example `tracker.cpp` draws the pattern view) |
| `core/FileManager/` | Project, pattern and instrument load and save, old version upgrade (`mtFileManagerOldVersionRecovery`) |
| `core/include/platformFeatures.h` | Per-platform feature switches (`FEATURE_REALTIME_FX`, ...) |
| `core/config/` | Global config (`config.h`) |
| `platforms/imx`, `platforms/mk66` | Board support, drivers, linker scripts, `PlatformImpl.cpp`, entry points |
| `synth/` | Prebuilt `libPolySynth.a` / `libPolyDSP.a` + headers (closed source, do not edit) |
| `util/` | Shared utilities + 3rd party (boost, etc.), compiled into client modules |
| `editor-fx-sdk/` | `.ptfx` plugin SDK, `ptfx_builder.py`, example effects |
| `openspec/` | One record per feature: `changes/<name>/{proposal,design,tasks}.md` + `specs/*/spec.md`. Status table in `openspec/README.md` |
| `docs/` | User and developer docs (synced to the GitHub wiki). **Read `docs/developer_guide.md` before touching memory, the UI module table, fonts or platform guards** |
| `version.mk` | All version numbers. `*_VER_BETA := 255` means a full release |

## Rules and gotchas

- **Platform guards**: use `#ifdef PLATFORM_IMX` / `#ifndef PLATFORM_IMX`, `#ifdef TRACKER_MINI`, and `#if GUI_HANDLE_SYNTH` (it is always defined, so do **not** use `#ifdef`). Never use `__MK66FX1M0__` in `core/`, because it is defined on every platform. New gated features should go through a switch in `platformFeatures.h`, not test the platform directly.
- **Real-time audio code** (`update()`, `renderVoiceAudio()`, `processBlock()`, sequencer tick handlers): no heap allocation, no SD I/O, no blocking, no `%f` in `LOGF` from interrupts.
- **File formats**: `PROJECT_FILE_VERSION`, `PATTERN_FILE_VERSION` and `INSTRUMENT_FILE_VERSION` are in `core/source/mtStructs.h`. Never reorder serialized enums or shift existing struct fields. To bump a version, add an upgrade step in `mtFileManagerOldVersionRecovery.cpp`. ENH files are already incompatible with stock firmware (this is intended), but projects must stay portable between all three ENH devices.
- **Memory**: patterns are 512 rows on every device. `SAMPLE_MEMORY_SIZE` was reduced to pay for this, so check the memory regions before adding big static buffers.
- **UI**: never shift `Interface::modules[]` indices. Any UI or shortcut change needs to work on both the desktop units (pads and jog wheel) and the Mini (D-pad, Shift combos).
- **Line endings**: some files are CRLF (for example `core/libs/Audio/synth_PolySynth.cpp`). Keep each file's line endings, and check `git diff --stat` for whole-file changes. Python rewrites must use `open(p, newline='')`.
- **Style**: `core/.clang-format` (Mozilla based, Allman braces, 4 spaces, 100 columns). Only format the lines you change, never sweep a whole file. Many older comments are in Polish -- leave them alone.

## Workflow for a feature or fix

1. Branch from `main` with a kebab-case name matching the feature (for example `legato-rolls`). Never commit to `main`. PRs are merged on GitHub with merge commits.
2. For any feature, add or update its OpenSpec record (`openspec/changes/<name>/`, with a **Status** and a **Platforms:** line in `proposal.md`) and the status table in `openspec/README.md`. Tick `tasks.md` items as you go. Follow the format of an existing record such as `legato-rolls`.
3. User-visible changes also update the `README.md` feature list and `CHANGELOG.md`.
4. Commits: lowercase, imperative, under 72 characters (for example `gate roll hits for the full period so rolls play legato`), one logical change each, no em dashes, no AI co-author trailers.
5. Fill in `.github/PULL_REQUEST_TEMPLATE.md` (target devices, hardware testing table, how to test). Keep it concise, no em dashes.

## Release

Edit `version.mk` (or `make bump-patch|bump-minor TARGET=all|plus|mini|original`). Pushing a `v*` tag makes CI publish a GitHub release. A tag with a suffix (`v0.1.0-beta.1`) becomes a pre-release.
