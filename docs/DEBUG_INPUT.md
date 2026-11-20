# Optional keyboard and telemetry interface

This local debugging interface is disabled unless
`MECHWARRIOR_DEBUG_CONTROL_DIRECTORY` names an existing private directory.
It is independent of the Support pilot. Commands generate ordinary SDL key
events; they cannot set health, positions, mission results or game speed.
Use it only in your own test session, not alongside another controller.

The directory contains a client-written `command.txt` and a game-written
`status.json`. A command starts with an increasing request ID and step count,
followed by one line per step: duration in milliseconds, then SDL key names.
For example, this holds Up for one second, then fires Space for 100 ms:

```text
1 2
1000 Up
100 Space
```

Use underscores for spaces in key names. Each step allows up to four keys and
25–30000 ms; a request allows up to 31 steps and 60000 ms of input, followed by
a 200 ms release/settling step. These are upper limits, not recommended combat
reaction intervals. Commands are consumed, including rejected requests. Keep
the request ID increasing for the lifetime of the game process.

Held keys are released between steps, on completion and on interface shutdown.
Leaving the environment variable unset disables the interface. It never sends
keys to other applications or changes desktop input bindings.

Status includes command progress/error, campaign menus and contract information,
combat units, targets and radar. Jump diagnostics expose movement mode, vertical
step, remaining jump charge and the player's camera position. Coordinates are
native game units, not metres. Clients must retry partial/concurrent JSON reads;
the file is not an atomic frame snapshot. Combat-return summaries also appear
in the runtime log to help distinguish mission completion from a crash.

For golden traces (`tools/trace`), status also carries the campaign's two
random words (`campaign.random`) and an `input` block: `polls` counts the
game's keyboard checks, so a client can tell when it is waiting for a key;
`reads` counts key reads; `commandReads` lists each key read since the current
command began as `[BIOS key word, random word 0, random word 1]`, the random
words as they stood when the game read that key. The original draws a random
number on every pass of its wait-for-a-key loop (081B:02FB), so these words,
not a seed, are what a replay must restore before each key.

`MECHWARRIOR_DEBUG_BIOS_TICKS=<n>` (only with the control directory set) fixes
the BIOS clock that the campaign's one startup read sees (081B:0C0A mixes it
into the random words), as if the DOS clock had been set before the run.

The interface changes neither jump physics nor the approved default
`cpu_cycles_per_ms=2500`.
