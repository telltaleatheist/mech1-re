# Golden traces (FIXES 130 Phase 0)

The rebuild has to behave like the original. These tools record how the port
(which runs the original's logic) responds to a key script, so the rebuild's
tests can run the same script and compare.

| File | What it does |
|---|---|
| `record.py` | Runs the port headless with a poll-fed key script (`feed.txt`, `docs/DEBUG_INPUT.md`) and writes one JSON line per `@snap`. |
| `check.py` | Compares a candidate (a port build, the rebuild, or recorded traces) with the golden traces, snap by snap (`docs/phase0/REPLAY.md`). |
| `exe_free.py` | Runs the traces on a copy of the game folder with its exes, then with every `*.EXE` deleted, and checks the two runs are the same snap by snap (ctest `exe_free`; `docs/phase2/PLAN.md` 8.5, 6f). |
| `state.py` | Turns MWSNAP's captures of the original under DOSBox-X (`tools/capture/`) into a trace in the same format, so `check.py --traces` compares the original with the goldens. |
| `run.py` | Runs the golden scripts against the original under DOSBox-X (`tools/capture/`: keys.py, MWSNAP, run_wsl.sh), turns the captures into traces with `state.py`, and compares them with `check.py --traces`. |
| `repeat.py` | Records a script twice and checks the runs are byte-identical. |
| `scripts/*.keys` | Key scripts, each from a new campaign: `campaign_contract` (an offer accepted), `company` (crew, a pilot assigned, NewsNet), `mech_complex` (component repairs, repair all, rearm, reserve ammo bought and sold, the market), `bar` (drinks, a recruit hired and one rejected), `travel` (the house browser, a trip), `options` (save, audio, terrain detail, the load menu). |
| `traces/*.jsonl` | Recorded traces, the golden references. |
| `handler_map.py` | The menu-handler map: every original 081B code offset a menu row can carry, the port's name and handler for it, and which traces cover it. Writes `docs/phase0/HANDLER_MAP.md`. |
| `frames.py` | Turns recorded frames into PNGs for a person to look at. |

## Scripts

The same approach as crescent-hawks-rebuild's reference rig (its
`docs/reference.md`), so both rebuilds replay the same way:

    @snap:title
    Return   @snap:arrival     # a key, then the screen the game next waits on
    idle:20  Down @snap:x      # 20 more empty polls before the key

Tokens: SDL key names, `idle:N`, `@snap:NAME`, `random:WWWWWWWW` (sets the two
random words, for a side branch). '#' comments run to the end of the line.

In combat the game reads keys once per tick, and a key is a press and a
release. `make:KEY` and `break:KEY` press or release only (held keys steer and
fire), and `ticks:N` lets N ticks pass:

    Return
    ticks:0 @snap:briefing     # ticks: waits for combat; the briefing map
    Return ticks:10 @snap:cockpit
    make:Up ticks:40 break:Up ticks:20 @snap:walking
    Escape ticks:5 Y           # retreat
    idle:0 @snap:results       # idle: waits for the campaign again

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

  In combat the menu lines give way to the battle:

      state tick 88 screen 0 target 0
      state unit 0 side 0 chassis 1 position X Y Z orientation A B C step 66 mode 0 jump 800 torso A B C
      state unit 0 heat 229 engine 0 gyro 0 cockpit 0 armor ... internal ... ammo ... weapons 1/0 1/29 ...
      state objective position X Y Z damage D      (when the mission has one)

  `callback` is the original 081B code offset the row activates. `label` is a
  12-hex HMAC-SHA-256 of the row's text, keyed by the SHA-256 of MW_MAIN.EXE's
  key region (081B:0100, 0x1400 bytes, as the import table's hashes) in the
  `--game` folder: MW1's own text never enters a trace, and a short label
  can't be found from its hash by trying a dictionary. A row with a label
  prints `picture -`: its picture word is only where the label lives in the
  original's data segment.
- `menu`: the same rows as data; `screenKey`: a hash of the menu's shape
  (each row's callback, picture, links and label hash).
- `frame`: SHA-256 (32 hex) of the indexed 320x200 frame and of its palette.
  The frames are MW1 art: the recorder copies them to `--frames`, outside the
  repo.

The rebuild's test runs the same script, prints the same `state` lines and
frame hashes at each snap, and compares.

## Without the exes (the gate's step, slice 6f)

`exe_free.py` runs the 8 traces on a copy of a game folder with its exes,
deletes every `*.EXE` and `*.COM` from the copy, runs them again from the
first run's story and compares the two runs snap by snap; it fails, naming
the file and the opener, if play opens a program. It is opt-in
(`MW1O_EXE_FREE=1`, minutes rather than seconds) and the gate runs it once
per folder, from the repository root after a build in `build-fresh`, with
`GAME` the game folder and `WORK` a scratch directory outside the repo:

    MW1O_EXE_FREE=1 MW1O_HIDDEN=1 nice -n 10 python3 -I tools/trace/exe_free.py \
        --port build-fresh/mechwarrior --game "$GAME" --work "$WORK/exe_free"

It ends with `exe-free: 8 of 8 traces as with the exes`. ctest's `exe_free`
runs the same with `MW1O_EXE_FREE=1` and `MW1O_GAME` set
(`MW1O_EXE_FREE=1 MW1O_GAME="$GAME" ctest -R exe_free`).

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

## The original

`state.py` reads a run of MWSNAP captures (`MW1Snnnn.BIN`, in the script's
`@snap` order) and writes the trace the port would have written: the frame
from the four planes and the attribute registers, and the state from
MW_MAIN's data segment (the menu table at 03E3, the row at 03E5, the random
words at 03F5/03F7, the 0x718-byte exchange block at 0448, which is the
save format byte for byte). In combat the state is BTECH's (tick D244, screen
0E1C, target CD36, the unit bindings at CD5A and controls at D374, the
objective at CD2E/CD30) and the campaign is the exchange block MW.EXE keeps
for BTECH; the random words and row are those MW_MAIN left. Then
`check.py --traces` compares it with the goldens:

    python3 tools/trace/state.py --script tools/trace/scripts/bar.keys \
        --run ~/mwsnap/run-bar --game ~/mwsnap/game --out ~/mw1-orig/bar.jsonl
    python3 tools/trace/check.py --traces ~/mw1-orig --only bar

or all of it in one command, on Linux or WSL with DOSBox-X and nasm, and a
copy of the game in `~/mwsnap/game`:

    python3 tools/trace/run.py --home ~/mwsnap --out ~/mw1-orig

## How to check against the original

On Linux or WSL, with DOSBox-X, nasm and a copy of the game (1990-01-22
BTECH.EXE) in `~/mwsnap/game`:

    python3 tools/trace/run.py --home ~/mwsnap --out ~/mw1-orig

It runs every golden script against the original, headless and one run at a
time (about 3.5 minutes for all seven), and prints check.py's verdict; exit 0
means every frame and state line matches. `--dosbox-x PATH` names an unpacked
DOSBox-X (set `LD_LIBRARY_PATH` to its library folder). Last run (main
3b226e8): 7 of 7, no MWSNAP flags.

## Limits

- Combat runs with the 1990-01-22 BTECH.EXE (174,533 bytes), which the
  port loads since upstream 621961a. Game files are found whatever their
  case, as DOS finds them, so an upper-case install works on Linux too.
- A key script is fixed: if a screen gains or loses rows, the script needs
  editing. Check `state row` and `screenKey` in a fresh recording.
