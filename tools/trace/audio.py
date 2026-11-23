#!/usr/bin/env python3
"""Record or check the sound-port goldens (docs/phase5/AUDIO_PLAN.md, level 1).

    audio.py record --port BUILD/mechwarrior --game DIR --work DIR   # write the goldens
    audio.py check  --port BUILD/mechwarrior --game DIR --work DIR   # compare a build

Each script in audio/scripts/ runs once per sound device (speaker, adlib,
mt32) through record.py, with:
- MECHWARRIOR_AUDIO_LOG: every access to the PIT channel 0, AdLib and MPU
  ports, as [PIT cycle, "w" or "r", port, value]; each change of the tone
  voice's tone (the speaker music, S6, and combat's effects, S5, where PIT
  channel 2 and port 61h were), as [PIT cycle, "tone", Hz to 0.001, 0 for
  silence]; and each snap as [PIT cycle, "snap", name];
- MECHWARRIOR_DEBUG_VIRTUAL_CLOCK: time moves only where the port would wait,
  so the log depends on the script alone;
- MT-32 with no ROMs: the MPU byte stream runs and the synthesizer is silent.

The log is cut after the last snap (what follows is the shutdown, timed by
the harness) and kept as audio/traces/<script>.<device>.jsonl.gz. A check
compares every line exactly and names the first difference.

Exit status: 0 all match, 1 a mismatch, 77 skipped (no candidate or game).
"""
import argparse, gzip, hashlib, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
AUDIO = os.path.join(HERE, "audio")
DEVICES = ("speaker", "adlib", "mt32")
SKIP = 77


PIT_HZ, PCM_HZ = 1193182, 48000


def pcm_hash(path, device, cycle):
    """SHA-256 of the PCM dump up to the last snap's PIT cycle."""
    channels = 1 if device == "speaker" else 2
    frames = cycle * PCM_HZ // PIT_HZ
    with open(path, "rb") as f:
        data = f.read(frames * channels * 4)
    if len(data) < frames * channels * 4:
        sys.exit(f"{path}: the PCM dump ends before the last snap")
    return hashlib.sha256(data).hexdigest()[:32], frames


def run(port, game, work, script, device, pcm=False):
    """The candidate's log for one script and device, cut after the last snap,
    and with pcm, the hash of its PCM up to that snap."""
    name = f"{os.path.basename(script)[:-5]}.{device}"
    base = os.path.join(work, name)
    os.makedirs(base, exist_ok=True)
    ini = os.path.join(base, "sound.ini")
    with open(ini, "w") as f:
        f.write(f"combat_tick_ms=60\ncombat_speed=1\nsound_system={device}\n")
    log = os.path.join(base, "sound.log")
    env = dict(os.environ, MECHWARRIOR_TIMING_CONFIG=ini, MECHWARRIOR_AUDIO_LOG=log,
               MECHWARRIOR_DEBUG_VIRTUAL_CLOCK="1")
    if pcm:
        env["MECHWARRIOR_AUDIO_PCM"] = os.path.join(base, "sound.pcm")
    rec = subprocess.run([sys.executable, os.path.join(HERE, "record.py"), "--exe", port, "--game", game,
                          "--script", script, "--out", os.path.join(base, "trace.jsonl"),
                          "--frames", os.path.join(base, "frames"), "--work", os.path.join(base, "run"),
                          "--timeout", "600"], env=env, capture_output=True, text=True)
    if rec.returncode:
        sys.exit(f"{name}: record.py failed\n{rec.stdout[-2000:]}{rec.stderr[-2000:]}")
    with open(log, encoding="utf-8") as f:
        lines = f.read().splitlines()
    last = max((i for i, line in enumerate(lines) if '"snap"' in line), default=-1)
    if last < 0:
        sys.exit(f"{name}: the log has no snap")
    digest = pcm_hash(os.path.join(base, "sound.pcm"), device, int(lines[last][1:].split(",")[0])) if pcm else None
    return name, lines[:last + 1], digest


def golden_path(name):
    return os.path.join(AUDIO, "traces", name + ".jsonl.gz")


def compare(name, golden, ours, say):
    snap = "(start)"
    for i, (a, b) in enumerate(zip(golden, ours)):
        if '"snap"' in a:
            snap = a
        if a != b:
            say(f"{name}: line {i + 1} differs, after {snap}\n  golden {a}\n  ours   {b}")
            return False
    if len(golden) != len(ours):
        say(f"{name}: {len(golden)} golden lines, {len(ours)} ours (the shorter one matches as far as it goes)")
        return False
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mode", choices=("record", "check"))
    ap.add_argument("--port", help="the port build to run")
    ap.add_argument("--game", help="MW1 game folder (a copy; it is only read)")
    ap.add_argument("--work", required=True, help="logs, saves and frames (outside the repo)")
    ap.add_argument("--only", nargs="*", help="script names, or script.device")
    ap.add_argument("--pcm", action="store_true",
                    help="also print each run's PCM hash up to its last snap (synthesis, compared by hand)")
    a = ap.parse_args()
    if not a.port or not os.path.exists(a.port) or not a.game or not os.path.isdir(a.game):
        print("skipped: no candidate build or game folder")
        return SKIP
    port, game, work = os.path.abspath(a.port), os.path.abspath(a.game), os.path.abspath(a.work)
    scripts = sorted(f for f in os.listdir(os.path.join(AUDIO, "scripts")) if f.endswith(".keys"))
    failed, checked = [], 0
    for script in scripts:
        for device in DEVICES:
            name = f"{script[:-5]}.{device}"
            if a.only and script[:-5] not in a.only and name not in a.only:
                continue
            name, ours, digest = run(port, game, work, os.path.join(AUDIO, "scripts", script), device, a.pcm)
            if digest:
                print(f"{name}: PCM {digest[0]} over {digest[1]} frames")
            if a.mode == "record":
                os.makedirs(os.path.dirname(golden_path(name)), exist_ok=True)
                with gzip.GzipFile(golden_path(name), "wb", mtime=0) as f:
                    f.write(("\n".join(ours) + "\n").encode())
                print(f"{name}: {len(ours)} lines recorded")
                continue
            if not os.path.exists(golden_path(name)):
                print(f"{name}: no golden")
                failed.append(name)
                continue
            with gzip.open(golden_path(name), "rt", encoding="utf-8") as f:
                golden = f.read().splitlines()
            checked += 1
            if compare(name, golden, ours, print):
                print(f"{name}: {len(ours)} lines match")
            else:
                failed.append(name)
    if a.mode == "check":
        print(f"{checked - len([n for n in failed if os.path.exists(golden_path(n))])} of {checked} sound logs match"
              + (f"; failed: {' '.join(failed)}" if failed else ""))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
