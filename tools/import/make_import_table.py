#!/usr/bin/env python3
"""Writes src/Import/import_table.cpp from src/Import/pieces.txt,
src/Import/word_pieces.txt and the player's exes: each piece's id, exe,
SEG:OFF, length and the first 16 hex digits of its SHA-256. A word's hash is
keyed (HMAC-SHA-256 under the SHA-256 of its exe's key region, as
src/Import/ImportTable.h's key_region), so a short word can't be found from
it. Prints nothing from the exes but lengths and hashes.

    tools/import/make_import_table.py [--rules | --ui] GAME_DIR [BTECH_IMAGE_TOOL]

--rules writes the rule table instead, src/Rules/rule_table.cpp from
src/Rules/rule_pieces.txt (byte counts only; a fifth column "tuned" keeps a
row without its hash). --ui writes the layout table the same way,
src/Ui/ui_table.cpp from src/Ui/ui_pieces.txt.

BTECH.EXE is EXEPACKed: its pieces are hashed in the unpacked image, rebased
into the reference layout and loaded at 0810, which the game's own decoder
writes. BTECH_IMAGE_TOOL is that decoder's command-line form (the build's
btech_image, from tools/import/btech_image.cpp); it is needed only when
pieces.txt names BTECH.EXE.
"""
import hashlib
import hmac
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
PIECES = os.path.join(ROOT, "src", "Import", "pieces.txt")
WORD_PIECES = os.path.join(ROOT, "src", "Import", "word_pieces.txt")
UI_PIECES = os.path.join(ROOT, "src", "Ui", "ui_pieces.txt")
OUT = os.path.join(ROOT, "src", "Import", "import_table.cpp")
RULE_PIECES = os.path.join(ROOT, "src", "Rules", "rule_pieces.txt")
RULE_OUT = os.path.join(ROOT, "src", "Rules", "rule_table.cpp")
# Our own tables (byte counts, "tuned" allowed): flag -> (pieces, output,
# namespace, piece type, table function, header, what).
OWN = {
    "--rules": (RULE_PIECES, RULE_OUT, "rules", "RulePiece", "rule_table", "RuleTable.h", "rule"),
    "--ui": (os.path.join(ROOT, "src", "Ui", "ui_pieces.txt"), os.path.join(ROOT, "src", "Ui", "ui_table.cpp"),
             "ui", "UiPiece", "ui_table", "UiTable.h", "layout"),
}
LOAD_SEGMENT = 0x0800  # where DOS put the image in the traces (the table's addresses)
BTECH_EXE = "BTECH.EXE"
BTECH_LOAD_SEGMENT = 0x0810  # the unpacked image, as src/Import/ImportTable.h
# Each exe's key region (segment, offset, length), as ImportTable.h's key_region.
KEY_REGIONS = {BTECH_EXE: (0x2C53, 0x0010, 0x1400), "MW_MAIN.EXE": (0x081B, 0x0100, 0x2000)}


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


def rows_of(source):
    """(line number, fields) for each row of `source`. In word_pieces.txt a line
    `captions <screen>` stands for a text row per caption of that screen in
    ui_pieces.txt: `<caption id>.text`, at the caption's offset plus its head's
    length (5 bytes, or 1 for a colour caption), NUL-ended. A caption whose text
    is a fixed-width row is listed in word_pieces.txt with its byte count
    instead, and the line skips it, so no caption is listed twice."""
    with open(source, encoding="utf-8") as f:
        lines = [(n, line.split("#", 1)[0].split()) for n, line in enumerate(f, 1)]
    lines = [(n, fields) for n, fields in lines if fields]
    if source != WORD_PIECES:
        return lines
    listed = {fields[0] for _, fields in lines if fields[0] != "captions"}
    out = []
    for number, fields in lines:
        if fields[0] != "captions":
            out.append((number, fields))
            continue
        if len(fields) != 2:
            sys.exit(f"word_pieces.txt:{number}: expected captions and a screen")
        prefix = fields[1] + ".caption."
        derived = 0
        with open(UI_PIECES, encoding="utf-8") as f:
            for ui in f:
                ui = ui.split("#", 1)[0].split()
                if len(ui) != 4 or not ui[0].startswith(prefix):
                    continue
                head = int(ui[3], 0)
                if head not in (1, 5):
                    sys.exit(f"ui_pieces.txt: {ui[0]} is a caption of {head} bytes, not a head of 5 or 1")
                derived += 1
                if ui[0] + ".text" in listed:
                    continue
                segment, offset = (int(part, 16) for part in ui[2].split(":"))
                out.append((number, [ui[0] + ".text", ui[1], f"{segment:04X}:{offset + head:04X}", "z"]))
        if not derived:
            sys.exit(f"word_pieces.txt:{number}: {fields[1]} has no captions in ui_pieces.txt")
    return out


def main():
    args = sys.argv[1:]
    own = OWN.get(args[0]) if args else None
    flag = args[0] if own else None
    rules = own is not None
    if rules:
        args = args[1:]
    if len(args) not in (1, 2):
        sys.exit(__doc__)
    game_dir = args[0]
    tool = args[1] if len(args) == 2 else None
    sources = [own[0]] if rules else [PIECES, WORD_PIECES]
    out = own[1] if rules else OUT
    images = {}
    keys = {}
    rows = []
    for source in sources:
        name = os.path.basename(source)
        word = source == WORD_PIECES
        for number, fields in rows_of(source):
            tuned = rules and len(fields) == 5 and fields[4] == "tuned"
            if len(fields) != (5 if tuned else 4):
                sys.exit(f"{name}:{number}: expected id, exe, SEG:OFF and length")
            piece_id, exe, address, length = fields[:4]
            if rules and length == "z":
                sys.exit(f"{name}:{number}: a {own[6]} piece has a byte count")
            segment, offset = (int(part, 16) for part in address.split(":"))
            if exe not in images:
                images[exe] = btech_image(tool, find(game_dir, exe)) if exe == BTECH_EXE else image(find(game_dir, exe))
            at = (segment - (BTECH_LOAD_SEGMENT if exe == BTECH_EXE else LOAD_SEGMENT)) * 16 + offset
            data = images[exe]
            if length == "z":
                end = data.find(b"\0", at)
                if end < 0:
                    sys.exit(f"{name}:{number}: {piece_id} has no NUL")
                size = end + 1 - at
            else:
                size = int(length, 0)
            piece = data[at:at + size]
            if len(piece) != size:
                sys.exit(f"{name}:{number}: {piece_id} runs past the end of {exe}")
            if word:
                if exe not in keys:
                    k_segment, k_offset, k_length = KEY_REGIONS[exe]
                    k_at = (k_segment - (BTECH_LOAD_SEGMENT if exe == BTECH_EXE else LOAD_SEGMENT)) * 16 + k_offset
                    keys[exe] = hashlib.sha256(data[k_at:k_at + k_length]).digest()
                digest = hmac.new(keys[exe], piece, hashlib.sha256).hexdigest()[:16]
            else:
                digest = hashlib.sha256(piece).hexdigest()[:16]
            if rules:
                kept = "" if tuned else digest
                rows.append(f'    {{"{piece_id}", "{exe}", 0x{segment:04X}, 0x{offset:04X}, {size}, "{kept}"}},')
                print(f"{piece_id:32} {exe} {segment:04X}:{offset:04X} {size:5} {digest}{' (tuned)' if tuned else ''}")
                continue
            kind = ("Word" if length == "z" else "WordBytes") if word else ("String" if length == "z" else "Bytes")
            rows.append(f'    {{ImportPiece::Kind::{kind}, "{piece_id}", "{exe}", 0x{segment:04X}, 0x{offset:04X}, '
                        f'{size}, "{digest}"}},')
            print(f"{piece_id:24} {exe} {segment:04X}:{offset:04X} {size:5} {digest}")
    if rules:
        space, piece_type, function, header, what = own[2:]
        with open(out, "w", encoding="utf-8", newline="\n") as f:
            f.write(f"// Written by tools/import/make_import_table.py {flag} from {name}; don't edit.\n"
                    f"#include \"{header}\"\n\n"
                    f"namespace mw1o::{space} {{\n\n"
                    "namespace {\n"
                    f"constexpr {piece_type} kTable[] = {{\n")
            f.write("\n".join(rows) + "\n")
            f.write("};\n"
                    "}  // namespace\n\n"
                    f"std::span<const {piece_type}> {function}() {{ return kTable; }}\n\n"
                    f"}}  // namespace mw1o::{space}\n")
        print(f"{len(rows)} {what} pieces -> {os.path.relpath(out, ROOT)}")
        return
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write("// Written by tools/import/make_import_table.py from pieces.txt and word_pieces.txt; don't edit.\n"
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
