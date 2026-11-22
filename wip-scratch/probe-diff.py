"""Slice 6 planning: which globals each exe loader changes. Names, files,
groups and byte counts only (never values).

    python3 -I probe-diff.py REPO BUILD DUMPS OUT
"""
import collections, glob, os, re, subprocess, sys

repo, build, dumps, out = sys.argv[1:5]
exe = os.path.join(build, "mechwarrior")
seg = subprocess.run(["otool", "-l", exe], capture_output=True, text=True).stdout
m = re.search(r"segname __DATA\n\s+vmaddr (0x[0-9a-f]+)\n\s+vmsize (0x[0-9a-f]+)", seg)
base, size = int(m.group(1), 16), int(m.group(2), 16)

def symbols(path):
    out = []
    for l in subprocess.run(["nm", "-n", path], capture_output=True, text=True).stdout.splitlines():
        p = l.split()
        if len(p) == 3 and p[1].lower() in "dbs":
            out.append((int(p[0], 16), re.sub(r"^ZL\d+", "", p[2].lstrip("_"))))
    return out

syms = [(a, n) for a, n in symbols(exe) if base <= a < base + size]
syms.sort()
starts = [a - base for a, _ in syms]
names = [n for _, n in syms]

# Which source file defines each symbol: the build's objects.
owner = {}
for obj in glob.glob(os.path.join(build, "CMakeFiles", "**", "*.o"), recursive=True):
    src = obj.split(".dir/", 1)[1][:-2]
    for l in subprocess.run(["nm", "--defined-only", obj], capture_output=True, text=True).stdout.splitlines():
        p = l.split()
        if len(p) == 3 and p[1].lower() in "dbs":
            owner.setdefault(re.sub(r"^ZL\d+", "", p[2].lstrip("_")), src)

# The inventory's groups for named items (non-code groups list names).
group = {}
heading = None
for l in open(os.path.join(repo, "docs/phase2/INVENTORY.md"), encoding="utf-8"):
    if l.startswith("## "):
        heading = l[3:].strip()
    hit = re.match(r"\| (\S+) \| `([^`]+)` \|", l)
    if hit and heading:
        group[hit.group(2)] = heading.split(":")[0] + (": " + heading.split(":")[1].split("(")[0].strip() if ":" in heading else "")

import bisect
def sym_at(off):
    i = bisect.bisect_right(starts, off) - 1
    return names[i] if i >= 0 else "?"

def load(n):
    f = glob.glob(os.path.join(dumps, f"{n:02d}-*.bin"))[0]
    return open(f, "rb").read()

pairs = {"MW_MAIN (start)": [(0, 1)], "MW.EXE launcher": [(1, 2)], "MW_MAIN (EXEC)": [(3, 4), (9, 10)],
         "MW_EGA.EXE overlay": [(5, 6), (11, 12)], "BTECH.EXE": [(7, 8)]}
with open(out, "w") as o:
    for label, ps in pairs.items():
        changed = collections.Counter()
        nonzero = collections.Counter()
        for a, b in ps:
            A, B = load(a), load(b)
            for i in range(min(len(A), len(B))):
                if A[i] != B[i]:
                    changed[sym_at(i)] += 1
            for i in range(len(B)):
                if B[i] and A[i] != B[i]:
                    nonzero[sym_at(i)] += 1
        o.write(f"## {label}: {len(changed)} symbols changed\n")
        for n in sorted(changed, key=lambda n: (owner.get(n, "?"), n)):
            o.write(f"{owner.get(n, '?'):55} {n:45} changed {changed[n]:6} nonzero {nonzero[n]:6} group {group.get(n, '-')}\n")
print("done", out)
