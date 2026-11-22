#!/usr/bin/env python3
"""Turn MWSNAP's captures of the original (tools/capture/, DOSBox-X) into a
trace in record.py's format, so check.py --traces compares the original with
the golden traces snap by snap.

    state.py --script scripts/campaign_contract.keys --run RUN_DIR --game GAME_DIR \\
        --out DIR/campaign_contract.jsonl [--frames DIR]

GAME_DIR is the game copy the run used: its MW_MAIN.EXE keys the label hashes
(record.label_key).

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

The palette's 17th byte is the overscan colour (attribute register 11h),
which BTECH sets during the briefing (2A36 BtechEgaSetOverscanColour). A
version 2 capture holds it; a version 1 capture does not, and reads as 0. Frames are MW1 art: --frames
writes them outside the repo, in the recorder's .indices/.palette form.
"""
import argparse, json, os, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import record  # noqa: E402  (state_lines, menu_rows, read_script, sha)

PLANE = 8000
MEMORY = 640 * 1024
# MWSNAP.COM's resident fields (tools/capture/mwsnap.asm, nasm listing + 100h).
MWSNAP_TAG, MWSNAP_IN_COMBAT, MWSNAP_GAME_DS = 0x103, 0x128, 0x131
EXCHANGE, EXCHANGE_BYTES = 0x0448, 0x0718
MENU_TABLE, SELECTED_ROW, RANDOM0, RANDOM1 = 0x03E3, 0x03E5, 0x03F5, 0x03F7
RECORD_BYTES = 14
MW_EXCHANGE = 0x0164   # MW.EXE's copy of the exchange block, from its PSP
# BTECH's DS (the 1990-01-22 revision; records are packed, the port's
# combat_mechs.h / combat_world.h field order).
TICK, SCREEN, TARGET = 0xD244, 0x0E1C, 0xCD36        # 0810 key dispatch: C sets [0E1C]=2,
OBJECTIVE, OBJECTIVE_DAMAGE = 0xCD2E, 0xCD30         # Enter cycles [CD36]; 1D24 sets [CD30]=28h
BINDINGS, BINDING_BYTES = 0xCD5A, 0x0E               # mech, torso, right leg, left leg, ...
CONTROLS, CONTROL_BYTES = 0xD374, 0x55               # side +10h, movement mode +51h
SLOTS = 8
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
        if d[:4] != b"MW1S" or d[4] not in (1, 2):
            raise ValueError(f"{path}: not an MWSNAP capture (version 1 or 2)")
        self.mode, self.start = d[5], struct.unpack_from("<H", d, 6)[0]
        # Version 2 adds the overscan colour (attribute register 11h) at 24.
        at = 24 if d[4] == 1 else 25
        self.attr, self.overscan = d[8:24], (d[24] if d[4] == 2 else 0)
        self.dac = d[at:at + 768]
        at += 768
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
        self.mw_psp = self.program_psp(b"MW")

    def program_psp(self, name):
        """The PSP of the program DOS's memory chain names NAME (its block's owner)."""
        for start in range(0x40, 0x400):
            seg = start
            while seg < 0xA000 and self.mem[seg * 16] in (0x4D, 0x5A):
                owner, size = struct.unpack_from("<HH", self.mem, seg * 16 + 1)
                if owner == seg + 1 and self.mem[seg * 16 + 8: seg * 16 + 16].split(b"\0")[0] == name:
                    return owner
                if self.mem[seg * 16] == 0x5A:
                    break
                seg += size + 1
        return None

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
        return bytes(self.attr) + bytes([self.overscan])


def cstring(cap, ds, off, limit=96):
    raw = cap.bytes(ds, off, limit)
    end = raw.find(b"\0")
    return raw[:end if end >= 0 else limit].decode("latin-1")


def combat_status(cap):
    """tick, screen, target, units and objective, as the port's status.json has them."""
    ds = cap.ds
    w = lambda o: cap.word(ds, o)
    sw = lambda o: struct.unpack_from("<h", cap.mem, ds * 16 + o)[0]
    sd = lambda o: struct.unpack_from("<i", cap.mem, ds * 16 + o)[0]
    words = lambda o, n: [w(o + 2 * i) for i in range(n)]
    position = lambda obj: [sd(obj + 8), sd(obj + 0x0C), sd(obj + 0x10)]
    orientation = lambda obj: words(obj + 0x14, 3)
    units = []
    for slot in range(SLOTS):
        b = BINDINGS + slot * BINDING_BYTES
        mech, torso, body = w(b), w(b + 2), w(b + 6)
        if not mech or not body:
            continue
        control = CONTROLS + slot * CONTROL_BYTES
        units.append({"slot": slot, "side": w(control + 0x10), "chassis": w(mech),
                      "position": position(body), "orientation": orientation(body),
                      "step": sw(body + 0x3C), "movementMode": w(control + 0x51), "jumpCharge": w(mech + 0x09),
                      "torsoOrientation": orientation(torso) if torso else [0, 0, 0],
                      "heat": w(mech + 0x06), "engineHits": w(mech + 0x11), "gyroHits": w(mech + 0x13),
                      "cockpitDisabled": w(mech + 0x15), "armor": words(mech + 0x19, 9),
                      "internal": words(mech + 0x2B, 8), "ammo": words(mech + 0x59, 6),
                      "weapons": [{"operational": cap.mem[ds * 16 + mech + 0x3B + 3 * i],
                                   "reload": w(mech + 0x3C + 3 * i)} for i in range(10)]})
    obj = w(OBJECTIVE)
    return {"tick": w(TICK), "screen": w(SCREEN), "target": w(TARGET), "units": units,
            "objective": {"position": position(obj), "damageRemaining": w(OBJECTIVE_DAMAGE)} if obj else None}


def campaign_status(cap, frozen):
    """The status dict record.state_lines reads, from MW_MAIN's memory. In
    combat MW_MAIN is gone: the campaign is the exchange block MW.EXE keeps
    for BTECH (MW_EXCHANGE past its PSP), and the random words and the row are
    those MW_MAIN left (FROZEN, from the last campaign capture), as the port
    holds them while a battle runs."""
    ds = cap.game_ds
    ex = cap.bytes(cap.mw_psp, MW_EXCHANGE, EXCHANGE_BYTES) if cap.combat else cap.bytes(ds, EXCHANGE, EXCHANGE_BYTES)
    w = lambda o: struct.unpack_from("<H", ex, o)[0]
    c = {"planet": w(0x21), "contractActive": ex[0x20], "missionCategory": ex[0x01], "scenario": ex[0x07],
         "destination": ex[0x06], "targetHouse": w(0x70B), "paymentThousands": w(0x5B2), "result": ex[0x1F],
         "selectedRow": frozen["row"] if cap.combat else cap.word(ds, SELECTED_ROW),
         "random": frozen["random"] if cap.combat else [cap.word(ds, RANDOM0), cap.word(ds, RANDOM1)],
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
    status = {"combat": cap.combat, "campaign": c}
    if cap.combat:
        status.update(combat_status(cap))
    return status, ex, problems


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--script", required=True)
    ap.add_argument("--run", required=True, help="the run's folder of MW1Snnnn.BIN")
    ap.add_argument("--out", required=True)
    ap.add_argument("--frames", help="write .indices/.palette here (outside the repo)")
    ap.add_argument("--game", required=True, help="the game copy; its MW_MAIN.EXE keys the label hashes")
    a = ap.parse_args()
    tokens, snaps = record.read_script(a.script)
    key = record.label_key(a.game)
    keys_before, pending = {}, []
    for t in tokens:
        if t.startswith("@snap:"):
            keys_before[t[6:]], pending = pending, []
        else:
            pending.append(t)
    if a.frames:
        os.makedirs(a.frames, exist_ok=True)
    missing = 0
    frozen = {"row": 0, "random": [0, 0]}
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
            status, ex, problems = campaign_status(cap, frozen)
            if not cap.combat:
                frozen = {"row": status["campaign"]["selectedRow"], "random": status["campaign"]["random"]}
            lines = record.state_lines(status, key)
            if not cap.combat:
                lines.append(f"state exchange {record.sha(ex, 32)}")
            menu = record.menu_rows(status, key)
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
