#!/usr/bin/env python3
"""Turns a feed script (the port's feed.txt syntax, docs/DEBUG_INPUT.md; the
tools/trace/scripts/*.keys files) into KEYS.BIN for MWSNAP.COM, so the same
script drives the port and the original under DOSBox-X.

    keys.py SCRIPT KEYS.BIN [--names NAMES.TXT]

Key names are SDL's, as the port reads them (case ignored, '_' for a space),
mapped to the same BIOS words the port's feed produces (no modifiers: the
scan code in the high byte, the unshifted character in the low byte).
`wait:MS` (MWSNAP only; the port's feed doesn't read it) holds the next item
until that much emulated time has passed, in BIOS ticks.
MWSNAP numbers its captures MW1S0000.BIN, MW1S0001.BIN, ...; --names writes
the @snap names in that order, one per line.
"""
import argparse
import re
import struct
import sys

SNAP, IDLE, MAKE, BREAK, TICKS, RANDOM, WAIT = 0xFFFF, 0xFFFE, 0xFFFB, 0xFFFA, 0xFFF9, 0xFFF8, 0xFFF7
MAX_WORDS = 16384

# SDL key name -> (XT scan code, character), the port's ScanCode()/BiosKey().
KEYS = {
    "escape": (0x01, 27), "return": (0x1C, 13), "keypad enter": (0x1C, 13),
    "space": (0x39, 32), "tab": (0x0F, 9), "backspace": (0x0E, 8),
    "left ctrl": (0x1D, 0), "right ctrl": (0x1D, 0),
    "left shift": (0x2A, 0), "right shift": (0x36, 0),
    "left alt": (0x38, 0), "right alt": (0x38, 0),
    "capslock": (0x3A, 0), "numlock": (0x45, 0),
    "-": (0x0C, ord("-")), "=": (0x0D, ord("=")),
    "keypad -": (0x4A, 0), "keypad +": (0x4E, 0),
    ",": (0x33, ord(",")), ".": (0x34, ord(".")), "/": (0x35, ord("/")),
    "up": (0x48, 0), "down": (0x50, 0), "left": (0x4B, 0), "right": (0x4D, 0),
    "home": (0x47, 0), "end": (0x4F, 0), "pageup": (0x49, 0), "pagedown": (0x51, 0),
    "keypad 7": (0x47, 0), "keypad 8": (0x48, 0), "keypad 9": (0x49, 0),
    "keypad 4": (0x4B, 0), "keypad 5": (0x4C, 0), "keypad 6": (0x4D, 0),
    "keypad 1": (0x4F, 0), "keypad 2": (0x50, 0), "keypad 3": (0x51, 0),
}
for i, ch in enumerate("qwertyuiop"):
    KEYS[ch] = (0x10 + i, ord(ch))
for i, ch in enumerate("asdfghjkl"):
    KEYS[ch] = (0x1E + i, ord(ch))
for i, ch in enumerate("zxcvbnm"):
    KEYS[ch] = (0x2C + i, ord(ch))
for i, ch in enumerate("1234567890"):
    KEYS[ch] = (0x02 + i, ord(ch))
for i in range(10):
    KEYS[f"f{i + 1}"] = (0x3B + i, 0)
# SDL's other spellings of the same keys.
for alias, name in (("enter", "return"), ("page up", "pageup"), ("page down", "pagedown"),
                    ("caps lock", "capslock"), ("numlockclear", "numlock"),
                    ("lctrl", "left ctrl"), ("rctrl", "right ctrl"),
                    ("lshift", "left shift"), ("rshift", "right shift"),
                    ("lalt", "left alt"), ("ralt", "right alt"),
                    ("minus", "-"), ("equals", "="), ("comma", ","), ("period", "."),
                    ("slash", "/"), ("keypad minus", "keypad -"), ("keypad plus", "keypad +")):
    KEYS[alias] = KEYS[name]

SNAP_NAME = re.compile(r"[A-Za-z0-9_.-]{1,48}")


def key(name, where):
    found = KEYS.get(name.replace("_", " ").lower())
    if not found:
        sys.exit(f"{where}: unknown key {name!r}")
    return found


def compile_script(path):
    words, names = [], []
    with open(path, encoding="utf-8") as f:
        for n, line in enumerate(f, 1):
            for t in line.split("#", 1)[0].replace(",", " ").split():
                where = f"{path}:{n}"
                if t.startswith("@snap:"):
                    if not SNAP_NAME.fullmatch(t[6:]) or t[6:] in names:
                        sys.exit(f"{where}: bad or repeated snap name {t[6:]!r}")
                    names.append(t[6:])
                    words.append(SNAP)
                elif t.startswith(("idle:", "ticks:")):
                    kind, _, count = t.partition(":")
                    if not count.isdigit() or int(count) > 0xFFFF:
                        sys.exit(f"{where}: bad {t!r}")
                    words += [IDLE if kind == "idle" else TICKS, int(count)]
                elif t.startswith("wait:"):
                    # MWSNAP's own (the port's feed has none): milliseconds of
                    # emulated time, as BIOS ticks, rounded up.
                    if not t[5:].isdigit():
                        sys.exit(f"{where}: bad {t!r} (wait:MILLISECONDS)")
                    ticks = -(-int(t[5:]) * 1193182 // (65536 * 1000))
                    if ticks > 0xFFFF:
                        sys.exit(f"{where}: {t!r} is too long")
                    words += [WAIT, ticks]
                elif t.startswith("random:"):
                    if not re.fullmatch(r"[0-9A-Fa-f]{8}", t[7:]):
                        sys.exit(f"{where}: bad {t!r} (random:WWWWWWWW, hex)")
                    v = int(t[7:], 16)
                    words += [RANDOM, v >> 16, v & 0xFFFF]
                elif t.startswith(("make:", "break:")):
                    kind, _, name = t.partition(":")
                    words += [MAKE if kind == "make" else BREAK, key(name, where)[0]]
                else:
                    scan, ch = key(t, where)
                    words.append(scan << 8 | ch)
    if len(words) > MAX_WORDS:
        sys.exit(f"{path}: {len(words)} words; MWSNAP holds {MAX_WORDS}")
    return words, names


def main():
    p = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    p.add_argument("script")
    p.add_argument("out")
    p.add_argument("--names")
    a = p.parse_args()
    words, names = compile_script(a.script)
    with open(a.out, "wb") as f:
        f.write(struct.pack(f"<{len(words) + 1}H", len(words), *words))
    if a.names:
        with open(a.names, "w", encoding="utf-8") as f:
            f.writelines(n + "\n" for n in names)
    print(f"{a.out}: {len(words)} words, {len(names)} snaps")


if __name__ == "__main__":
    main()
