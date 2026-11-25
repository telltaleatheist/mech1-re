# Runtime status

1.0.1 (tag `mw1r-v1.0.1`, commit e223eb2) is out for Windows x64, macOS 12 or later (Apple silicon and Intel) and Linux
x86-64. It plays the campaign and combat of the 1989 release and of the one of 1990-01-22 in EGA, with the campaign's
music from the bundled soundfont, as speaker tones, or off, and combat's effects as speaker tones
([audio](AUDIO.md)). Combat runs at a fixed `combat_tick_ms=60` per tick at 1x, with speed keys for 2x to 4x
([game speed](CPU_TIMING_PROFILE.md)).

The checks below are automatic: tests, and key scripts whose results are compared with recordings. They don't say that
a whole campaign has been played. [The notes](release-notes/mw1r-v1.0.1.md) list the known differences and limits.

## What is checked

1.0.1 is 1.0.0 with the window's new picture and a graphics device that resets ([WINDOW](WINDOW.md)). What changed was
checked on e223eb2, on the Mac, Linux and Windows, with the packages' own SDL:

- ctest `picture`, `canvas`, `joystick`, `sdl_joystick`, `music_setting`, `notice`, `setup`, `status_setup` and
  `read_only_install`: all pass. `picture` draws in software, on no display. On the Mac, SDL's Metal, OpenGL and GPU
  drawing were tried too, in hidden windows; Direct3D, Vulkan and Linux's OpenGL weren't.
- The recorded runs `bar`, `mech_complex`, `combat_cockpit` and `combat_retreat`: they match, but for the two of combat
  with the 1989 edition, as below.
- The five packages, as 1.0.0's below. The installer's twin ran over 1.0.0's.

The rest is 1.0.0's and wasn't repeated. Its release gate ran on cadfb67, with both editions of the game
([results](phase5/GATE_PLAN.md#results-on-cadfb67)). From there to cd33ea4 no source, data or build file changed, only packaging, its tests and documents. The packages'
own checks ran again on the packages built from cd33ea4.

| What | By | Where |
| --- | --- | --- |
| The import, the typed story, rules and layout files, the game files at play, saves written and read back, joysticks, the setup, the music and tone code, and that no text of the game is in the repository | ctest: 38 tests with the 1990 edition, none skipped; 13 of them, which use the game's files, with the 1989 one | all 38: Mac, Linux, Windows. The 1989 run: Mac, Linux |
| The campaign's screens and state: a contract accepted, the company, the Mech complex, the bar, travel, the options and saving | recorded runs `campaign_contract`, `company`, `mech_complex`, `bar`, `travel`, `options` (`tools/trace`): frames and state at each step | all three, both editions: all match |
| Combat: walking, turning, firing, the HUD, the pause, a jump, the inspection and orders screens, the retreat, the results and the return to the company | recorded runs `combat_cockpit` and `combat_retreat` | all three. 1990: both match. 1989: both differ from the recording, the 1990 edition's, as expected (its tick is one ahead) |
| Play without the game's programs, after the import | `exe_free`: the recorded runs with the exes and with every exe deleted are the same | 1990: all three. 1989: Mac, Linux |
| What the game asks for: notes, tones, sounds, keys | `events`: 12 scripts, every song and effect among them, against recorded logs | 1990: all three. 1989: Mac, Linux |
| The speaker's tones | `audio.py`: 2 logs | Mac, 1990 |
| The same frames as before the release's last changes | 864 frames of the recorded runs, both editions, against those kept from 4632030 | Mac |
| Memory errors | the address and bounds sanitizers on the 8 recorded runs and the 2 sound logs: no report | Mac, 1990 |
| The packages | `--help`, the setup served to the launcher (72 checks), and a first start that imports the story and reaches the game, on each edition | win64.zip, tar.gz, AppImage, and the Mac app on arm64 and, under Rosetta, x86_64 |
| The Mac app's Intel half in play | `combat_cockpit` under Rosetta | Mac |
| The Windows installer | its twin under a test AppId: over the files of an earlier preview, then uninstalled | Windows |

The recordings come from the program itself. On 2026-11-25, with the tools and recordings of 436a4d3, the original of
1990 under DOSBox-X ran all eight scripts and matched the recordings, frames and state (`tools/trace/README.md`); the
original of 1989 wasn't run. The music's recordings were compared at an earlier commit, on sound paths taken out since
(`tools/trace/audio/README.md`), and not again: this time no sound was. The port this is built on had checks of its own,
listed here before: all eight chassis' start and deployment, and one contract won and paid. They aren't in this
repository and weren't repeated.

## What isn't checked

- **The whole story,** from start to end, and every kind of contract with every Mech. The recorded runs are two battles
  and one path through each screen.
- **Nobody has played or heard** the Linux package, or the Intel half of the Mac one on a real Intel Mac. Linux ran only
  on Ubuntu 24.04.
- **Real game controllers.** Only simulated ones were used.
- **The message box** that says why a start was refused: its text was checked, the box wasn't seen.
- **The setup on the launcher's screens.** The program's side of it is checked; nobody has gone through the screens.
- **The packages in a window,** 1.0.1's new one included: on Windows and Linux at all, on a Mac maximised or in full
  screen. Resizing it on a Mac (Apple silicon) was tried by hand, and works. The runs are hidden and play nothing aloud.
- **A real reset or loss of a graphics device.** The tests send what SDL would say, and refuse to draw.
- **`Setup.exe` itself.** It wasn't run before the release; its twin was ([RELEASING](RELEASING.md), step 5).
- **The 1989 edition** under the sanitizers, and its sound logs.
- **The same picture and sound as the original's.** Nothing here claims it. CGA and Tandy graphics, and the AdLib, MT-32
  and Tandy sound, aren't in the program.

## Behaviour to know

The campaign has planet services, repair, rearming and trading, recruitment and Mech assignments, contract negotiation,
travel and arrival, and save and load. The Extra Ammo shop buys and sells company reserves separately from reloading a
selected Mech; machine-gun ammunition trades in batches of ten. A contract's battle goes through the briefing, movement,
firing, weapon effects, the command and damage screens, and back to the company with its results.

Terrain probes use bounded grid coordinates and keep the mission's retreat rules. Line of sight uses initialized surface
queries. Targeting uses actual Mechs and objective health; resource ordering uses real surface metadata. Missing targets
and map projections don't read unrelated memory or abort. Full cosmetic-effect and projectile pools refuse further
allocations safely; a required startup allocation that fails keeps the fatal-message and shutdown path. Combat's sound
bank holds speaker effects only; no other weapon sounds or combat music are invented.

## When something looks or sounds wrong

Report the contract, chassis, company state, inputs and runtime log. See the [presentation
checks](VISUAL_ACCEPTANCE.md) for useful manual coverage and [build and play](PLAYABLE_TEST_BUILD.md) for building it
yourself. A successful build alone isn't a successful mission.
