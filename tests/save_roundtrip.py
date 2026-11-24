#!/usr/bin/env python3
"""A save written and read back gives the same exchange block (B6).

    save_roundtrip.py --program BUILD/save_roundtrip_test --data DATA --work DIR

Runs the program, then compares each case's block before and after by
tools/trace/schema.py's fields, so a failure names what moved
(exchange.<name>); values are never printed. With MW1O_DOS_SAVES (a folder
of .GAM files) copies of those are loaded and re-encoded too: the claim is
only that they load and give the same block, not who wrote them.
"""
import argparse, os, shutil, subprocess, sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "trace"))
import schema


def moved(before, after):
    a, b = schema.exchange_fields(before, None), schema.exchange_fields(after, None)
    return [name for name in a if a[name] != b[name]]


def main():
    ap = argparse.ArgumentParser()
    for name in ("--program", "--data", "--work"):
        ap.add_argument(name, required=True)
    a = ap.parse_args()
    work = os.path.abspath(a.work)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    command = [a.program, a.data, work]
    source = os.environ.get("MW1O_DOS_SAVES")
    kept = []
    if source:
        copies = os.path.join(work, "kept")
        os.makedirs(copies)
        kept = [f for f in sorted(os.listdir(source)) if f.upper().endswith(".GAM")]
        for f in kept:
            shutil.copyfile(os.path.join(source, f), os.path.join(copies, f.upper()))
        command.append(copies)
    done = subprocess.run(command, capture_output=True, text=True)
    print(done.stdout, end="")
    failed = done.returncode != 0
    if failed:
        print(f"FAIL the program, exit {done.returncode}")

    cases = sorted(f[:-7] for f in os.listdir(work) if f.endswith(".before"))
    for case in cases:
        before, after = (open(os.path.join(work, case + end), "rb").read() for end in (".before", ".after"))
        names = moved(before, after)
        if before != after and not names:
            names = ["(a byte no field covers)"]
        if names:
            print(f"FAIL {case}: " + ", ".join(names))
            failed = True
    if len(cases) != 4 + len(kept):
        print(f"FAIL {len(cases)} cases, expected {4 + len(kept)}")
        failed = True
    # The comparison itself: one byte changed names its own field and no other.
    block = bytes(schema.EXCHANGE_BYTES)
    for f in schema.EXCHANGE_FIELDS:
        changed = bytearray(block)
        changed[f.offset] ^= 1
        names = moved(block, bytes(changed))
        if len(names) != 1 or not (names[0] + ".").startswith(f"exchange.{f.name}."):
            print(f"FAIL a change in {f.name} is named {names}")
            failed = True
    print("save round trip: " + ("FAILED" if failed else f"{len(cases)} blocks the same, {len(kept)} of them kept saves"))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
