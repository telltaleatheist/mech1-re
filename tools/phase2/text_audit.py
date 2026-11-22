#!/usr/bin/env python3
"""Checks that no text from MW1's exes is in the repo (docs/phase2/PLAN.md 7.4).

    tools/phase2/text_audit.py BTECH_IMAGE_TOOL [GAME_DIR [BTECH.EXE ...]]

Without GAME_DIR it takes the tests' variables (ctest runs it so):
MW1O_GAME for the game folder, and MW1O_BTECH_REFERENCE and MW1O_BTECH_1990 for
the further BTECH.EXEs. Reads the data segments that the piece tables name (src/Import/pieces.txt,
src/Rules/rule_pieces.txt, src/Ui/ui_pieces.txt) from the player's exes:
MW_MAIN.EXE from GAME_DIR, and BTECH.EXE from GAME_DIR and from each further
BTECH.EXE given (another revision), unpacked by the game's own decoder
(BTECH_IMAGE_TOOL, the build's btech_image). Every run of printable characters
there long enough to be a word is searched for in the tracked tree (git
ls-files), case folded.

A hit names the file, the line, the exe and the address of the string and its
length, never the string. Exits 0 when the tree is clean, 1 on a hit, and 77
(ctest's skip) when there is no game folder or it holds no MW_MAIN.EXE: without
the player's exes there is nothing to compare.

What counts as wording: a line of an exe string (split at NULs and line
breaks) with at least MIN_WORDS words, or a phrase of at least PHRASE_LENGTH
characters, matched as whole words. Single words and two-word names (the
'Mechs, places, buttons), which our identifiers and prose share by meaning,
match ordinary text too often to search for; the keyed hash (PLAN.md 7.4)
keeps those out of the piece tables. Lines that are file names or printf
formats are skipped, since the code names the install's files and has its
own formats (TEXT_AUDIT_SKIP below).

tools/phase2/text_audit_allow.txt lists the hits reviewed and kept: a path
(or * for any), the exe, the string's SEG:OFF and the reason, in our words.
A listed hit is counted but doesn't fail; a listed entry that no longer hits
is reported, so the list shrinks as pieces move.
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
TABLES = [os.path.join(ROOT, *p) for p in (("src", "Import", "pieces.txt"), ("src", "Rules", "rule_pieces.txt"),
                                           ("src", "Ui", "ui_pieces.txt"))]
LOAD_SEGMENT = {"MW_MAIN.EXE": 0x0800, "BTECH.EXE": 0x0810}  # as make_import_table.py
SEGMENT_BYTES = 0x10000
MIN_WORDS = 3
PHRASE_LENGTH = 20
ALLOW = os.path.join(HERE, "text_audit_allow.txt")
SKIP = 77
# A file name (the install's files, which the code names) or a printf format.
TEXT_AUDIT_SKIP = re.compile(r"^[A-Za-z0-9_]{1,8}\.[A-Za-z0-9]{1,3}$|%[-0-9.]*[dsxXuc]")


def find(game_dir, name):
    for entry in os.listdir(game_dir):
        if entry.upper() == name:
            return os.path.join(game_dir, entry)
    return None


def mz_image(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:2] != b"MZ":
        sys.exit(f"text_audit: {path} is not an MZ exe")
    return data[(data[8] | data[9] << 8) * 16:]


def btech_image(tool, path):
    run = subprocess.run([tool, path], capture_output=True)
    if run.returncode != 0:
        sys.exit(f"text_audit: {tool} could not unpack {path}")
    return run.stdout


def segments():
    """The (exe, segment) pairs the piece tables read from."""
    found = set()
    for table in TABLES:
        with open(table, encoding="utf-8") as f:
            for line in f:
                fields = line.split("#", 1)[0].split()
                if len(fields) >= 3 and fields[1] in LOAD_SEGMENT:
                    found.add((fields[1], int(fields[2].split(":")[0], 16)))
    return sorted(found)


def is_word(text):
    """Long enough to be MW1's wording rather than a name our text shares."""
    count = len(re.findall(r"[A-Za-z]{2,}", text))
    return count >= MIN_WORDS or (count >= 2 and len(text) >= PHRASE_LENGTH)


def allowed():
    """{(path or *, exe, SEG:OFF)} from text_audit_allow.txt."""
    out = set()
    with open(ALLOW, encoding="utf-8") as f:
        for number, line in enumerate(f, 1):
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            fields = line.split(None, 3)
            if len(fields) != 4:
                sys.exit(f"text_audit_allow.txt:{number}: expected a path, the exe, SEG:OFF and the reason")
            out.add(tuple(fields[:3]))
    return out


def words(images):
    """{folded word: (exe, SEG:OFF, length)} from every named data segment."""
    out = {}
    for exe, label, data in images:
        for _, segment in [s for s in segments() if s[0] == exe]:
            base = (segment - LOAD_SEGMENT[exe]) * 16
            region = data[base:base + SEGMENT_BYTES]
            for m in re.finditer(rb"[\x20-\x7e]+", region):  # a line: NULs and CR/LF end it
                text = m.group().decode("ascii").strip()
                if not is_word(text):
                    continue
                if TEXT_AUDIT_SKIP.search(text):
                    continue
                out.setdefault(text.casefold(), (label, f"{segment:04X}:{m.start():04X}", len(text)))
    return out


def main():
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    tool = args[0]
    if len(args) > 1:
        game_dir, more = args[1], args[2:]
    else:
        game_dir = os.environ.get("MW1O_GAME", "")
        more = [os.environ[v] for v in ("MW1O_BTECH_REFERENCE", "MW1O_BTECH_1990") if os.environ.get(v)]
    if not game_dir or not os.path.isdir(game_dir) or not find(game_dir, "MW_MAIN.EXE"):
        print("text_audit: no game folder with MW_MAIN.EXE (MW1O_GAME); skipped")
        return SKIP
    images = [("MW_MAIN.EXE", "MW_MAIN.EXE", mz_image(find(game_dir, "MW_MAIN.EXE")))]
    btechs = [p for p in [find(game_dir, "BTECH.EXE")] + more if p]
    for number, path in enumerate(btechs):
        images.append(("BTECH.EXE", "BTECH.EXE" if number == 0 else f"BTECH.EXE ({number + 1})", btech_image(tool, path)))
    table = words(images)
    # Whole words only: a string inside a longer word or identifier is not it.
    pattern = re.compile(r"(?<![a-z0-9_])(?:" + "|".join(re.escape(w) for w in sorted(table, key=len, reverse=True)) +
                         r")(?![a-z0-9_])")
    files = subprocess.run(["git", "-C", ROOT, "ls-files", "-z"], capture_output=True, check=True).stdout.split(b"\0")
    allow = allowed()
    used = set()
    hits = kept = 0
    for name in (f.decode() for f in files if f):
        path = os.path.join(ROOT, name)
        try:
            with open(path, "rb") as f:
                text = f.read().decode("latin-1").casefold()
        except OSError:
            continue
        for m in pattern.finditer(text):
            exe, address, length = table[m.group()]
            line = text.count("\n", 0, m.start()) + 1
            exe_name = exe.split()[0]
            entry = next((e for e in ((name, exe_name, address), ("*", exe_name, address)) if e in allow), None)
            if entry:
                used.add(entry)
                kept += 1
                continue
            print(f"text_audit: {name}:{line}: {exe} {address} ({length} characters)")
            hits += 1
    for entry in sorted(allow - used):
        print(f"text_audit: text_audit_allow.txt: {' '.join(entry)} no longer hits; remove it")
    print(f"text_audit: {len(table)} strings from {len(images)} exes searched in {len([f for f in files if f])} files: "
          f"{hits} found, {kept} allowed")
    return 1 if hits or allow - used else 0


if __name__ == "__main__":
    sys.exit(main())
