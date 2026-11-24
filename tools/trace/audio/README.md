# The tone log's goldens

These were the sound-port goldens: the port lines left with S8 and S9, the
host's port bus with H5. What remains, the tones the host plays and the snaps,
is the tone log. `audio.py`, `tools/trace/audio` and `MECHWARRIOR_AUDIO_LOG`
keep their names.

Level 1 of the Phase 5 audio plan (`docs/phase5/AUDIO_PLAN.md`): an exact log
of what the game sends to the sound hardware, recorded from the port before
the synthesizers change. Every later audio slice must leave these logs
identical. The combat logs were re-recorded once, when the combat tick became
a fixed 60 ms (`docs/timing/PLAN.md`): they match the old ones up to the
first cockpit tick, and the music logs did not change.

    python3 tools/trace/audio.py check  --port build/mechwarrior --game ~/MECHWAR --work ~/mw1-audio
    python3 tools/trace/audio.py record --port build/mechwarrior --game ~/MECHWAR --work ~/mw1-audio

`--only music` limits the run. Exit status: 0 all
match, 1 a mismatch, 77 skipped. On macOS set `MW1O_HIDDEN=1`, as for
`check.py`.

## What runs

Each script in `scripts/` runs on the speaker through `record.py`. Until S8 it
also ran on AdLib and MT-32; those paths left the game, and their four goldens
with them (they are in the history up to 6aebf22). It runs with two switches:

- `MECHWARRIOR_AUDIO_LOG=FILE`: each change of the tone and each snap. The
  port lines left with the ports (S9). Each line is one JSON array:
  - `[PIT cycle, "tone", Hz]` for a tone's start, change or stop (0);
  - `[PIT cycle, "snap", name]` for each snap.
- `MECHWARRIOR_DEBUG_VIRTUAL_CLOCK=1`: the port's PIT time normally follows
  the wall clock. Under this switch, time moves only where the port would
  wait:
  - each combat tick's fixed length (`combat_tick_ms` ÷ speed);
  - idle pumps, at 100 µs;
  - port reads, at 1 µs, so timing loops that count reads still see time
    pass.

  The log then depends on the script alone. Game logic never reads the
  clock, so the snaps match the normal goldens: combat_cockpit matches under
  this clock.

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

## The original under DOSBox-X (level 3)

`tools/capture/run_audio_wsl.sh NAME KEYS.BIN A|R|S [LIMIT [HOLD]]` runs a
script against the original headless and captures what it sends: AdLib as
DROv2 (`DX-CAPTURE /O`; no mapper key needed), Roland MIDI through DOSBox-X's
OSS handler into a plain file, the speaker as WAV (`DX-CAPTURE /A`).
`compare_original.py` compares a capture with a golden section by section:

- AdLib: key-ons, channel-free, with onset timing;
- MIDI: bytes;
- speaker: notes from the WAV's square wave (semitones, onset timing). A note
  under 5 periods on one side only is skipped and counted, since the WAV
  can't always show one that brief; every longer note must match in order.

The original's empty polls take almost no emulated time, so `idle:` can't
hold a screen there. `wait:MS` (MWSNAP's own; the port's feed doesn't read
it) holds the next item for emulated time, by the BIOS tick count, which the
game's music timer keeps running. `original/music.keys` is the port's music
script with its idles as waits.

Results (`original/music.keys` against `traces/music.*`):

- AdLib: title 361 of 361 key-ons, bar 659 of 659, in order; onset gaps
  median 0 ms, at most 8 ms.
- MIDI: title 3,003 of 3,003 bytes, bar 4,730 of 4,730.
- Speaker: title 66 of 67 notes (one short note on one side only), bar 219
  of 219; onset gaps median 0 ms, at most 14 ms.
- Combat: the original sends no MIDI and keys no OPL note, as the plan
  assumes (the port's combat logs: 0 key-ons; only MPU commands).

Leaving the title early: the title's length decides the start world (the
campaign draws a random number on every poll), and leaving before the title
tune starts gives the bar other music. The original and the port agree on
it: titles of 0.5 to 6 s give the port's own title-early run (`idle:1000`)
exactly, AdLib 569 of 569 key-ons (within 7 ms) and MIDI 4,698 of 4,698
bytes. What looked like a stall in the original was this other music
measured against the full-title golden.
