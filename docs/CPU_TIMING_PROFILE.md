# Game speed and sound synchronisation

Combat runs one tick at a time, and each tick lasts a fixed number of
milliseconds, the same on any computer, however much it draws.
`MechWarrior.ini` sets it:

```ini
combat_tick_ms=60
combat_speed=1
```

- `combat_tick_ms` (20 to 500) is the tick at 1x. 60 ms is about 2.1 times as
  fast as the earlier cycle model, whose live cockpit ticks averaged 127 ms
  (measured, `docs/timing/PLAN.md` section 3).
- `combat_speed` (1 to 4) divides it: 60, 30, 20 and 15 ms per tick. In combat
  the main-row `-` and `=` (`+`) keys step it; the game shows the speed for a
  moment at the top right and remembers it in the player's own
  `MechWarrior.ini`, beside the saves. The keypad's `-` and `+` stay the
  game's (previous and next weapon). A tick never gets shorter than 10 ms,
  and a host too slow for the step simply runs as fast as it can, with no
  catch-up burst afterwards.
- The old `cpu_cycles_per_ms` key is ignored, with a note on stderr.

Only the wait between ticks changes. The combat logic reads no clock, so
movement, reloads, heat, the AI and every other count in ticks run the same
tick by tick; at a faster speed they simply happen sooner. The original's own
timed waits (the dialog and loss-flash delays, the orders screen's 76 ms
floor, every campaign wait) follow the PIT at its real rate and are not
changed by either setting.

Audio keeps its own hardware-rate scheduling. Sounds that start on a tick
(footsteps, weapons, impacts) start on an even grid; the speed changes how
often they start, not their pitch or length.

For timing reports, record the tick setting and speed, the mission and
chassis, and compare player movement, enemy movement, gait and weapon effects
together. With `MECHWARRIOR_DIAGNOSTICS` and a capture directory, the stderr
status line prints `tickMs` and `speed` once a second in combat.
