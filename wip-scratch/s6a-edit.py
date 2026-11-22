"""6a: class A reads (zero in the exe) become zeros. Statement-level, per item.
Prints the items it could not place, for hand edits.

    python3 -I s6a-edit.py SWEEP REPO
"""
import re, sys, collections

sweep, repo = sys.argv[1:3]
zero = []
on = False
for l in open(sweep):
    if l.startswith("## "):
        on = "zero-in-exe" in l
        continue
    if on:
        zero.append(l.split())
by_file = collections.defaultdict(list)
for f, n in zero:
    by_file[f].append(n)

def addr(stmt):
    m = re.search(r"0x([0-9A-Fa-f]{3,4})", stmt)
    return m.group(1).upper().zfill(4) if m else None

unplaced = []
done = 0
for f, names in sorted(by_file.items()):
    path = f"{repo}/src/Original/Memory/{f}"
    s = open(path).read()
    for n in names:
        placed = False
        # memcpy(&X,data+0xNNNN,sizeof(X)); / memcpy(X,data+..,sizeof(X));
        pat = re.compile(r"memcpy\((&?)" + n + r",data\s*\+\s*(0x[0-9A-Fa-f]+),sizeof\((" + n + r")\)\);")
        def rep(m):
            return f"memset({m.group(1)}{n},0,sizeof({n})); /*{m.group(2)[2:].upper().zfill(4)}*/"
        s, k = pat.subn(rep, s)
        placed |= k > 0
        # ImportInitialWords(data,0xNNNN,(uint16_t *)X,count);
        pat = re.compile(r"ImportInitialWords\(data,(0x[0-9A-Fa-f]+),\(uint16_t \*\)" + n + r",[^;]*\);")
        s, k = pat.subn(lambda m: f"memset({n},0,sizeof({n})); /*{m.group(1)[2:].upper().zfill(4)}*/", s)
        placed |= k > 0
        # NAME[...](.field)?=RHS-with-data;  (RHS has no ';')
        pat = re.compile(r"(?<![\w.>])(" + n + r"(?:\[[^\]=;]*\])*(?:\.\w+)*\s*=)([^;=]*\bdata\b[^;]*);")
        def rep2(m):
            a = addr(m.group(2))
            return f"{m.group(1)}0;" + (f" /*{a}*/" if a else "")
        s, k = pat.subn(rep2, s)
        placed |= k > 0
        if placed:
            done += 1
        else:
            unplaced.append((f, n))
    open(path, "w").write(s)
print("placed", done, "unplaced", len(unplaced))
for f, n in unplaced:
    print(" ", f, n)
