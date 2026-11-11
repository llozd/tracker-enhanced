# Tracker Enhanced

**Tracker Enhanced** is a community firmware for the **Polyend Tracker+**, **Polyend Tracker Mini** and, with a reduced feature set, the **Original Polyend Tracker**, built on top of the open-source [Polyend Tracker Community Edition](https://github.com/polyend/tracker-ce). It adds finer sequencing resolution, deeper MIDI CC control and more audio processing, taking some of the ideas that make [Renoise](https://www.renoise.com/) good for detailed tracking.

> **Status: beta.** The first beta, [0.1.0 beta 1](https://github.com/llozd/tracker-enhanced/releases), has been tested on a Tracker+. It has not been tested on the Original or Mini yet. It includes:
> * Lines per beat (1, 2, 4, 8, 12 or 16)
> * Row number beat shading
> * Legato rolls
> * Position on slices
> * Effects on rows without a note
> * Original Tracker build (reduced feature set)
>
> Extended MIDI CC control and the new effects are still planned. Every change, finished or planned, is recorded in [`openspec/`](openspec/README.md).

---

## tl;dr
Hopefully, the Tracker Enhanced becomes a mini Renoise, and just generally have more polished features.

Use this firmware at your own risk and curiousity. Things may still be in development, so it is recommended to stick to [released](https://github.com/llozd/tracker-enhanced/releases) versions - building from source on `main` may contain partially complete or features that aren't fully tested. If you find any bugs, or have any general issues or suggestions, feel free to open an [issue](https://github.com/llozd/tracker-enhanced/issues)!

Some features will not make it to the OG Tracker unfortunately, due to hardware limitations. Below is a list of features for each device.

#### All Versions (OG, Mini, Plus)
- LPB (Lines per Beat) settings
- Smoother rolls/fast retriggering notes, without the audible stop between each retrigger
- Effects on a row without a note, so things like microtune/pitchbend work after the key has been released
- Extended MIDI settings
    - More assignable CCs per instrument
    - More than two CCs per row
- Better chord implementation

#### Plus and Mini only
- More realtime effects
    - Vintage-modeled parametric EQs
    - More compressor types
    - Signal processing, like exciters, signal colouring, saturation

## Important: Read Before Flashing

> **Project files are NOT backwards compatible.**
> Tracker Enhanced changes the project and pattern file structures (for example to store lines per beat, extra MIDI CC assignments and new effect settings).
> * Projects saved with Tracker Enhanced **will not open** on stock Polyend firmware or the upstream Community Edition.
> * Existing projects made on other firmware may need to be converted, and a one-way conversion may be required.
> * **Back up your microSD card before flashing.**

---

## Features and Roadmap

Each item has an OpenSpec record in [`openspec/changes/`](openspec/README.md).

### 1. Lines Per Beat (LPB)
Renoise-style control over how many pattern rows make up one beat. Stock firmware is fixed at 4 steps per beat.
* LPB selectable as **1, 2, 4, 8, 12 or 16** per project (12 gives triplet subdivisions) (**Project Settings > Lines per beat**). Changing it also sets the pattern divider to one beat.
* A **global default** (**General > Default LPB**) is given to new projects.
* Step FX (Roll, Arp, Micromove, Swing, Slide, Tempo) and envelope/LFO sync are measured in steps, so they speed up with LPB. The short delay sync stays tied to the beat.
* MIDI clock out and in, and Song Position, follow the LPB.
* **Patterns hold up to 512 rows** (128 before). When LPB changes, pattern **lengths scale with it**, so a 64 row pattern at 4 LPB becomes 256 rows at 16 LPB and keeps its duration. Note content is not moved or stretched, only the length changes. Scaling down rounds the length to the nearest row.
* New patterns default to 32 rows at 4 LPB, scaled to the current LPB. The preset length values in the pattern editor scale the same way.
* Changing LPB stops playback and clears the undo history.
* To make room for the larger pattern buffers (about 2.3 MB), the sample memory is reduced from 60 MB to 55 MB on Tracker+ and Mini, and from 11.25 MB to 10.25 MB on the Original. Long samples are streamed from the SD card as before.
* **Row number beat shading**: row numbers show where each row sits in the beat. Beat rows are full brightness, half-beat rows use a dimmed note colour, and other rows are dimmed. See `row-number-beat-shading`.

### 2. Extended MIDI CC Control (planned)
Today each MIDI instrument has 6 CC slots and each step has 2 FX slots.
* More CC assignments per MIDI instrument.
* More than 2 CC messages on a single row.

### 3. New Effects (per track and master) (planned)
New realtime effects, available on individual tracks and on the master channel:
* **Parametric EQ**, inspired by large-format broadcast console EQs.
* **Saturation**, **exciter** and **noise / colouring**.
* **Console / sound desk** style processing, simulating audio passing through different pieces of studio equipment.
* **More compressors**: vintage, FET and glue styles.

CPU usage will be measured on hardware, and the number of simultaneous per-track effects may be limited accordingly.

### 4. Playback and Interface Fixes
* **Legato rolls**: roll hits are gated for the full period, so rolls play as full-length, click-free hits instead of sounding gappy. See `legato-rolls`.
* **Position on slices**: in Slice and Beat Slice modes, the Position effect scrubs within the played slice instead of the whole sample. See `position-fx-slices`.
* **Effects on rows without a note**: Microtune and synth effects on an empty row use the track's last played instrument on MIDI and synth tracks, so a note can be bent later, including on its release. See `fx-only-row-instrument`.
* **Held button release** (investigating): a held button (such as FX1) sometimes acts as released while turning the knob. A suspected cause is the key driver clearing buttons after an I2C error. See `button-hold-release`.

### 5. Original Tracker Support (in progress)
The Original Tracker (`MK66`) is built again, with a reduced feature set. It gets the sequencer improvements: lines per beat (up to 16), legato rolls, Position on slices, and the MIDI part of effects on rows without a note. Realtime effects and other features with significant CPU or memory overhead will be compiled out. Projects will stay portable between devices. It has not been tested on hardware yet, and a hardware tester is still needed. See `original-tracker-support`.

---

## Based On

Tracker Enhanced is derived from the source code released by **Polyend** as [Polyend Tracker Community Edition](https://github.com/polyend/tracker-ce), and stays under its licence (see [License](#license)).

This is an independent community project. It is **not** made, endorsed or supported by Polyend. See the disclaimer below.

> **Provided "AS IS" without warranty or support.** Flashing custom firmware is done at your own risk. Thanks to the isolated bootloader and hardware recovery button (see [Flashing on Hardware & Emergency Recovery](#4-flashing-on-hardware--emergency-recovery)), a build that fails to boot can be recovered by re-flashing a known-good `.ptf`. Issues with this firmware should be reported in this repository, not to Polyend support.

---

## Contributing

Bug reports and pull requests are welcome. Please read [`CONTRIBUTING.md`](CONTRIBUTING.md) first. Note that it was inherited from upstream and will be updated for this project.

---

## Building Firmware

### 1. Prerequisites
* **[GNU Arm Embedded Toolchain `10.3-2021.10`](https://developer.arm.com/downloads/-/gnu-rm)**:
  * Direct official downloads from Arm:
    * **Linux (`x86_64`)**: [`gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2`](https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2)
    * **Linux (`aarch64` / ARM64)**: [`gcc-arm-none-eabi-10.3-2021.10-aarch64-linux.tar.bz2`](https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-aarch64-linux.tar.bz2)
    * **macOS (`x86_64` / Apple Silicon via Rosetta)**: [`gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2`](https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2)
    * **Windows (ZIP)**: [`gcc-arm-none-eabi-10.3-2021.10-win32.zip`](https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-win32.zip)
  * Ensure `arm-none-eabi-gcc` is on your `PATH`, extract the toolchain into `./agt/10.3/`, or pass `TOOLCHAIN=/path/to/arm-none-eabi-` to `make`.
* `make` (for building firmware) and `python3` (used by `make ptfx` to package `.ptfx` Sample Editor plugins and by the `make bump-*` version helpers; standard library only).

### 2. Build Commands
From the repository root:

```bash
# Build all devices (Original, Tracker+, Tracker Mini)
make all

# Or build a single target:
make original   # -> build/PolyendTracker_ENH_0.1.0b1.ptf
make plus       # -> build/TrackerPlus_ENH_0.1.0b1.ptf
make mini       # -> build/TrackerMini_ENH_0.1.0b1.ptf

# Build reference .ptfx Sample Editor effects into editor-fx-sdk/build/Effects/:
make ptfx

# Clean all build directories:
make clean
```

### 3. Version Management (`version.mk`)
All firmware version numbers are defined in [`version.mk`](version.mk). The **Build** identifier displayed on the hardware under **Config > Firmware > Build** is automatically populated from the short Git commit hash (`git rev-parse --short=7 HEAD`):

```bash
# Show current version, Git build hash & output filenames
make version

# Increment patch version (e.g. 1.0.0 -> 1.0.1)
make bump-patch TARGET=all        # or TARGET=plus | TARGET=mini

# Increment minor version (e.g. 1.0.x -> 1.1.0)
make bump-minor TARGET=all        # or TARGET=plus | TARGET=mini
```

`*_VER_BETA` marks a beta: any value other than `255` adds `b<n>` to the `.ptf` name (e.g. `TrackerPlus_ENH_0.1.0b1.ptf`) and shows `beta <n>` on the device. Set it to `255` for a full release. Pushing a tag with a suffix, such as `v0.1.0-beta.1`, publishes a GitHub pre-release.

### 4. Flashing on Hardware & Emergency Recovery

1. Download a `.ptf` file from [Releases](https://github.com/llozd/tracker-enhanced/releases), or copy the generated one from `build/` (`PolyendTracker_ENH_*.ptf`, `TrackerPlus_ENH_*.ptf` or `TrackerMini_ENH_*.ptf`) to the `/Firmware/` folder on your Tracker's microSD card.
2. On the device, open **Config > Firmware > Firmware update**, select the `.ptf` file, and confirm.

> **Bootloader Resilience & Emergency Firmware Recovery**:
> * The Polyend Tracker bootloader is isolated from the application firmware:
>   * On **Tracker+ and Tracker Mini (`IMX`)**, the bootloader resides in a protected region at the bottom of QSPI Flash (`0x60000000`–`0x6002FFFF`), while `.ptf` application builds are linked starting at `0x60030000` (do not modify the flash `ORIGIN` address in the linker scripts).
> * **Emergency Recovery**: If a custom firmware build ever hangs or fails to boot into the UI, you can always recover your device:
>   1. Place a known-good, stable `.ptf` firmware file for your device inside the `/Firmware/` folder on your microSD card (ideally as the only `.ptf` file in `/Firmware/`).
>   2. Insert the microSD card into your Tracker and power it on.
>   3. Use a paperclip or SIM-eject pin to press the **recessed hardware Reset / Emergency Bootloader pinhole button** (located on the bottom of the Tracker+, or next to the USB-C port on Tracker Mini). The hardware bootloader will automatically pick up the `.ptf` file from `/Firmware/` and flash it.

---

## Documentation & Developer Guides

Full user and developer documentation is available in [`docs/`](docs/Home.md) and on the repository **Wiki**:

* **User Guides**:
  * [Custom Themes & Fonts](docs/custom_themes_and_fonts.md) + [Live Web Editor](https://polyend.github.io/tracker-theme-editor/) ([`polyend/tracker-theme-editor`](https://github.com/polyend/tracker-theme-editor))
  * [Wavetable Envelopes, LFOs & Step FX](docs/wavetable_envelopes.md)
  * [SD Card Image Viewer](docs/image_viewer.md)
* **Developer & Architecture Guides**:
  * [Firmware Architecture, Hardware Limits & Developer Gotchas](docs/developer_guide.md)
  * [Custom Synth Engine SDK](docs/custom_synth_sdk.md)
  * [Modular Sample Editor Effects & `.ptfx` Developer SDK](docs/modular_effects_system.md)

---

## Related Projects

* **[Polyend Tracker Community Edition](https://github.com/polyend/tracker-ce)**: the upstream source this project is based on.
* **[`polyend/tracker-lib`](https://github.com/polyend/tracker-lib)**: library and tools for reading, writing and inspecting Polyend Tracker project (`.mt`), pattern (`.mtp`) and instrument (`.pti`) files. Note that Tracker Enhanced project files will differ from the stock formats.

---

## License

* **Source Code, SDKs, Open Synth Engines (`core/synth/engines/`), `.ptfx` Sample Editor SDK (`editor-fx-sdk/`), Drivers & Utilities**: Licensed under the **MIT License with a Wholesale Commercial Clone Restriction** (see [`LICENSE`](LICENSE)).
  * **Musicians & Creators**: Using the firmware for commercial music production, albums, paid live performances/gigs, sample packs, and monetized streaming/YouTube content is **expressly permitted**.
  * **Developers & Plugin Authors**: Reusing individual source files, drivers, DSP algorithms, UI widgets, or open-source C++ synth engines (`CHIP`, etc.) in your own open-source or commercial projects—and selling `.ptfx` Sample Editor plugins—is **expressly permitted** under the MIT License.
  * **No Wholesale Commercial Clones**: You may **not** sell compiled `.ptf` firmware builds, manufacture/sell commercial clone hardware running the Tracker firmware, or sell a direct port/wholesale clone of the complete Tracker application as a paid software product.
* **Precompiled Synth & DSP Libraries (`synth/lib/libPolySynth.a`, `synth/lib/libPolyDSP.a`)**: Provided under the [Polyend Precompiled Synthesizer & DSP Library License](synth/lib/LICENSE-PROPRIETARY.txt), permitting free use on authentic Polyend hardware as well as personal/DIY non-commercial projects (commercial embedding of the precompiled `.a` binaries is prohibited).
* **Third-Party Components**: See [`NOTICE.md`](NOTICE.md).
