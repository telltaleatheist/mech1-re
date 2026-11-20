# Golden traces (FIXES 130 Phase 0)

The rebuild has to behave like the original. These tools record how the port
(which runs the original's logic) responds to scripted keys, so the rebuild's
tests can replay the same keys and compare.

| File | What it does |
|---|---|
| `record.py` | Runs the port headless, feeds a key script through the debug-control directory (`docs/DEBUG_INPUT.md`), writes one JSON line per command. |
| `scripts/*.keys` | Key scripts. `campaign_contract.keys`: a new campaign to an accepted contract. |
| `traces/*.jsonl` | Recorded traces, the golden references. |
| `handler_map.py` | The menu-handler map: every original 081B code offset a menu row can carry, the port's name and handler for it, and which traces cover it. Writes `docs/phase0/HANDLER_MAP.md`. |
| `frames.py` | Turns recorded frames into PNGs for a person to look at. |

## A record

One line per command, taken once the game is polling for its next key (the
recorder waits for that, then sends an empty command to capture the screen):

- `keys`: the steps sent, `[ms, [SDL key names]]`.
- `keyReads`: each key the game read for them, `[BIOS key word, random word 0,
  random word 1]`, with the random words as they stood at the read.
- `campaign`: planet, contract fields, selected row and the random words now.
- `menu`: each row's `callback` (the original 081B code offset it activates),
  `picture`, `links` (up, down, left, right), `enabled`, and `label`, a 12-hex
  SHA-256 of the row's text. MW1's own text never enters a trace.
- `screenKey`: a hash of the menu's shape, to tell screens apart.
- `frame`: SHA-256 (32 hex) of the indexed 320x200 frame and of its palette,
  plus the video page fields. The frames themselves are MW1 art: the recorder
  copies them to `--frames`, which must be outside the repo.

## Why traces carry random words, not a seed

The original draws a random number on every pass of its wait-for-a-key loop
(MW_MAIN 081B:02FB), so its random state follows how long the player took.
Two recordings of the same script take the same screens and rows, but they
differ in everything the random words decide (the starting planet, offers,
frames). A replay must therefore set the random words from `keyReads` before
feeding each key, and compare from there. `--bios-ticks` pins the one startup
clock read so boot is the same too.

## Running

Linux or WSL, with a port build that has the debug-control fields
(`input`, `campaign.random`) and a copy of the game folder (it is only read):

    python3 tools/trace/record.py --exe build/mechwarrior --game ~/MECHWAR \
        --script tools/trace/scripts/campaign_contract.keys \
        --out tools/trace/traces/campaign_contract.jsonl --frames ~/mw1-frames
    python3 tools/trace/handler_map.py --src . --md docs/phase0/HANDLER_MAP.md \
        --json docs/phase0/handler_map.json tools/trace/traces/*.jsonl
    python3 tools/trace/frames.py ~/mw1-frames ~/mw1-frames/png --sheet ~/mw1-frames/sheet.png

It uses SDL's offscreen video and dummy audio: no window, no sound.

## Limits

- Campaign only. Combat needs the 174,267-byte BTECH.EXE. With any other size
  the first combat falls into the original's disk prompt.
- A key script is fixed: if a screen gains or loses rows, the script needs
  editing. Check `screenKey` and `selectedRow` in a fresh recording.
