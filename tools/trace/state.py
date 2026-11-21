#!/usr/bin/env python3
"""Turn MWSNAP's captures of the original (tools/capture/, DOSBox-X) into a
trace in record.py's format, so check.py --traces compares the original with
the golden traces snap by snap.

    state.py --script scripts/campaign_contract.keys --run RUN_DIR --out DIR/campaign_contract.jsonl \\
        [--frames DIR]

RUN_DIR holds MW1S0000.BIN, MW1S0001.BIN, ... in the script's @snap order
(run_wsl.sh's run-NAME/). Each capture holds the screen (mode 0Dh: the
attribute registers, the DAC and the four planes from the CRTC start), the
game's DS and the first 640K of memory, and MWSNAP itself is resident in
that memory: its tag 'MWSN' at seg:103h, with seg from the int 16h vector.

The state is read where the original keeps it, at MW_MAIN's DS (the port
imports the same offsets, src/Original/Memory/MW_MAIN_1090.cpp):
    03E3  the menu's record table (14-byte records, MW_MAIN081B:080E)
    03E5  the selected row
    03F5, 03F7  the random words
    0448  the campaign exchange block, 0x718 bytes; the port's
          MwEncodeCampaignExchange writes the same layout (MW_MAIN_SAVE_FORMAT.cpp)
The original stores no row count, so a menu's rows are counted by its
record table (MENU_ROWS, from the port's imports); an unknown table is
reported, never guessed.

The palette's 17th byte (the overscan register) is not in the capture; it is
written as 0, which is all the port has seen. Frames are MW1 art: --frames
writes them outside the repo, in the recorder's .indices/.palette form.
"""
import argparse, json, os, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import record  # noqa: E402  (state_lines, menu_rows, read_script, sha)

HEADER = 8 + 16 + 768
PLANE = 8000
MEMORY = 640 * 1024
# MWSNAP.COM's resident fields (tools/capture/mwsnap.asm, nasm listing + 100h).
MWSNAP_TAG, MWSNAP_IN_COMBAT, MWSNAP_GAME_DS = 0x103, 0x128, 0x131
EXCHANGE, EXCHANGE_BYTES = 0x0448, 0x0718
MENU_TABLE, SELECTED_ROW, RANDOM0, RANDOM1 = 0x03E3, 0x03E5, 0x03F5, 0x03F7
RECORD_BYTES = 14
# Record table (DS offset) -> rows, as the port imports them (MW_MAIN_1090.cpp);
# each was seen with the same count in every golden snap that shows it.
MENU_ROWS = {
    0x1083: 6, 0x10D7: 3, 0x1101: 6,          # planet, contract and remote hubs
    0x222E: 3, 0x238A: 5, 0x2538: 3,          # company, crew, news
    0x2632: 4, 0x283B: 13, 0x2A1B: 13,        # mech complex, owned units, market units
    0x2D53: 5, 0x2D99: 2, 0x30F0: 2, 0x316D: 2,  # mech and DropShip service, sale, rearm
    0x3558: 22, 0x4268: 13,                   # component repair, reserve ammo
    0x44A9: 3, 0x44D3: 2, 0x480F: 1,          # market mech, DropShip, component
    0x486F: 2, 0x4A15: 6,                     # contract office, contract actions
    0x5809: 31, 0x5A7B: 6, 0x5B8C: 5,         # travel: house planets, houses, map
    0x904C: 3, 0x90EA: 2,                     # bar, recruit interview
    0x9DEB: 7, 0x9F7B: 13,                    # options, saved-game list
}


class Capture:
    def __init__(self, path):
        d = open(path, "rb").read()
        if d[:4] != b"MW1S" or d[4] != 1:
            raise ValueError(f"{path}: not an MWSNAP capture (version 1)")
        self.mode, self.start = d[5], struct.unpack_from("<H", d, 6)[0]
        self.attr, self.dac = d[8:24], d[24:HEADER]
        at = HEADER
        self.planes = None
        if self.mode == 0x0D:
            self.planes = [d[at + p * PLANE: at + (p + 1) * PLANE] for p in range(4)]
            at += 4 * PLANE
        if d[at:at + 4] != b"MEMS":
            raise ValueError(f"{path}: no MEMS block")
        self.ds = struct.unpack_from("<H", d, at + 4)[0]
        self.mem = d[at + 6: at + 6 + MEMORY]
        if len(self.mem) != MEMORY:
            raise ValueError(f"{path}: memory cut short")
        seg = self.word(0, 0x16 * 4 + 2)
        if self.mem[seg * 16 + MWSNAP_TAG: seg * 16 + MWSNAP_TAG + 4] != b"MWSN":
            raise ValueError(f"{path}: MWSNAP not found through the int 16h vector")
        self.mwsnap = seg
        self.combat = self.mem[seg * 16 + MWSNAP_IN_COMBAT] != 0
        self.game_ds = self.word(seg, MWSNAP_GAME_DS)

    def word(self, seg, off):
        return struct.unpack_from("<H", self.mem, seg * 16 + off)[0]

    def bytes(self, seg, off, n):
        return self.mem[seg * 16 + off: seg * 16 + off + n]

    def indices(self):
        out = bytearray(320 * 200)
        for y in range(200):
            for xb in range(40):
                o = y * 40 + xb
                b = [self.planes[p][o] for p in range(4)]
                for bit in range(8):
                    m = 0x80 >> bit
                    out[y * 320 + xb * 8 + bit] = ((b[0] & m and 1) | (b[1] & m and 2)
                                                   | (b[2] & m and 4) | (b[3] & m and 8))
        return bytes(out)

    def palette(self):
        return bytes(self.attr) + b"\0"


def cstring(cap, ds, off, limit=96):
    raw = cap.bytes(ds, off, limit)
    end = raw.find(b"\0")
    return raw[:end if end >= 0 else limit].decode("latin-1")


def campaign_status(cap):
    """The status dict record.state_lines reads, from MW_MAIN's memory."""
    ds = cap.game_ds
    ex = cap.bytes(ds, EXCHANGE, EXCHANGE_BYTES)
    w = lambda o: struct.unpack_from("<H", ex, o)[0]
    c = {"planet": w(0x21), "contractActive": ex[0x20], "missionCategory": ex[0x01], "scenario": ex[0x07],
         "destination": ex[0x06], "targetHouse": w(0x70B), "paymentThousands": w(0x5B2), "result": ex[0x1F],
         "selectedRow": cap.word(ds, SELECTED_ROW), "random": [cap.word(ds, RANDOM0), cap.word(ds, RANDOM1)],
         "funds": w(0x49) | (w(0x4B) << 16), "months": w(0x39), "menu": []}
    problems = []
    if not cap.combat:
        table = cap.word(ds, MENU_TABLE)
        rows = MENU_ROWS.get(table, 0) if table else 0
        if table and not rows:
            problems.append(f"menu table {table:04x}: rows unknown (add it to MENU_ROWS)")
        for i in range(rows):
            r = cap.bytes(ds, table + i * RECORD_BYTES, RECORD_BYTES)
            picture = struct.unpack_from("<H", r, 8)[0]
            c["menu"].append({"index": i, "enabled": r[0], "callback": struct.unpack_from("<H", r, 6)[0],
                              "picture": picture, "links": list(r[10:14]),
                              "label": cstring(cap, ds, picture + 1) if r[1] and picture else ""})
    return {"combat": cap.combat, "campaign": c}, ex, problems


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--script", required=True)
    ap.add_argument("--run", required=True, help="the run's folder of MW1Snnnn.BIN")
    ap.add_argument("--out", required=True)
    ap.add_argument("--frames", help="write .indices/.palette here (outside the repo)")
    a = ap.parse_args()
    tokens, snaps = record.read_script(a.script)
    keys_before, pending = {}, []
    for t in tokens:
        if t.startswith("@snap:"):
            keys_before[t[6:]], pending = pending, []
        else:
            pending.append(t)
    if a.frames:
        os.makedirs(a.frames, exist_ok=True)
    missing = 0
    with open(a.out, "w", encoding="utf-8") as out:
        for seq, name in enumerate(snaps):
            path = os.path.join(a.run, f"MW1S{seq:04d}.BIN")
            if not os.path.exists(path):
                print(f"{seq:3d} {name}: MISSING", flush=True)
                missing += 1
                continue
            cap = Capture(path)
            if cap.planes is None:
                print(f"{seq:3d} {name}: video mode {cap.mode:02x}h, not 0Dh", flush=True)
                missing += 1
                continue
            indices, palette = cap.indices(), cap.palette()
            status, ex, problems = campaign_status(cap)
            lines = record.state_lines(status)
            if not cap.combat:
                lines.append(f"state exchange {record.sha(ex, 32)}")
            menu = record.menu_rows(status)
            rec = {"snap": name, "keys": keys_before[name], "state": lines, "menu": menu,
                   "screenKey": record.sha(json.dumps([(r["callback"], r["picture"], r["links"], r["label"])
                                                      for r in menu]).encode(), 12),
                   "frame": {"indices": record.sha(indices, 32), "palette": record.sha(palette, 32)}}
            out.write(json.dumps(rec) + "\n")
            if a.frames:
                for ext, data in (("indices", indices), ("palette", palette)):
                    with open(os.path.join(a.frames, f"{seq:03d}-{name}.{ext}"), "wb") as f:
                        f.write(data)
            print(f"{seq:3d} {name:<24} {lines[0]} {'combat' if cap.combat else lines[3]}"
                  + "".join(f"  [{p}]" for p in problems), flush=True)
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
