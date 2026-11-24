# Game-event goldens

Slice T1 of [the no-emulation plan](../../../docs/phase5/NO_EMULATION_PLAN.md).

These are exact logs of what the game *asked for*, recorded from today's port before the hardware layer under the game is rebuilt:
- the notes the sequencer played;
- the tones combat's effect player chose;
- the sounds and keys it requested, and combat's ticks.

[The sound-port goldens](../audio/README.md) record what the game wrote to the hardware. That changes once the hardware layer is rebuilt, but these logs must not. Every later slice of the plan leaves them identical, apart from one planned exception. H4 removes the port model's time charges, and with them the few sequencer ticks they add between a key and the music (below), so H4 re-records the note ticks once and says so.

    python3 tools/trace/events.py check  --port build/mechwarrior --game ~/MECHWAR --work ~/mw1-events
    python3 tools/trace/events.py record --port build/mechwarrior --game ~/MECHWAR --work ~/mw1-events

`--only campaign_contract audio-music feed-effects` limits the run, and `--no-devices` skips the soundfont runs.

- **Exit status:** 0 all match, 1 a mismatch, 77 skipped.
- **macOS:** set `MW1O_HIDDEN=1`, as for `check.py`.
- **ctest:** the `events` test runs the check only with `MW1O_EVENTS=1`, since the runs take minutes.

## What runs

Every trace script (`../scripts/*.keys`), every sound script (`../audio/scripts/*.keys`, named `audio-<script>`) and every feed script (`scripts/*.keys`, named `feed-<script>`) runs through `record.py`, with two switches:

- `MECHWARRIOR_EVENT_LOG=FILE`: the log below.
- `MECHWARRIOR_DEBUG_VIRTUAL_CLOCK=1`: time moves only where the port would wait, so the log depends on the script alone. This is the same clock the sound-port goldens use.

The log is cut after the last snap, since what follows is the shutdown.

## The feed scripts: every song and effect

Play reaches only some music and effects in a script of practical length. LAUNCH needs a won contract, DEATH lost lancemates, and VICTORY the end of the story. So two scripts here ask for the rest through the game's own routines, with two feed tokens:

- **`song:NAME`** (campaign) starts bar, death, launch, mech or victory. Each is the game's own loader paired with its own start call and offset (`SDLBackend_DebugControl.cpp`, `FeedSong`). `songs.keys` plays bar, death and launch first, then mech, then victory last. MECH and VICTORY overwrite the buffer that holds the other three, so keep that order.
- **`sound:N`** (combat) requests effect N through `BtechPlaySoundIfEnabled`, at the priority the game requests it with (`FeedSound`). `effects.keys` plays every effect the game requests. Priority decides only who wins a busy channel.

These are the only feed tokens that call the game, so they are fenced:
- the port refuses them unless `MECHWARRIOR_EVENT_LOG` is set;
- `record.py` refuses them without `--feed-calls`, which only `events.py` passes, and only for scripts in this folder;
- `test_feed_calls.py` (ctest `feed_calls`) proves all of that, and that no trace or sound script holds one.

Game-state traces (`check.py`, `exe_free.py`) still come from input alone.

`feed-songs` records MECH a second time, beside the title's real one in `audio-music`. If the two ever differ, that's a finding.

**Effects 9 and 12** are in BTECHSND.SND's bank, but neither BTECH revision requests them. Both revisions have the same sixteen call sites, requesting 0–8, 10, 11, 13 and 14, so these two have no golden.

The goldens are the speaker's, the default device. The sound scripts also run with `music=soundfont`, against goldens of their own (`*.soundfont.jsonl.gz`) and the speaker's run. Against the speaker's:
- **Notes**, ticks and snap marks left out, are cut into pieces wherever a key can cut the music: at each song start and at each all-notes-off sweep (a stop). The runs must give the same number of pieces. Within each piece, one run's notes must be a prefix of the other's, because a key, or the last snap, may cut a piece a note or so earlier or later.
- **The tone and game streams** must match exactly.

The ticks, and where a snap falls among the notes, can differ between the two: the music paths run the sequencer at different rates, so a key or a snap can arrive some sequencer ticks earlier or later.

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
| `["cpoll", n]` | n combat input polls in a row (one per tick), written before the next sound, key or snap line. Notes and tones don't end a run, because the timer drives them and how it interleaves with combat polls depends on the device. Campaign polls aren't logged, because how often the host polls while the game waits depends on the port model's time charges, not on the game | game |
| `["snap", name]` | A trace snap, kept in every stream as a mark | all |

**The clocks are the game's own.**
- A note's `tick` counts the sequencer's tick calls (`MwTickMidiTrack`). That rate doesn't depend on the device, even though the timer interrupt's rate does.
- A tone's `tick` counts the effect player's tick calls (`BtechTickSpeakerToneChannels`).

**Comparison.** `check` splits each log into its three streams, note, tone and game, and compares each exactly, naming the first difference after the last snap. A slice that changes one stream on purpose re-records with `record` and says so.

## What the goldens show (T1)

- **The five songs.** All are Standard MIDI type 0 at 96 PPQN, with the melodic parts on channels 1–6 (MIDI 2–7) and the rhythm part on channel 9 (MIDI 10).
  - MECHBAR's drum keys are 36, 38, 40, 44, 46 and 73. VICTORY uses 36 and 38, MECH uses 38 and 40, and LAUNCH and DEATH have no rhythm part.
  - No song sends a Program Change, and no controller is sent except all-notes-off (123), in the sequencer's stop sweep. Instruments are set only by SysEx (6 to 31 bytes).
- **Feed against play.** The feed's MECH matches the title's real one message for message, up to where a key stops the title. The feed's bar matches the real bar exactly after its 16-channel restart sweep. A song: is the game's own song.
- **The effects** were all requested at the game's own priorities, and all played on the speaker's tone path with no port failsafe. Their bank types 2 and 4 (sample and Tandy DAC paths) belong to no effect the game requests.
- **Revisions.** The campaign and music logs are identical in both revisions. The combat logs differ, as the battles do (the 1989 BTECH is a different build).

**Revisions.** Goldens are kept per game revision, in `<first 12 hex of BTECH.EXE's SHA-256>/`. A revision with no goldens is skipped.
