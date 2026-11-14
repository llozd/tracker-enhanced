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


ENGINES = {"MONO": mono_presets}


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
