# Playing the experimental build

Build with the existing SDL3 dependency, then launch against your own original
installation:

```powershell
./src/Build.ps1 -Configuration Release -SDL3Directory 'D:/Libraries/SDL3/cmake'
./src/Run-MechWarrior.ps1 -GameDirectory 'D:/Games/MechWarrior'
```

The launcher uses `build-visual-studio/Release`, keeps private saves separately
and reports the capture/log directory under ignored `local` for the run. SDL3.dll must remain next
to the executable. Do not set SDL dummy drivers for interactive play.
The campaign's music is chosen in the existing `MechWarrior.ini`: see [sound configuration](AUDIO.md). The default remains speaker.

For the default installation location, put your original game files directly
inside `assets/original-game`, then omit `-GameDirectory`. That entire folder
is Git-ignored except for its `.gitignore`; original files must never be committed.
The window title shows the selected card in campaign and combat. Combat also
uses the native PC-speaker effects bank.

## Preparing for a mission

The company starts with a damaged Jenner. Visit the Mech Complex to inspect,
repair and rearm it. Confirm the commander's assignment before accepting and
deploying under a House contract. Browse the offered terms: payment, salvage
and advance are separate. Acknowledge the combat briefing to enter the cockpit.

## Controls

Campaign: arrows and Enter select choices. Combat: Up/Down adjust speed,
Left/Right turn, Enter selects a target, Space fires the selected weapon and
Tab fires all weapons. Watch heat and ammunition. C opens command, D own
damage, E target damage and Q returns to the cockpit. T selects the map
instrument, Z changes zoom and P pauses. F12 saves a real display capture.

Combat runs at `combat_tick_ms=60` per tick at 1x, the same on any computer.
In combat the main-row `-` and `=` (`+`) keys step the speed through 1x, 2x,
3x and 4x; the choice is remembered in the player's own `MechWarrior.ini`
beside the saves (docs/CPU_TIMING_PROFILE.md).

## Reporting a problem

The launcher prints the location of `runtime.log` inside its private capture
directory. A port failsafe records its reason, function, source file and line
before stopping. Documented native CPU-fault emulation is labelled separately;
these host aborts are not original DOS error exits. On Windows the handler also
attempts a stack trace; frame names depend on available debugging symbols.
Without the PowerShell launcher, retain the program's standard error output.
An abort does not save the company or perform normal campaign shutdown.

Keep the runtime log and full F12 capture set privately. Report the planet,
contract objective, chassis, repair/rearm state, recent inputs and what happened
before the failure. Do not upload the game installation or saves publicly.

The complete campaign, every objective and all presentation/audio behaviour are
not yet qualified. See [current status](RUNTIME_STATUS.md). Existing recorded
screenshots are not evidence that a new run or a different contract completed.
