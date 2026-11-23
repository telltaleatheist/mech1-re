# No emulation

Status: plan for review. Docs only; no code until it's approved.

A project decision (2026-11-22) sets the direction:
- **No emulation.** The rebuild doesn't model a 1989 PC: not its sound chips, its video card, its I/O ports, interrupts, BIOS or timer chip, nor (pending, section 5) its memory layout.
- **Libraries.** Use modern open-source libraries, MIT or similarly permissive, where they fit.
- **SDL3.** Stay on SDL3 for output.
- **The bar is "play identical", not "byte identical".** That means the same songs, cues, timing, pictures and game, drawn and played by our own code rather than by a model of 1989 hardware.

mw1r-v1.0.0 is tagged only after sound, graphics and the hardware layer under them are rebuilt without emulation and gated (group A in section 5). Whether the memory images also come first (group B) is **pending a project decision**. Section 5 is the single path. Its slices are sized for checking, not speed: each one changes one thing that its check can see.

This plan replaces two parts of [the phase 5 audio plan](AUDIO_PLAN.md): its own FM synth (`FmChip`, a YM3812 model, so chip emulation) and its MT-32 options. That plan's finished work stays the base:
- its slice 1, the port-event log and golden audio traces;
- its slice 2, the output layer split into `src/Audio`;
- its slice 3, the DOSBox-X comparison with the original.

## 1. What is emulated today

### Sound

| File | What it models | Fate |
| --- | --- | --- |
| `src/Adlib/Adlib.cpp` | An AdLib card. Its index/data ports, status register and two timers feed Nuked-OPL3 (an OPL2/OPL3 chip model) | removed |
| `src/MT32/MT32.cpp` | A Roland MT-32, through Munt (libmt32emu) and the player's Roland ROMs | removed |
| `src/MT32/MPU401.cpp` | A Roland MPU-401 interface: UART mode, plus intelligent mode's tracks and conductor | removed |
| `src/Audio/SpeakerVoice.cpp` | The PC speaker. A band-limited square at PIT channel 2's divisor, gated by port 61h | becomes a tone voice that takes notes (pitch, on, off), not ports |
| `src/Audio/Mixer.cpp` | Mixes the speaker and the selected card on PIT time | kept, on game milliseconds |
| `src/SDL/SDLBackend_Audio.cpp` | The output layer: SDL3 device, bounded queue, stream callback. Also chooses the card | output kept; card choice removed |
| `src/SDL/SDLBackend_Time.cpp` | The PIT and IRQ0: the reprogrammed timer rate, the interrupt flag, port routing (`MwSdlWriteDevicePort`), 1 µs charged per port read | music clock replaces the IRQ0 model (section 2); the joystick's latch timing is input and stays for now |
| `src/Original/Runtime/MW_MAIN_0F41_ADLIB.cpp` | The campaign's AdLib backend: OPL register writes, the frequency table, rhythm mode, timer-calibrated bus delays | removed |
| `src/Original/Runtime/MW_MAIN_0F41_MPU.cpp` | The campaign's MPU backend: the UART transmit queue | removed |
| `src/Original/Runtime/MW_MAIN_0F41_SYNTH.cpp` | Backend selection and set-up: timer arguments, the vector swaps | reduced to "the rebuild's music" |
| `src/Original/Runtime/MW_MAIN_0F41.cpp` | The IRQ0 handler, BIOS chaining, the sequencer (SMF reader, tempo, bars) and the voice allocator | the sequencer stays (it is the score's timing); the IRQ entry goes |
| `src/Original/Audio/BTECH_1ADF*.cpp` | Combat's device probe and install: speaker, Tandy, AdLib, MPU | reduced to "the rebuild's sound" |
| `src/Original/Audio/BTECH_21CA.cpp`, `BTECH_2176.cpp` | Combat's AdLib driver and MPU intelligent-mode driver. They play nothing from shipped data | removed |
| `src/Original/Audio/BTECH_2134.cpp` | Combat's speaker channel player: priorities, pitch steps, bends, written as PIT divisors | stays as the effect player; outputs tone events, not ports |
| `src/Original/Audio/BTECH_229C.cpp`, `BTECH_0810_SOUND.cpp` | The BTECHSND.SND bank reader and the game's sound requests | kept: these are the game, not hardware |
| `src/Original/Memory/BTECH_304E_ADLIB.cpp`, `_MPU.cpp`, `_SOUND.cpp` | The drivers' state | ADLIB and MPU removed; SOUND kept for the effect player |

### Graphics

| File | What it models | Fate |
| --- | --- | --- |
| `src/SDL/SDLBackend_CombatVideo.cpp` | The EGA graphics controller, sequencer and CRTC registers. That covers the four read latches, map mask, bit mask, set/reset, data rotate, logical function, write modes 0-2, the CRTC start address for page flips, and the 3DAh retrace bit from the wall clock | removed |
| `src/Original/Video/BTECH_2A36*.cpp` | Combat's EGA driver. Pixels are drawn as byte addresses in the A000h aperture: a latch read before each write, register writes as port words between spans | rewritten to draw pixels into a canvas |
| `src/Original/Video/BTECH_1BF5*.cpp` | The graphics library: lines, polygons, clipping, ellipse, shapes, bitmaps | kept (it is drawing, not hardware); calls the canvas |
| `src/Original/Video/MW_EGA_0800.cpp` | The campaign's EGA overlay. Already one byte per pixel, but addressed by EGA page selectors (A000h/A200h), byte offsets and pixel masks, with palette changes through palette-register hooks | readdressed to canvas coordinates and palette entries |
| `src/Original/Memory/MW_EGA_0800_STATE.cpp`, `BTECH_304E_VIDEO.cpp`, `include/mechwarrior/ega_campaign.h`, `combat_video.h` | EGA tables: scanline offsets, pixel masks, plane stride, enabled planes, 6-bit palette codes | tables that only serve addressing go; palettes become RGB entries |
| `src/SDL/SDLBackend_Video.cpp` | The BIOS mode set (0Dh), EGA palette codes to RGB (`Mode0dColour`), page present, captures | presenter kept; the mode call and code table become a palette set once |

SDL3 is already the only video output, and nothing is emulated at the window. What remains is the model between the game's drawing code and the frame.

### The rest of the machine (group A unless marked B)

| Area | Files | What it models |
| --- | --- | --- |
| I/O ports | `SDLBackend_Time.cpp` (`MwSdlWriteDevicePort`, `ReadDevicePort`, routing), `SDLBackend_CombatVideo.cpp` (3C4h/3C5h, 3CEh/3CFh, 3D4h/3D5h, 3DAh), `SDLBackend_Input.cpp` (the game port), `SDLBackend_Audio.cpp` and `src/Audio` (61h, PIT 42h/43h), `MPU401.cpp` (330h/331h), `Adlib.cpp` (388h/389h) | a port bus: the game reaches every device by port number and byte |
| Interrupts | `SDLBackend_Time.cpp` and `SDLBackend_CombatPlatform.cpp` (`MwSdlSetInterrupts`, `MwSdlExchangeInterrupts`: the CLI/STI flag; vector install and restore; IRQ0 delivery), `MW_MAIN_0F41.cpp` (the INT 8 music handler and BIOS chain), `BTECH_1BF5_INSTALL.cpp`, `BTECH_1BF5_CLEANUP.cpp`, `BTECH_1BF5_KEYBOARD.cpp` (the IRQ1 handler), `MPU401.cpp` (its interrupt) | an interrupt controller and vector table |
| BIOS | `SDLBackend_Time.cpp` and `SDLBackend_CombatPlatform.cpp` (the 18.2 Hz tick count), `SDLBackend_Input.cpp`, `input_hardware.h`, `campaign_input.h`, `combat_input.h`, `MW_MAIN_081B_INPUT.cpp`, `BTECH_0810_INPUT.cpp`, `BTECH_1BF5_INPUT.cpp` (scancodes to BIOS keyboard words and shift flags; the INT 9 scancode ring), `SDLBackend_Video.cpp` (INT 10h mode get and set), `SDLBackend_Input.cpp` (the INT 33h-style mouse position) | the PC BIOS's keyboard, clock, video and mouse services |
| PIT and CPU timing | `SDLBackend_Time.cpp` (the channel-0 latch window, the virtual clock's per-read charges), `MwSdlDelayLoops` (calibrated busy loops at 0.4 µs each), `MW_MAIN_081B_INPUT.cpp` and `MW_MAIN_081B_STARTUP.cpp` (joystick calibration by counting game-port reads) | the 8253 timer chip and a CPU's speed |
| Memory images (B) | `src/Original/Memory/*_STATE.cpp`, `BTECH_304E_*.cpp`, `MW_MAIN_1090*.cpp`, `MW_MAIN_1AA5.cpp`, `MW_MAIN_28BE.cpp`, `BTECH_2FEB.cpp`, `BtechStartImage.cpp`, `NativeStart.cpp` | the programs' data segments as byte images at the original's offsets |
| Native addressing (B) | `src/Support/NativeAddress.cpp`, `native_address.h` (29 files include it), `NativePieces.cpp` | selector-to-segment bindings and byte windows |
| C runtime (B) | `BTECH_0810_HEAP.cpp`, `src/Support/CombatNearHeap.cpp`, `BTECH_0810_STREAMS.cpp`, `BTECH_0810_DIVISION.cpp`, `BTECH_1BF5_DIVIDE.cpp`, `BTECH_0810_EXIT.cpp`, `MW_MAIN_0800.cpp` | the 1989 compiler's heap, streams, divide faults and exit |
| DOS files (B) | `SDLBackend_Files.cpp`, `dos_files.h` (the INT 21h boundary: handles, carry flag, AX error), `BTECH_1BF5_ARCHIVE.cpp` | DOS handles and 8.3 names |

## 2. What replaces each piece

### Output: SDL3 audio, kept

The SDL3 stream and bounded queue in `SDLBackend_Audio.cpp` stay as they are (SDL is zlib-licensed and already a dependency). miniaudio (public domain or MIT-0) would also do the job, but it would duplicate SDL3, so it isn't used. Synthesis stays on the game thread, and the callback only drains the queue, as today.

### Music: the game's own sequencer, played as notes through TinySoundFont

The five .MUS songs are Standard MIDI Files. The game's sequencer (`MW_MAIN_0F41.cpp`) decides what to play and when:
- song order;
- tempo and bar arithmetic;
- the title-length rule that picks the start world.

That is game behaviour, and it stays.

**What changes is below it.** Today the sequencer feeds an OPL register driver or an MPU byte queue. In the rebuild it hands its channel events (note on and off, velocity, controllers, pitch bend) to a new `mw1o::audio::Music`, each timestamped in game milliseconds. `Music` renders them with **TinySoundFont** (MIT, one header, SF2) from a bundled soundfont.

**Instruments.** The songs carry no Program Change. The MT-32 version sets custom timbres by SysEx, and the AdLib version sets 11-byte FM instruments by its own records, so neither maps to General MIDI by itself. The rebuild therefore carries **our own instrument table**: per song and channel, a GM program, a level and a pan, plus a drum-key map for the rhythm channel.
- It is chosen by listening against the original's MT-32 and AdLib versions.
- It is project data: numbers only, no text from the game.

**Soundfont.** FluidR3 Mono GM (MIT, as secondary sources state: Frank Wen's FluidR3 GM, converted to mono by Michael Cowgill). It ships as a **subset holding only the programs the table uses**, which is expected to be a few MB.
- **Licence, confirmed at the source (S2).** S2 confirms the licence from the pinned upstream file's own licence text, not from secondary sources. The package carries that text beside the bank. If the Mono conversion's terms differ from FluidR3 GM's MIT, S2 stops and the lead decides before another bank is chosen.
- A small tool of our own builds the subset from the pinned upstream file, so the package doesn't carry the whole bank.
- SF2, not SF3, so TinySoundFont needs no Vorbis decoder.
- GeneralUser GS was considered and not chosen. Its licence allows bundling, but its samples' provenance is uncertain, and Debian has declined it for that reason.

**The tone voice** is our own small synth that plays pitches: a frequency, a start and a stop, with a band-limited square as its waveform so it doesn't alias. It takes notes, never port writes or PIT divisors. It is not a speaker model, and must not become one.

**Speaker-style music, as an option.** The original's speaker version is a single voice that the allocator picks from the same score. The option plays that voice's notes through the tone voice. No PIT or port-61h model is involved: the tone voice takes notes, not ports.

**The music clock.** The original reprograms IRQ0 so the sequencer runs about 145.6 times a second, and faster for the MPU's byte queue. The rebuild calls the sequencer's tick at that same rate from the game's millisecond clock, with each event keeping its own time inside the tick. Nothing models the PIT or the interrupt controller.

### Effects: the game's own effect player, played as tones

BTECHSND.SND's fifteen effects are bytecode: the 229C reader plus 2134's channels, priorities, pitch steps and bends. That logic stays.

Where 2134 writes PIT divisors and the 61h gate today, it hands **tone events** to `mw1o::audio::Effects` instead:
- start and stop;
- frequency in Hz;
- the step or bend schedule;
- priority.

`Effects` plays them on the same tone voice as speaker-style music. The effects are tones by nature, so playing them as tones is a rebuild, not emulation. A later, optional sample pack could replace any effect, keyed by effect number.

### What leaves the build

- Nuked-OPL3 (LGPL-2.1-or-later) and Munt libmt32emu (LGPL-2.1-or-later), with their shared libraries in the packages.
- The ROM settings (`mt32_control_rom`, `mt32_pcm_rom`).
- `MPU401.cpp`, `Adlib.cpp` and `MT32.cpp`.
- The campaign's AdLib and MPU backends, and combat's 21CA and 2176.
- The `sound_system` setting becomes `music=soundfont|speaker|off`. The old key is read once, and the log says it has been replaced.

### The rest of the machine

| Area | Replaced by |
| --- | --- |
| I/O ports | Nothing. Each device's caller talks to its replacement directly: `Music` and `Effects` (notes and tones), `Canvas` (pixels, `present`), input events. `MwSdlWriteDevicePort` and `ReadDevicePort` go once their last caller has. |
| Interrupts | Game events on the game thread. A key arrives as an input event, not an IRQ1. The music tick and the BIOS-tick count are calls from the game's millisecond clock, not IRQ0. There is no interrupt flag, no vector table and no BIOS chain. |
| BIOS keyboard | SDL3 key events into a typed input queue: key, modifiers, press or release. Campaign code asks for "the next key"; combat reads the keys held this tick. The campaign's read and combat's per-tick read keep their game meaning, since the key scripts depend on them. |
| BIOS clock | The game's millisecond clock. The few places that read the 18.2 Hz count (the title's length rule among them) get the same count, derived from milliseconds. That is a clock, not a timer chip. |
| BIOS video mode | `Canvas` set-up. |
| BIOS mouse | SDL3 mouse motion, scaled to the 320×200 frame. |
| PIT latch, delay loops | Removed. A wait is a wait on the game clock. A delay loop that only timed hardware goes; one that paces the game becomes a timed wait. |
| Joystick calibration | SDL3 gamepad and joystick axes, already centred and scaled. The calibration screen keeps its game role (it is what the player sees) but reads axes, not counted port reads. |
| Memory images (B) | Typed C++ state with owners: one struct per subsystem, fields by name. Data the game loads comes from our files (phase 2's JSON) into those structs, not into byte images at the original's offsets. |
| Native addressing (B) | Ordinary references and spans. |
| C runtime (B) | The C++ standard library. Where a 1989 limit is game-visible (a heap that runs out, a divide that faults), it stays as an explicit rule with a test, not as a copy of the allocator. |
| DOS files (B) | `std::filesystem` and streams, by the game's file names, case-insensitive where the game relies on it. |

### Graphics: a canvas, not an EGA card

`mw1o::video::Canvas`:
- 320×200, one byte per pixel (a 16-colour index);
- as many pages as the game uses;
- drawing operations in pixel coordinates: pixel, span with a logical function (copy, AND, OR, XOR), line, bitmap, glyph;
- `present(page)`;
- a 16-entry RGB palette, set from the game's palette tables once instead of through palette registers.

Changes per piece:
- **Combat driver (2A36):** draws spans and pixels into the canvas. Its latch reads, map-mask and bit-mask writes and CRTC start writes disappear.
- **Page flip:** becomes `present`.
- **Retrace waits:** become the frame pacing the fixed combat tick already does.
- **Campaign overlay (MW_EGA_0800):** keeps its drawing and addresses pixels by x and y instead of selector, byte offset and mask.
- **The overlay's documented unchecked reads** (a row past 200 and so on) keep their behaviour and their stderr note.

## 3. How we check "play identical"

### Sound

1. **The note log, exact.** A new log at the sequencer's output records each event as (game ms, channel, event, key, velocity).
   - Goldens are recorded from today's port, through its MPU path, before anything changes. They cover the title, the bar, each world's song, and the LAUNCH and DEATH cues.
   - Level 3 of the audio plan showed the port's MIDI byte stream already matches the original's under DOSBox-X: 3,003 title bytes and 4,730 bar bytes, exactly. So these goldens are anchored to the original.
   - Every sound slice keeps the note log identical. The instrument table changes the sound, never the notes or their times.
2. **The tone log, exact.** Combat's effects as (game ms, effect, frequency steps). The goldens are converted from today's port-event log, whose PIT divisors give the frequencies.
3. **The traces, unchanged.** Sound doesn't touch game state, so check.py stays at 8/8 for 1990 and 6/8 for 1989 (combat_cockpit and combat_retreat are the known two), and exe_free stays 8/8.
4. **Listening checks.**
   - A fixed list is rendered headless to files, never played aloud by automation: each song, LAUNCH, DEATH and the fifteen effects, on the soundfont and on speaker-style.
   - The project owner listens to them next to the original's MT-32, AdLib and speaker captures, using the listening sheet (section 5), and signs off the instrument table and levels.
   - Only a listener can judge timbre. The numbers above cover everything else.

### Graphics

1. **The frames, pixel-exact.** The canvas draws the same pixels with the same palette, so the traces' frame hashes stay identical. Graphics can stay byte-exact even though nothing models the card any more. Any difference is a bug.
2. **Looking checks.** The [visual acceptance](../VISUAL_ACCEPTANCE.md) list, captured side by side with the original.

### How the traces change

The traces compare snaps. Today a snap holds:
- the campaign's state, read where the original keeps it (MW_MAIN's data segment, at the original's offsets);
- the frame;
- with the audio scripts, the sound-port event log.

Two of those are port-level, and group A retires them:
- **The sound-port log** (OPL, MPU, PIT channel 2 and 61h writes) is replaced by the **note log and tone log** (section 3). These are game events: what the score and the effect player asked to sound, and when.
- **The video bytes** (planes and latches) are already reduced to the frame's 320×200 indices. That frame, and its hash, stays the graphics check.

**New, from slice T1:** a **game-event log** alongside each snap. It records:
- each input the game asked for and what it got (campaign key reads, combat ticks with the keys held);
- each screen the game drew to completion;
- each sound request.

These events survive every slice in both groups, so later slices compare against them.

**Key scripts don't change.** `idle:N`, `ticks:N` and `make`/`break` already speak in game events (the game's polls and ticks), not hardware.

**The original's side keeps its byte reading.** `state.py` reads MWSNAP's captures of the original by the original's offsets. With group B, that mapping (offset to field name) becomes the only place those offsets live. The port's own snaps then name fields directly, and `check.py` compares by name.

## 4. Libraries and licences

| Dependency | Licence | Use | Permissive? |
| --- | --- | --- | --- |
| SDL3 3.2 | zlib | window, input, audio output | yes |
| libarchive 3.7.9 | BSD-2-Clause | the setup reads downloads | yes |
| zlib 1.3.1 | zlib | libarchive | yes |
| xz 5.8.1 (liblzma) | 0BSD | libarchive | yes |
| re-setup-kit | MIT | the setup screen and folders | yes |
| stb_truetype, stb_image | MIT or public domain | in re-setup-kit | yes |
| miniz (inflate) | MIT | in re-setup-kit | yes |
| Saira Condensed Bold | SIL OFL 1.1 | the setup screen's face | yes, for a font (reserved name "Saira") |
| **Nuked-OPL3** | **LGPL-2.1-or-later** | AdLib option today | **no: removed by slice S9** |
| **Munt libmt32emu** | **LGPL-2.1-or-later** | MT-32 option today | **no: removed by slice S9** |
| Roland MT-32 ROMs | proprietary, supplied by the player, never shipped | MT-32 option today | removed with Munt |
| TinySoundFont | MIT | music synthesis (new) | yes |
| FluidR3 Mono GM, subset | MIT | music instruments (new) | yes |
| *miniaudio* | *public domain or MIT-0* | *not used: SDL3 covers output* | yes |
| *GeneralUser GS* | *its own licence; sample provenance uncertain* | *not used* | *not chosen* |

These are build and test tools only; none is linked in, and only the installer builder's runtime ends up in a package:
- **DOSBox-X (GPL-2.0):** the original's reference captures (`tools/capture`); never shipped.
- **Python:** the tests and packaging scripts.
- **Inno Setup:** builds Setup.exe. Its licence allows distributing the installers it makes.
- **appimagetool and the AppImage runtime:** MIT; the licence is to be confirmed against the pinned tool in slice S9.

## 5. The path to mw1r-v1.0.0

Every slice below is its own branch with the full gate green: both setups, the traces, exe_free and packaging. Once a check is recorded, it stays exact on every later slice unless that slice says otherwise. Those checks are the note log, the tone log, the game-event log and the frame hashes. A graphics or input slice must leave the sound logs exact, and a sound slice the frames.

### Group A, the hardware layer: before the tag

**The order, and why:**
1. **Measure first (T1).** Every golden is recorded from today's port before any code it observes changes: the game-event log, the note log, the tone log.
2. **Music with provisional instruments next (S2-S3),** so the listening material goes to the project early. Listening is the one step a person has to do.
3. **Graphics and input while the listening happens (G1-G5, H2-H3).** These are checked mechanically (frames pixel-exact, game events exact), so they need no one's time to sign off.
4. **Sound finished, timers and interrupts, ports retired (S4-S9, H4-H5).** The ports go last because every device has to move off them first.
5. **The release gate (R),** then the tag.

| # | Slice | Check | Days |
| --- | --- | --- | --- |
| T1 | The game-event log beside each snap; the note log at the sequencer's output and the tone log at 2134's output; check.py compares all three. Goldens from today's port: the six campaign scripts, the combat scripts, every song, LAUNCH, DEATH, the fifteen effects. The songs' channel layout and rhythm channel confirmed | goldens reproduce run to run | 4 |
| S2 | TinySoundFont vendored with its licence. The soundfont subset tool (our own) with tests: a subset holds exactly the listed programs and plays them the same as the full bank | tool tests; package licences | 2 |
| S3 | `Music` behind `music=soundfont`, with a first instrument table. The listening list rendered to files in `outbox/<date>/mech1-re-listening/` with a one-page sheet (below) | note log identical | 3 |
| G1 | `Canvas` and the RGB palette; the presenter takes a canvas page. The game still draws through the EGA model into it | frames identical | 2 |
| G2 | Combat text and single pixels (2A36's glyph and pixel paths) through the canvas | frames identical | 2 |
| G3 | Combat spans and lines (2A36's spans, lines, logical functions) through the canvas | frames identical | 3 |
| G4 | Combat bitmaps, blits and polygons through the canvas; then the EGA register, latch and plane model removed; flips become `present` | frames identical | 3-4 |
| G5 | The campaign overlay addressed by pixel; palette registers become palette entries; INT 10h mode calls become canvas set-up. The 3DAh status timing removed | frames identical; sound logs exact | 4 |
| H2 | Keyboard: SDL3 key events into a typed input queue. The BIOS keyboard words, shift flags and the INT 9 scancode ring removed, along with the IRQ1 handler | game-event log identical; traces 8/8 and 6/8 | 4 |
| H3 | Mouse and joystick: SDL3 mouse scaled to the frame; SDL3 axes for the joystick; the calibration screen reads axes. Game-port read counting and the delay loops that only timed hardware removed | game-event log identical | 3 |
| S4 | The instrument table from the listening notes, per song: programs, levels, pans, the drum map. Re-rendered for a second listen if asked | note log identical; the project owner's sign-off | 2, plus listening |
| S5 | `Effects`: 2134 outputs tone events to the tone voice | tone log identical | 3 |
| S6 | Speaker-style music on the tone voice. The PIT channel 2 and 61h decoding removed | note log identical | 2 |
| H4 | Timers and interrupts: the game's millisecond clock calls the music tick at its rate and supplies the 18.2 Hz count where the game reads it. The interrupt flag, vector install and restore, the INT 8 handler's IRQ entry and BIOS chain, the PIT latch window and the per-read time charges removed | note log identical, timestamps included; game-event log identical | 5 |
| S8 | The campaign's AdLib and MPU backends and combat's 21CA and 2176 removed. 1ADF reports the rebuild's sound | logs and traces exact | 2 |
| S9 | Nuked-OPL3, Munt and MPU401/Adlib/MT32 removed from the build. ROM settings dropped; `sound_system` replaced by `music`. Notices, AUDIO.md and packaging (no nukedopl or mt32emu library) updated. The AppImage tooling's licence confirmed | packaging tests; no LGPL in the package | 2 |
| H5 | The port bus retired: `MwSdlWriteDevicePort`, `ReadDevicePort` and the port-word hooks removed with their last callers; the sound-port event log retired in favour of the note and tone logs | everything above exact | 2 |
| R | The release gate: every gate on Windows, macOS and Linux from fresh builds. Packages from a local tag. The listening list and the visual acceptance list signed off on the packaged builds | all of the above | 2 |

**Group A total:** about 50-51 developer days plus the listening sessions. That is roughly 10 weeks, planned for **one developer, in order**.

(A note for later, not part of the plan: G1-G5 and H2-H3 touch different files from T1-S4, so a second developer could take them in parallel, with the gate order unchanged. The lead revisits this when a second developer is free.)

**The listening sheet (S3, and S4 if a second listen is asked for).** Listening is the project owner's time, so the list is short and grouped:
- campaign songs;
- the LAUNCH and DEATH cues;
- combat effects;
- speaker-style.

For each file, one line on the sheet gives:
- the song or cue;
- which original capture to play beside it (MT-32, AdLib or speaker, from `tools/capture`);
- what to judge (instrument fit, balance, drums, the effect's character).

Note timing isn't on the sheet: the note log already checks it.

(The earlier S7, the music clock, is folded into H4. S1 became T1.)

### Group B, memory images to typed state: PENDING

**Before or after the tag is pending a project decision.** Until it is made, group B is not on the path to the tag.

Group B overlaps the fork plan's phases 3-6, which give the game's state its owners. It is sized with those phases rather than here. Its order, if it goes ahead:

| # | Slice | Check |
| --- | --- | --- |
| B1 | Snaps by name: a snapshot schema of named fields. `state.py` maps the original's offsets to those names; `check.py` compares by name | traces identical, now by name |
| B2 | Campaign state (MW_MAIN's 1090, 0F41, 28BE, 1AA5 images; MW_0806's and MW_EGA's) as typed structs with owners | traces by name |
| B3 | Combat state (BTECH's 304E, 0810, 1ADF, 1BF5, 22B6 and 2FEB images, the start image) as typed structs, one subsystem per slice | traces and frames |
| B4 | Native addressing removed: references and spans in place of selector bindings and byte windows | traces and frames |
| B5 | The C runtime: the standard library in place of the 1989 heap, streams and divide, keeping any game-visible limit as an explicit rule with a test | traces and frames |
| B6 | DOS files: `std::filesystem` in place of the INT 21h handle layer and 8.3 names | traces; the setup and save tests |

## 6. Risks

1. **Instrument choice is taste.** No GM program is the MT-32's custom timbre. The table is chosen by ear, and the listening pass signs it off. Until then, S3's first table plays the right notes at the right times with provisional instruments.
2. **Soundfont size.** The full FluidR3 Mono bank is large. The subset tool (S2) keeps the package small; S3 measures and states the subset's size.
3. **Tone voice and the original speaker.** The original speaker's timbre came from the PC's speaker cone, not from a square-wave formula. The bar is the same pitches and timing (the tone log), not that hardware's sound.
4. **Clock-derived BIOS ticks.** Game logic that reads the 18.2 Hz count (the title's length rule picks the start world) must see the same counts from the millisecond clock. The game-event log catches any difference exactly. Likewise the music clock depends on the combat tick's timing. The note log's timestamps catch any tempo drift exactly.
5. **Players who used the MT-32 or AdLib options** lose them. That is the decision. MIDI out to a real device isn't emulation and could return later as an option, but it isn't in this plan.
