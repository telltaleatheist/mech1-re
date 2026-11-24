#!/usr/bin/env python3
"""Menu-handler map (FIXES 130 Phase 0).

Joins three sources into one table, keyed by the original MW_MAIN.EXE code
offset in segment 081B that a menu row carries as its activate callback:

  1. the port's names and notes for each offset (campaign_continuations.h);
  2. the port function the dispatcher runs for it (the switch in
     MwCampaignUiEventLoop, MW_MAIN_081B_UI.c);
  3. golden traces: on which screens the offset was seen, and whether a trace
     activated it (Return read while that row was selected).

The rebuild's menus must route every offset here to its own handler; the
"traced" columns say which ones a golden trace already covers.

    python3 handler_map.py --src <port tree> --md out.md --json out.json trace.jsonl...
"""
import argparse, collections, json, os, re, sys

ENUM = re.compile(r"^\s*(CAMPAIGN_[A-Z0-9_]+)\s*=\s*(0x[0-9A-Fa-f]+)\s*,?\s*(?:/\*\s*(.*?)\s*\*/)?")
CASE = re.compile(r"case\s+(CAMPAIGN_[A-Z0-9_]+)\s*:")
CALL = re.compile(r"\b(Mw[A-Za-z0-9_]+|Btech[A-Za-z0-9_]+)\s*\(")
RETURN_KEYS = {"Return", "Keypad_Enter"}  # script tokens that activate the selected row


def read_names(path):
    names = {}
    for line in open(path, encoding="utf-8"):
        m = ENUM.match(line)
        if m:
            names.setdefault(int(m.group(2), 16), []).append((m.group(1), m.group(3) or ""))
    return names


def read_dispatch(path):
    """case label -> the port function the dispatcher calls (or a note)."""
    text = open(path, encoding="utf-8").read()
    # Blank comments out, keeping every newline so line numbers stay true.
    blank = lambda m: re.sub(r"[^\n]", " ", m.group(0))
    text = re.sub(r"/\*.*?\*/|//[^\n]*", blank, text, flags=re.S)
    start = text.index("switch (codeOffset)")
    depth, i = 0, text.index("{", start)
    for j in range(i, len(text)):
        depth += {"{": 1, "}": -1}.get(text[j], 0)
        if depth == 0:
            body = text[i + 1:j]
            break
    line_of = lambda pos: text.count("\n", 0, i + 1 + pos) + 1
    table, pending = {}, []
    for m in re.finditer(r"\s*(case\s+CAMPAIGN_[A-Z0-9_]+\s*:|default\s*:|[^;]*;)", body):
        piece = m.group(1)
        if piece.startswith("case"):
            pending.append((CASE.match(piece).group(1), line_of(m.start(1))))
            continue
        if piece.startswith("default"):
            pending = []
            continue
        if not pending:
            continue
        calls = [c for c in CALL.findall(piece) if not c.startswith("MW_")]
        if calls:
            for name, line in pending:
                table.setdefault(name, (calls[0], line))
        elif re.search(r"\breturn\b", piece):
            for name, line in pending:
                table.setdefault(name, ("(returns to MwRunCampaign)", line))
        if re.search(r"\bbreak\b|\breturn\b", piece):
            pending = []
    return table


def read_uses(root, names):
    """Port files (other than the enum) that name each offset."""
    uses = collections.defaultdict(set)
    wanted = {n for entries in names.values() for n, _ in entries}
    word = re.compile(r"\bCAMPAIGN_[A-Z0-9_]+\b")
    for base, _, files in os.walk(os.path.join(root, "src")):
        for name in files:
            if not name.endswith((".c", ".cpp", ".h")) or name == "campaign_continuations.h":
                continue
            path = os.path.join(base, name)
            for hit in set(word.findall(open(path, encoding="utf-8", errors="replace").read())) & wanted:
                uses[hit].add(name)
    return uses


def selected_row(record):
    return record.get("fields", {}).get("campaign.row")


def read_traces(paths):
    seen = collections.defaultdict(lambda: {"screens": set(), "records": 0, "activated": 0, "traces": set()})
    for path in paths:
        trace = os.path.basename(path).rsplit(".", 1)[0]
        records = [json.loads(line) for line in open(path, encoding="utf-8") if line.strip()]
        for before, after in zip(records, records[1:]):
            # The first key after a snap acts on the screen that snap shows.
            keys = [k for k in after.get("keys", []) if not k.startswith(("idle:", "random:"))]
            row = selected_row(before)
            menu = {r["index"]: r for r in before.get("menu", [])}
            if keys and keys[0] in RETURN_KEYS and row in menu and int(menu[row]["callback"], 16):
                seen[int(menu[row]["callback"], 16)]["activated"] += 1
        for record in records:
            for row in record.get("menu", []):
                if int(row["callback"], 16) == 0:
                    continue  # an empty row: no handler
                entry = seen[int(row["callback"], 16)]
                entry["screens"].add(record["screenKey"])
                entry["records"] += 1
                entry["traces"].add(trace)
    return seen


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--src", required=True, help="port source tree (has src/include/mechwarrior)")
    ap.add_argument("--md")
    ap.add_argument("--json")
    ap.add_argument("traces", nargs="*")
    args = ap.parse_args()

    names = read_names(os.path.join(args.src, "src/include/mechwarrior/campaign_continuations.h"))
    ui = os.path.join(args.src, "src/Original/Campaign/MW_MAIN_081B_UI")
    dispatch = read_dispatch(next(ui + ext for ext in (".cpp", ".c") if os.path.exists(ui + ext)))
    seen = read_traces(args.traces)
    uses = read_uses(args.src, names)

    rows = []
    for offset in sorted(set(names) | set(seen)):
        named = names.get(offset, [("?", "")])
        handlers = sorted({dispatch[n][0] for n, _ in named if n in dispatch})
        lines = sorted({dispatch[n][1] for n, _ in named if n in dispatch})
        s = seen.get(offset)
        rows.append({
            "offset": f"081B:{offset:04X}",
            "names": [n for n, _ in named],
            "note": next((note for _, note in named if note), ""),
            "handler": handlers,
            "dispatchLine": lines,
            "usedIn": sorted({f for n, _ in named for f in uses.get(n, ())}),
            "screens": len(s["screens"]) if s else 0,
            "records": s["records"] if s else 0,
            "activated": s["activated"] if s else 0,
            "traces": sorted(s["traces"]) if s else [],
        })

    dispatched = sum(1 for r in rows if r["handler"])
    traced = sum(1 for r in rows if r["records"])
    activated = sum(1 for r in rows if r["activated"])
    unknown = [r["offset"] for r in rows if r["names"] == ["?"]]
    summary = (f"{len(rows)} offsets: {dispatched} dispatched by the menu loop, {traced} seen on a traced menu, "
               f"{activated} activated by a trace" + (f"; not named by the port: {', '.join(unknown)}" if unknown else ""))
    print(summary)

    if args.json:
        with open(args.json, "w", encoding="utf-8") as f:
            json.dump({"summary": summary, "traces": [os.path.basename(p) for p in args.traces], "offsets": rows}, f, indent=1)
            f.write("\n")
    if args.md:
        with open(args.md, "w", encoding="utf-8") as f:
            f.write("# Menu-handler map\n\n")
            f.write("Generated by `tools/trace/handler_map.py`; do not edit by hand. Offsets are\n"
                    "MW_MAIN.EXE code addresses in segment 081B, as a menu row's activate\n"
                    "callback carries them. Names and notes are the port's\n"
                    "(`campaign_continuations.h`); the handler is the port function that\n"
                    "`MwCampaignUiEventLoop` (`MW_MAIN_081B_UI.c`) runs for the offset.\n\n")
            f.write(summary + ".\n\n")
            if args.traces:
                f.write("Traces: " + ", ".join(f"`{os.path.basename(p)}`" for p in args.traces) + ".\n\n")
            f.write("| Offset | Port name | Port handler | Used in | Screens | Activated | Note |\n|---|---|---|---|---:|---:|---|\n")
            for r in rows:
                handler = ", ".join(f"`{h}`" for h in r["handler"]) or "—"
                note = r["note"].replace("|", "\\|")
                f.write(f"| {r['offset']} | {'<br>'.join(n.replace('CAMPAIGN_', '') for n in r['names'])} | {handler} | "
                        f"{', '.join(r['usedIn'])} | {r['screens'] or ''} | {r['activated'] or ''} | {note} |\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
