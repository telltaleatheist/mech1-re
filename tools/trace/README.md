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
| `events.py` | Records or checks the game-event goldens: the sequencer's notes, combat's tones, sound requests, key reads and polls, per script, under the test clock (`events/README.md`; `docs/phase5/NO_EMULATION_PLAN.md`, T1). The sound scripts also run with `music=soundfont`, which must log exactly what MT-32 logs. |
| `listening.py` | Renders every song to WAV files, as the rebuild plays it (`music=soundfont`) and beside the speaker version, with a one-page listening sheet. Headless; nothing plays aloud (`listening/songs.keys`; `docs/phase5/NO_EMULATION_PLAN.md`, S3). |
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
- `fields`: the compared state by name, an ordered object (`schema.py`):

      "campaign.random": [28832, 30291], "campaign.planet": 11, "campaign.contract": 1,
      "campaign.category": 2, ... "campaign.funds": 1000000, "campaign.row": 2, "combat": 0,
      "menu.0.enabled": 1, "menu.0.callback": 9338, "menu.0.picture": "-",
      "menu.0.links": [5, 1, 255, 3], "menu.0.label": "3f1c...",
      "exchange.currentPlanetIndex": 11, "exchange.campaignFundsLow": 16960, ...

  In combat the menu gives way to the battle: `combat.tick`, `combat.screen`,
  `combat.target`; `unit.S.side`, `.chassis`, `.position`, `.orientation`,
  `.step`, `.mode`, `.jump`, `.torso`, `.heat`, `.engine`, `.gyro`,
  `.cockpit`, `.armor`, `.internal`, `.ammo` and `.weapons` (operational and
  reload) for each unit on the field; and `objective.position` and
  `objective.damage` when the mission has one.

  On a campaign snap the fields end with the campaign as the save format
  encodes it (0x718 bytes, `MW_MAIN_SAVE_FORMAT.cpp`), by its own fields:
  `exchange.<name>`, named as `MwEncodeCampaignExchange` names them. Words
  are numbers or lists; byte arrays are hex; the salvage and unit-damage
  arrays are one hex field per record (`exchange.ownedUnitDamage.3`). The
  ctest `snap_schema` (`test_schema.py`) checks the schema against the
  encoder, statement by statement, and that its fields cover every byte.

  `callback` is the original 081B code offset the row activates. `label` is a
  12-hex HMAC-SHA-256 of the row's text, keyed by the SHA-256 of MW_MAIN.EXE's
  key region (081B:0100, 0x1400 bytes, as the import table's hashes) in the
  `--game` folder: MW1's own text never enters a trace, and a short label
  can't be found from its hash by trying a dictionary. A row with a label
  has `picture` "-": its picture word is only where the label lives in the
  original's data segment. A field that can hold text (`schema.HASHED`, the
  commander's name) is the same keyed hash, never its bytes.
- `menu`: the same rows as data; `screenKey`: a hash of the menu's shape
  (each row's callback, picture, links and label hash).
- `frame`: SHA-256 (32 hex) of the indexed 320x200 frame (`indices`), of the
  colours it shows (`rgb`: 17 RGB triples, the 16 colours and the border) and
  of its raw palette codes (`palette`, kept but not compared). check.py
  compares `indices` and `rgb`. On the original's side, state.py turns the
  codes into colours by `palette.py`'s own mode 0Dh rule, which the ctest
  `palette_rule` checks against the port's on every code.
  The frames are MW1 art: the recorder copies them to `--frames`, outside the
  repo.

The rebuild's test runs the same script, writes the same fields and frame
hashes at each snap, and compares. check.py compares field by field: a field
that differs, or is missing or extra, fails by its name, with the first
element or byte that differs (and, in a record, the record's field):

    FAIL the fields: 1 differ
      exchange.ownedUnitDamage.0 byte 6 (actuatorDamageTiers[1]): golden 0x00 ours 0x02

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

## The real-clock check (not a gate)

`record.py` runs on the test clock, so the gate's snaps don't depend on how
fast the host draws. Whether real time still gives the goldens is a separate
check, run by hand after a change to the host's timing or waits (as H4d did):

    python3 tools/trace/check.py --port build/mechwarrior --game ~/MECHWAR \
        --work ~/mw1-realclock --only combat_cockpit bar --real-clock

It can fail by speed alone. combat_cockpit's deploy snap is taken while the
hired transport's lift-off waits on the timer, so a slow build (Debug, or a
sanitizer build) can snap it a frame further on. A failure there is a
question to look into, not a regression by itself; the test-clock run decides.

## The original

`state.py` reads a run of MWSNAP captures (`MW1Snnnn.BIN`, in the script's
`@snap` order) and writes the trace the port would have written: the frame
from the four planes and the attribute registers, and the fields from
MW_MAIN's data segment (the menu table at 03E3, the row at 03E5, the random
words at 03F5/03F7, the 0x718-byte exchange block at 0448, which is the
save format byte for byte, so `schema.py`'s one decoder gives it the port's
`exchange.*` names). In combat the state is BTECH's (tick D244, screen
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
means every frame and field matches. `--dosbox-x PATH` names an unpacked
DOSBox-X (set `LD_LIBRARY_PATH` to its library folder). Last run (main
3b226e8): 7 of 7, no MWSNAP flags.

## Limits

- Combat runs with the 1990-01-22 BTECH.EXE (174,533 bytes), which the
  port loads since upstream 621961a. Game files are found whatever their
  case, as DOS finds them, so an upper-case install works on Linux too.
- A key script is fixed: if a screen gains or loses rows, the script needs
  editing. Check `campaign.row` and `screenKey` in a fresh recording.
