#!/usr/bin/env python3
"""Play without the game's programs (docs/phase2/PLAN.md 8.5, slice 6f).

    MW1O_EXE_FREE=1 exe_free.py --port BUILD/mechwarrior --work DIR [--game DIR] [--only NAME ...]

Opt-in (MW1O_EXE_FREE=1): two full trace runs take minutes, so ctest skips
it otherwise; the gate runs it as its own step, once per folder
(tools/trace/README.md gives the command).

The game folder (--game, or MW1O_GAME) is copied to DIR/game. The traces run
on the copy with the exes present (DIR/clean): that run imports the story
and records each trace. Every *.EXE and *.COM is then deleted from the copy
and the traces run again (DIR/free) from the first run's story, compared snap by
snap with the first run's recordings, not the goldens, so the 1989 folder
(whose combat traces are not the goldens') is checked as hard as the 1990
one. The goldens themselves are the gate's trace step (check.py).

It fails if any trace doesn't finish in either run, if a trace differs
without the exes, or if a trace with a battle doesn't reach the battle's
return in both runs (each combat EXEC launches and returns). The host stops
on any program (.EXE or .COM) opened at play, naming the file and the opener
(find_game_file); those lines are printed. Names and counts only.

The copy is deleted at the end, also on a failure; the runs and their logs
stay in DIR.

Exit status: 0 the same without the exes, 1 not, 77 skipped (not opted in,
or no game or port).
"""
import argparse, json, os, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
SKIP = 77
GUARD = "play opened the game's program"
COMBAT_RETURN = "Combat return:"


def check(a, game, work, extra):
    cmd = [sys.executable, "-I", os.path.join(HERE, "check.py"), "--port", a.port, "--game", game,
           "--work", work, "--quiet"] + extra
    if a.only:
        cmd += ["--only"] + a.only
    done = subprocess.run(cmd, capture_output=True, text=True)
    lines = done.stdout.strip().splitlines() or ["(no summary)"]
    return done.returncode, lines


def log(work, name):
    try:
        with open(os.path.join(work, name, "run", "run.log"), encoding="latin-1") as f:
            return f.read()
    except OSError:
        return ""


def guard_lines(work, names):
    """The guard's lines in each run's log, as "<trace>: <line>"."""
    return [f"{n}: {line.strip()}" for n in names for line in log(work, n).splitlines() if GUARD in line]


def has_battle(trace):
    with open(trace, encoding="utf-8") as f:
        return any(json.loads(line)["fields"].get("combat") == 1 for line in f if line.strip())


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", required=True, help="a port build with the feed")
    ap.add_argument("--work", required=True, help="scratch directory, outside the repo (emptied)")
    ap.add_argument("--game", default=os.environ.get("MW1O_GAME"), help="the game folder (default MW1O_GAME)")
    ap.add_argument("--only", nargs="*", help="trace names (default: all)")
    a = ap.parse_args()
    if os.environ.get("MW1O_EXE_FREE") != "1":
        print("skipped: set MW1O_EXE_FREE=1 (minutes: two full trace runs)")
        return SKIP
    if not a.game or not os.path.isdir(a.game) or not os.path.exists(a.port):
        print("skipped: set MW1O_GAME and build the port")
        return SKIP
    names = a.only or sorted(f[:-6] for f in os.listdir(os.path.join(HERE, "traces")) if f.endswith(".jsonl"))

    work = os.path.abspath(a.work)
    shutil.rmtree(work, ignore_errors=True)
    game, clean, free, story = (os.path.join(work, d) for d in ("game", "clean", "free", "story"))
    try:
        shutil.copytree(a.game, game)
        return traces(a, names, game, clean, free, story)
    finally:
        # Not left behind, whatever happened: setup looks for the game near
        # the program and would find this copy. The runs and logs stay.
        shutil.rmtree(game, ignore_errors=True)


def traces(a, names, game, clean, free, story):
    """The traces with the exes, then without them. The exit status."""
    _, lines = check(a, game, clean, [])
    print(f"with the exes: {lines[-1]}")
    unfinished = [n for n in names if not os.path.exists(os.path.join(clean, n + ".jsonl"))]
    if unfinished:
        print(f"FAIL with the exes, did not finish: {', '.join(unfinished)}")
        for line in guard_lines(clean, unfinished):
            print("  " + line)
        return 1
    shutil.copytree(os.path.join(clean, names[0], "run", "saves", "story"), story)

    is_program = lambda e: e.upper().endswith((".EXE", ".COM"))
    exes = [e for e in os.listdir(game) if is_program(e)]
    for e in exes:
        os.remove(os.path.join(game, e))
    left = [e for e in os.listdir(game) if is_program(e)]
    print(f"deleted {len(exes)} programs from the copy" + (f"; {len(left)} left" if left else ""))
    if not exes or left:
        print("FAIL the copy's programs")
        return 1

    status, lines = check(a, game, free, ["--story", story, "--label-game", a.game,
                                          "--golden", clean, "--strict"])
    print(f"without the exes, against the run with them: {lines[-1]}")
    failed = status != 0
    if failed:
        print("\n".join("  " + line for line in lines[:-1][:40]))
    for line in guard_lines(free, names):
        print("  " + line)
        failed = True
    for n in names:
        if has_battle(os.path.join(clean, n + ".jsonl")):
            returned = [w for w, d in (("with", clean), ("without", free)) if COMBAT_RETURN in log(d, n)]
            if len(returned) != 2:
                print(f"FAIL {n}: the battle returned only {' and '.join(returned) or 'in neither run'}")
                failed = True
    print("exe-free: " + ("FAILED" if failed else f"{len(names)} of {len(names)} traces as with the exes"))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
