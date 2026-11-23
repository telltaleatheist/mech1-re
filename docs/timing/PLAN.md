# Milliseconds, not cycles: the timing plan

The goal: the game runs at the same speed on any system. Every timing that was
counted in frames, loops or modelled CPU cycles becomes an explicit length in
milliseconds. Combat keeps its fixed tick (the logic reads no clock, so the
traces stay byte-exact), and each tick lasts a fixed number of milliseconds,
however much is drawn and whatever the host. Combat also gets speed steps:
the first preview's combat was judged far too slow, and the target set was at
least twice as fast, with keys for 2x to 4x.

## 1. Every timing source today

The PIT runs at 1,193,182 Hz. Everything the original timed with the PIT is
already wall-clock time in the port (`MwSdlPumpTimer` delivers each IRQ0 edge
at its real time, or at the test clock's), so those rows are already in ms and
only need writing down. The rows that were not are marked **slice 1**.

### 1.1 Combat

| Source | Where | Counts | At 8bbb679 | Becomes |
|---|---|---|---|---|
| The combat tick | `MwSdlPaceCombatCpu`, now `MwSdlPaceCombatTick` (SDLBackend_Time.cpp), called at the top of BTECH 0810:0019's loop | modelled cycles: 24,080 + 263,005 × (video bytes read + written) / 14,154, ÷ `cpu_cycles_per_ms`, plus 1,000 ns per video read and 750 ns per write | varies with what is drawn (section 3) | **slice 1**: `combat_tick_ms` ÷ speed from the tick's start; no video accounting |
| Delay loops | `MwSdlDelayCpuLoops(n)`, now `MwSdlDelayLoops`: n loops as n cycles at `cpu_cycles_per_ms` | loops | n × 0.4 µs at 2500 | **slice 1**: n × 0.4 µs, fixed (1.3) |
| The orders screen's frame floor | `runtimeDelayRemaining=COMBAT_FRAME_DELAY_TICKS` (18) each tick; 17FB waits for it before the page swap | scheduler IRQs: divisor 0xFFFF/0x0D = 5,041, 236.70 Hz, 4.225 ms each | 76.05 ms minimum per orders-screen tick | already ms; stays. A `combat_tick_ms` below 76 ms leaves the orders screen at 76 ms, as on the original |
| Scaled runtime delays | `BtechWaitScaledRuntimeDelay(u)`: u × 4 scheduler IRQs = u × 16.90 ms | scheduler IRQs | dialog (0x18) 405.6 ms; prepare (0x78) 2,027.9 ms; exit (0x3C) 1,013.9 ms; loss flash (6) 101.4 ms × 20 = 2,028 ms | already ms; stay |
| Retrace wait | `BtechSwapScreenPages(waitForRetrace)` | 60 Hz scanout from the clock | every caller passes false: never waits | nothing |
| Sound and music | scheduler slot callbacks (Tandy/speaker), the MT-32 and AdLib paths | PIT edges | wall clock | stay (section 4) |
| In-game counters | reload timers, jump recharge, heat, AI, projectiles, effect sequences, the outcome countdown (`COMBAT_OUTCOME_DELAY_TICKS`, 20), emitters (8, 25), order-marker repeat (5), pose perturbation (5), blink masks | combat ticks | tick × count, so they vary today with the tick | stay in ticks; become ms by way of the fixed tick (20 ticks = 20 × `combat_tick_ms`) |

### 1.2 The campaign

The campaign has no CPU pacing. Its waits count `campaignTimerTicks`, which
`MwCampaignTimerTick` advances once per BIOS tick (the 18.2065 Hz chain,
54.925 ms; `MwCampaignTimerInterrupt` keeps that rate when the music speeds the
PIT up). All of them are already ms:

| Wait | Ticks | ms |
|---|---|---|
| Travel animation frame (`TRAVEL_ANIMATION_FRAME_TICKS`) | 2 | 109.85 |
| Travel animation end (`TRAVEL_ANIMATION_FINAL_TICKS`) | 10 | 549.25 |
| Victory transition step (`VICTORY_TRANSITION_TICKS`) | 5 | 274.62 |
| Victory animation (`VICTORY_ANIMATION_CYCLES`, 20 cycles of 2 steps) | 200 | 10,985 (plus the 4 opening steps, 1,098) |
| UI flash first wait (`UI_FLASH_FIRST_WAIT_TICKS`) | 2 | 109.85 |
| Joystick key repeat (`JOYSTICK_REPEAT_TICKS`) | 6 | 329.55 |
| Mech service animation step | 1 | 54.93 |
| Text cursor blink (`CAMPAIGN_TIMER_PHASE_PERIOD`) | per phase | phase × 54.93 |

`VICTORY_ANIMATION_CYCLES` counts animation cycles, not CPU cycles: each step
already waits on the BIOS tick. Nothing in the campaign counts loop iterations
as time; the change is to name these lengths in ms in the code (a constant
beside each tick count), not to change them.

### 1.3 The delay loops in detail

`MwSdlDelayCpuLoops` has three callers:
- The CGA CRTC probe (BTECH 1BF5:00FD, 0x64 loops): on the CGA path only,
  which stops on a port failsafe. Never reached in play.
- `BtechCalibrateJoystickAxisPairs` (each combat start, from 0810 import, and
  Alt+J): 1,000 game-port reads timed by the PIT give
  `joystickTimingLoopCount` = 0x6FCC / elapsed PIT counts. Then each of the 4
  axes runs `BtechMeasureJoystickAxisTiming`. There is no game port, so it reads
  0xFF and the loop runs all 255 steps of `DelayCpuLoops(joystickTimingLoopCount)`,
  then a 20-loop presence delay.
- That makes the one host-dependent wait left in the port. The elapsed count
  comes from how fast this host performs 1,000 port reads. A fast host gives
  a small count, so a large loop count and a long wait: 4 × 255 × loopCount
  × 0.4 µs: on the PC 1,020 calls of 2,044 loops, 0.83 s at every combat
  start (section 3).
- **Slice 1:** each delay loop waits a fixed 0.4 µs (the 2500 model's
  loop, so the test clock's timeline is unchanged). Between the calibration's
  two PIT latches, each game-port read counts a fixed 1 µs. The loop count
  is then 0x6FCC / 1,193 = 23 on every host, and the calibration takes
  4 × 255 × 23 × 0.4 µs = 9.4 ms. It was 0.83 s on the PC (loop count 2,044).
  No stick is emulated, so the count changes no play.

## 2. The fixed tick and the speed steps (slice 1)

- `MwSdlPaceCombatTick` (was `MwSdlPaceCombatCpu`) keeps its place at the
  top of 0810:0019's loop and its no-debt rule. The next tick starts
  `combat_tick_ms ÷ combat_speed` after this one started, never sooner than
  10 ms. If the host or an explicit original wait (a dialog, the loss flash)
  ran past that, the next tick starts at once, with no catch-up burst.
- The video-byte accounting, the `REFERENCE_*` constants, the status line's
  `cpuCycles` and `cpu_cycles_per_ms` are gone. An old ini's
  `cpu_cycles_per_ms` is ignored, with one stderr line naming `combat_tick_ms`.
- `MechWarrior.ini`:
  - `combat_tick_ms=60` (20 to 500): the tick at 1x;
  - `combat_speed=1` (1 to 4): the divisor.
- **The player's own `MechWarrior.ini`,** in the write directory beside the
  saves (`MECHWARRIOR_SAVE_DIRECTORY`, or SDL's per-user folder), is read
  after the shipped one and overrides it. The speed keys save
  `combat_speed` there; nothing is written beside the program.
- **Speed keys:** in combat, the main-row `-` (0x0C) and `=`/`+` (0x0D). BTECH
  reads neither; its number-key weapon range ends at `0` (0x0B). The host
  takes them only in combat, so the game never sees them, and feeds and
  traces are unchanged. The keypad `-` and `+` stay the game's: previous and
  next ready weapon (0810 player controls), and Right and Down on 1BF5's
  keyboard-translation path.
- **The speed notice:** "SPEED 2x" for 1.5 s at the top right. It is drawn
  with SDL's 8×8 debug font at present time only, so captures and trace
  frames (the displayed indices) never contain it. The status line prints
  `tickMs` and `speed`.
- No variable dt: the logic still steps one tick at a time.
- **The test clock** (`MECHWARRIOR_DEBUG_VIRTUAL_CLOCK`) spends the fixed
  tick instead of the modelled cycles. So the combat sound-port goldens
  (`tools/trace/audio`, docs/phase5) are re-recorded in this slice; the
  campaign music goldens must stay identical. The traces don't change:
  they step ticks and read no clock.

## 3. Measured tick lengths at 2500 (the baseline)

**Method:** an uncommitted probe in the old `MwSdlPaceCombatCpu` logged each
tick's video reads and writes, the modelled length, the host's own work
time and the wall time. It ran on the PC (Windows 11, MSVC Release, 8bbb679,
1990 folder) on:
- `combat_cockpit`, offscreen;
- `combat_cockpit` with a real hidden window (Direct3D renderer);
- a measurement-only script: `combat_cockpit` with long stretches (standing
  300 ticks, a full turn 400, walking 320, paused 200).

The modelled length depends only on the bytes drawn, and the traces draw the
same frames on every host, so it is the same on the Mac.

| Scene | Ticks | Modelled min / avg / max (ms) | Wall avg (ms) |
|---|---|---|---|
| standing | 300 | 126.2 / 126.8 / 127.6 | 127.3 |
| turning | 401 | 115.5 / 123.8 / 148.1 | 124.3 |
| walking | 320 | 121.9 / 132.0 / 215.2 | 133.5 |
| `combat_cockpit` live (cockpit, walk, turn) | 85 | 126.2 / 131.4 / 154.7 | 131.9 (offscreen and real window) |
| paused | 200 | 120.2 / 143.5 / 200.1 | 146.4 |
| inspection screen | 6 | 624 / 747 / 1,358 | 747 |
| orders screen | 12–28 | 106 / 123–145 / 368 | (the 76 ms floor never binds) |

- **The live cockpit average: 127.2 ms over 1,031 ticks,** about 7.9 ticks
  a second.
- **Why so long:** about 14,000 video bytes a tick dominate the model:
  263,005 cycles ÷ 2,500 = 105 ms, plus 9.6 ms of logic and about 12 ms of
  EGA bus time. The ticks hardly vary, because the cockpit redraws about the
  same bytes every tick.
- **The sleep is not the cause.** Wall time minus max(modelled, host work)
  averages 0.2–0.8 ms per tick on Windows; SDL3's `SDL_DelayNS` uses a
  high-resolution waitable timer. Presenting happens only when the shown page
  changes, with vsync off, and the audio feed never blocks the game thread.
- **The default, 60 ms,** is 2.1 times the baseline: about 16.7 ticks a
  second at 1x, then 33, 50 and 67 at 2x to 4x.
- **Host headroom:** the PC's own work per tick was 13–17 ms (section 5). So
  4x (15 ms) runs about 3.5x there until that is trimmed.

## 4. Audio sync

- Sound and music run on PIT edges at wall-clock rate, independent of the
  tick, and the fixed tick doesn't touch them.
- What changes is when they start. Footsteps (from the gait's progress per
  tick), weapon sounds and impacts start on a tick. They used to start on
  uneven ticks, longer when more was drawn; now they fall on an even grid.
  At a higher speed they start more often, at the same pitch and length.
- `BtechStopSoundEventAndTimer(0)` runs every tick, so sound event 0 lasts
  one tick: now exactly `combat_tick_ms ÷ speed`.
- Check by ear on the device: walking, firing and impacts at 1x and 4x, in a
  busy and a quiet scene.

## 5. Slices

1. **The fixed tick, the speed steps and the fixed delay loops** (branch
   `combat-speed`):
   - section 2;
   - the joystick calibration (1.3);
   - the combat sound-port goldens re-recorded;
   - docs: CPU_TIMING_PROFILE.md rewritten, README, AUDIO, PLAYABLE_TEST_BUILD,
     RUNTIME_STATUS, DEBUG_INPUT.

   Gate: traces 8/8 on both folders, unit tests, the music sound-port goldens
   unchanged, a listening check.
2. **Host headroom:** profile the 13–17 ms of host work per tick (the pump's
   64,000-pixel present compare is the first suspect) and trim it.
3. **Naming:** ms constants beside the campaign and runtime-delay tick
   counts, and a test that pins the PIT-derived lengths in section 1, so a
   change shows up. Whether to speed any campaign wait up (the 12 s victory
   animation, the travel frames) is a separate decision; they are unchanged.

## 6. Decisions

- The default from the live cockpit ticks (standing, turning, walking),
  then at least twice that speed: 60 ms.
- Speed steps 1x to 4x (60, 30, 20, 15 ms), with a 10 ms floor; the keys are
  the main-row `-` and `=`/`+`, in combat only; the step is remembered in the
  player's ini.
- `cpu_cycles_per_ms` is dropped: an old ini's key is ignored with one stderr
  line naming `combat_tick_ms`.
- `combat_tick_ms` ranges from 20 to 500.
- The joystick calibration: a fixed length per loop and 1 µs per read.
- Every combat tick is fixed, including the inspection and full-screen ones.
- Campaign waits are unchanged for now.
