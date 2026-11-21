# Replaying the golden traces

`tools/trace/traces/*.jsonl` record what the original does for each key
script in `tools/trace/scripts/`. A candidate (a port build, or the rebuild)
runs the same scripts, and `tools/trace/check.py` compares its records with
the golden ones, snap by snap. This is what the rebuild must do to take part.
The approach is crescent-hawks-rebuild's (its `docs/reference.md` and
`tools/reference/check.py`), so both rebuilds are checked the same way.

## Running the check

    python3 tools/trace/check.py --port build/mechwarrior --game ~/MECHWAR --work ~/mw1-check
    python3 tools/trace/check.py --rebuild build/mw1 --game ~/MECHWAR --work ~/mw1-check
    python3 tools/trace/check.py --traces DIR        # traces recorded elsewhere

`--only NAME...` limits it to some traces and `--quiet` prints only the
failing snaps. Exit status: 0 everything matches, 1 a mismatch, 77 skipped
(no candidate or no game folder). A port build checked this way is the
regression test for any change that must not alter behaviour, such as the
C++20 conversion.

## The rebuild's side

    mw1 --game-dir DIR --save-dir DIR --keys SCRIPT --trace OUT.jsonl

It starts a new campaign headless, reads the script, and writes one JSON line
per `@snap` to OUT.jsonl, then exits 0.

**The keys.** Tokens are SDL key names, `idle:N`, `@snap:NAME` and
`random:WWWWWWWW` (`tools/trace/README.md`). The rebuild feeds them as the
port's `feed.txt` does (`docs/DEBUG_INPUT.md`):

- a key goes in only when the game waits for one: on a blocking read, or on
  the 4th empty "key waiting?" poll since the last key read;
- `idle:N` adds N empty polls before the next item; a blocking read skips it;
- `@snap` is taken at the same moment a key would go in;
- `random:` sets the two random words.

The original draws one random number on every pass of its wait-for-a-key loop
(MW_MAIN 081B:02FB). The rebuild keeps that draw in its wait loop, so the same
polls give the same draws. Startup mixes the BIOS clock into the random words
once (081B:0C0A); the traces pin that clock at 0x10000, and so must the
rebuild.

**A record** (`check.py` compares `frame` and `state`; the rest is for people):

    {"snap": "hub", "keys": ["Return"], "state": [...], "frame": {"indices": "...", "palette": "..."}}

- `frame.indices`: SHA-256, first 32 hex digits, of the displayed 320x200
  frame as 64,000 EGA colour indices (0-15), one byte per pixel, rows top to
  bottom. `frame.palette`: the same hash of the 17 palette bytes (16 EGA
  palette registers, then the overscan register), as 6-bit EGA colour codes.
- `state`: these lines, in this order:

      state random W0 W1           the two random words, 4 hex digits each
      state campaign planet P contract C category M scenario S destination D house H payment K result R
      state funds F months N       C-Bills; billing months elapsed
      state row R                  the selected menu row
      state combat 0|1
      state menu I enabled E callback XXXX picture P links U D L R label HASH   (one per row)
      state exchange HASH          the save image's SHA-256 (32 hex)

  `callback` is the original's 081B code offset the row activates (the rebuild
  keeps them as identifiers, `docs/phase0/HANDLER_MAP.md`). `label` is the
  first 12 hex digits of the SHA-256 of the row's text bytes, as the
  original holds them, up to the terminating zero (at most 512); `-` for a row
  without text. `exchange` hashes the campaign
  as the save format encodes it (0x718 bytes, MW_MAIN_SAVE_FORMAT.c), so the
  rebuild's saves must keep that format byte for byte.

## Known differences

`tools/trace/known/<trace>.json` lists differences that are understood and
not yet fixed, so they are reported rather than failed:

    {"hub": {"frame": "the rebuild draws the clock later", "state": {"state menu 3": "why"}}}

`state` keys are line prefixes. Each entry should name its cause and go when
it is fixed.
