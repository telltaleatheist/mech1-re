#!/usr/bin/env python3
"""Record one script twice and check the runs are byte-identical.

    python3 repeat.py --exe build/mechwarrior --game ~/MECHWAR --script scripts/X.keys --work DIR

Exit 0 when both traces and every frame match. A difference means something
still depends on the host (time, speed, the order of events): fix that before
trusting a trace as golden.
"""
import argparse, filecmp, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", required=True)
    ap.add_argument("--game", required=True)
    ap.add_argument("--script", required=True)
    ap.add_argument("--work", required=True, help="scratch directory (outside the repo)")
    args = ap.parse_args()
    runs = []
    for run in ("a", "b"):
        d = os.path.join(args.work, run)
        code = subprocess.call([sys.executable, os.path.join(HERE, "record.py"), "--exe", args.exe, "--game", args.game,
                                "--script", args.script, "--out", os.path.join(d, "trace.jsonl"),
                                "--frames", os.path.join(d, "frames"), "--work", os.path.join(d, "work")],
                               stdout=subprocess.DEVNULL)
        if code:
            sys.exit(f"run {run} failed ({code}); see {d}/work/run.log")
        runs.append(d)
    a, b = runs
    bad = [] if filecmp.cmp(f"{a}/trace.jsonl", f"{b}/trace.jsonl", shallow=False) else ["trace.jsonl"]
    frames = sorted(os.listdir(f"{a}/frames"))
    if frames != sorted(os.listdir(f"{b}/frames")):
        bad.append("the frame lists")
    bad += [f for f in frames if os.path.exists(f"{b}/frames/{f}")
            and not filecmp.cmp(f"{a}/frames/{f}", f"{b}/frames/{f}", shallow=False)]
    if bad:
        print("differ: " + ", ".join(bad))
        return 1
    print(f"identical: the trace and {len(frames)} frame files")
    return 0


if __name__ == "__main__":
    sys.exit(main())
