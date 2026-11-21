#!/usr/bin/env python3
"""Writes src/Import/import_table.cpp from src/Import/pieces.txt and the
player's exes: each piece's id, exe, SEG:OFF, length and the first 16 hex
digits of its SHA-256. Prints nothing from the exes but lengths and hashes.

    tools/import/make_import_table.py GAME_DIR [BTECH_IMAGE_TOOL]

BTECH.EXE is EXEPACKed: its pieces are hashed in the unpacked image, rebased
into the reference layout and loaded at 0810, which the game's own decoder
writes. BTECH_IMAGE_TOOL is that decoder's command-line form (the build's
btech_image, from tools/import/btech_image.cpp); it is needed only when
pieces.txt names BTECH.EXE.
"""
import hashlib
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
PIECES = os.path.join(ROOT, "src", "Import", "pieces.txt")
OUT = os.path.join(ROOT, "src", "Import", "import_table.cpp")
LOAD_SEGMENT = 0x0800  # where DOS put the image in the traces (the table's addresses)
BTECH_EXE = "BTECH.EXE"
BTECH_LOAD_SEGMENT = 0x0810  # the unpacked image, as src/Import/ImportTable.h


def find(game_dir, name):
    for entry in os.listdir(game_dir):
        if entry.upper() == name.upper():
            return os.path.join(game_dir, entry)
    sys.exit(f"{name} is not in {game_dir}")


def image(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:2] != b"MZ":
        sys.exit(f"{path} is not an MZ exe")
    header_paragraphs = data[8] | data[9] << 8
    return data[header_paragraphs * 16:]


def btech_image(tool, path):
    """BTECH.EXE unpacked by the game's decoder (read from a pipe, never written)."""
    if not tool:
        sys.exit("pieces.txt names BTECH.EXE: give the btech_image tool as the second argument")
    run = subprocess.run([tool, path], capture_output=True)
    if run.returncode != 0:
        sys.exit(run.stderr.decode(errors="replace").strip() or f"{tool} failed")
    print(run.stderr.decode(errors="replace").strip())
    return run.stdout


def main():
    if len(sys.argv) not in (2, 3):
        sys.exit(__doc__)
    game_dir = sys.argv[1]
    tool = sys.argv[2] if len(sys.argv) == 3 else None
    images = {}
    rows = []
    with open(PIECES, encoding="utf-8") as f:
        for number, line in enumerate(f, 1):
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            piece_id, exe, address, length = line.split()
            segment, offset = (int(part, 16) for part in address.split(":"))
            if exe not in images:
                images[exe] = btech_image(tool, find(game_dir, exe)) if exe == BTECH_EXE else image(find(game_dir, exe))
            at = (segment - (BTECH_LOAD_SEGMENT if exe == BTECH_EXE else LOAD_SEGMENT)) * 16 + offset
            data = images[exe]
            if length == "z":
                end = data.find(b"\0", at)
                if end < 0:
                    sys.exit(f"pieces.txt:{number}: {piece_id} has no NUL")
                size = end + 1 - at
            else:
                size = int(length, 0)
            piece = data[at:at + size]
            if len(piece) != size:
                sys.exit(f"pieces.txt:{number}: {piece_id} runs past the end of {exe}")
            digest = hashlib.sha256(piece).hexdigest()[:16]
            kind = "String" if length == "z" else "Bytes"
            rows.append(f'    {{ImportPiece::Kind::{kind}, "{piece_id}", "{exe}", 0x{segment:04X}, 0x{offset:04X}, '
                        f'{size}, "{digest}"}},')
            print(f"{piece_id:24} {exe} {segment:04X}:{offset:04X} {size:5} {digest}")
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write("// Written by tools/import/make_import_table.py from pieces.txt; don't edit.\n"
                "#include \"ImportTable.h\"\n\n"
                "namespace mw1o::import {\n\n"
                "namespace {\n"
                "constexpr ImportPiece kTable[] = {\n")
        f.write("\n".join(rows) + "\n")
        f.write("};\n"
                "}  // namespace\n\n"
                "std::span<const ImportPiece> import_table() { return kTable; }\n\n"
                "}  // namespace mw1o::import\n")
    print(f"{len(rows)} pieces -> {os.path.relpath(OUT, ROOT)}")


if __name__ == "__main__":
    main()
