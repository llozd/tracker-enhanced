#!/usr/bin/env python3
"""Display themes for Tracker Enhanced: writes <out>/Themes/<Name>.json (docs/custom_themes_and_fonts.md).

Each theme is a small palette; every other colour is derived from it (panels, inactive text and dimmed
pattern cells are blends of the background and the text), so dark and light themes work the same way.
Keys not written fall back to the Original theme ("base"). Checks: names up to 31 characters, files
under the 8 KB the firmware reads, and readable contrast for text and the pattern colours.

Usage: gen_themes.py OUT_DIR
"""
import json
import os
import sys

# name: (background, text, accent, note, instrument, fx1, fx2, green, orange, yellow, cyan)
PALETTES = {
    "Midnight Teal": ("#0b1416", "#d8eef0", "#ff5c5c", "#3fe0c5", "#f2d16b", "#b48cff", "#5cc8ff", "#3ddc84", "#ff9f43", "#ffd166", "#4dd0e1"),
    "Ember": ("#140b08", "#f4e3d7", "#ff6b35", "#ffb347", "#ffd93d", "#ff6f91", "#f9a03f", "#9bdb4d", "#ff8c42", "#ffd23f", "#6ec6ca"),
    "Deep Ocean": ("#06101e", "#d6e6ff", "#ff4f79", "#4fc3f7", "#ffe082", "#9575cd", "#4dd0e1", "#26c6a4", "#ffab40", "#ffee58", "#40c4ff"),
    "Forest Floor": ("#0d140c", "#e4ecd8", "#e2583e", "#8fd16a", "#e7c95b", "#c49a6c", "#7fc8a9", "#6fcf4a", "#e0903b", "#e8d24a", "#69c2b4"),
    "Neon Arcade": ("#08040f", "#f2e9ff", "#ff2e88", "#00f5d4", "#fee440", "#c77dff", "#00bbf9", "#39ff14", "#ff9e00", "#fee440", "#00f5ff"),
    "Rose Gold": ("#160e10", "#f7e7e4", "#e8577e", "#f2a7a0", "#e9c46a", "#d4a5ff", "#9fd8cb", "#8fd694", "#f4a261", "#f2d16b", "#8ecae6"),
    "Arctic": ("#0e1620", "#eaf4fb", "#ff6b6b", "#88d8f7", "#f9e79f", "#b9a6ff", "#a0f0e4", "#7bdc9a", "#ffb26b", "#f9e79f", "#7fdbff"),
    "Paper": ("#f4f1ea", "#1f1d1a", "#c8102e", "#00797a", "#9a6a00", "#7a3fb0", "#1f62b5", "#2e8b3a", "#c0621a", "#9a7a00", "#00838f"),
    "Sepia": ("#e9dcc4", "#2b2218", "#a8321e", "#6b4a1e", "#8a5a00", "#7a3b52", "#2f5d62", "#4e7a2a", "#a65a1a", "#8a6d00", "#2f6f73"),
    "Phosphor Amber": ("#0d0700", "#ffcf7a", "#ff7a1a", "#ffb000", "#ffd27a", "#ff9a3c", "#ffc35a", "#ffd27a", "#ff9a3c", "#ffd27a", "#ffc35a"),
    "Phosphor Green": ("#010a03", "#9cff9c", "#e8ff5a", "#33ff66", "#b6ff6b", "#5aff9c", "#7dffb0", "#33ff66", "#c8ff5a", "#e8ff5a", "#5affc8"),
    "Ice Blue": ("#081018", "#dbeeff", "#ff5a7a", "#7ec8ff", "#c6e2ff", "#a5b4ff", "#9ef0ff", "#7fe0b0", "#ffb38a", "#fff0a0", "#9ef0ff"),
    "Lava": ("#100606", "#f9e2dc", "#ff2d2d", "#ff7b54", "#ffcc33", "#ff4f81", "#ffa64d", "#b5e655", "#ff7b2d", "#ffcc33", "#5ec4c4"),
    "Sakura": ("#150c12", "#fbe9f1", "#ff4d8d", "#ffa6c9", "#ffe066", "#c9a0ff", "#8fe3d4", "#9be09b", "#ffb077", "#ffe066", "#8fd3ff"),
    "Desert Dusk": ("#160f0c", "#f3e1cf", "#e4572e", "#f2a65a", "#f6d36b", "#c97b9c", "#6fb3b8", "#9cc069", "#ef8a3b", "#f6d36b", "#6fb3b8"),
    "Moss": ("#0c100a", "#e2e8d2", "#d65a31", "#a3c45a", "#d9c46a", "#8faa8b", "#6fb79b", "#8ccf4d", "#d8913f", "#d9c46a", "#6fb79b"),
    "Ultraviolet": ("#0c0716", "#ece3ff", "#ff4fd8", "#a07bff", "#ffd36e", "#ff7ad9", "#6ad3ff", "#6fe7a6", "#ffa15c", "#ffd36e", "#6ad3ff"),
    "Graphite": ("#121212", "#e6e6e6", "#e05252", "#bdbdbd", "#e0c068", "#9f9fd1", "#86c6c6", "#7ccf7c", "#e0a060", "#e0c068", "#86c6c6"),
    "Copper": ("#120c09", "#f1e0d3", "#d9534f", "#e08e55", "#e8c07a", "#c9876a", "#7fb6a6", "#9fc36a", "#e08e55", "#e8c07a", "#7fb6a6"),
    "Sea Glass": ("#0a1413", "#ddf2ee", "#f06d6d", "#7fd8c2", "#e9dc8a", "#a3b8e0", "#94e0d8", "#86d98f", "#f0a96d", "#e9dc8a", "#7fd0e8"),
    "Retro Beige": ("#e8e0cc", "#26231d", "#b5371f", "#1f6f5c", "#8a6408", "#6b3f8c", "#24588f", "#3b7d2b", "#b8601c", "#8a6e08", "#1f7a80"),
    "Blood Moon": ("#0f0507", "#f5dcdc", "#ff3048", "#ff6f61", "#ffb86b", "#d96aa0", "#f08080", "#c4d96b", "#ff8a4c", "#ffb86b", "#7fbfbf"),
    "Lime Twist": ("#0b0f06", "#eef7d9", "#ff5a36", "#c6ff3d", "#ffe14d", "#7ee0a1", "#66e0e0", "#9cff3d", "#ffa63d", "#ffe14d", "#66e0e0"),
    "Storm": ("#0d1117", "#d9e1ea", "#f85149", "#79b8ff", "#e3b341", "#b392f0", "#56d4dd", "#56d364", "#f0883e", "#e3b341", "#56d4dd"),
    "Candy": ("#130a14", "#fde9ff", "#ff3fa4", "#ff8fd8", "#fff07a", "#8ff0ff", "#b4a6ff", "#8cff9e", "#ffb36b", "#fff07a", "#8ff0ff"),
}

MAX_THEMES = 29  # ThemeManager::kMaxCustomThemes minus the 3 built-in themes


def rgb(hex_color):
    return tuple(int(hex_color[i:i + 2], 16) for i in (1, 3, 5))


def to_hex(c):
    return "#{:02x}{:02x}{:02x}".format(*(max(0, min(255, round(x))) for x in c))


def mix(a, b, t):
    """t = 0 gives a, t = 1 gives b."""
    ca, cb = rgb(a), rgb(b)
    return to_hex(tuple(x + (y - x) * t for x, y in zip(ca, cb)))


def luminance(c):
    def ch(v):
        v /= 255
        return v / 12.92 if v <= 0.03928 else ((v + 0.055) / 1.055) ** 2.4
    r, g, b = rgb(c)
    return 0.2126 * ch(r) + 0.7152 * ch(g) + 0.0722 * ch(b)


def contrast(a, b):
    la, lb = sorted((luminance(a), luminance(b)), reverse=True)
    return (la + 0.05) / (lb + 0.05)


def quad(text, background, frame, text2):
    return {"text": text, "background": background, "frame": frame, "text2": text2}


def theme(name, palette):
    bg, text, accent, note, instr, fx1, fx2, green, orange, yellow, cyan = palette
    t = lambda amount: mix(bg, text, amount)  # a step from the background towards the text
    grid_bg = t(0.03)
    label_bg = t(0.13)
    return {
        "name": name,
        "base": "Original",
        "background": bg,
        "topBar": quad(text, t(0.04), text, text),
        "songTimer": quad(text, t(0.04), accent, text),
        "labels": {
            "active": quad(t(0.5), label_bg, accent, text),
            "inactive": quad(t(0.2), label_bg, accent, t(0.33)),
            "activeButton": quad(t(0.5), label_bg, accent, t(0.5)),
            "inactiveButton": quad(t(0.2), label_bg, accent, t(0.2)),
            "highlightedButton": quad(t(0.5), t(0.09), accent, t(0.5)),
            "redButton": quad(t(0.5), accent, accent, t(0.5)),
            "performActiveTrack": quad(t(0.5), accent, accent, text),
            "performMutedTrack": quad(bg, accent, accent, bg),
        },
        "frames": {"active": accent, "inactive": t(0.25), "green": green},
        "patternTracker": {
            "trackName": bg,
            "background": grid_bg,
            "note": note,
            "instrument": instr,
            "fx1": fx1,
            "fx2": fx2,
            "inactiveNote": mix(grid_bg, note, 0.25),
            "inactiveInstrument": mix(grid_bg, instr, 0.25),
            "inactiveFx1": mix(grid_bg, fx1, 0.25),
            "inactiveFx2": mix(grid_bg, fx2, 0.25),
            "selection": accent,
            "gridDivider": t(0.06),
            "inactiveTrack": t(0.09),
            "playhead": mix(grid_bg, accent, 0.18),
            "selectionText": text,
            "verticalDivider": bg,
            "stepNumber": t(0.4),
            "greenMode": green,
            "unfocusedTrack": t(0.27),
            "activeTrack": t(0.4),
            "trackEnd": instr,
        },
        "songPlayer": {
            "text": text,
            "activeBlock": text,
            "scrollbar": t(0.34),
            "background": t(0.04),
            "inactiveText": t(0.2),
            "editMode": accent,
            "scrollbarBackground": bg,
            "playhead": t(0.27),
            "emptyTrack": t(0.07),
            "playheadActive": green,
        },
        "lists": {
            "default": {
                "frameNormal": text,
                "frameSelect": text,
                "scrollbar": t(0.34),
                "background": t(0.04),
                "textNormal": text,
                "textSelect": text,
                "scrollbarTrack": bg,
            },
        },
        "bars": {
            "active": {"primary": text, "accent1": accent, "accent2": green, "container": t(0.03), "background": t(0.04)},
            "inactive": {"primary": t(0.13), "accent1": accent, "accent2": green, "container": t(0.03), "background": t(0.04)},
        },
        "accents": {"white": text, "red": accent, "orange": orange, "yellow": yellow, "green": green, "cyan": cyan,
                    "grey": t(0.13)},
    }


def check(name, data, text):
    problems = []
    if len(name) > 31:
        problems.append("name longer than 31 characters")
    if len(text.encode()) > 8192:
        problems.append("file over 8 KB")
    p = data["patternTracker"]
    if contrast(data["background"], data["topBar"]["text"]) < 7.0:
        problems.append("text contrast below 7:1")
    for key in ("note", "instrument", "fx1", "fx2"):
        if contrast(p["background"], p[key]) < 3.0:
            problems.append(f"{key} contrast below 3:1")
    return problems


def main():
    out = os.path.join(sys.argv[1] if len(sys.argv) > 1 else "build", "Themes")
    os.makedirs(out, exist_ok=True)
    assert len(PALETTES) <= MAX_THEMES, "more themes than the firmware indexes"
    failed = 0
    for name, palette in PALETTES.items():
        data = theme(name, palette)
        text = json.dumps(data, indent=2) + "\n"
        problems = check(name, data, text)
        if problems:
            failed += 1
            print(f"{name}: {', '.join(problems)}")
        with open(os.path.join(out, name.replace(" ", "") + ".json"), "w") as f:
            f.write(text)
    print(f"wrote {len(PALETTES)} themes to {out}")
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
