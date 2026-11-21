# Golden traces (FIXES 130 Phase 0)

The rebuild has to behave like the original. These tools record how the port
(which runs the original's logic) responds to a key script, so the rebuild's
tests can run the same script and compare.

| File | What it does |
|---|---|
| `record.py` | Runs the port headless with a poll-fed key script (`feed.txt`, `docs/DEBUG_INPUT.md`) and writes one JSON line per `@snap`. |
| `check.py` | Compares a candidate (a port build, the rebuild, or recorded traces) with the golden traces, snap by snap (`docs/phase0/REPLAY.md`). |
| `repeat.py` | Records a script twice and checks the runs are byte-identical. |
| `scripts/*.keys` | Key scripts, each from a new campaign: `campaign_contract` (an offer accepted), `company` (crew, a pilot assigned, NewsNet), `mech_complex` (component repairs, repair all, rearm, reserve ammo bought and sold, the market), `bar` (drinks, a recruit hired and one rejected), `travel` (the house browser, a trip), `options` (save, audio, terrain detail, the load menu). |
| `traces/*.jsonl` | Recorded traces, the golden references. |
| `handler_map.py` | The menu-handler map: every original 081B code offset a menu row can carry, the port's name and handler for it, and which traces cover it. Writes `docs/phase0/HANDLER_MAP.md`. |
| `frames.py` | Turns recorded frames into PNGs for a person to look at. |

## Scripts

The same approach as crescent-hawks-rebuild's reference rig (its
`docs/reference.md`), so both rebuilds replay the same way:

    @snap:title
    Return   @snap:opening     # a key, then the screen the game next waits on
    idle:20  Down @snap:x      # 20 more empty polls before the key

Tokens: SDL key names, `idle:N`, `@snap:NAME`, `random:WWWWWWWW` (sets the two
random words, for a side branch). '#' comments run to the end of the line.

Each key reaches the game only when it is waiting for one (a blocking read,
or the 4th empty "key waiting?" poll since the last read). The original draws
a random number on every pass of its wait-for-a-key loop (081B:02FB), so
counting polls rather than milliseconds makes every draw repeat. With the
startup clock pinned too (`--bios-ticks`), a run is byte-identical every time,
and no host speed or load changes it.

## A record

- `snap`: its name; `keys`: the tokens since the previous snap.
- `state`: the compared state, one fact per line, in the style of
  crescent-hawks-rebuild's `chr --state`:

      state random 70a0 7653
      state campaign planet 11 contract 1 category 2 scenario 11 destination 142 house 4 payment 120 result 0
      state row 2
      state combat 0
      state menu 0 enabled 1 callback 247a picture 1 links 5 1 255 3 label 3f1c...

  `callback` is the original 081B code offset the row activates. `label` is a
  12-hex SHA-256 of the row's text: MW1's own text never enters a trace.
- `menu`: the same rows as data; `screenKey`: a hash of the menu's shape.
- `frame`: SHA-256 (32 hex) of the indexed 320x200 frame and of its palette.
  The frames are MW1 art: the recorder copies them to `--frames`, outside the
  repo.

The rebuild's test runs the same script, prints the same `state` lines and
frame hashes at each snap, and compares.

## Running

Linux or WSL, with a port build that has the feed (`docs/DEBUG_INPUT.md`) and
a copy of the game folder (it is only read). SDL's offscreen video and dummy
audio: no window, no sound. A script to the first contract takes about two
seconds.

    python3 tools/trace/record.py --exe build/mechwarrior --game ~/MECHWAR \
        --script tools/trace/scripts/campaign_contract.keys \
        --out tools/trace/traces/campaign_contract.jsonl --frames ~/mw1-frames
    python3 tools/trace/repeat.py --exe build/mechwarrior --game ~/MECHWAR \
        --script tools/trace/scripts/campaign_contract.keys --work ~/mw1-repeat
    python3 tools/trace/handler_map.py --src . --md docs/phase0/HANDLER_MAP.md \
        --json docs/phase0/handler_map.json tools/trace/traces/*.jsonl
    python3 tools/trace/frames.py ~/mw1-frames ~/mw1-frames/png --sheet ~/mw1-frames/sheet.png

## Limits

- Campaign only. Combat needs the 174,267-byte BTECH.EXE. With any other size
  the first combat falls into the original's disk prompt.
- A key script is fixed: if a screen gains or loses rows, the script needs
  editing. Check `state row` and `screenKey` in a fresh recording.
