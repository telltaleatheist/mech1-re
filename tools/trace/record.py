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
In combat (one input poll per tick; a key is a press and a release):
    make:KEY break:KEY   press or release only (held keys steer and fire)
    ticks:N           N more combat polls (ticks) before the next item
These three wait for combat, so "ticks:0" marks where combat starts; idle:N
waits for the campaign, so "idle:0" marks where it resumes after a battle.
Event-log scripts only (tools/trace/events/scripts, with --feed-calls; the
port also refuses them unless MECHWARRIOR_EVENT_LOG is set):
    song:NAME         start a song through the game's own routine (campaign)
    sound:N           request effect N through the game's own routine (combat)
These call the game, so a game-state trace may never use them: without
--feed-calls, or in a script outside that folder, the script is refused.

A record:
    snap    the snap's name
    keys    the tokens since the previous snap
    state   the compared state, as lines ("state random 1234 5678", ...);
            menu labels only as a 12-hex keyed hash, never MW1's text
    menu    each row's 081B callback offset, picture, links, enabled, label hash
    screenKey  a hash of the menu's shape
    frame   SHA-256 of the indexed 320x200 frame and of its colours (rgb: 17
            RGB triples); "palette", the codes' hash, is null since the port
            stopped writing codes (G5), and is never compared
The state ends with "state exchange <sha>", the hash of the whole campaign as
the save format encodes it (0x718 bytes, NAME.exchange beside the frames).

Frames are MW1 art: the recorder copies them to --frames, outside the repo.

A run that doesn't finish writes no trace: --out is deleted before the run,
so an earlier run's trace can't stand in for it. --story copies a story
folder (an earlier run's saves/story) into the run, so a folder without the
game's exes plays without importing; --label-game names the folder whose
MW_MAIN.EXE keys the labels (default --game).

    python3 record.py --exe build/mechwarrior --game ~/MECHWAR \\
        --script scripts/campaign_contract.keys --out traces/campaign_contract.jsonl \\
        --frames /tmp/frames
"""
import argparse, hashlib, hmac, json, os, re, shutil, signal, subprocess, sys, tempfile, time

POLL_S = 0.05
SNAP_NAME = re.compile(r"^[A-Za-z0-9_-]{1,47}$")
FEED_CALLS = ("song:", "sound:")
# The event-log feed scripts, and the listening renders' (listening.py), which run the same way.
FEED_CALL_SCRIPTS = tuple(os.path.join(os.path.dirname(os.path.abspath(__file__)), *p)
                          for p in (("events", "scripts"), ("listening",)))
# MW_MAIN's key region (segment, offset, length) and load segment, as
# src/Import/ImportTable.h's key_region: a label's hash is keyed as the import
# table's are, so a short label can't be found from it.
MAIN_KEY_REGION = (0x081B, 0x0100, 0x1400)
LOAD_SEGMENT = 0x0800


def sha(data, n=16):
    return hashlib.sha256(data).hexdigest()[:n]


def read_script(path, feed_calls=False):
    """The tokens, checked, and the snap names in order. song: and sound: only with feed_calls, and only in a
    script in FEED_CALL_SCRIPTS (they call the game, so a game-state trace may never use them)."""
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
        elif t.startswith(("idle:", "ticks:")):
            if not t.split(":", 1)[1].isdigit():
                sys.exit(f"{path}: bad {t!r}")
        elif t.startswith("random:"):
            if not re.fullmatch(r"[0-9A-Fa-f]{8}", t[7:]):
                sys.exit(f"{path}: bad {t!r} (random:WWWWWWWW, hex)")
        elif t.startswith(FEED_CALLS):
            if not feed_calls:
                sys.exit(f"{path}: {t!r} calls the game: refused (event-log scripts only, with --feed-calls)")
            if os.path.dirname(os.path.realpath(path)) not in [os.path.realpath(p) for p in FEED_CALL_SCRIPTS]:
                sys.exit(f"{path}: {t!r} is allowed only in a script in {' or '.join(FEED_CALL_SCRIPTS)}")
    if not snaps:
        sys.exit(f"{path}: no @snap")
    return tokens, snaps


def read_json(path, deadline):
    while True:
        try:
            # latin-1: the port writes text bytes as they are, so each byte
            # becomes one character and a label hashes as the original's bytes.
            with open(path, encoding="latin-1") as f:
                return json.load(f)
        except (OSError, ValueError):
            if time.monotonic() > deadline:
                return None
            time.sleep(POLL_S)


def label_key(game_dir):
    """The labels' key: the SHA-256 of MW_MAIN.EXE's key region in `game_dir`."""
    names = [e for e in os.listdir(game_dir) if e.upper() == "MW_MAIN.EXE"]
    if not names:
        sys.exit(f"MW_MAIN.EXE is not in {game_dir}")
    with open(os.path.join(game_dir, names[0]), "rb") as f:
        data = f.read()
    segment, offset, length = MAIN_KEY_REGION
    at = (data[8] | data[9] << 8) * 16 + (segment - LOAD_SEGMENT) * 16 + offset
    if data[:2] != b"MZ" or at + length > len(data):
        sys.exit(f"{names[0]} in {game_dir} is not MechWarrior's MW_MAIN.EXE")
    return hashlib.sha256(data[at:at + length]).digest()


def label_hash(row, key):
    """A 12-hex HMAC-SHA-256 of the row's label under `key` (label_key)."""
    label = row.get("label", "")
    return hmac.new(key, label.encode("latin-1"), hashlib.sha256).hexdigest()[:12] if label else ""


def picture(row):
    """A text row's picture word is where its label lives in the original's
    data segment, not a picture: the label hash stands for it ("-")."""
    return "-" if row.get("label") else row["picture"]


def state_lines(status, key):
    """The state the rebuild must match, one fact per line; `key` hashes the labels."""
    c = status.get("campaign", {})
    r = c.get("random", [0, 0])
    lines = [f"state random {r[0]:04x} {r[1]:04x}",
             "state campaign planet {planet} contract {contractActive} category {missionCategory} scenario {scenario}"
             " destination {destination} house {targetHouse} payment {paymentThousands} result {result}".format(**c),
             f"state funds {c.get('funds')} months {c.get('months')}",
             f"state row {c.get('selectedRow')}",
             f"state combat {1 if status.get('combat') else 0}"]
    if status.get("combat"):
        lines += combat_lines(status)
        return lines
    for row in c.get("menu", []):
        lines.append("state menu {} enabled {} callback {:04x} picture {} links {} label {}".format(
            row["index"], row["enabled"], row["callback"], picture(row), " ".join(map(str, row["links"])),
            label_hash(row, key) or "-"))
    return lines


def combat_lines(status):
    """The battle: the tick and screen, then each unit on the field."""
    words = lambda v: " ".join(map(str, v))
    lines = [f"state tick {status.get('tick')} screen {status.get('screen')} target {status.get('target')}"]
    for u in status.get("units", []):
        lines.append(f"state unit {u['slot']} side {u['side']} chassis {u['chassis']} position {words(u['position'])}"
                     f" orientation {words(u['orientation'])} step {u['step']} mode {u['movementMode']}"
                     f" jump {u['jumpCharge']} torso {words(u['torsoOrientation'])}")
        lines.append(f"state unit {u['slot']} heat {u['heat']} engine {u['engineHits']} gyro {u['gyroHits']}"
                     f" cockpit {u['cockpitDisabled']} armor {words(u['armor'])} internal {words(u['internal'])}"
                     f" ammo {words(u['ammo'])} weapons "
                     + " ".join(f"{w['operational']}/{w['reload']}" for w in u["weapons"]))
    obj = status.get("objective")
    if obj:
        lines.append(f"state objective position {words(obj['position'])} damage {obj['damageRemaining']}")
    return lines


def menu_rows(status, key):
    return [{"index": row["index"], "enabled": row["enabled"], "callback": f"0x{row['callback']:04X}",
             "picture": None if row.get("label") else row["picture"], "links": row["links"],
             "label": label_hash(row, key)}
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
    ap.add_argument("--story", help="a story folder to play from (an earlier run's saves/story)")
    ap.add_argument("--label-game", help="the folder whose MW_MAIN.EXE keys the labels (default --game)")
    ap.add_argument("--feed-calls", action="store_true",
                    help="allow song: and sound: (events.py only, for tools/trace/events/scripts)")
    args = ap.parse_args()

    tokens, snaps = read_script(args.script, args.feed_calls)
    if os.path.exists(args.out):
        os.remove(args.out)
    key = label_key(args.label_game or args.game)
    work = args.work or tempfile.mkdtemp(prefix="mw1trace-")
    ctl, saves, caps = (os.path.join(work, d) for d in ("ctl", "saves", "caps"))
    for d in (ctl, saves, caps):
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d, mode=0o700)
    if args.story:
        if not os.path.isfile(os.path.join(args.story, "import.json")):
            sys.exit(f"{args.story} is not a story folder (no import.json)")
        shutil.copytree(args.story, os.path.join(saves, "story"))
    os.makedirs(args.frames, exist_ok=True)

    # No window on screen: SDL's offscreen video on Linux and Windows; macOS's
    # offscreen driver has no renderer, so there the port's hidden window, in a
    # background app (never activated, so it can't take the keyboard).
    headless = ({"MW1O_HIDDEN": "1", "SDL_MAC_BACKGROUND_APP": "1"} if sys.platform == "darwin"
                else {"SDL_VIDEO_DRIVER": "offscreen"})
    env = dict(os.environ, **headless, SDL_AUDIO_DRIVER="dummy",
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
            for ext in ("indices", "palette", "rgb", "exchange"):
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
            menu = menu_rows(st, key)
            lines = state_lines(st, key) + ([f"state exchange {exchange}"] if exchange else [])
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
