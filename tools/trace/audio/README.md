# Sound-port goldens

Level 1 of the Phase 5 audio plan (`docs/phase5/AUDIO_PLAN.md`): an exact log
of what the game sends to the sound hardware, recorded from the port before
the synthesizers change. Every later audio slice must leave these logs
identical.

    python3 tools/trace/audio.py check  --port build/mechwarrior --game ~/MECHWAR --work ~/mw1-audio
    python3 tools/trace/audio.py record --port build/mechwarrior --game ~/MECHWAR --work ~/mw1-audio

`--only music` or `--only combat.adlib` limits the run. Exit status: 0 all
match, 1 a mismatch, 77 skipped. On macOS set `MW1O_HIDDEN=1`, as for
`check.py`.

## What runs

Each script in `scripts/` runs once per device (speaker, adlib, mt32) through
`record.py`, with three switches:

- `MECHWARRIOR_AUDIO_LOG=FILE`: every access to the speaker (42h, 43h, 61h),
  PIT channel 0 (40h), AdLib (388h, 389h) and MPU (330h, 331h) ports, plus
  ports 0 and 1. The rhythm note-off quirk writes there; see
  `SDLBackend_Time.cpp`. Each line is one JSON array:
  - `[PIT cycle, "w", port, value]` for a write;
  - `[PIT cycle, "r", port, value]` for a read, with the value returned;
  - `[PIT cycle, "snap", name]` for each snap.

  Latched PIT counter reads aren't logged; they're timing, not sound.
- `MECHWARRIOR_DEBUG_VIRTUAL_CLOCK=1`: the port's PIT time normally follows
  the wall clock. Under this switch, time moves only where the port would
  wait:
  - CPU-budget deadlines;
  - idle pumps, at 100 µs;
  - port reads, at 1 µs, so timing loops that count reads still see time
    pass.

  The log then depends on the script alone. Game logic never reads the
  clock, so the snaps match the normal goldens: combat_cockpit matches under
  this clock on all three devices.
- MT-32 with no ROM paths: while logging, the MPU and its byte stream run as
  usual and the synthesizer is silent. No ROMs are needed to record or check.

The log is cut after the last snap, because what follows is the shutdown,
timed by the harness. It's stored as `traces/<script>.<device>.jsonl.gz`.

## The scripts

- `music.keys`: the title tune for about 36 s, then the hub and the bar's
  tune for about 36 s. `idle:60000` is about 18 s of test-clock time on these
  screens.
- `combat.keys`: a copy of `../scripts/combat_cockpit.keys` (every weapon
  class, the HUD keys, pause, retreat, jump, inspection, results). It's a
  copy so that its golden moves only when this file does.

The logs hold port numbers and values only, no game text or assets.

## PCM hashes (`--pcm`)

`MECHWARRIOR_AUDIO_PCM=FILE` writes every rendered frame as raw 32-bit floats
at 48 kHz: one channel with the speaker alone, two with a card. `--pcm`
turns it on and prints each run's SHA-256 up to its last snap. It's for
comparing synthesis by hand before and after a change; for example, slice 2
moved the speaker and the mixer and left all six hashes identical.

PCM hashes aren't goldens in the repo. Floating-point synthesis (`sin`,
`fmod`) can differ between C libraries, so a hash is only comparable on one
machine. The level 2 metrics (onsets, pitch, envelopes) are what cross
platforms.
