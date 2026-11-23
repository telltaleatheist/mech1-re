#!/usr/bin/env python3
"""Record or check the game-event goldens (docs/phase5/NO_EMULATION_PLAN.md, slice T1).

    events.py record --port BUILD/mechwarrior --game DIR --work DIR   # write the goldens
    events.py check  --port BUILD/mechwarrior --game DIR --work DIR   # compare a build

Every trace script (scripts/*.keys), every sound script (audio/scripts/*.keys)
and every feed script (events/scripts/*.keys: song: and sound: ask for every
song and every effect the game requests, through the game's own routines;
record.py --feed-calls, events/README.md) runs through record.py with:
- MECHWARRIOR_EVENT_LOG: what the game asked for (src/include/mechwarrior/event_log.h):
  the sequencer's notes, combat's tones, sound requests, key reads, input polls,
  snaps;
- MECHWARRIOR_DEBUG_VIRTUAL_CLOCK: time moves only where the port would wait, so
  the log depends on the script alone.

The log is cut after the last snap and split into three streams, each compared
on its own, each keeping the snap lines as marks:
- note: the sequencer's output and the songs started ("note", "song");
- tone: combat's speaker channel output ("tone");
- game: sound requests, key reads and combat polls ("sound", "key", "cpoll").

The goldens are the speaker's, the default device. What the game asks for
doesn't depend on the device, but the test clock charges 1 us for every port
read, and AdLib's calibrated reads make more of them, so on another device a
key, or a snap, can arrive some sequencer ticks earlier or later. The sound
scripts also run on AdLib and MT-32 (MT-32 without ROMs, silent) and must give:
- the speaker run's notes, ticks and snap marks left out, cut into pieces where
  the music can be cut by a key: at each song start and each all-notes-off sweep
  (a stop). The same number of pieces, and within each piece one run's notes a
  prefix of the other's within DEVICE_NOTE_SLACK (a key, or the last snap, may
  cut a piece a note or so earlier or later);
- its tone and game streams exactly.
They also run with music=soundfont (slice S3), which sets the game up as for
an MT-32 and plays its sequencer's output through the soundfont: that run must
give the MT-32 run's log exactly, every stream, ticks included.

Goldens are kept per game revision, by BTECH.EXE's SHA-256
(events/<first 12 hex>/<script>.jsonl.gz), so each revision checks against its
own. A revision with no goldens is skipped.

ctest runs it with --opt-in: skipped unless MW1O_EVENTS=1, since the runs take
minutes (as exe_free's MW1O_EXE_FREE). The game folder defaults to MW1O_GAME.

Exit status: 0 all match, 1 a mismatch, 77 skipped (no candidate, game or goldens,
or not opted in).
"""
import argparse, gzip, hashlib, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
EVENTS = os.path.join(HERE, "events")
FEED_SCRIPTS = os.path.join(EVENTS, "scripts")  # song: and sound: (record.py --feed-calls)
SKIP = 77
STREAMS = {"note": ("note", "song"), "tone": ("tone",), "game": ("sound", "key", "cpoll")}
DEVICE_RUNS = ("adlib", "mt32", "soundfont")  # beside the speaker, for the sound scripts
DEVICE_NOTE_SLACK = 32  # notes a song may be cut earlier or later on another device


def revision(game):
    path = next((os.path.join(game, n) for n in os.listdir(game) if n.upper() == "BTECH.EXE"), None)
    if not path:
        return None
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()[:12]


def scripts():
    found = []
    for folder in (os.path.join(HERE, "scripts"), os.path.join(HERE, "audio", "scripts"), FEED_SCRIPTS):
        found += [os.path.join(folder, f) for f in sorted(os.listdir(folder)) if f.endswith(".keys")]
    return found


def name_of(script):
    stem = os.path.basename(script)[:-5]
    folder = os.path.basename(os.path.dirname(os.path.dirname(script)))
    return {"audio": "audio-", "events": "feed-"}.get(folder, "") + stem


def run(port, game, work, script, device):
    """The candidate's event log for one script on one device, cut after the last snap."""
    name = f"{name_of(script)}.{device}"
    base = os.path.join(work, name)
    os.makedirs(base, exist_ok=True)
    ini = os.path.join(base, "sound.ini")
    with open(ini, "w") as f:
        setting = "music=soundfont" if device == "soundfont" else f"sound_system={device}"
        f.write(f"combat_tick_ms=60\ncombat_speed=1\n{setting}\n")
    log = os.path.join(base, "events.log")
    # The sound-port log too: with it set and no ROM paths, MT-32 runs silent (SDLBackend_Audio.cpp).
    env = dict(os.environ, MECHWARRIOR_TIMING_CONFIG=ini, MECHWARRIOR_EVENT_LOG=log,
               MECHWARRIOR_AUDIO_LOG=os.path.join(base, "sound.log"), MECHWARRIOR_DEBUG_VIRTUAL_CLOCK="1")
    feed = ["--feed-calls"] if os.path.dirname(script) == FEED_SCRIPTS else []
    rec = subprocess.run([sys.executable, os.path.join(HERE, "record.py"), "--exe", port, "--game", game,
                          "--script", script, "--out", os.path.join(base, "trace.jsonl"),
                          "--frames", os.path.join(base, "frames"), "--work", os.path.join(base, "run"),
                          "--timeout", "900"] + feed, env=env, capture_output=True, text=True)
    if rec.returncode:
        sys.exit(f"{name}: record.py failed\n{rec.stdout[-2000:]}{rec.stderr[-2000:]}")
    with open(log, encoding="utf-8") as f:
        lines = f.read().splitlines()
    last = max((i for i, line in enumerate(lines) if line.startswith('["snap"')), default=-1)
    if last < 0:
        sys.exit(f"{name}: the log has no snap")
    return lines[:last + 1]


def untimed(lines):
    """The log with each note's and song's tick replaced by '*'."""
    out = []
    for line in lines:
        if line.startswith(('["note", ', '["song", ')):
            head, rest = line.split(", ", 1)
            line = head + ", *, " + rest.split(", ", 1)[1]
        out.append(line)
    return out


def is_sweep(line):
    """An all-notes-off message (controller 123), as the sequencer's stop sends on every channel."""
    return line.startswith('["note", *, "cc", ') and line.split(", ")[4] == "123"


def songs(lines):
    """The note stream, untimed and without snap marks, cut where a key can cut the music: at each song
    start and at the first message of each all-notes-off sweep."""
    out, previous = [[]], ""
    for line in untimed(stream(lines, STREAMS["note"])):
        if not line.startswith('["') or line.startswith('["snap"'):
            continue
        if line.startswith('["song"') or (is_sweep(line) and not is_sweep(previous)):
            out.append([])
        out[-1].append(line)
        previous = line
    return out


def same_on_device(name, device, speaker, other, say):
    """The device check (module doc): song by song, then the tone and game streams exactly."""
    ok = True
    ours, theirs = songs(speaker), songs(other)
    if len(ours) != len(theirs):
        say(f"{name}: {device} gave {len(theirs)} pieces of music, the speaker {len(ours)}")
        ok = False
    for i, (a, b) in enumerate(zip(ours, theirs)):
        n = min(len(a), len(b))
        if a[:n] != b[:n] or abs(len(a) - len(b)) > DEVICE_NOTE_SLACK:
            compare(f"{name}: {device} piece {i} against the speaker's, untimed", a, b, say)
            ok = False
    for k in ("tone", "game"):
        ok = compare(f"{name}: {device} {k} against the speaker's", stream(speaker, STREAMS[k]),
                     stream(other, STREAMS[k]), say) and ok
    return ok


def stream(lines, kinds):
    """The lines of one stream, with the header and every snap mark kept."""
    keep = tuple(f'["{k}"' for k in kinds) + ('["snap"', "{")
    return [line for line in lines if line.startswith(keep)]


def compare(label, golden, ours, say):
    snap = "(start)"
    for i, (a, b) in enumerate(zip(golden, ours)):
        if a.startswith('["snap"'):
            snap = a
        if a != b:
            say(f"{label}: line {i + 1} differs, after {snap}\n  golden {a}\n  ours   {b}")
            return False
    if len(golden) != len(ours):
        say(f"{label}: {len(golden)} golden lines, {len(ours)} ours (the shorter one matches as far as it goes)")
        return False
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mode", choices=("record", "check"))
    ap.add_argument("--port", help="the port build to run")
    ap.add_argument("--game", default=os.environ.get("MW1O_GAME"), help="MW1 game folder (a copy; it is only read)")
    ap.add_argument("--work", required=True, help="logs, saves and frames (outside the repo)")
    ap.add_argument("--only", nargs="*", help="script names (audio-music, campaign_contract, ...)")
    ap.add_argument("--no-devices", action="store_true", help="skip the AdLib and MT-32 runs of the sound scripts")
    ap.add_argument("--opt-in", action="store_true", help="skip unless MW1O_EVENTS=1 (ctest)")
    a = ap.parse_args()
    if a.opt_in and os.environ.get("MW1O_EVENTS") != "1":
        print("skipped: set MW1O_EVENTS=1 to run the event logs")
        return SKIP
    if not a.port or not os.path.exists(a.port) or not a.game or not os.path.isdir(a.game):
        print("skipped: no candidate build or game folder")
        return SKIP
    port, game, work = os.path.abspath(a.port), os.path.abspath(a.game), os.path.abspath(a.work)
    rev = revision(game)
    folder = os.path.join(EVENTS, rev or "none")
    if a.mode == "check" and not os.path.isdir(folder):
        print(f"skipped: no event goldens for this game's BTECH.EXE ({rev})")
        return SKIP
    failed, checked = [], 0
    for script in scripts():
        name = name_of(script)
        if a.only and name not in a.only:
            continue
        ours = run(port, game, work, script, "speaker")
        if name.startswith(("audio-", "feed-")) and not a.no_devices:
            runs = {}
            for device in DEVICE_RUNS:
                runs[device] = run(port, game, work, script, device)
                if not same_on_device(name, device, ours, runs[device], print):
                    failed.append(f"{name}.{device}")
            if not all(compare(f"{name}: soundfont {k} against MT-32's", stream(runs["mt32"], kinds),
                               stream(runs["soundfont"], kinds), print) for k, kinds in STREAMS.items()):
                failed.append(f"{name}.soundfont")
        golden_path = os.path.join(folder, name + ".jsonl.gz")
        if a.mode == "record":
            os.makedirs(folder, exist_ok=True)
            with gzip.GzipFile(golden_path, "wb", mtime=0) as f:
                f.write(("\n".join(ours) + "\n").encode())
            counts = ", ".join(f"{k} {len(stream(ours, v)) - 1}" for k, v in STREAMS.items())
            print(f"{name}: {len(ours)} lines recorded ({counts}, snaps included)")
            continue
        if not os.path.exists(golden_path):
            print(f"{name}: no golden")
            failed.append(name)
            continue
        with gzip.open(golden_path, "rt", encoding="utf-8") as f:
            golden = f.read().splitlines()
        checked += 1
        bad = [k for k, kinds in STREAMS.items()
               if not compare(f"{name} {k}", stream(golden, kinds), stream(ours, kinds), print)]
        if bad:
            failed.append(f"{name} ({' '.join(bad)})")
        else:
            print(f"{name}: note, tone and game streams match ({len(ours)} lines)")
    if a.mode == "check":
        print(f"{checked - len([f for f in failed if '(' in f])} of {checked} event logs match"
              + (f"; failed: {', '.join(failed)}" if failed else ""))
    elif failed:
        print(f"recorded, but the sound device changed the log: {', '.join(failed)}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
