#!/usr/bin/env python3
"""Golden-trace recorder (FIXES 130 Phase 0).

Drives the port headless through its debug-control directory: scripted keys go
in, and after every command it writes one JSON line holding the menu (each
row's original handler offset, links, picture, enabled), the campaign fields,
the random words and the indexed 320x200 frame's hash.

MW1's own text never enters a trace: a menu label is kept only as a short
SHA-256 so screens can be told apart. Frames are MW1 art, so the indexed bytes
and palettes are copied into --frames, which must live outside the repo.

Script format (one command per line, '#' starts a comment):
    <name> | <ms> <KEY> [KEY...] ; <ms> <KEY> ...
Keys are SDL key names ("Return", "Escape", "Up", "Down", "F1", "A"...); '_'
stands for a space inside a name, '-' holds no key (a wait).

Example:
    python3 record.py --exe out/linux-x64/mechwarrior --game ~/MECHWAR \\
        --script traces/campaign_contract.keys --out run.jsonl \\
        --frames /tmp/frames --bios-ticks 0x10000
"""
import argparse, hashlib, json, os, shutil, signal, subprocess, sys, tempfile, time

POLL_S = 0.05


def sha(data, n=16):
    return hashlib.sha256(data).hexdigest()[:n]


def parse_script(path):
    commands = []
    with open(path, encoding="utf-8") as f:
        for number, raw in enumerate(f, 1):
            line = raw.split("#", 1)[0].strip()
            if not line:
                continue
            if "|" not in line:
                sys.exit(f"{path}:{number}: expected '<name> | <ms> <keys>'")
            name, body = (part.strip() for part in line.split("|", 1))
            steps = []
            for chunk in body.split(";"):
                words = chunk.split()
                if not words:
                    continue
                ms = int(words[0])
                if not 25 <= ms <= 30000:
                    sys.exit(f"{path}:{number}: step time {ms} outside 25..30000 ms")
                keys = [] if words[1:] == ["-"] else words[1:]
                if len(keys) > 4:
                    sys.exit(f"{path}:{number}: at most four keys per step")
                steps.append((ms, keys))
            if not steps or len(steps) > 30 or sum(ms for ms, _ in steps) > 59000:
                sys.exit(f"{path}:{number}: 1..30 steps and under 59 s per command")
            commands.append((name, steps))
    return commands


def read_status(path, deadline):
    while True:
        try:
            with open(path, encoding="utf-8", errors="replace") as f:
                return json.load(f)
        except (OSError, ValueError):
            if time.monotonic() > deadline:
                return None
            time.sleep(POLL_S)


def clean_menu(menu):
    rows = []
    for row in menu:
        label = row.get("label", "")
        rows.append({
            "index": row["index"],
            "enabled": row["enabled"],
            "callback": f"0x{row['callback']:04X}",
            "picture": row["picture"],
            "links": row["links"],
            "label": sha(label.encode("latin-1", "replace"), 12) if label else "",
        })
    return rows


def snapshot(status):
    """Everything the trace keeps from one status.json, with labels hashed."""
    campaign = dict(status.get("campaign", {}))
    menu = clean_menu(campaign.pop("menu", []))
    record = {
        "combat": status.get("combat"),
        "campaign": campaign,
        "menu": menu,
        "screenKey": sha(json.dumps([(r["callback"], r["picture"], r["links"], r["label"]) for r in menu]).encode(), 12),
    }
    if status.get("combat"):
        for key in ("tick", "screen", "target", "aimY", "requestedStep", "objectiveRules", "units", "objective"):
            if key in status:
                record[key] = status[key]
    return record


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", required=True)
    ap.add_argument("--game", required=True, help="MW1 game folder (a copy; it is only read)")
    ap.add_argument("--script", required=True)
    ap.add_argument("--out", required=True, help="trace JSONL to write")
    ap.add_argument("--frames", required=True, help="directory for indexed frames (outside the repo)")
    ap.add_argument("--bios-ticks", default="0x10000", help="pinned BIOS clock for the RNG mix; '' = wall clock")
    ap.add_argument("--work", help="control/save/capture dir (default: a fresh temp dir)")
    ap.add_argument("--ready-s", type=float, default=30.0, help="longest wait for the game to poll for a key")
    args = ap.parse_args()

    commands = parse_script(args.script)
    work = args.work or tempfile.mkdtemp(prefix="mw1trace-")
    ctl, saves, caps = (os.path.join(work, d) for d in ("ctl", "saves", "caps"))
    for d in (ctl, saves, caps):
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d, mode=0o700)
    os.makedirs(args.frames, exist_ok=True)

    env = dict(os.environ, SDL_VIDEO_DRIVER="offscreen", SDL_AUDIO_DRIVER="dummy",
               MECHWARRIOR_SAVE_DIRECTORY=saves, MECHWARRIOR_DEBUG_CONTROL_DIRECTORY=ctl)
    if args.bios_ticks:
        env["MECHWARRIOR_DEBUG_BIOS_TICKS"] = args.bios_ticks
    exe = os.path.abspath(args.exe)
    log = open(os.path.join(work, "run.log"), "wb")
    game = subprocess.Popen([exe, os.path.abspath(args.game), caps], cwd=os.path.dirname(exe),
                            env=env, stdout=log, stderr=subprocess.STDOUT)
    status_path = os.path.join(ctl, "status.json")
    state = {"next_id": 1}

    def send(steps, label):
        """One debug command; returns the status once it has completed."""
        cid = state["next_id"]
        lines = [f"{cid} {len(steps)}"] + [f"{ms} {' '.join(keys)}".rstrip() for ms, keys in steps]
        tmp = os.path.join(ctl, "command.txt.tmp")
        with open(tmp, "w") as f:
            f.write("\n".join(lines) + "\n")
        os.replace(tmp, os.path.join(ctl, "command.txt"))
        deadline = time.monotonic() + sum(ms for ms, _ in steps) / 1000 + 15
        while True:
            st = read_status(status_path, deadline)
            if st and int(st.get("completedId", 0)) >= cid:
                state["next_id"] = cid + 1
                return cid, st
            if st and st.get("error") and int(st.get("activeId", 0)) < cid:
                sys.exit(f"'{label}' rejected: {st['error']}")
            if time.monotonic() > deadline or game.poll() is not None:
                sys.exit(f"'{label}' did not complete (game {'exited' if game.poll() is not None else 'busy'})")
            time.sleep(POLL_S)

    def wait_ready(label):
        """Until the game is polling for a key: polls rising across two status
        publishes, with no command running."""
        deadline = time.monotonic() + args.ready_s
        last, rising = None, 0
        while time.monotonic() < deadline:
            if game.poll() is not None:
                sys.exit(f"the game exited (code {game.returncode}) at '{label}'")
            st = read_status(status_path, deadline)
            if st and not st.get("busy"):
                polls = st.get("input", {}).get("polls", 0)
                rising = rising + 1 if last is not None and polls > last else 0
                last = polls
                if rising >= 2:
                    return True
            time.sleep(0.12)
        return False

    def capture(seq, name, steps, key_reads):
        """Snap the screen the game is waiting on: an empty command."""
        ready = wait_ready(name)
        cid, st = send([(25, [])], name)
        frame = {}
        base = os.path.join(caps, f"debug-{cid}")
        for ext in ("indices", "palette"):
            try:
                with open(f"{base}.{ext}", "rb") as f:
                    data = f.read()
                frame[ext] = sha(data, 32)
                shutil.copyfile(f"{base}.{ext}", os.path.join(args.frames, f"{seq:03d}-{name}.{ext}"))
            except OSError:
                frame[ext] = None
        try:
            with open(f"{base}.json", encoding="utf-8") as f:
                meta = json.load(f)
            frame.update({k: meta.get(k) for k in ("mode", "displayPage", "drawPage", "displayByteOffset")})
        except (OSError, ValueError):
            pass
        # A key can be read after its command has completed (the game was busy):
        # the snap sends none, so its reads belong to this record's keys.
        late = [[k, r0, r1] for k, r0, r1 in st.get("input", {}).get("commandReads", [])]
        record = {"seq": seq, "name": name, "keys": [[ms, keys] for ms, keys in steps],
                  "keyReads": key_reads + late, "ready": ready, **snapshot(st), "frame": frame}
        return record

    failures = 0
    try:
        if read_status(status_path, time.monotonic() + 30) is None:
            sys.exit("no status.json: is this a build with debug control?")
        with open(args.out, "w", encoding="utf-8") as out:
            def emit(record):
                nonlocal failures
                if not record["ready"] or not record["frame"].get("indices"):
                    failures += 1
                out.write(json.dumps(record) + "\n")
                out.flush()
                print(f"{record['seq']:3d} {record['name']:<24} screen {record['screenKey']} row {record['campaign'].get('selectedRow')}"
                      f" rows {len(record['menu'])} reads {len(record['keyReads'])} frame {str(record['frame'].get('indices'))[:12]}"
                      f"{'' if record['ready'] else ' NOT READY'}", flush=True)
            emit({**capture(0, "boot", [], []), "biosTicks": args.bios_ticks})
            for seq, (name, steps) in enumerate(commands, 1):
                wait_ready(name)
                _, st = send(steps, name)
                reads = [[k, r0, r1] for k, r0, r1 in st.get("input", {}).get("commandReads", [])]
                emit(capture(seq, name, steps, reads))
    finally:
        if game.poll() is None:
            game.send_signal(signal.SIGTERM)
            try:
                game.wait(5)
            except subprocess.TimeoutExpired:
                game.kill()
        log.close()
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
