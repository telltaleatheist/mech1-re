# Game-event goldens

Slice T1 of [the no-emulation plan](../../../docs/phase5/NO_EMULATION_PLAN.md).

These are exact logs of what the game *asked for*, recorded from today's port before the hardware layer under the game is rebuilt:
- the notes the sequencer played;
- the tones combat's effect player chose;
- the sounds and keys it requested, and combat's ticks.

[The sound-port goldens](../audio/README.md) record what the game wrote to the hardware. That changes once the hardware layer is rebuilt, but these logs must not. Every later slice of the plan leaves them identical, apart from one planned exception. H4 removes the port model's time charges, and with them the few sequencer ticks they add between a key and the music (below), so H4 re-records the note ticks once and says so.

    python3 tools/trace/events.py check  --port build/mechwarrior --game ~/MECHWAR --work ~/mw1-events
    python3 tools/trace/events.py record --port build/mechwarrior --game ~/MECHWAR --work ~/mw1-events

`--only campaign_contract audio-music` limits the run, and `--no-devices` skips the AdLib and MT-32 runs.

- **Exit status:** 0 all match, 1 a mismatch, 77 skipped.
- **macOS:** set `MW1O_HIDDEN=1`, as for `check.py`.
- **ctest:** the `events` test runs the check only with `MW1O_EVENTS=1`, since the runs take minutes.

## What runs

Every trace script (`../scripts/*.keys`) and every sound script (`../audio/scripts/*.keys`, named `audio-<script>`) runs through `record.py`, with two switches:

- `MECHWARRIOR_EVENT_LOG=FILE`: the log below.
- `MECHWARRIOR_DEBUG_VIRTUAL_CLOCK=1`: time moves only where the port would wait, so the log depends on the script alone. This is the same clock the sound-port goldens use.

The log is cut after the last snap, since what follows is the shutdown.

The goldens are the speaker's, the default device. The sound scripts also run on AdLib and on MT-32 (no ROMs, silent), and those runs must match the speaker's:
- **Notes**, ticks and snap marks left out, are cut into pieces wherever a key can cut the music: at each song start and at each all-notes-off sweep (a stop). The runs must give the same number of pieces. Within each piece, one run's notes must be a prefix of the other's, because a key, or the last snap, may cut a piece a note or so earlier or later.
- **The tone and game streams** must match exactly.

The ticks, and where a snap falls among the notes, can differ by device. The test clock charges 1 µs for every port read, and AdLib's calibrated reads make more of them, so a key or a snap can arrive some sequencer ticks earlier or later. That cost comes from the port model, not the game, and slice H4 removes it.

## The log

One JSON array per line, after a header line (`src/include/mechwarrior/event_log.h`):

| Line | What it is | Stream |
| --- | --- | --- |
| `["note", tick, kind, channel, a, b]` | One message of the sequencer's output (`MwDispatchMidiByte`), before any device's driver. The kind is on, off, poly, cc, prog, press or bend; a bend gives its 14-bit value as `a` | note |
| `["note", tick, "sysex", length, hash]` | A system-exclusive message, by length and SHA-256 prefix (no song bytes) | note |
| `["song", tick, hash]` | A song started, by the SHA-256 prefix of its track's first 32 bytes | note |
| `["tone", tick, period, on]` | Combat's speaker channel output, on each change. The period is the PIT divisor the effect player chose (Hz = 1193182 ÷ period) | tone |
| `["sound", event, priority, enabled]` | A combat sound request (`BtechPlaySoundIfEnabled`) | game |
| `["key", word]` | A campaign key read | game |
| `["cpoll", n]` | n combat input polls in a row (one per tick). Campaign polls aren't logged, because how often the host polls while the game waits depends on the port model's time charges, not on the game | game |
| `["snap", name]` | A trace snap, kept in every stream as a mark | all |

**The clocks are the game's own.**
- A note's `tick` counts the sequencer's tick calls (`MwTickMidiTrack`). That rate doesn't depend on the device, even though the timer interrupt's rate does.
- A tone's `tick` counts the effect player's tick calls (`BtechTickSpeakerToneChannels`).

**Comparison.** `check` splits each log into its three streams, note, tone and game, and compares each exactly, naming the first difference after the last snap. A slice that changes one stream on purpose re-records with `record` and says so.

**Revisions.** Goldens are kept per game revision, in `<first 12 hex of BTECH.EXE's SHA-256>/`. A revision with no goldens is skipped.
