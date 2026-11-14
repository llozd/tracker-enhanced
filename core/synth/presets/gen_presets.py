#!/usr/bin/env python3
"""Factory .patch files for the Tracker Enhanced synth engines (openspec pocket-rack-engines).

Writes <out>/Patches/<ENGINE>/<NAME>.patch. A .patch is the stock protobuf Patch message:
1 engine (InstrumentTypeId), 6 repeated ParamValue {1: fixed32 float} in parameter-ID order,
7 four names, 8 eight Macro {1 name, 2 bipolar, 3 target count, 4 six Target {1 param, 2 fixed32}},
9 the constant Extra block the stock encoder writes.

Each engine's PARAMS list must follow its C++ parameter enum (for MONO: MonoParamId).
Usage: gen_presets.py OUT_DIR
"""
import math
import os
import struct
import sys

# --- protobuf wire helpers ---------------------------------------------------------------


def varint(v):
    out = bytearray()
    while True:
        b = v & 0x7F
        v >>= 7
        out.append(b | (0x80 if v else 0))
        if not v:
            return bytes(out)


def tag(field, wire):
    return varint((field << 3) | wire)


def sub(field, body):
    return tag(field, 2) + varint(len(body)) + body


def fixed32(field, value):
    return tag(field, 5) + struct.pack("<f", value)


def extra_block():
    head = bytes([0x0A, 0x08, 0x12, 0x04, 0x08, 0x03, 0x10, 0x01, 0x1A, 0x00, 0x12, 0x80, 0x0A])
    rows = (bytes([0x12, 0x12, 0x12, 0x10]) + bytes(16)) * 64
    body = head + rows
    assert len(body) == 1293
    return sub(9, body)


def encode(engine_id, values, macros):
    out = tag(1, 0) + varint(engine_id)
    for v in values:
        out += sub(6, fixed32(1, v))
    for _ in range(4):
        out += sub(7, b"")
    for m in macros:
        body = b""
        if m["name"]:
            name = m["name"].encode()
            body += tag(1, 2) + varint(len(name)) + name
        if m.get("bipolar"):
            body += tag(2, 0) + varint(1)
        targets = m.get("targets", [])
        if targets:
            body += tag(3, 0) + varint(len(targets))
        for t in range(6):
            tb = b""
            if t < len(targets):
                param, amount = targets[t]
                if param > 0:
                    tb += tag(1, 0) + varint(param)
                if amount != 0:
                    tb += fixed32(2, amount)
            body += sub(4, tb)
        out += sub(8, body)
    return out + extra_block()


# --- Pocket Rack conversions -------------------------------------------------------------


def pr_time_ms(v):
    """Pocket Rack's envelope time for a 0..127 value: 1 ms to 10 s, exponential."""
    return 10000 ** (v / 127)


def pr_attack(v):
    return pr_time_ms(v) / 1000


def pr_decay(v):
    """Pocket Rack's decay and release reach 99 % in their time, enginekit::Adsr 99.9 %."""
    return pr_time_ms(v) / 1000 * 1.5


# --- MONO (MonoSynthVoice.cpp, InstrumentTypeId::kMonoSynth) ------------------------------

MONO_ID = 10
MONO_PARAMS = [  # (name, default), in MonoParamId order
    ("saw", 0.79), ("pulse", 0.0), ("sub", 0.63), ("noise", 0.0), ("sub_mode", 0), ("pulse_width", 0.0),
    ("cutoff", 0.32), ("resonance", 0.24), ("env_amount", 0.3), ("key_track", 0.5),
    ("lfo_wave", 1), ("lfo_rate", 4.0), ("lfo_pitch", 0.0), ("lfo_filter", 0.0), ("lfo_pw", 0.0),
    ("attack", 0.003), ("decay", 0.4), ("sustain", 0.5), ("release", 0.15),
    ("tune", 0.0), ("fine", 0.0), ("play_mode", 0), ("glide_time", 0.0), ("volume", 1.0),
]
MONO_INDEX = {name: i for i, (name, _) in enumerate(MONO_PARAMS)}
VELOCITY_MACRO = 6  # Patch::SystemMacroId::kVelocity


def mono_macros():
    i = MONO_INDEX
    macros = [
        {"name": "Cutoff", "bipolar": True, "targets": [(i["cutoff"], 1.0)]},
        {"name": "Resonance", "bipolar": True, "targets": [(i["resonance"], 1.0)]},
        {"name": "Env Amount", "bipolar": True, "targets": [(i["env_amount"], 1.0)]},
        {"name": "Pulse Width", "bipolar": True, "targets": [(i["pulse_width"], 1.0)]},
        {"name": "Sub / Noise", "bipolar": True, "targets": [(i["sub"], 0.8), (i["noise"], 0.5)]},
        {"name": "Decay / Release", "bipolar": True, "targets": [(i["decay"], 0.6), (i["release"], 0.6)]},
        {"name": "", "targets": [(i["volume"], 1.0)]},
        {"name": ""},
    ]
    assert len(macros) == 8 and VELOCITY_MACRO == 6
    return macros


# Pocket Rack's SH-101 presets: (name, (SAW, PULS, SUB, NOIS, CUT, RES, PW, SUBM), (A, D, S, R), fenv)
SH101_PRESETS = [
    ("MONO BASS", (110, 0, 100, 0, 36, 30, 0, 0), (0, 60, 40, 20), 35),
    ("SUB THUMP", (0, 40, 127, 0, 28, 20, 0, 50), (0, 50, 20, 15), 40),
    ("PULSE LEAD", (0, 120, 40, 0, 70, 50, 50, 0), (5, 60, 100, 40), 20),
    ("FAT SAW LEAD", (120, 80, 60, 0, 80, 25, 20, 0), (5, 60, 110, 40), 15),
    ("RESO ZAP", (100, 0, 40, 0, 20, 105, 0, 0), (0, 88, 0, 30), 60),
    ("NOISE HIT", (0, 0, 0, 127, 90, 30, 0, 0), (0, 75, 0, 20), 30),
    ("STRING PAD", (90, 90, 0, 0, 55, 15, 30, 0), (70, 60, 127, 90), 10),
    ("HOLLOW PWM", (0, 120, 50, 0, 50, 20, 90, 85), (40, 60, 120, 70), 10),
]


def mono_presets():
    for name, e, env, fenv in SH101_PRESETS:
        v = [d for _, d in MONO_PARAMS]
        i = MONO_INDEX
        v[i["saw"]], v[i["pulse"]], v[i["sub"]], v[i["noise"]] = (x / 127 for x in e[:4])
        v[i["cutoff"]], v[i["resonance"]], v[i["pulse_width"]] = e[4] / 127, e[5] / 127, e[6] / 127
        v[i["sub_mode"]] = 0 if e[7] < 43 else 1 if e[7] < 85 else 2
        v[i["attack"]], v[i["decay"]] = pr_attack(env[0]), pr_decay(env[1])
        v[i["sustain"]], v[i["release"]] = env[2] / 127, pr_decay(env[3])
        # Pocket Rack: ENV -> FILTER fenv moves the cutoff by fenv * 0.0709 octaves at full envelope;
        # MONO's Env Amount 1.0 is 8 octaves.
        v[i["env_amount"]] = max(-1.0, min(1.0, fenv * 0.0709 / 8))
        yield name, MONO_ID, v, mono_macros()


# --- PRISM (PrismSynthVoice.cpp, InstrumentTypeId::kPrismSynth) ----------------------------

PRISM_ID = 9
PRISM_PARAMS = [  # (name, default), in PrismParamId order
    ("structure", 0.5039), ("tilt", 0.551), ("comb", 0.0), ("sweep", 0.0), ("smear", 0.0), ("drift", 0.0),
    ("warp", 0.0), ("freeze", 0.0), ("odd_even", 0.0), ("shift", 0.0), ("scramble", 0.0), ("gate", 0.0),
    ("spectral_decay", 0.0), ("unfold", 0.0), ("phase", 0.0), ("fold", 0.0),
    ("attack", 0.003), ("decay", 0.4), ("sustain", 0.8), ("release", 0.4),
    ("tune", 0.0), ("fine", 0.0), ("play_mode", 0), ("glide_time", 0.0), ("volume", 1.0),
]
PRISM_INDEX = {name: i for i, (name, _) in enumerate(PRISM_PARAMS)}


def prism_macros():
    i = PRISM_INDEX
    return [
        {"name": "Tilt", "bipolar": True, "targets": [(i["tilt"], 1.0)]},
        {"name": "Comb", "bipolar": True, "targets": [(i["comb"], 1.0)]},
        {"name": "Sweep", "bipolar": True, "targets": [(i["sweep"], 1.0)]},
        {"name": "Warp", "bipolar": True, "targets": [(i["warp"], 1.0)]},
        {"name": "Smear / Drift", "bipolar": True, "targets": [(i["smear"], 0.8), (i["drift"], 0.6)]},
        {"name": "Structure", "bipolar": True, "targets": [(i["structure"], 1.0)]},
        {"name": "", "targets": [(i["volume"], 1.0)]},
        {"name": ""},
    ]


# Pocket Rack's PRISM presets: (name, (STRC, TILT, COMB, SWEP, SMER, DRFT, WARP, FRZ), (A, D, S, R),
# (ODD, SHFT, SCRM, GATE, DCAY, UNFD, PHSE, FOLD))
PRISM_PRESETS = [
    ("GLASS CHOIR", (90, 70, 0, 40, 70, 50, 20, 0), (70, 60, 120, 90), None),
    ("ALIEN BELL", (64, 80, 0, 0, 20, 10, 127, 0), (0, 90, 20, 70), None),
    ("BREATH COMB", (64, 60, 70, 0, 90, 30, 0, 0), (60, 40, 110, 80), None),
    ("GHOST PAD", (70, 40, 20, 70, 100, 90, 45, 0), (90, 60, 127, 100), None),
    ("FROZEN HARP", (64, 90, 0, 90, 0, 0, 10, 127), (0, 100, 10, 70), None),
    ("METAL SWEEP", (110, 100, 30, 127, 0, 0, 80, 0), (10, 70, 90, 60), None),
    ("PRISM SAW", (64, 85, 0, 0, 0, 0, 0, 0), (5, 60, 110, 50), None),
    ("LOW DRONE", (40, 60, 50, 20, 80, 70, 0, 0), (80, 60, 127, 100), None),
    ("CRYSTAL KEYS", (64, 78, 0, 0, 25, 8, 0, 0), (0, 70, 20, 60), None),
    ("FORMANT CHOIR", (70, 55, 0, 100, 60, 40, 0, 0), (60, 50, 115, 90), None),
    ("GLASS HARMONICA", (95, 85, 0, 0, 10, 5, 30, 0), (2, 95, 30, 95), None),
    ("GAMELAN", (80, 90, 0, 0, 0, 0, 100, 0), (0, 85, 10, 65), None),
    ("SLOW BLOOM", (64, 35, 25, 30, 100, 40, 0, 0), (110, 70, 127, 110), None),
    ("SPECTRAL LEAD", (64, 90, 0, 60, 0, 5, 0, 0), (3, 70, 100, 40), None),
    ("DEEP BREATH", (50, 45, 90, 0, 70, 20, 0, 0), (50, 50, 100, 70), None),
    ("BELL TOWER", (105, 95, 0, 0, 0, 0, 60, 0), (0, 110, 0, 100), None),
    ("SHIMMER STRINGS", (64, 60, 0, 20, 127, 60, 0, 0), (80, 60, 120, 90), None),
    ("WARP BASS", (58, 55, 0, 0, 0, 0, 25, 0), (0, 60, 100, 30), None),
    ("CHOPPED COMB", (64, 95, 127, 0, 0, 0, 0, 0), (0, 60, 100, 50), None),
    ("FROZEN CHOIR", (75, 60, 0, 30, 60, 30, 15, 127), (70, 60, 127, 110), None),
    ("SCAN ORGAN", (64, 100, 0, 127, 20, 0, 0, 0), (2, 50, 127, 30), None),
    ("DUST PLUCK", (70, 85, 0, 0, 0, 25, 10, 127), (0, 105, 10, 60), None),
    ("ALIEN CHOIR", (90, 60, 0, 50, 80, 70, 90, 0), (90, 60, 120, 100), None),
    ("SPACE WIND", (45, 30, 60, 100, 127, 100, 35, 0), (110, 80, 127, 127), None),
    ("HOLLOW REED", (64, 78, 0, 0, 0, 12, 0, 0), (12, 70, 110, 60), (56, 0, 0, 0, 0, 0, 0, 0)),
    ("RING SHIFT", (64, 72, 0, 0, 8, 6, 0, 0), (0, 95, 25, 75), (0, 13, 0, 0, 0, 0, 0, 0)),
    ("SCRAMBLED PAD", (64, 45, 0, 25, 60, 25, 0, 0), (85, 60, 127, 100), (0, 0, 80, 0, 0, 0, 0, 0)),
    ("SPARSE STARS", (64, 92, 0, 0, 95, 35, 0, 0), (45, 50, 115, 100), (0, 0, 0, 72, 0, 0, 0, 0)),
    ("PLUCKED GLASS", (95, 92, 0, 0, 0, 0, 25, 0), (0, 105, 0, 70), (0, 0, 0, 0, 42, 0, 0, 0)),
    ("BLOOM PAD", (64, 68, 0, 30, 35, 40, 0, 0), (70, 70, 127, 115), (0, 0, 0, 0, -42, 0, 0, 0)),
    ("UNFOLDING", (64, 85, 0, 0, 0, 10, 0, 0), (35, 60, 120, 85), (0, 0, 0, 0, 0, 72, 0, 0)),
    ("BLUR KEYS", (64, 72, 0, 0, 20, 10, 0, 0), (0, 70, 60, 65), (0, 0, 0, 0, 0, 0, 127, 0)),
    ("FOLDED METAL", (80, 96, 0, 0, 0, 0, 55, 0), (0, 90, 40, 70), (0, 0, 0, 0, 0, 0, 0, 85)),
    ("MIRROR BELL", (105, 96, 0, 0, 0, 0, 70, 0), (0, 115, 0, 100), (0, -18, 0, 0, 28, 0, 0, 45)),
    ("SHIFT CHOIR", (90, 60, 0, 50, 70, 60, 20, 0), (80, 60, 120, 100), (22, -6, 0, 0, 0, 0, 50, 0)),
    ("SPECTRAL RAIN", (64, 92, 55, 0, 100, 40, 0, 0), (30, 70, 100, 100), (0, 0, 0, 58, 22, 0, 0, 0)),
    ("SIREN WARP", (110, 82, 0, 100, 0, 5, 60, 0), (4, 70, 100, 50), (0, 22, 0, 0, 0, 45, 0, 0)),
    ("DIGITAL FOG", (70, 45, 0, 40, 100, 70, 30, 0), (90, 60, 127, 110), (-30, 0, 45, 34, 0, 0, 60, 0)),
    ("TIME STRETCH", (64, 58, 0, 0, 75, 12, 0, 0), (110, 70, 127, 120), (0, 0, 0, 0, -58, 85, 0, 0)),
    ("CLUSTER PLUCK", (70, 88, 0, 0, 0, 15, 35, 0), (0, 100, 10, 60), (0, 0, 0, 0, 36, 60, 0, 18)),
]


def prism_presets():
    names = ["structure", "tilt", "comb", "sweep", "smear", "drift", "warp", "freeze"]
    for name, e, env, x in PRISM_PRESETS:
        v = [d for _, d in PRISM_PARAMS]
        i = PRISM_INDEX
        for key, raw in zip(names, e):
            v[i[key]] = raw / 127
        x = x or (0,) * 8
        v[i["odd_even"]] = x[0] / 64
        v[i["shift"]] = x[1] / 32
        v[i["scramble"]], v[i["gate"]] = x[2] / 127, x[3] / 127
        v[i["spectral_decay"]] = x[4] / 64
        v[i["unfold"]], v[i["phase"]], v[i["fold"]] = x[5] / 127, x[6] / 127, x[7] / 127
        v[i["attack"]], v[i["decay"]] = pr_attack(env[0]), pr_decay(env[1])
        v[i["sustain"]], v[i["release"]] = env[2] / 127, pr_decay(env[3])
        v[i["attack"]] = min(v[i["attack"]], 10.0)
        v[i["decay"]], v[i["release"]] = min(v[i["decay"]], 10.0), min(v[i["release"]], 10.0)
        yield name, PRISM_ID, v, prism_macros()


ENGINES = {"MONO": mono_presets, "PRISM": prism_presets}


def file_name(name):
    return "".join(c if c.isalnum() else "_" for c in name.title()).strip("_")


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "build"
    count = 0
    for engine, presets in ENGINES.items():
        folder = os.path.join(out, "Patches", engine)
        os.makedirs(folder, exist_ok=True)
        for name, engine_id, values, macros in presets():
            assert all(math.isfinite(x) for x in values)
            with open(os.path.join(folder, file_name(name) + ".patch"), "wb") as f:
                f.write(encode(engine_id, values, macros))
            count += 1
    print(f"wrote {count} patches to {os.path.join(out, 'Patches')}")


if __name__ == "__main__":
    main()
