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

**A record** (`check.py` compares `frame` and `fields`; the rest is for people):

    {"snap": "hub", "keys": ["Return"], "fields": {...}, "frame": {"indices": "...", "palette": "..."}}

- `frame.indices`: SHA-256, first 32 hex digits, of the displayed 320x200
  frame as 64,000 EGA colour indices (0-15), one byte per pixel, rows top to
  bottom. `frame.palette`: the same hash of the 17 palette bytes (16 EGA
  palette registers, then the overscan register), as 6-bit EGA colour codes.
- `fields`: the state by name (`tools/trace/schema.py`), in this order:

      campaign.random              the two random words
      campaign.planet, .contract, .category, .scenario, .destination, .house, .payment, .result
      campaign.funds, .months      C-Bills; billing months elapsed
      campaign.row                 the selected menu row
      combat                       0 or 1
      menu.I.enabled, .callback, .picture, .links, .label   (each row; picture
                                   is "-" on a row with a label)
      exchange.<name>              the save image's own fields (campaign snaps)

  In combat the menu gives way to the battle (`tools/trace/README.md`):
  `combat.tick`, `.screen` and `.target`; each unit on the field
  (`unit.S.*`: position, orientation, step, movement mode, jump charge,
  torso; heat, engine, gyro, cockpit, armour, internal structure, ammunition
  and each weapon's operational/reload); and `objective.*` when there is one.
  The combat keys are `make:KEY`, `break:KEY` and `ticks:N`, fed at counted
  combat input polls (0810:2A17), one per tick; combat uses no clock.

  `callback` is the original's 081B code offset the row activates (the rebuild
  keeps them as identifiers, `docs/phase0/HANDLER_MAP.md`). `label` is the
  first 12 hex digits of the HMAC-SHA-256 of the row's text bytes, as the
  original holds them, up to the terminating zero (at most 512), keyed by the
  SHA-256 of MW_MAIN.EXE's key region (081B:0100, 0x1400 bytes, as the import
  table's hashes); `-` for a row without text. The `exchange.*` fields are the
  campaign as the save format encodes it (0x718 bytes, MW_MAIN_SAVE_FORMAT.cpp),
  field by field, so the rebuild's saves must keep that format byte for byte.

## Known differences

check.py reads `tools/trace/known/<trace>.json`, if there is one: differences
that are understood and not yet fixed, reported rather than failed. There are
none now, so the folder is not in the tree. An entry is:

    {"hub": {"frame": "the rebuild draws the clock later", "fields": {"menu.3.": "why"}}}

`fields` keys are field-name prefixes. Each entry should name its cause and go when
it is fixed. The files don't know the game's revision, so the two traces that
differ on the 1989 game are not entries (docs/phase5/NO_EMULATION_PLAN.md says
why they do).
