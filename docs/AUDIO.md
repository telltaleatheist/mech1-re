# Sound configuration

Use the existing `MechWarrior.ini`, alongside `combat_tick_ms`. Changing the
music does not change the combat tick length or the timer.

```ini
combat_tick_ms=60
music=soundfont
```

- `soundfont` (the default) plays the campaign's songs from a soundfont (below).
- `speaker` plays them as tones on one voice, as the original's PC speaker had
  them.
- `off` plays no music. The effects stay.

The player's own `MechWarrior.ini`, in the save directory, overrides the one
beside the program when it names the music. An INI with the old `sound_system`
still works: `speaker` is read as `music=speaker`, `adlib` and `mt32` as
`music=soundfont`, and the log says so in one line. With neither key in either
file the music is the soundfont; an old INI with `sound_system=speaker` keeps
the speaker. `music` wins over it in the
same file. The MT-32 ROM settings are ignored, with a line in the log. The AdLib
and MT-32 paths are gone (docs/phase5/NO_EMULATION_PLAN.md, S8 and S9).

Combat's effects play on the speaker's tone voice whatever the music. The
installed `BTECHSND.SND` holds only PC-speaker effects, and the port does not
invent others. The original has no combat music.

## The rebuild's music: `music=soundfont`

```ini
music=soundfont
```

The campaign's songs play from a General MIDI soundfont instead of a card. The
game's own sequencer sends the notes the original sent, at the MT-32 version's
times, to the rebuild's own synthesizer (`src/Audio/Music`, TinySoundFont). Combat's
effects stay on the speaker's tone voice.

The instruments come from `data/music/instruments.json`: for each song and
MIDI channel, a General MIDI program, a level and a pan, plus the drum map. The
songs carry no Program Change. The MT-32 version picks the MT-32's built-in
timbres by SysEx, which the rebuild skips. The first table plays the nearest
General MIDI program to each of those timbres, and listening refines it
(docs/phase5/NO_EMULATION_PLAN.md, S4). The soundfont is a subset of FluidR3 GM
cut at build time (`tools/soundfont/subset.py`).

`tools/trace/listening.py` renders every song to WAV files, beside the
speaker version, with a listening sheet. It runs headless, and nothing is
played aloud.

## Listening and playtesting

Set `music=soundfont` or `music=speaker` in your INI, keep
`combat_tick_ms=60` and speed 1x while comparing them, and select the INI with
the existing launcher option:

```powershell
.\src\Run-MechWarrior.ps1 -TimingConfiguration .\MechWarrior.ini
```

Restart the game after changing it. The launcher prints the exact configuration
and runtime-log paths.

For a useful first test:

1. Let the title music play, then visit a Bar and listen to the change of cue.
2. Repair and rearm, accept a contract, and enter combat. Walk, turn, fire and
   listen for footfalls, weapon reports and impacts. These remain PC-speaker
   effects with either music.
3. Return from the mission and revisit the Bar. Check that music resumes without
   stuck notes, growing delay or effects continuing from the previous mission.
4. Repeat with the other setting and the same company save.

### Campaign music cues

| File | When to listen |
| --- | --- |
| `MECH.MUS` | Intro/title sequence |
| `MECHBAR.MUS` | Entering the Bar |
| `LAUNCH.MUS` | Successful contract debrief, despite its filename; not mission launch |
| `DEATH.MUS` | Lost lancemates during debrief, or the commander's death/restart event |
| `VICTORY.MUS` | Dark Wing story ending, not an ordinary contract victory |

A failed contract with the commander and lancemates surviving does not itself
start Death. Deployments resolved without combat do not start the successful
contract cue. Returning to the planet hub stops music, so dismissing a debrief
quickly can cut its cue short. These are original campaign decisions. Leave the results screen open to hear its music.

The automated smoke check runs campaign → combat → Bar with one continuously
open SDL audio device, including the original fifteen effects.
It checks that output is present and that the audio queue stays bounded. It is
not a completed-mission or subjective listening acceptance test. Report any
audible issue with the sound setting, screen/action and launcher runtime log.

### Supported scope and remaining limits

The five campaign music files, their debrief/death/ending triggers and the
fifteen installed combat effects have automated playback coverage. Trigger
checks enter the real campaign with controlled combat-return results; they
do not amount to a complete campaign playthrough.

Tandy PSG/DAC and sampled-speaker library paths are not supported playback
options. Arbitrary replacement banks, malformed data and unknown stale-memory references are not guaranteed to work.
Speaker output is a digital approximation, not an analogue hardware recording.

## Building and distributing

The synthesizer is our own code and TinySoundFont, built into the program; the
soundfont and its licences are in [the third-party notices](THIRD_PARTY_NOTICES.md).
