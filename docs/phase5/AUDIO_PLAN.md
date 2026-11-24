# Phase 5: audio plan

Status: plan for review. No code until it's approved. **Superseded in part** by
[sound and graphics rebuilt, no emulation](NO_EMULATION_PLAN.md) (2026-11-22):
its own FM synth (`FmChip`, a chip model) and its MT-32 options are dropped;
its slices 1-3 (the port-event log, the output layer split, the DOSBox-X
comparison) are done and stay the base.

Project rule: no emulation libraries. Munt (libmt32emu) and Nuked-OPL3 leave
the build and are replaced by our own SDL3 audio. This plan says what the
game drives today, what replaces each part, how we prove the result, how it
meets the fixed tick, and what it costs.

## 1. What the game drives today

The game code is the same on every path: the original drivers decide every
note, instrument and register write. Only the last step, turning port writes
into sound, is a library. Everything above that line stays (it's game logic
and is covered by the traces); everything below it is what this phase
replaces.

### The hardware line in the port

| Device | Ports the game writes | Port entry point | Synthesis today |
| --- | --- | --- | --- |
| PC speaker | PIT 43h/42h (channel 2, mode 3), 61h gate | `MwSdlWriteSpeakerPort` | our own 32-harmonic band-limited square (SDLBackend_Audio.cpp, MIT host code) |
| AdLib (YM3812) | 388h index, 389h data, 388h status reads | `MwAdlibWritePort`, `MwAdlibReadStatus` | Nuked-OPL3 in OPL2 mode, mono; the card's two timers are our own code (Adlib.cpp) |
| MT-32 (MPU-401) | 330h data, 331h status/command | MPU401.cpp, then `MwMt32ReceiveByte` | libmt32emu, with the player's own Roland ROMs |

All three render inside `MwSdlAdvanceAudio(pitTime)`: the game thread
advances PIT time, one 48 kHz sample is synthesized at a time, and the PCM
goes into a bounded queue that the SDL3 stream callback drains. The SDL
callback never runs game code. `mwSdlAudioObserver` already exposes every
rendered block, which the tests below use.

### Campaign music (MW_MAIN, segment 0F41)

- **Data:** the five .MUS files are Standard MIDI Files, format 0, one track,
  96 ticks per quarter. They contain no Program Change events. Instruments
  arrive as SysEx:
  - Roland DT1 messages (41 10 16 12) that write the MT-32's patch and timbre
    memory: about 9 to 18 per song;
  - records under the driver's own manufacturer byte (the selected backend's
    `sysexManufacturer`). The driver parses them (0F41:0D41..0E81) into
    channel configuration, its 32-entry instrument table and the voice count.
    Each backend only accepts records with its own byte, so one file carries
    every device's setup side by side. Confirmed in slice 1 from the backend
    records in MW_MAIN's segment 28BE (AdLib at 00B0, MPU at 01EE, speaker at
    07FC): **7Ah AdLib** (11 voices), **41h MPU** (the Roland DT1 messages pass
    through to the MT-32), **7Fh speaker**. Every .MUS file has all three.
- **Sequencer:** MW_MAIN's own (0F41:02A1..0573) reads the SMF bytes and runs
  tempo and bar arithmetic. A priority voice allocator (11 voices, 0F41:0C37)
  feeds the selected backend:
  - **speaker:** one voice through PIT channel 2. Its instrument decoder is a
    bare RET;
  - **AdLib** (0F41:069A..0890): packed 11-byte instruments, authored route
    triples to chip channels and operators, rhythm mode and waveform select,
    a 115-entry frequency table, and calibrated bus-delay reads timed with
    the card's timers;
  - **MPU:** UART mode. A transmit queue drains one byte per fast timer
    callback (0F41:0912..0A01).
- **Clock:** the music handler runs from IRQ0, reprogrammed to divisor
  high byte 20h (8192, about 145.6 Hz), with rate shifts of up to 5 (about
  4.66 kHz) for the fast callback, and chains to the BIOS tick.

### Combat sound (BTECH, segments 1ADF, 2134, 2176, 21CA, 229C)

- **Data:** BTECHSND.SND, a small bytecode bank (229C parses it) with
  fifteen effects: footfalls, weapons, impacts.
- **Drivers:** 1ADF probes and installs speaker, Tandy, AdLib and MPU. 2134
  plays speaker channels with priorities (0810:096B gates events by
  priority). 21CA is a separate AdLib note and instrument driver, and 2176 an
  MPU intelligent-mode driver.
- **What the shipped bank actually uses:** only the speaker. The installed
  BTECHSND.SND holds no AdLib or Roland content, so with a card selected the
  effects still come out of the speaker, and combat has no music. The card
  drivers run but play nothing from shipped data.
- **Clock:** BTECH's own timer handler (1BF5:25E2) on IRQ0.

## 2. What we build instead

One `mw1o::audio` module, in two layers (as the scoping report sets out):

- **Output layer (SDL3):** device, mixer, bounded queue and stream callback.
  This is today's SDLBackend_Audio.cpp, kept and moved behind an interface.
  It owns no synthesis.
- **Synths**, each taking timestamped port events and rendering PCM:
  - `SpeakerVoice`: today's band-limited square, moved as it is;
  - `FmChip`: our own two-operator FM synth, written here (below);
  - `MidiOut`: the raw MIDI byte stream to a platform port (below).

Tandy stays dropped, as in the scoping report.

### AdLib: our own FM synth (`FmChip`)

A YM3812-style synth behind the same three calls the game already uses
(write index or data, read status, render).

- **What it covers:**
  - 9 channels of 2 operators, with multiplier, KSL, total level, ADSR rates,
    sustain flag, KSR, tremolo and vibrato;
  - feedback and connection;
  - the four OPL2 waveforms behind the wave-select enable;
  - **rhythm mode**: bass drum, snare, tom, cymbal, hi-hat with the noise
    generator. The campaign driver switches it on, so it's required, not
    optional;
  - the two timers and status bits, already our own code: Adlib.cpp keeps its
    1 µs bus model unchanged so calibration behaves as today.
- **Accuracy target:**
  - functionally faithful, not cycle-exact;
  - log-sine and exponent tables with the chip's attenuation steps;
  - envelope rates from the chip's documented rate tables;
  - phase increments from F-number and block;
  - mono output, rendered at 49,716 Hz and resampled to 48 kHz by the
    output layer.
- **Size:** about 700-900 lines of C++20.
- **Trade-off:** our synth will not match Nuked sample for sample. Nuked
  models the die. We aim for the same pitches, the same envelope shapes and
  timing, and the same timbre to the ear, which section 3 measures.

### MT-32: four options without Munt

| Option | What it sounds like | Cost | Assets |
| --- | --- | --- | --- |
| **A. MIDI out**: send the game's exact byte stream (SysEx included) to a platform MIDI port | real MT-32 if the player owns one (or runs an external synth); otherwise whatever the port is | small: three platform backends (CoreMIDI, WinMM, ALSA sequencer), about 300 lines, or one MIT MIDI I/O library (I/O only, not emulation) | none |
| **B. General MIDI remap**: parse the Roland patch SysEx, map each part's MT-32 timbre to a GM program, send Program Changes, drop the SysEx; route to the OS GM synth or a MIDI port | wrong in places: the songs program custom timbres by SysEx, which have no GM equivalent; drums map well | medium: an MT-32→GM table plus a SysEx reader, about 2-3 days; macOS needs an AudioUnit DLS synth host, Linux has no system GM synth | none (OS synth) |
| **C. Our own SF2 sampler** with the remap from B | as B, with the player's choice of soundfont | large: SF2 reader, sample voices, envelopes, filters, about 2 weeks | a soundfont the player supplies |
| **D. Drop MT-32**; keep AdLib (default) and speaker | AdLib and speaker only | none; removes the ROM settings and the MPU code | none |

**Recommendation: A, then D for everything else.**
- A keeps the MT-32 version authentic for anyone with the hardware (or a
  separately installed synth) at small cost, and carries no emulation or
  ROMs. The game's MPU UART path stays as it is: the bytes it sends today to
  libmt32emu go to the port instead.
- B and C are mismatched by design (custom timbres), so they'd ship a sound
  the game never had. Defer them unless playtesting asks for them.
- The combat MPU intelligent-mode adapter (2176 plus MPU401.cpp's track and
  conductor machinery) has no shipped content (section 1), so it's dropped
  along with the combat AdLib driver (21CA).
  - **Decision for review:** keep the 1ADF probe sequence as code (it decides
    which devices "exist"), but answer "absent" for AdLib and MPU in combat.
    Trace-visible behaviour then stays the speaker's.

Also from the scoping report, and cheap alongside: the player's own recordings
(OGG per track, as MW3's CD music works) as an optional override. That isn't
part of this phase.

## 3. How we prove it

Audio can't be bit-compared after the synth changes, but everything above the
synth can, and that's most of the risk. Three levels:

**Level 1: the port-event log, exact, against today's port.**
- A log of every hardware event, timestamped in PIT cycles. In the port, PIT
  time follows the wall clock, so the log runs under a test clock
  (`MECHWARRIOR_DEBUG_VIRTUAL_CLOCK`). Time then moves only where the port
  would wait (CPU-budget deadlines, idle pumps, 1 µs per port read), and the
  log depends on the script alone (slice 1). Events logged:
  - OPL index/data writes and status reads;
  - MPU bytes and status reads;
  - PIT channel 2 and 61h writes.
- It hooks at the four entry points in the table in section 1, and is enabled
  by `MECHWARRIOR_AUDIO_LOG`. It writes JSONL, one event per line.
- **Golden audio traces**, recorded once from the current port before
  anything changes, through scripted runs (tools/trace scripts, headless,
  `SDL_AUDIO_DRIVER=dummy`):
  - title music (MECH.MUS) for 30 s, then the bar (MECHBAR.MUS), on each
    device;
  - the debrief cues (LAUNCH, DEATH);
  - a combat script firing every weapon class: the fifteen effects.
- `check.py` gains an audio comparison that is exact on (time, port, value).
  Every refactor in this phase must leave these logs identical, so any change
  in the game-side drivers shows up exactly.

**Level 2: synth against synth, measured, against today's renders.**
- Before Nuked is removed, render each golden AdLib log through it once
  (offline, from the log, at 49,716 Hz) and keep the result locally. These
  are renders of the game's music, so they never go into the repo.
- Store only derived numbers in the repo, per note:
  - onset time;
  - fundamental frequency;
  - RMS envelope in 5 ms windows;
  - spectral centroid.
- Our `FmChip` renders the same log, and a tool compares the two within
  stated tolerances:
  - onsets within 1 ms;
  - fundamental within 3 cents;
  - envelope within 1.5 dB per window over the attack and decay;
  - centroid within 10%.
- Rhythm-mode voices are checked separately: noise has no fundamental, so
  envelope and centroid only.
- The speaker needs no level 2: its code moves without change, so a PCM hash
  over a golden log must stay identical.

**Level 3: the port against the original, under DOSBox-X.**
- MWSNAP can't log port writes: a real-mode TSR can't trap OUT. DOSBox-X has
  its own captures, which do the job:
  - **raw OPL capture (`mapper_caprawopl`, .dro):** every register write
    with millisecond delays;
  - **raw MIDI capture (`mapper_caprawmidi`):** the MPU byte stream.
- **Spike (slice 3), done:** no mapper key is needed.
  - **AdLib:** DOSBox-X's `DX-CAPTURE /O` shell command wraps the game and
    writes DROv2 from the first key-on.
  - **MIDI:** DOSBox-X's OSS MIDI handler, with `midiconfig` pointed at a
    plain file, writes every MIDI byte unbuffered (4-byte `SEQ_MIDIPUTC`
    records, no timestamps).
  - **Speaker:** `DX-CAPTURE /A` writes WAV.
  - SIGTERM makes DOSBox-X close the captures properly.
  - `tools/capture/run_audio_wsl.sh` runs it; `tools/trace/audio/compare_original.py`
    compares a capture with a golden, section by section.
- **What level 3 found (music, `tools/trace/audio/original/music.keys`):**
  the original matches the port's goldens on all three devices.
  - AdLib: 361 title and 659 bar key-ons, in order, onsets within 8 ms.
  - MIDI: 3,003 title and 4,730 bar bytes, exactly.
  - Speaker (WAV notes): 66 of 67 title notes (one short note seen on one
    side only) and 219 of 219 bar notes, onsets within 14 ms.
  - Combat: the original sends no MIDI and keys no OPL note, which supports
    dropping the combat card drivers (section 5).
- **Holding a screen on the original:** its empty polls take almost no
  emulated time, so MWSNAP has `wait:MS`, by the BIOS tick count.
- **Leaving the title early:** the title's length decides the start world,
  and leaving before its tune gives the bar other music. The original and
  the port agree on that path too (AdLib 569 of 569 key-ons, MIDI 4,698 of
  4,698 bytes). The "stall" seen earlier was that music measured against the
  full-title golden: nothing to change in the port.
- **Comparison:** same register writes in the same order; inter-event times
  within the capture's 1 ms resolution, with the start aligned on the first
  key-on.
  - **As built:** register order isn't comparable across a different history
    (DRO drops writes that don't change a register; voice allocation picks
    channels), so AdLib compares key-ons channel-free, MIDI its bytes, the
    speaker its WAV's notes. Onset gaps: median 0 ms, at most 8 ms (AdLib)
    and 14 ms (speaker), not the 1 ms planned. On AdLib, 1,004 of 1,018
    gaps are within 1 ms (the DRO's resolution); the 8 ms is one gap in each
    tune, and the tunes drift at most 8 ms over 36 s.
- MWSNAP's existing clock pin and fixed cycles keep the original
  deterministic.
- **Speaker against the original:** DOSBox-X's WAV capture, compared by onset
  and pitch track only, since its speaker model isn't ours.

Everything runs headless. Nothing plays aloud on the Mac: renders are files,
and the comparisons are numbers.

## 4. The fixed tick

Phase 5 replaces the CPU-work budget with a fixed simulation tick from SDL's
clock. For audio this is mostly good news. Audio already runs on PIT time,
not CPU time: `MwSdlAdvanceAudio(pitTime)` renders up to wherever the PIT
timeline has reached.

- **One audio timeline, owned by the fixed tick.** Each simulation tick
  advances PIT time by its fixed amount and delivers the IRQ0 callbacks due
  in that window, at the rate the game has programmed:
  - campaign: divisor 2000h, or faster with the rate shift;
  - combat: BTECH's own rate.
  - So the music handler runs exactly as many times per tick as the hardware
    would, whatever the frame rate.
- **Events keep sub-tick timestamps.** Port writes made inside a callback
  carry its PIT time, not the tick's start, so notes don't snap to tick
  edges. Synths render between events, as now.
- **Producer stays on the game thread.** PCM is synthesized on the game
  thread up to the current PIT time and queued. The SDL callback only drains.
  This keeps level 1 and level 2 deterministic, and the existing bounded
  queue and drift correction carry over.
- **Speed is set by the tick, not by audio.** If the host stalls, the tick
  catches up by running callbacks (bounded), and late PCM is counted as today
  (`lateFrames`). Audio never paces the game.
- **Dependency:** the tick rate itself is measured against DOSBox-X (scoping
  risk 4). Audio only needs IRQ0 delivered at the programmed rate inside each
  tick, so it can land before or after that measurement.

## 5. Size, slices and risks

**Slices**, each its own branch, traces green at every step:

| # | Slice | Days |
| --- | --- | --- |
| 1 | Port-event log, golden audio traces on today's port, and `check.py` support; confirm the SysEx manufacturer bytes | 3 |
| 2 | Output layer split from synthesis (`mw1o::audio`); speaker voice moved; PCM hash unchanged | 2 |
| 3 | DOSBox-X spike: headless OPL and MIDI capture; compare the original against the level 1 logs (finds any existing driver drift) | 2-3 |
| 4 | `FmChip`: melodic voices, envelopes, LFOs, waveforms | 4-5 |
| 5 | `FmChip` rhythm mode and noise; level 2 metrics tool and tolerances; reference renders made with Nuked before it goes | 3 |
| 6 | MT-32 as MIDI out (option A): platform ports, device choice in the config, ROM settings removed | 3 |
| 7 | Combat card drivers dropped (21CA, 2176, MPU intelligent mode); 1ADF answers "absent" for AdLib and MPU in combat; traces | 2 |
| 8 | Munt and Nuked-OPL3 removed from CMake, notices and docs (AUDIO.md, THIRD_PARTY_NOTICES.md); the game's own libraries then hold no LGPL | 1 |
| 9 | Fixed-tick integration: IRQ0 callbacks per tick at the programmed rate, sub-tick timestamps | 2-3 |

**Total:** about 22-25 developer days, roughly 4.5-5 weeks for one developer.
Slices 1-3 and 6-8 can run in parallel with the renderer work. Slice 9 waits
on the tick.

**Risks:**
1. **FM timbre by ear.** A functional OPL2 can sound thin or harsh next to
   the chip, especially through feedback and rhythm mode. Mitigations:
   - level 2 metrics on every note;
   - a listening pass by the project before release, since only a human can
     sign off timbre;
   - the documented YM3812 tables, not guesses.
2. **Rhythm mode is the hardest part** (shared operators and the noise
   generator), and the campaign uses it. Slice 5 is budgeted on its own for
   that reason.
3. **MT-32 users lose the built-in synth.** Option A needs hardware or an
   external synth. That's a deliberate trade, to be stated in the docs.
   B and C stay available as later work if wanted.
4. **macOS has no system MIDI synth on a port.** On a Mac without hardware,
   MIDI out plays nothing unless the player runs a synth. AdLib stays the
   default everywhere, so nobody hears silence by default.
5. **The headless DOSBox-X capture may not start without a key** (slice 3).
   Resolved: `DX-CAPTURE` and the OSS MIDI handler need no key (level 3).
6. **Fixed-tick timing.** If IRQ0 delivery per tick drifts from the
   programmed rate, tempo drifts. The level 1 timestamps catch it exactly.
7. **Dropping the combat card drivers changes what "exists" in combat.**
   Answering "absent" keeps the speaker path the traces already pin. Review
   point above.
