#!/usr/bin/env python3
"""Run the golden scripts against the original under DOSBox-X and compare.

    run.py --home ~/mwsnap --out ~/mw1-orig [--only bar travel] [--dosbox-x PATH]

For each script: tools/capture/keys.py compiles it to KEYS.BIN, MWSNAP.COM is
assembled from tools/capture/mwsnap.asm (nasm), tools/capture/run_wsl.sh runs
the game headless under DOSBox-X (no window, one run at a time), state.py
turns the captures into a trace, and check.py --traces compares the traces
with the goldens. Linux or WSL.

--home is MWSNAP's work folder (run_wsl.sh's MWSNAP_HOME): game/ holds a copy
of the game, which is only read; in/ and run-g-NAME/ are written here. --out
gets NAME.jsonl. Captures and frames are MW1 art and memory: keep both
folders outside the repo.

Exit status: check.py's (0 all match, 1 a mismatch), or 2 when a run fails.
"""
import argparse, os, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
CAPTURE = os.path.join(os.path.dirname(HERE), "capture")
FLAGS = ("NOPATCH.TXT", "KEYFAIL.TXT", "ITEMFAIL.TXT")


def run(cmd, env=None):
    print("+ " + " ".join(cmd), flush=True)
    return subprocess.call(cmd, env=env)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--home", required=True, help="MWSNAP_HOME: game/ is the game copy")
    ap.add_argument("--out", required=True, help="where the original's traces go")
    ap.add_argument("--only", nargs="*", help="script names (default: every golden trace)")
    ap.add_argument("--dosbox-x", default=os.environ.get("DOSBOX_X", "dosbox-x"))
    ap.add_argument("--cycles", default="3000")
    ap.add_argument("--limit", default="600", help="seconds per run")
    a = ap.parse_args()

    home, out = os.path.abspath(os.path.expanduser(a.home)), os.path.abspath(os.path.expanduser(a.out))
    if not os.path.isdir(os.path.join(home, "game")):
        sys.exit(f"{home}/game: no game copy")
    names = a.only or sorted(f[:-6] for f in os.listdir(os.path.join(HERE, "traces")) if f.endswith(".jsonl"))
    inbox = os.path.join(home, "in")
    os.makedirs(inbox, exist_ok=True)
    os.makedirs(out, exist_ok=True)
    if run(["nasm", "-f", "bin", "-o", os.path.join(inbox, "MWSNAP.COM"), os.path.join(CAPTURE, "mwsnap.asm")]):
        sys.exit("nasm failed")
    env = dict(os.environ, MWSNAP_HOME=home, DOSBOX_X=a.dosbox_x)
    failed = []
    for name in names:
        script = os.path.join(HERE, "scripts", name + ".keys")
        keys = name + ".bin"
        rundir = os.path.join(home, "run-g-" + name)
        shutil.rmtree(rundir, ignore_errors=True)
        if (run([sys.executable, os.path.join(CAPTURE, "keys.py"), script, os.path.join(inbox, keys)])
                or run(["bash", os.path.join(CAPTURE, "run_wsl.sh"), "g-" + name, keys, a.cycles, a.limit], env)):
            failed.append(name)
            continue
        flags = [f for f in FLAGS if os.path.exists(os.path.join(rundir, f))]
        if not os.path.exists(os.path.join(rundir, "MWSNAPEN.TXT")) or flags:
            print(f"{name}: the script did not finish" + (f" ({', '.join(flags)})" if flags else ""))
            failed.append(name)
            continue
        if run([sys.executable, os.path.join(HERE, "state.py"), "--script", script, "--run", rundir,
                "--out", os.path.join(out, name + ".jsonl")]):
            failed.append(name)
    if failed:
        print("runs failed: " + ", ".join(failed))
    done = [n for n in names if n not in failed]
    status = run([sys.executable, os.path.join(HERE, "check.py"), "--traces", out, "--quiet", "--only", *done]) if done else 1
    return 2 if failed else status


if __name__ == "__main__":
    sys.exit(main())
