"""Compare two probe runs symbol by symbol after each loader (names and byte
counts only). Symbols that differ between the two runs' 'before' dumps of the
same point are reported separately (state that play or ASLR changes).

    python3 -I probe-eq.py OLD_BIN OLD_DUMPS NEW_BIN NEW_DUMPS
"""
import glob, os, re, subprocess, sys

def layout(exe):
    seg = subprocess.run(["otool", "-l", exe], capture_output=True, text=True).stdout
    m = re.search(r"segname __DATA\n\s+vmaddr (0x[0-9a-f]+)\n\s+vmsize (0x[0-9a-f]+)", seg)
    base, size = int(m.group(1), 16), int(m.group(2), 16)
    syms = []
    for l in subprocess.run(["nm", "-n", exe], capture_output=True, text=True).stdout.splitlines():
        p = l.split()
        if len(p) == 3 and p[1].lower() in "dbs":
            a = int(p[0], 16)
            if base <= a < base + size:
                syms.append((a - base, re.sub(r"^ZL\d+", "", p[2].lstrip("_"))))
    syms.sort()
    out = {}
    for i, (a, n) in enumerate(syms):
        end = syms[i + 1][0] if i + 1 < len(syms) else size
        out.setdefault(n, (a, end))
    return out

ob, od, nb, nd = sys.argv[1:5]
L0, L1 = layout(ob), layout(nb)
common = set(L0) & set(L1)
def dumps(d):
    return sorted(f for f in glob.glob(os.path.join(d, "*.bin")) if re.match(r"\d\d-", os.path.basename(f)))
D0, D1 = dumps(od), dumps(nd)
assert [os.path.basename(f) for f in D0] == [os.path.basename(f) for f in D1], "dump points differ"
skip = {"sequence", "ZZ9ProbeDumpPKcE8sequence"}
for f0, f1 in zip(D0, D1):
    A, B = open(f0, "rb").read(), open(f1, "rb").read()
    diff = []
    for n in sorted(common - skip):
        a0, e0 = L0[n]; a1, e1 = L1[n]
        x, y = A[a0:e0], B[a1:e1]
        n_ = min(len(x), len(y))
        def ptr(v):
            return 0x100000000 <= v < 0x800000000000
        d = 0
        for j in range(0, n_, 8):
            u, w = x[j:j+8], y[j:j+8]
            if u == w:
                continue
            if len(u) == 8 and len(w) == 8 and (a0 + j) % 8 == 0 and ptr(int.from_bytes(u, "little")) and ptr(int.from_bytes(w, "little")):
                continue
            d += sum(1 for i in range(min(len(u), len(w))) if u[i] != w[i])
        if d or len(x) != len(y):
            diff.append(f"{n}({d}{'' if len(x)==len(y) else f',size {len(x)}->{len(y)}'})")
    print(f"{os.path.basename(f0)}: {len(diff)} symbols differ" + (": " + " ".join(diff) if diff else ""))
print("only old:", len(set(L0) - set(L1)), "only new:", len(set(L1) - set(L0)))
