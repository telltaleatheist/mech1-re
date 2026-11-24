#!/usr/bin/env python3
"""Compare a candidate with the golden traces, snap by snap.

    check.py --port BUILD/mechwarrior --game DIR --work DIR       # a port build (feed.txt)
    check.py --rebuild BUILD/mw1 --game DIR --work DIR            # the rebuild (docs/phase0/REPLAY.md)
    check.py --traces DIR                                         # traces already recorded

For every golden trace in traces/ (or those named with --only), the candidate
runs the same key script from scripts/, and each snap is compared: the frame
(indexed pixels and the colours they show, rgb, by hash; the palette codes
are kept but not compared: codes that show the same colour are the same
screen) and every field, by name (schema.py): a field that differs, or is
missing or extra, fails, and is named with the first element or byte that
differs. A known difference (known/<trace>.json) is reported, not failed, so
each one is fixed on purpose. The style follows crescent-hawks-rebuild's
tools/reference/check.py.

--story and --label-game pass to record.py (a folder without the exes:
tools/trace/exe_free.py); --strict ignores known/, for a comparison with a
recording of the same build rather than the goldens. --real-clock runs a port
build on the host's clock instead of record.py's test clock: a check that
real time still gives the goldens, not a gate (a slow build can snap an
animation a frame further on).

Exit status: 0 all match, 1 a mismatch, 2 an unknown --only name, 77 skipped (no
candidate or game).
"""
import argparse, json, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import schema  # noqa: E402  (how a field differs)
from selection import check_only  # noqa: E402
SKIP = 77


def load(path):
    with open(path, encoding="utf-8") as f:
        return [json.loads(line) for line in f if line.strip()]


def load_known(name):
    """known/<trace>.json: {"<snap>": {"frame": "why", "fields": {"<field name prefix>": "why"}}}"""
    path = os.path.join(HERE, "known", name + ".json")
    if not os.path.exists(path):
        return {}
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def compare(name, golden, ours, say, strict=False):
    """True when every snap matches, known differences aside (none if `strict`)."""
    known = {} if strict else load_known(name)
    ok = True
    by_snap = {r["snap"]: r for r in ours}
    for ref in golden:
        snap = ref["snap"]
        say(f"== {name} {snap}")
        got = by_snap.get(snap)
        if got is None:
            say("  FAIL missing from the candidate")
            ok = False
            continue
        k = known.get(snap, {})
        same_frame = got.get("frame", {}).get("indices") == ref["frame"]["indices"]
        if not ref["frame"].get("rgb"):
            say("  FAIL the golden snap has no rgb (recorded before G1b)")
            ok = False
            continue
        same_palette = got.get("frame", {}).get("rgb") == ref["frame"]["rgb"]
        if same_frame and same_palette:
            say("  ok   the screen")
        elif "frame" in k:
            say(f"  known the screen: {k['frame']}")
        else:
            say("  FAIL the screen: " + ", ".join(w for w, same in (("pixels", same_frame), ("colours", same_palette)) if not same))
            ok = False
        differ, known_fields = [], []
        ref_fields, got_fields = ref["fields"], got.get("fields", {})
        for field in list(ref_fields) + [n for n in got_fields if n not in ref_fields]:
            if field not in got_fields:
                line = f"{field}: missing (golden {ref_fields[field]})"
            elif field not in ref_fields:
                line = f"{field}: not in the golden (ours {got_fields[field]})"
            elif ref_fields[field] == got_fields[field]:
                continue
            else:
                line = schema.difference(field, ref_fields[field], got_fields[field])
            why = next((w for p, w in k.get("fields", {}).items() if field.startswith(p)), None)
            (known_fields if why else differ).append(line + (f"\n         known: {why}" if why else ""))
        say(f"  {'ok  ' if not differ else 'FAIL'} the fields: {len(differ)} differ"
            + (f", {len(known_fields)} known" if known_fields else ""))
        for d in differ + known_fields:
            say("    " + d)
        ok = ok and not differ
    extra = [r["snap"] for r in ours if r["snap"] not in {g["snap"] for g in golden}]
    if extra:
        say(f"  note: snaps not in the golden trace: {', '.join(extra)}")
    return ok


def run_candidate(a, name, out):
    script = os.path.join(HERE, "scripts", name + ".keys")
    work = os.path.join(a.work, name)
    if a.port:
        cmd = [sys.executable, os.path.join(HERE, "record.py"), "--exe", a.port, "--game", a.game,
               "--script", script, "--out", out, "--frames", os.path.join(work, "frames"),
               "--work", os.path.join(work, "run")]
        cmd += ["--story", a.story] if a.story else []
        cmd += ["--label-game", a.label_game] if a.label_game else []
        cmd += ["--real-clock"] if a.real_clock else []
    else:
        cmd = [a.rebuild, "--game-dir", a.game, "--save-dir", os.path.join(work, "saves"),
               "--keys", script, "--trace", out]
    os.makedirs(work, exist_ok=True)
    return subprocess.call(cmd, stdout=subprocess.DEVNULL) == 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    who = ap.add_mutually_exclusive_group(required=True)
    who.add_argument("--port", help="a port build with the feed (record.py drives it)")
    who.add_argument("--rebuild", help="the rebuild's program (docs/phase0/REPLAY.md)")
    who.add_argument("--traces", help="a directory of candidate traces, <name>.jsonl")
    ap.add_argument("--game", help="the game folder (a copy; only read)")
    ap.add_argument("--work", help="scratch directory, outside the repo")
    ap.add_argument("--golden", default=os.path.join(HERE, "traces"))
    ap.add_argument("--only", nargs="*", help="trace names (default: all)")
    ap.add_argument("--quiet", action="store_true", help="print only failures and the summary")
    ap.add_argument("--story", help="with --port: a story folder each run plays from (record.py)")
    ap.add_argument("--label-game", help="with --port: the folder whose MW_MAIN.EXE keys the labels")
    ap.add_argument("--strict", action="store_true", help="ignore known/ differences")
    ap.add_argument("--real-clock", action="store_true",
                    help="with --port: the host's clock, not the test clock (a check, not a gate)")
    a = ap.parse_args()

    valid = sorted(f[:-6] for f in os.listdir(a.golden) if f.endswith(".jsonl"))
    check_only(a.only, valid)
    names = a.only or valid
    if not a.traces:
        exe = a.port or a.rebuild
        missing = [w for w, p in (("the candidate", exe), ("the game folder", a.game)) if not p or not os.path.exists(p)]
        if missing:
            print("skipped: missing " + ", ".join(missing))
            return SKIP
        if not a.work:
            ap.error("--work is required with --port or --rebuild")
    failed = []
    for name in names:
        golden = load(os.path.join(a.golden, name + ".jsonl"))
        if a.traces:
            out = os.path.join(a.traces, name + ".jsonl")
        else:
            out = os.path.join(a.work, name + ".jsonl")
            if os.path.exists(out):
                os.remove(out)  # an earlier run's trace never stands in for this one
            if not run_candidate(a, name, out):
                print(f"== {name}\n  FAIL the candidate did not finish the script")
                failed.append(name)
                continue
        if not os.path.exists(out):
            print(f"== {name}\n  FAIL no candidate trace {out}")
            failed.append(name)
            continue
        report = []
        if not compare(name, golden, load(out), report.append, a.strict):
            failed.append(name)
        groups, group = [], []
        for line in report:
            if line.startswith("== ") and group:
                groups.append(group)
                group = []
            group.append(line)
        groups.append(group)
        for g in groups:
            if not a.quiet or any("FAIL" in line or "known" in line for line in g):
                print("\n".join(g))
    print(f"{len(names) - len(failed)} of {len(names)} traces match" + (f"; failed: {', '.join(failed)}" if failed else ""))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
