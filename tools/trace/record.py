#!/usr/bin/env python3
"""Golden-trace recorder (FIXES 130 Phase 0).

Runs the port headless with a poll-fed key script and writes one JSON line
per @snap. The approach follows crescent-hawks-rebuild's reference rig
(tools/capture/snap.asm there): each key goes into the BIOS buffer only when
the game is waiting for one, on a blocking read or on the 4th empty "key
waiting?" poll since the last read, and idle:N lets N more polls pass. The
original draws one random number per wait-loop pass (081B:02FB), so feeding
at counted polls, with the startup clock pinned, makes a run repeat exactly:
two recordings of one script are byte-identical.

Script tokens (whitespace or commas; '#' comments to the end of the line):
    Return Escape Up Down Left Right Space Tab F1 A 1 ...   SDL key names
    idle:N            N more empty polls before the next item
    @snap:NAME        record the screen and state when the game next waits
    random:WWWWWWWW   set the two random words (a test aid for side branches)

A record:
    snap    the snap's name
    keys    the tokens since the previous snap
    state   the compared state, as lines ("state random 1234 5678", ...);
            menu labels only as a 12-hex SHA-256, never MW1's text
    menu    each row's 081B callback offset, picture, links, enabled, label hash
    screenKey  a hash of the menu's shape
    frame   SHA-256 of the indexed 320x200 frame and of its palette
The state ends with "state exchange <sha>", the hash of the whole campaign as
the save format encodes it (0x718 bytes, NAME.exchange beside the frames).

Frames are MW1 art: the recorder copies them to --frames, outside the repo.

    python3 record.py --exe build/mechwarrior --game ~/MECHWAR \\
        --script scripts/campaign_contract.keys --out traces/campaign_contract.jsonl \\
        --frames /tmp/frames
"""
import argparse, hashlib, json, os, re, shutil, signal, subprocess, sys, tempfile, time

POLL_S = 0.05
SNAP_NAME = re.compile(r"^[A-Za-z0-9_-]{1,47}$")


def sha(data, n=16):
    return hashlib.sha256(data).hexdigest()[:n]


def read_script(path):
    """The tokens, checked, and the snap names in order."""
    tokens = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            tokens += line.split("#", 1)[0].replace(",", " ").split()
    snaps, seen = [], set()
    for t in tokens:
        if t.startswith("@snap:"):
            name = t[6:]
            if not SNAP_NAME.match(name) or name in seen:
                sys.exit(f"{path}: bad or repeated snap name {name!r}")
            seen.add(name)
            snaps.append(name)
        elif t.startswith("idle:"):
            if not t[5:].isdigit():
                sys.exit(f"{path}: bad {t!r}")
        elif t.startswith("random:"):
            if not re.fullmatch(r"[0-9A-Fa-f]{8}", t[7:]):
                sys.exit(f"{path}: bad {t!r} (random:WWWWWWWW, hex)")
    if not snaps:
        sys.exit(f"{path}: no @snap")
    return tokens, snaps


def read_json(path, deadline):
    while True:
        try:
            with open(path, encoding="utf-8", errors="replace") as f:
                return json.load(f)
        except (OSError, ValueError):
            if time.monotonic() > deadline:
                return None
            time.sleep(POLL_S)


def label_hash(row):
    label = row.get("label", "")
    return sha(label.encode("latin-1", "replace"), 12) if label else ""


def state_lines(status):
    """The state the rebuild must match, one fact per line."""
    c = status.get("campaign", {})
    r = c.get("random", [0, 0])
    lines = [f"state random {r[0]:04x} {r[1]:04x}",
             "state campaign planet {planet} contract {contractActive} category {missionCategory} scenario {scenario}"
             " destination {destination} house {targetHouse} payment {paymentThousands} result {result}".format(**c),
             f"state funds {c.get('funds')} months {c.get('months')}",
             f"state row {c.get('selectedRow')}",
             f"state combat {1 if status.get('combat') else 0}"]
    for row in c.get("menu", []):
        lines.append("state menu {} enabled {} callback {:04x} picture {} links {} label {}".format(
            row["index"], row["enabled"], row["callback"], row["picture"], " ".join(map(str, row["links"])),
            label_hash(row) or "-"))
    return lines


def menu_rows(status):
    return [{"index": row["index"], "enabled": row["enabled"], "callback": f"0x{row['callback']:04X}",
             "picture": row["picture"], "links": row["links"], "label": label_hash(row)}
            for row in status.get("campaign", {}).get("menu", [])]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", required=True)
    ap.add_argument("--game", required=True, help="MW1 game folder (a copy; it is only read)")
    ap.add_argument("--script", required=True)
    ap.add_argument("--out", required=True, help="trace JSONL to write")
    ap.add_argument("--frames", required=True, help="directory for indexed frames (outside the repo)")
    ap.add_argument("--bios-ticks", default="0x10000", help="the pinned startup clock")
    ap.add_argument("--work", help="control/save/capture dir (default: a fresh temp dir)")
    ap.add_argument("--timeout", type=float, default=300.0)
    args = ap.parse_args()

    tokens, snaps = read_script(args.script)
    work = args.work or tempfile.mkdtemp(prefix="mw1trace-")
    ctl, saves, caps = (os.path.join(work, d) for d in ("ctl", "saves", "caps"))
    for d in (ctl, saves, caps):
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d, mode=0o700)
    os.makedirs(args.frames, exist_ok=True)

    env = dict(os.environ, SDL_VIDEO_DRIVER="offscreen", SDL_AUDIO_DRIVER="dummy",
               MECHWARRIOR_SAVE_DIRECTORY=saves, MECHWARRIOR_DEBUG_CONTROL_DIRECTORY=ctl,
               MECHWARRIOR_DEBUG_BIOS_TICKS=args.bios_ticks)
    # The feed is in place before the game starts, so it counts every poll.
    with open(os.path.join(ctl, "feed.txt"), "w") as f:
        f.write(" ".join(tokens) + "\n")
    exe = os.path.abspath(args.exe)
    log = open(os.path.join(work, "run.log"), "wb")
    game = subprocess.Popen([exe, os.path.abspath(args.game), caps], cwd=os.path.dirname(exe),
                            env=env, stdout=log, stderr=subprocess.STDOUT)
    status_path = os.path.join(ctl, "status.json")
    try:
        if read_json(status_path, time.monotonic() + 30) is None:
            sys.exit("no status.json: is this a build with debug control?")
        deadline = time.monotonic() + args.timeout
        last = os.path.join(caps, snaps[-1] + ".state.json")
        while True:
            st = read_json(status_path, deadline) or {}
            feed = st.get("feed", {})
            if feed.get("error"):
                sys.exit(f"feed: {feed['error']} (at item {feed.get('next')})")
            if os.path.exists(last) and read_json(last, time.monotonic() + 5) is not None:
                break
            if game.poll() is not None:
                sys.exit(f"the game exited (code {game.returncode}) at feed item {feed.get('next')}")
            if time.monotonic() > deadline:
                sys.exit(f"timed out at feed item {feed.get('next')} of {feed.get('items')}")
            time.sleep(POLL_S)
    finally:
        if game.poll() is None:
            game.send_signal(signal.SIGTERM)
            try:
                game.wait(5)
            except subprocess.TimeoutExpired:
                game.kill()
        log.close()

    keys_before, pending = {}, []
    for t in tokens:
        if t.startswith("@snap:"):
            keys_before[t[6:]] = pending
            pending = []
        else:
            pending.append(t)
    failures = 0
    with open(args.out, "w", encoding="utf-8") as out:
        for seq, name in enumerate(snaps):
            base = os.path.join(caps, name)
            st = read_json(base + ".state.json", time.monotonic() + 1)
            frame = {}
            for ext in ("indices", "palette", "exchange"):
                try:
                    with open(f"{base}.{ext}", "rb") as f:
                        frame[ext] = sha(f.read(), 32)
                    shutil.copyfile(f"{base}.{ext}", os.path.join(args.frames, f"{seq:03d}-{name}.{ext}"))
                except OSError:
                    frame[ext] = None
            exchange = frame.pop("exchange", None)
            if st is None or not frame.get("indices"):
                failures += 1
                print(f"{seq:3d} {name}: MISSING", flush=True)
                continue
            menu = menu_rows(st)
            lines = state_lines(st) + ([f"state exchange {exchange}"] if exchange else [])
            record = {"snap": name, "keys": keys_before[name], "state": lines, "menu": menu,
                      "screenKey": sha(json.dumps([(r["callback"], r["picture"], r["links"], r["label"])
                                                   for r in menu]).encode(), 12),
                      "frame": frame}
            out.write(json.dumps(record) + "\n")
            print(f"{seq:3d} {name:<24} screen {record['screenKey']} {record['state'][3]} rows {len(menu)}"
                  f" {record['state'][0]} frame {frame['indices'][:12]}", flush=True)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
