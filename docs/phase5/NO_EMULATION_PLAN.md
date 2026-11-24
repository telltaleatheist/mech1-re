# No emulation

Status: plan for review. Docs only; no code until it's approved.

A project decision (2026-11-22) sets the direction:
- **No emulation.** The rebuild doesn't model a 1989 PC: not its sound chips, its video card, its I/O ports, interrupts, BIOS or timer chip, nor its memory layout (group B, section 5).
- **Libraries.** Use modern open-source libraries, MIT or similarly permissive, where they fit.
- **SDL3.** Stay on SDL3 for output.
- **The bar is "play identical", not "byte identical".** That means the same songs, cues, timing, pictures and game, drawn and played by our own code rather than by a model of 1989 hardware.

mw1r-v1.0.0 is tagged only after sound, graphics and the hardware layer under them are rebuilt without emulation and gated (group A in section 5), and the memory images are rebuilt as typed state (group B, a project decision). Section 5 is the single path. Its slices are sized for checking, not speed: each one changes one thing that its check can see.

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

**Instruments.** The songs carry no Program Change. The MT-32 version picks the MT-32's built-in timbres for its parts by SysEx (S2 found preset timbres, not custom ones), and the AdLib version sets 11-byte FM instruments by its own records, so neither maps to General MIDI by itself. S2's provisional programs are the nearest GM program to each built-in timbre (`data/music/soundfont_programs.txt`). The rebuild therefore carries **our own instrument table**: per song and channel, a GM program, a level and a pan, plus a drum-key map for the rhythm channel.
- It is chosen by listening against the original's MT-32 and AdLib versions.
- It is project data: numbers only, no text from the game.

**Soundfont.** FluidR3 GM (MIT, © 2000-2002, 2008 Frank Wen), the SF2 in Debian's fluid-soundfont 3.1 upstream tarball. It ships as a **subset holding only the programs the table uses**.
- **Licence, confirmed at the source (S2).** The tarball carries Frank Wen's `COPYING`, the MIT grant, and his `README`, which credits the samples to public-domain sources, his own recordings and named contributors, all released under that MIT grant. No sample source is credited under other terms. Both texts are kept verbatim in `data/music` and copied into the package's `licenses` folder. Debian's own packaging (`debian/*`, GPL-2+) is not used.
- **Pinned and fetched once.** The tarball is pinned by its SHA-256 and a snapshot.debian.org URL. `tools/deps/fetch_soundfont.sh` fetches it once per machine into `build-deps/`, as `build_archive.sh` does libarchive's sources. A build or a player's install never downloads: configure refuses without the source.
- **Generated, not committed.** Our tool (`tools/soundfont/subset.py`) cuts the subset from the pinned bank at build time. The repo holds the program list, the subset's SHA-256 and the licence texts, and the build refuses a subset that hashes otherwise. A ctest proves through TinySoundFont that every kept program has the full bank's regions and sample points.
- **Size.** S2's 12 provisional programs and the standard kit come to 34.6 MiB, because the bank is stereo throughout and the kit alone is 10.6 MiB. That was too large to commit, hence the fetch. Cutting to the keys the songs play (about 16 MiB) or downmixing to mono are left for R, checked by listening, because the instrument table changes in S4 and pitch bends can reach outside today's note ranges.
- **Not FluidR3 Mono.** Michael Cowgill's mono conversion is also MIT, but it is published only as SF3 (MuseScore, Debian's fluidr3mono-gm-soundfont). Its SF2 exists only behind a forum's file-sharing link, which can't be pinned.
- SF2, not SF3, so TinySoundFont needs no Vorbis decoder.
- GeneralUser GS was considered and not chosen. Its licence allows bundling, but its samples' provenance is uncertain, and Debian has declined it for that reason.

**The tone voice** is our own small synth that plays pitches: a frequency, a start and a stop, with a band-limited square as its waveform so it doesn't alias. It takes notes, never port writes or PIT divisors. It is not a speaker model, and must not become one.

**Speaker-style music, as an option.** The original's speaker version is a single voice that the allocator picks from the same score. The option plays that voice's notes through the tone voice. No PIT or port-61h model is involved: the tone voice takes notes, not ports.

**The music clock.** The original reprograms IRQ0 so the sequencer runs about 145.6 times a second, and faster for the MPU's byte queue. The rebuild calls the sequencer's tick at that same rate from the game's millisecond clock, with each event keeping its own time inside the tick. Nothing models the PIT or the interrupt controller.

### Effects: the game's own effect player, played as tones

BTECHSND.SND's effects are bytecode: the 229C reader plus 2134's channels, priorities, pitch steps and bends. That logic stays.

**The effects the game requests.** The bank holds fifteen effects, 0 to 14. Both BTECH revisions request thirteen of them, the same thirteen at the same priorities from the same sixteen call sites (checked in each revision's unpacked image): 0 to 8, 10, 11, 13 and 14. Effects 9 and 12 are in the bank but neither revision ever requests them. The rebuild plays what the game plays, so they get no golden and no render.

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

**From G1** (`src/Video/Canvas`):
- **Pages:** eight, each a 64K-pixel stride in one block of memory. A page selector's page (A000h, A200h, ...) is a canvas page, and the rest of each stride past row 199 is the page's off-screen tail: the game's unchecked drawing and reads (a row past 200 and so on) still reach it.
- **Palette:** RGB entries, set from the game's palette codes by `PaletteCodeColour`, what the codes show in the game's 320×200 mode.
- **Presenter:** takes a canvas page and draws it with the canvas palette. The display start (the campaign's page selector, combat's CRTC start) always falls on a page.
- **Trace frame hash (G1b):** covers the indices and the colours they show (`rgb`), no longer the raw palette codes. Codes that show the same colour are the same screen.
  - The original stays the reference. `state.py` turns the original's captured codes into colours by `tools/trace/palette.py`, a mode 0Dh rule written separately from the port's `PaletteCodeColour`. The ctest `palette_rule` requires the two to agree on all 256 codes.
  - The goldens gained `rgb` from a port recording that equals them in every other field. Over the goldens, the code sets and the colour sets correspond one to one (3 and 3).
  - Until G5 the port also wrote the codes (`.palette`) as raw data. Since G5 it writes only indices and colours. The original's captures keep their codes, which `state.py` turns into colours.

**From G2-G4** (combat's drawing, `src/Original/Video/BTECH_2A36_CANVAS`):
- **Every 2A36 path draws on the canvas by pixel:** glyphs, pixels, spans, lines, polygons, the bitmap blit, bitmap preparation, capture and restore, the rectangle copy and the packed screen upload. 2A36 keeps its own addressing (a page selector, a byte offset of eight pixels, a mask with the leftmost pixel at bit 7) and draws through a pen: the colour where the mask has a 1.
- **The game's planar form stays where it is game data.** Bitmaps and saved rectangles keep one colour bit of eight pixels per byte, as the game stores them; the canvas reads and writes that form from pixels. The bitmap blit ORs each colour bit its colour selects into the pixels.
- **No card is modelled any more.** The graphics controller, sequencer, latches and planes are gone, and so are their port writes and reads in 2A36 and 1BF5. A write to those ports stops the game with a message. The CRTC index and data pair, for combat's adapter check, and the retrace status, for the flip's wait, stayed until G5.
- **Flips present.** A combat page flip presents the new front page on the canvas where the original sets the CRTC start, and the host shows the canvas's presented page. A mode set clears the canvas and presents page 0.
  - *The hand-over into combat, against the original.* Recorded frame by frame under DOSBox-X (every emulated frame of the combat_cockpit opening), the original goes straight from the deploy screen's last frame to a black screen, then the briefing and the cockpit. BTECH sets its video mode first thing, before it loads anything, so the black is its loading time (361 frames at 3,000 cycles). No other frame appears between the deploy screen and the black. The port shows the same sequence (deploy, black, briefing, cockpit; the share of black pixels agrees at each step), and presents nothing between combat starting and BTECH's mode set.
- **The latches: no game rule was needed.** G3 had kept the EGA's latch reuse in pixel form (a byte written with no read of its own takes the last read byte's pixels where its mask is 0, and combines with them under a non-copy function). G4 dropped it, because nothing the player sees depends on it:
  - *Combat draws only by copy.* Both BTECH revisions (1989-11-03 and 1990-01-22) contain exactly four writes to the data rotate register, each selecting copy with no rotation. Neither writes enable set/reset. A temporary entry check on every 2A36 drawing routine but the glyph and pixel, over all eight traces (about 165,000 calls), found write mode 2, copy, no set/reset and all four planes every time. G2's check had found mode 2 and all four planes at every glyph and pixel. So the XOR-span case the latches would change cannot arise in the original.
  - *By copy, the reuse cancels within the routine.* The only bytes written without their own read are the whole bytes of a span or polygon row. Solid rows use mask FFh, so the latches don't show. Dithered rows (masks AAh, then 55h) take the leading byte's pixels in the 55h columns in the first phase. The second phase reloads the latches from its first whole byte, whose AAh columns now hold the first colour, and overwrites the 55h columns. The pixels left are those of drawing each byte where its mask has a 1.
  - *The original agrees.* Dithered polygon rows with whole bytes, the case above, occur about 70,000 times in the two combat traces. Those frames match the goldens, which the original matches under DOSBox-X (G1b). They still match with the latches gone. The alternating span (1D40) has the same phases, but no EGA play reaches it, so it is covered by the reasoning and the canvas tests, not by a trace. Its only callers, the styled rectangle and the ellipse, take it for a dithered line colour, and dithered entries are only ever 16-31 (selecting a palette leaves 0-15 plain). Every line colour those callers get is 0-15: constants, the EGA HUD scheme's colours, and the ellipses' colours. The only ellipses in all ten shape banks are OTHPCK's, in colours 8, 12, 14 and 15, and no camera remaps colours.

**From G5** (the campaign overlay, the palette, the mode, the status port):
- **The campaign overlay (MW_EGA_0800) draws on the canvas by pixel.** It keeps its own addressing, as 2A36 does: a page selector, a byte offset of eight pixels, and a mask with the leftmost pixel at bit 7. Fills, copies, sprites, glyphs, transparent text, pixels and the page clear all draw through the canvas's pixel and bit-pattern operations. Nothing writes the canvas's memory directly any more.
  - The overlay's 16-bit offsets still run past row 199 into the page's tail, and its documented unchecked reads keep their stderr note.
  - The function a sprite or pixel inherits from the overlay's last routine (copy, or OR after transparent text) is the overlay's own state. A mode set resets it to copy, as the adapter's does.
- **Flips present.** The overlay's flip presents its new display page, as combat's does. The host shows the canvas's presented page in both programs. A mode set presents page 0, and the campaign always flips before it next shows anything.
- **Palette registers are palette entries.** Every BIOS palette call (one register, the whole table, the border) sets the canvas's entry for that code by `PaletteCodeColour`. This covers the campaign's default and alternate tables, the intro text's colour, combat's palette slots and its border. The host keeps no codes: it decides a palette change by the colours, and captures write `.indices` and `.rgb` only.
- **Mode calls are canvas set-up** (`src/Support/VideoMode.cpp`). Mode 0Dh sets up the canvas: every page cleared, page 0 presented, the mode's 16 default colours and a black border. Mode 3 closes it, and a closed canvas is not shown. The campaign saves the mode at start and sets it again on exit; the launcher's text clear and combat's exit are mode 3 sets. Any other mode stops the game with a message.
- **No video port is modelled.** The 3DAh retrace wait after a combat flip is never requested: all three callers pass no wait. It is gone, and the fixed combat tick paces frames. Combat's CRTC check (1BF5:00FD) runs only when the BIOS reports no EGA, to tell CGA or Tandy apart; it now stops with a message, as it already did at its end. A read or write of any video port reaches the port bus's unknown-port failure.
- **The check.** The traces match (8/8 on 1990). The event logs match (12/12). A temporary log of every frame the host presented, over all eight traces and never committed, was byte-identical between G4 and G5: the same count, the same pages and the same pixels and colours, in order.

**From H2** (the keyboard):
- **Keys are typed events** (`key_events.h`). A key is its PC scan-code number (set 1), which both games' key tables are written in. The host turns SDL3 key events into them; no BIOS keyboard data, port 60h or 61h, or IRQ1 is modelled.
- **The campaign reads typed presses**: the key, the character it types and whether Control was held. It composes the key word (key above character) it has always compared, so the event log's key reads are the same words. The Control test that used the BIOS shift flags asks the input calls whether Control is held. The host's queue keeps 15 keys, as the BIOS ring did.
- **Combat's keyboard handler (1BF5:2CCB) handles one key event.** Its translation, held-key masks, shift flags and key queue are the game's own state now: the BIOS ring and shift-flag byte became `combatKeyQueue` and `keyboardShiftFlags`, empty and zero at each combat's start, as the port's BIOS data was. The queue keeps the original's room test, which drops a key while exactly one waits; that is the game's rule and stays. Ctrl-Escape and the cleanup empty the queue as before. The handler no longer acknowledges a port or the interrupt controller.
- **Delivery keeps its point and its gate until H4.** A combat key event is delivered where the IRQ1 was, under the same interrupt flag and controller mask, and the test clock still charges the time the two port reads took (one before the child's handler is installed). H4 removes those with the rest of the port timing.
- **Still read from the BIOS data:** install's keyboard-compatibility byte (40:96h), which only the PCjr/Tandy path uses.
- **The sound-port goldens changed only by the acknowledges.** The three combat logs lost the handler's 56 port-61h read-and-write triples and nothing else: with them removed from the old goldens, every remaining line matches, the same values at the same cycles. They were re-recorded. The music logs are unchanged.
- **The check.** The traces match (8/8 on 1990). The event logs match (12/12).

**From H3** (the mouse and the joystick):
- **The mouse is in the frame's pixels.** Positions set and read are the 320×200 frame's, not INT 33h's doubled x; the travel map's shift up and down are gone (they cancelled exactly). The campaign asks the host whether there is a mouse instead of resetting INT 33h for FFFFh. Motion was already SDL3's, scaled to the frame.
- **Joysticks are SDL3's** (`joystick.h`). Joysticks A and B are the first two connected: a gamepad's left stick and its south and east buttons, or another joystick's first two axes and buttons. Each axis comes centred and scaled to -127..127. The test clock's runs open none, so a controller on the test machine never reaches the traces.
- **The games' calibration rules read the axes.** Each axis becomes the reading the games' rules take where the original counted game-port reads: 127 at the centre, 0 and 254 at full deflection. Both calibrations keep their game role and rules on that reading: the campaign's centre and its fifths for near and far, and combat's centre, 7F00h reciprocal, dead zone under 8 and directions past -52 and from 53. A joystick is installed when it is connected; the original's presence tests (counting reads until the axes discharged, and checking that they still charged after 20 delay loops) are gone.
- **Combat reads a connected joystick.** Its direction read (1BF5:2376, with 21C7 and 22F9) was never converted, because the game port always read FFh: it is converted now, with its values in 41D9h, 41DDh and 41DFh. The button read (23D6) reads the host's buttons.
- **No original reference.** The original was never run with a joystick here, so a connected joystick's behaviour rests on the games' own calibration rules, not on a trace. `tests/joystick_test.cpp` drives both calibrations with fake axes (centred, full left, full right, small and diagonal deflections, buttons, calibrating off centre, none connected) and checks what each game reports.
- **The hardware timing is gone.** The campaign's read counting and its retry when a tick came between, and combat's PIT-timed delay-loop count (41DB), delay loops and port reads, all timed hardware only. The PIT latch window in the host, used by nothing else, is gone with them.
- **The test clock still charges their time until H4.** With no joystick connected, combat's calibration and axis reads took a fixed time of reads and delay loops. The test clock charges that time where they made it, with the same host waits in the same order, so the event logs stay identical; real time spends nothing. H4 removes the charge.
- **The sound-port goldens changed only by the latches.** The three combat logs lost the calibration's two channel-0 latch writes (43h: 00h, 06h) and nothing else: with those removed from the old goldens, every remaining line matches. They were re-recorded. The music logs are unchanged.
- **The check.** The traces match (8/8 on 1990). The event logs match (12/12). The joystick test passes.

**From H4a** (the test clock's charges removed; the one deliberate re-record):
- **What went.** Under the test clock, the host charged time the original spent on hardware: 1 µs per port read, combat's game-port waits (joystick calibration with no joystick: the calibration, four axis waits and the presence check), the two port reads of each combat key event (one before the child's handler) and combat's per-tick 61h read (kept by S5). Real play never charged any of them. They are gone, and so are `MwSdlChargeGamePortWait`, `MwSdlDelayLoops` and the calibrated loop count.
- **What stayed identical.** The traces (8/8 on 1990). Every note stream in the event logs, ticks included, so the music and its tempo are untouched. Every game stream (sound requests, keys, combat polls) up to the last snap. The device runs still agree with the speaker runs.
- **What moved, and why.** Each class of charge was dropped alone to attribute every move:
  - combat's tone ticks came 26 earlier from the cockpit on: the game-port waits took 109.1 ms before combat's tone tick started counting (1.0 ms calibration, four axis waits of 26.3-27.9 ms each, 0.2 ms presence), and a tone tick is 4.225 ms (combat's divisor 5041), so 109.1 / 4.225 = 25.8;
  - the effect tick's 1 µs per tone tick drifted them one more tick every 4,225 ticks: predicted -26.8 at tick 4044 and -27.4 at tick 6885, observed -27 and -28 (the largest, 118 ms);
  - that sub-tick drift moved two interleavings by under one tone tick: at the 'cycle' snap, one more tone tick of the running effect comes before the next combat tick's sound request; in 'fired2', a new effect's first step falls between tone ticks, so the tone log goes from its silence straight to its second step;
  - the port reads alone and the key reads alone moved nothing in the event logs. In the sound logs, the campaign's AdLib bus delays (23 or 4 status reads around each register write, inside the music's timer handler) charged 150,374 µs before the title key, which now lands 150.2 ms earlier in the title song (MT-32: 3,018 reads, 2.9 ms). The calibrated delay counts are unchanged.
- **Re-recorded.** The four tone-bearing event goldens (combat_cockpit, combat_retreat, audio-combat, feed-effects) for both revisions: their non-tone lines are identical to the old ones. The sound goldens: combat on all three devices (port lines identical in order, times moved), music.mt32 (lines identical with the snaps left out, times moved) and music.adlib (byte-identical up to the title key, which now comes 2,175 port lines earlier in the song, so the song is stopped at a different note and the register writes after it differ).

**From H4b** (PIT channel 0's programming as a host timer period):
- **What went.** The games' PIT channel 0 programming: the campaign's 0F41 timer (install, restore and the fast slot's rate) and combat's scheduler install and restore wrote 43h and 40h's two bytes; combat's music timer (1ADF) wrote 40h's two bytes alone (nothing in the port installs that timer, so its restores return early; the same call replaces those writes too). Each is now one call, `setTimerPeriod` (campaign) or `btechHostSetTimerPeriod` (combat), with the period in the PIT's clock units, made where the high byte took effect. The host's 43h/40h decoding is gone, and a write there now fails like any other unsupported port.
- **What it still is.** The period stays in the PIT's units (1/1193182 s), the games' own numbers (0x2000 for the campaign's 145.6 Hz, 5041 for combat's scheduler (65535 / 13), which also drives its tone tick, 65536 for the BIOS's 18.2 Hz), so every tick falls on the PIT cycle it did; the host's clock runs it. The 18.2 Hz count still reaches the game through the chain the games install (the campaign's slot 0, combat's every-period chain); H4c keeps that chain as an explicit host event at its own cadence.
- **The start-world check.** A new campaign writes ["start", ticks, word0, word1, planet] to `MECHWARRIOR_START_LOG`; `events.py` checks it against `events/<revision>/start.json`, recorded on H4a's timers before this change. Twelve scripts start a new campaign; audio-music plays the title through (655 ticks to the draw), the rest leave it before the first tick.
- **Identical.** Traces 8/8; events 12/12, tones included; start worlds 12/12. The sound goldens were re-recorded with only the 43h/40h writes gone: 24 lines in each combat log (8 programmings), 6 in each music log (2), every other line the same, times included.

**From H4c, the CLI pumps** (H4c's first commit; the one re-record of H4c):
- **What it was.** Combat's interrupt flag (`btechHostSetInterrupts`) ran the host's pump whenever the game turned interrupts off outside an interrupt: due timer ticks, key and MPU events were delivered there, and a pump that found nothing due spent the host's 100 µs idle step (under the real clock a real sleep). The campaign's flag never pumped.
- **What needed a delivery point.** Counted by call site over combat_cockpit: combat's two key reads (`BtechPopKey`, 300 CLIs, and `BtechReadHeldKeyState`, 266, each delivering on 5) and ten one-time set-up CLIs (joystick calibration, the scheduler, the timer callback, the divide-error handler, shutdown) that delivered nothing. The two key reads now call `btechHostRunDue` just before their CLI: the due work, taking no time. The flag no longer pumps.
- **The waits.** Combat's waits for a key (the briefing, the pause, the retreat question and the dialogs, through four loops in BTECH_0810_INPUT.cpp) end each pass with `btechHostWaitIdle`: the idle step the pass's key read found owing, so a wait sleeps as it did and doesn't spin. A combat pause held 10 s under the real clock: 27% of a core, as at H4b.
- **What moved, and why.** Put back, 100 µs at each of the ten set-up CLIs (1.0 ms) with the two delivery points reproduces H4b's logs exactly; the set-up CLIs only ever spent that time. Outside the waits, the per-tick key reads (the main loop's poll and the Alt check) no longer idle. Combat's tone ticks move -1 at most, alternating with 0 (the paced combat tick re-syncs every tick, so the moves don't add up), and where one effect's step follows another's by a tick the order can change by under a tone tick: on 1990 three interleavings (at 'cycle' and in retreat's 'group'), on 1989 one effect's first step in each combat file, which now falls in the tick of its next step (the log keeps one write per tick).
- **Re-recorded.** The four tone-bearing event goldens on both revisions (non-tone lines identical) and the three combat sound goldens (port lines identical in order, times moved). Traces, start worlds and the music goldens unchanged.

**From H4c, the interrupt model** (H4c's other three commits; every log identical, no re-record):
- **The interrupt flag is gone.** Combat's `btechHostSetInterrupts` and the campaign's `setInterruptsEnabled` were removed at every call. A probe of every trace and event script on both revisions found no pump with the flag off outside a handler, and none inside a handler with it off. It found 11,068 nested pumps, all from the campaign's MIDI tick inside 0F41's coarse callback after its STI. That STI is the one place a handler takes nested ticks, and it is now `allowNestedTicks()`. The host tracks whether a handler is running and whether it allowed nesting; a debug build stops if a pump runs inside a handler that didn't.
- **Vectors are typed owners** (`hardware_ports.h`): IRQ0 (the previous owner, combat's scheduler, combat's music timer), IRQ1 (the system's, combat's), INT 0 (DOS's, combat's divide handler) and IRQ2 (the system's, combat's probe and receive handlers, the campaign's receive handler). The host holds each owner; no vector table is kept. INT 1Ch was never installed (its only install call passed 0) and INT 24h was never dispatched; both are gone.
  - *The image's saved vectors* (1BF5:2BC0, 2BC4 and 253C, 1ADF:0E7D) are 0000:0000 in both revisions: nothing saved yet. Each starts as its interrupt's previous owner; any other bytes stop the game, naming the address and the bytes. The CRT's extra vector (304E:4B8A) is inactive in both, and an active one stops the same way.
  - *Two places used vectors as data, and still do.* The tone fallback with a missing ITM reads its tone bytes from 0000:0000-0017, vectors 0 to 5. They are a named table of the bytes the original found there, with INT 0 chosen by whether combat's divide handler is installed. The campaign's MPU install copies a byte at the offset DOS's GET VECTOR left in BX. After an MT-32 combat, which doesn't restore IRQ2, that is combat's receive handler (2176:0106); each owner gives its handler's original offset.
  - *A known deviation: the system's IRQ2 owner gives offset 0000.* Under real DOS it would be the offset of the BIOS's own IRQ2 handler, which varies by machine, so the original's copy lands somewhere machine-dependent. The port has always supplied 0000, so the copy moves byte 03h of 0F41's runtime header to byte 17h. Nothing reads that header after the image load (its only uses are the load and this copy), so nothing audible or logged depends on it.
- **No PIC.** Every end-of-interrupt write is gone. The host had ignored them, and each only advanced the audio to the time it was already at. The mask port is now a set of device IRQs, `MwDeviceIrq` (timer, keyboard, MPU). Combat's mask 28h and the campaign's mask 0 pass all three, and the MPU install, probe and restore pass or stop the MPU's. The mask's other IRQs had no device on the host, and the PIC ports were never in the sound log.
- **The chain is an explicit host event at its own cadence** (a project decision: neither the vector emulated nor the count resampled). In combat, IRQ0's previous owner is not the BIOS: it is the campaign's 0F41 timer handler, installed around the whole campaign. Every 13th scheduler tick (13 × 5041 = 65533 PIT cycles), the scheduler asks the host to run that owner (`btechHostRunPreviousTimerOwner`). 0F41's slot 0 runs the BIOS tick (the 18.2 Hz count and the campaign tick) on every 8th of its calls, so combat gets a BIOS tick every 104 scheduler ticks, as in the original. The count's readers are the campaign's random mix and 1610's tick-equality wait; combat never reads 40:6Ch. The host's BIOS word read, whose only special case was 40:6Ch/6Eh, had no caller and is gone.
- **The check, at each commit.** Traces 8/8 on 1990 (1989's known pair unchanged). Events 12/12 and start worlds 12/12 on both revisions. Sound logs 6/6. A debug build's traces, events and sound with the nesting check live.

**From H4d** (idle; every log identical, no re-record):
- **What changed.** When nothing was due, the host idled in 100 µs steps: at the end of a pump, at the end of a pass of combat's waits for a key, and in each step of a combat tick's pacing. Under the real clock it now sleeps until the next due event: IRQ0's next tick, the MPU's next event in an MT-32 combat, or the pacing's deadline. It never sleeps more than 10 ms, so the window, the input and the audio queue (at least 43 ms ahead) are still served. The game's counted waits (2176's and 1ADF's MPU polls) wait for an MPU byte, which is a due event, so they end as soon as before. Under the test clock the step stays 100 µs.
- **CPU, held 10 s** (macOS, Release, two runs each): a combat pause took 25.4% and 26.0% of a core before, and 3.3% and 2.1% after. The title's wait took 25.6% and 26.1% before, and 4.1% and 3.2% after.
- **The real clock, checked on both builds.** combat_cockpit and bar, recorded under the real clock, match their golden traces. combat_cockpit's late and underrun audio frames are about the same (about 4,000 on each build), and so is its worst timer lag (9.5 ms). bar takes about 0.6 s longer (3.3-3.5 s before, 3.9-4.1 s after). The feed sends its next key on the fourth empty key poll, and an empty poll now waits for the next tick (at most 10 ms) instead of 100 µs. A player's key press is likewise picked up at most 10 ms later.
- **The check.** Every log identical: traces, events and start worlds on both revisions, and the sound logs.

**From H5, part one** (ports 0 and 1; every log identical, no re-record):
- **What changed.** The campaign's AdLib rhythm release inherits its port from the high word of the last variable-length quantity read, normally 0, so the original sent that register pair to DMA channel 0's ports (0 and 1), not to the card. The host accepted those writes and the delay reads between them, and did nothing with them. Now 0F41's register-pair writer stops at port 0: nothing is written or read, and the release still reaches no chip. Any other port that isn't the AdLib's stays a failure, as it was at the host. The host's port bus no longer knows ports 0 and 1, and the sound log no longer lists them.
- **No log moves.** None of the six sound goldens holds a port 0 or 1 line, and no other log records ports.
- **What's left of the port bus** is the MPU's two ports and the AdLib's two. Their callers are the MT-32 and AdLib backends S8 removes, so the rest of H5 follows S4, S8 and S9.
- **The check.** Every log identical: traces, events and start worlds on both revisions, and the sound logs.

**From B1** (snaps by name; went ahead while S4 waited; the goldens converted, not re-recorded):
- **The schema.** `tools/trace/schema.py` names every compared fact of a trace record. The status: `campaign.*`, `menu.N.*`, and in a battle `combat.*`, `unit.S.*` and `objective.*`. The campaign exchange block, by the save format's own fields: `exchange.<name>`, as `MwEncodeCampaignExchange` names them. Words are numbers or lists, byte arrays hex, and the salvage and unit-damage arrays one hex field per record. `record.py` (from the port's status and `.exchange` file) and `state.py` (from the original's memory) use the one decoder, so the original's block maps onto the same names.
- **The schema can't drift from the save format.** The ctest `snap_schema` parses the encoder statement by statement and fails on any form it doesn't know. The schema's table must be exactly the parsed one, the fields must tile 0..0x718, the records must be the structs' fields, and decoding then encoding a block gives it back.
- **No text.** A field that can hold text is a keyed hash, as the menu labels are. A scan of all 116 exchange blocks in the goldens for printable runs of 4 or more flagged one field, the commander's name, which is hashed. No other byte field holds even two printable bytes in a row.
- **check.py compares by name.** A field that differs, or is missing or extra, fails by its name, with the first element or byte that differs and, in a record, the record's field. `known/` is keyed by field-name prefix.
- **The goldens were converted, with proof.** The status lines of all 158 snaps parse to fields, and the old line format rendered from those fields gives back all 1,826 lines byte for byte. Each of the 116 campaign snaps' `.exchange` files from a recording at ab9f3ff has the hash its golden held, and its bytes became the exchange fields, which encode back to the same bytes. The exchange hash is gone. The goldens grow from 240 KB to 1.25 MB.
- **The new recorder gives what the old one gave.** On the same binary, on both revisions, every snap's lines from the old recorder parse to exactly the new recorder's status fields, and every exchange hash is that of the block the new fields decode. 1989's two known failures fail on the same snaps, now with their fields named (in combat_retreat, `combat.tick` one tick late, and from the orders screen `unit.0.armor[2]`).
- **Not checked: the original through `state.py`.** No captures of the original were at hand, and the capture rig wasn't set up again for this slice. `state.py` builds its fields with the same two functions `record.py` uses.
- **The check.** The traces match by name: 8/8 on 1990, 6/8 on 1989 with the same two. `snap_schema` passes, and a changed field fails by its name.

**From B2** (campaign state as owner structs; a pure move, every log identical, no re-record):
- **What changed.** The campaign's globals are now the fields of 17 owner structs, 751 fields in all, each declared where its header's externs were. A use `name` is now `instance.name`, and every name is kept, so the save codec, `new_campaign.json`, the snap fields and the goldens don't change. The commits: menus and UI (`campaignUi`, `campaignInput`, `campaignOptions`), economy and market (`campaignEconomy`, `campaignMarket`, `campaignService`), pilots and mechs (`campaignPilots`, `campaignMechanics`), travel and contracts (`campaignTravel`, `campaignContracts`, `campaignEvents`, `campaignEventText`, `campaignMission`, `campaignHandoff`), the music driver (`campaignMusicTimer`, `campaignMusic`), the EGA overlay (`egaOverlay`), and the core, start-up and launcher (`campaignCore`, `campaignStartup`, `launcherResident`).
- **One struct per header, and per segment image.** A struct never spans two memory files. campaign_events.h's globals live in 1090 and in 1AA5's text bank, and campaign_music.h's in 0F41's timer and 28BE's driver, so each of those is two structs, each beside the importer that fills it.
- **Initial values.** 36 globals had them: 30 menus, the event class handlers, the after-action and settlement message tables, and the random generator's two words. They are now designated initialisers of their instance, with the same expressions. The start worlds show the starting state is identical.
- **Reads across the layout** (risk 6). Separately defined globals had no guaranteed adjacency, so none was read through a neighbour, and grouping can't add such a read. The one byte past the pilot identity union stays the explicit rule it was (`pilotDepartureFlagTail`). The EGA overlay's offset table points at the instance's fields.
- **Left outside the owners**, in the eight campaign memory files: ten constants (the picture file names, the overlay's start-zeroed ranges) and `campaignTransfer`, the exchange union, already one typed object.
- **Two invariants, at every commit.** `src/Rules/new_campaign_fields.cpp` regenerates byte-identical: the generator names a codec field without its owner instance. `tools/phase2/inventory.py` reads an owner instance as its members again, and its output is identical, so every item keeps its offset and its door.
- **The check.** Every commit: Release and Debug builds, ctest on both setups, the traces by name (8/8 on 1990, 6/8 on 1989 with the same two), events and start worlds, and the sound logs. Commits 4 and 7: the full gate (exe_free, packaging, the debug runs, the Windows package with MSVC).

**From the clock slice** (the traces on the test clock; no golden moved):
- **Why.** `record.py` pinned the startup clock but ran on the host's clock. The feed counts polls and combat reads no clock, but a campaign wait that polls for keys while it waits for IRQ0 sees real ticks. The hired transport's lift-off (`MwPlayHiredTransportAnimation`) polls once per frame, so in a slow build a frame can take longer than a tick, and the 4th empty poll, where combat_cockpit's deploy snap is taken, lands a frame further on. B2's full gate hit it in Debug. ASan builds of B2's tip and of the main before it each failed on some runs and passed on others, with the same frame, and no sanitizer report.
- **What changed.** `record.py` sets `MECHWARRIOR_DEBUG_VIRTUAL_CLOCK`, as `events.py` and `audio.py` already did: time moves only where the port would wait. `--real-clock` (on `record.py` and `check.py`) runs on the host's clock instead.
- **The real clock stays a check, not a gate.** combat_cockpit and bar on the host's clock, run by hand after a change to the host's timing or waits, as H4d did (`tools/trace/README.md`). A failure there is a question, not a regression by itself.
- **The check.** Under the test clock all 158 snaps match the goldens, frames and fields, with no re-record; Release and Debug traces on both revisions; events, start worlds and sound logs identical.

**From S6** (the campaign's speaker music as tones):
- **The speaker backend plays tones.** The campaign's speaker music (0F41's backend) hands the host each period as a tone, where it programmed PIT channel 2. It sounds from the backend's start to its shutdown, as the 61h gate did. The backend's silent period (14h, an inaudible 59.7 kHz on the speaker) is silence. Voice rotation, the period table and the single-voice path are 0F41's own and unchanged. The CLI/STI pairs stay until H4.
- **The host has no speaker ports any more.** The PIT channel 2 and 61h decoding is gone, and any write to them, or a read of 61h, is now a failure. One host entry, `MwSdlPlayTone`, plays both games' tones; combat's binding still charges its per-tick 61h read on the test clock until H4. The period-to-Hz rule both games use is `mechwarrior/tone.h`.
- **The campaign's three 61h reads are no longer charged** (one at each speaker backend start or shutdown in the scripts, 1 µs each on the test clock). Nothing in the game reads time across them, so the traces and event logs stay identical. The sound log's later lines move by up to 4 PIT cycles.
- **The sound log records the tone voice's input.** Each change of tone is a line, [PIT cycle, "tone", Hz to 0.001, 0 for silence], so the speaker goldens still hold what the speaker plays and when. They were checked against the old goldens before re-recording:
  - the music: the old goldens' channel 2 and 61h writes, decoded as the host decoded them (with changes at the same PIT cycle taken as where they end up, and the silent period as silence), give the same 484 tones, each within 4 PIT cycles;
  - combat: its 1,019 tones are the event log's tone changes, under the speaker, AdLib and MT-32, and every other line matches the old goldens in order (exactly, times too, under AdLib and MT-32).
- **The check.** The traces match (8/8 on 1990). The event logs match (12/12), the note log with them. The sound logs match (6/6) once re-recorded.

**From H3b** (controllers through the real SDL host):
- **No physical controller is available**, so the controller path is checked with SDL3's virtual devices instead. `tests/sdl_joystick_test.cpp` attaches a virtual gamepad and a virtual four-axis flight stick (X, Y, twist, throttle) with no window and no video driver. It feeds their plug-in and removal events to the host as its loop does (`MwSdlHandleJoystickEvent`, which the loop now calls), and reads them through `MwSdlReadJoystick` and the campaign's and combat's own joystick code. It checks: centre, full left, right, up and down, and both buttons; A and B together, only a stick's X and Y driving them; the window losing focus changing nothing; a device taken out mid-play; one plugged in again taking the free slot; and hot-plug order (the first in is A). It exits 77 (skipped) if a real controller is connected, since that would take a slot. The approach follows the MW3 port's virtual-stick tests.
- **A joystick taken out mid-battle reads centred in combat** (a project decision). The original never had to handle a stick removed mid-battle, so this rule is ours. Combat installs its joysticks at calibration, and an installed one that is no longer connected read the timing limit, which is full right and down: a held turn. Now combat's axis read gives 0 and 0 when the host says the joystick is gone, and its buttons read released, as they already did. The campaign already read a missing stick as centred and released. The traces never connect a joystick, so they are unchanged.
- **The check.** The traces match (8/8 on 1990). The event logs match (12/12). The `sdl_joystick` test passes, and fails on the held turn without the fix.

**From S5** (combat's effects as tones):
- **The tone voice** (`src/Audio/ToneVoice`, which was `SpeakerVoice`) plays a frequency or silence: a square of its odd harmonics up to the 31st, none at or above the output's Nyquist. A new tone starts at the wave's start, and a repeated or changed pitch keeps its place. It takes no ports. Before, the speaker model restarted its wave at every divisor write, so 145.6 times a second in combat; that restart is gone, so the PCM differs (the PCM was never a golden).
- **2134 plays tones.** Each tick, the channel the effect player chooses sounds at its period while its level is not zero, as a frequency (1,193,182 Hz over the period, the clock the effect bank and pitch tables count in), or the tick is silent. The host plays it on the tone voice at the hardware time now, where the PIT channel 2 divisor and the 61h gate were written. The priorities, steps, bends and level sequences are 2134's own and unchanged, so the effects the game requests (0-8, 10, 11, 13, 14) sound as their bytecode says. Effects 9 and 12 are never requested.
- **No `Effects` class.** The plan named one to take start, stop, frequency, schedule and priority. The schedule and priority are 2134's game logic, which picks one tone a tick. A class between it and the tone voice would only pass the frequency on, so the host binding goes to the tone voice directly. A sample pack keyed by effect number, if one ever comes, would add it.
- **The campaign's speaker music still writes ports until S6.** The host decodes its PIT channel 2 and 61h writes into tones on the same voice. Combat never writes them any more.
- **The test clock still charges the 61h read** each tick made (1 µs), until H4, so the timeline stays the same.
- **The tone log is identical** (the event logs match, 12/12). The sound-port goldens' combat logs lost 2134's port I/O and nothing else: per tick one 61h read and one 61h write (4,975 ticks), and the 42h divisor's two bytes on the ticks that chose a channel (2,146 writes). All of them fall between the deploy and results snaps, and every other line matches in order, including the campaign's speaker shutdown and restart around combat. They were re-recorded. The music logs are unchanged.
- **The check.** The traces match (8/8 on 1990). The event logs match (12/12), the tone log with them. `tests/tone_voice_test.cpp` checks the voice.

**From S4** (the listening; signed off as it stands, no change):
- **Signed off as it stands.** The project owner listened to the listening set (the five songs and the thirteen effects, on the soundfont and speaker-style) and signed off S3's instrument table and levels: no program, level, pan or drum-map change, and no second listen.
- **Against AdLib and speaker.** The comparison was with the original's AdLib and speaker versions.
- **MT-32: not comparable.** The project has no MT-32 ROMs, so the original's MT-32 version can't be played for a comparison. That is recorded as not compared, not as matched, and the project works with the two versions it can hear.
- **What it unblocks.** S8 (the AdLib and MT-32 backends removed), S9 (their libraries), then the rest of H5.

**From S8** (the AdLib and MT-32 backends removed; one named re-record):
- **What went.** The campaign's AdLib and MPU backends (0F41), combat's 21CA and 2176 with their state, and 1ADF's AdLib and MPU probes. `sound_system=adlib` or `mt32` still reads: it is `music=soundfont`, with one log line (S9 replaces the key). The host's own Adlib, MT32 and MPU401 files, Nuked-OPL3 and Munt stay until S9, with nothing writing to them.
- **'R' is the rebuild's music.** It has no card: the probe succeeds and nothing is set up, queued or drained. The MPU path's timer rate stays, through an empty callback, so IRQ0's programming under 'R' is unchanged.
- **Combat.** Whatever the launcher's letter, 1ADF installs the speaker's tones. The combat event logs and traces are exact on both revisions, the soundfont runs included.
- **The soundfont goldens** were recorded in S8's first commit, while the MT-32 path was there (they matched its log exactly). audio-music and feed-songs were re-recorded in the second, on both revisions, once the cause was proved: the old MPU queue waited while its 256 bytes were full, and a song's opening sysex burst fills it. Put back in throwaway code, that wait alone reproduces the goldens exactly, and the drain at song stops accounts for nothing. So only the ticks of notes and songs moved, all earlier (audio-music up to 6 ticks, 82 ms; feed-songs up to 14, 192 ms; a tick is 13.73 ms), with no drift within a song; the commit lists the songs. Every other line is identical, and audio-combat and feed-effects stay exact.
- **The sound-port log.** Combat's card probes wrote and read the AdLib's and MPU's ports even on the speaker. `combat.speaker` was re-recorded without them: 16,558 of its 17,603 lines were those ports, all at one PIT cycle after deploy, and with them taken out the old log equals the new one in all 1,045 lines, cycles included. `music.speaker` is unchanged. The four AdLib and MT-32 goldens left with the code (`tools/trace/audio/README.md`).
- **Not re-recorded.** The speaker event goldens and the start worlds.
- **Not removed yet.** `MwStoryCopyAdlibInstruments` and `story/music.json` have no caller left in the game; the importer still writes and reads the file, for S9 or the importer's own slice.

**From S9** (the libraries, the settings, the importer and the AppImage's pins):
- **What went.** Nuked-OPL3, Munt and the MPU401, Adlib and MT32 host files, the ROM settings and `mwSdlSound`; C left `project()`. `music=speaker|soundfont|off` replaces `sound_system`, which is still read, with a log line; the player's own ini overrides the packaged one. `off` takes the 'R' path with no music loaded, and the effects stay. The default stayed speaker in S9; it is the soundfont since (a project decision; the traces match on both).
- **The importer** no longer writes `story/music.json`. A folder made with the table this replaced is current, not imported again: `story_state` accepts the old table's hash for the new one only, and a test fails at the next change of the table. An older build does import again on a folder made by this one, and that is accepted; R's notes say so.
- **Packaging** needs SDL3 only and refuses a package holding a library named mt32emu or nukedopl, so an old build folder can't ship one.
- **The AppImage** is built with the tools and runtime the MW1 remake pins, each refused with any other hash. Its runtime's notice was read against the runtime's build recipe at its tag, and its licence texts against each upstream.

**From H5, the rest** (the port bus; no golden moves):
- **What went.** The host's `MwSdlWriteDevicePort` and `MwSdlReadDevicePort`, combat's `btechHostReadPort` and `btechHostWritePort`, and the `writePort`, `readPort` and `waitForInterrupt` members of `MwMusicHardwareCalls`, which had no caller left.
- **One procedure left deliberately:** 2134:0086, `BtechTickTandyToneChannels`, combat's Tandy sound chip tick, a hardware backend like the ones S8 removed, and the port bus's last writer. It couldn't be reached: its one caller is the selector 1ADF:0000, which needs the Tandy bit, and the probe (1ADF:00BE's platform half) returns the speaker alone unless the BIOS model byte is FFh, where the host returns an AT's. The selector's case is now a failsafe that says so. `TONE_TANDY_DATA_PORT` went with it; the Tandy pitch table and 2134's other Tandy branches stay, the table as checked start-image data.
- **`docs/phase2/INVENTORY.md` is unchanged:** it lists memory items, and none moved.
- **The sound-port log** is kept as the tone log, its goldens untouched.

**From B3b** (combat's sound state and 2FEB's tone channels as owner structs; a pure move, no golden re-recorded):
- **What changed.** 60 globals are now the fields of two structs in combat_sound.h, one per memory file: `combatSound` (36, BTECH_304E_SOUND) and `combatTones` (24, BTECH_2FEB).
- **Left outside.** `combatSoundIrq` (B3a) and the enums.
- **Reads across the layout** (risk 6). Searched for memory calls, casts, pointer arithmetic, and addresses taken of a moved global, and read each hit: none depends on the order of globals. The stream identity is a difference inside `runtimeStreams`, the memset is `musicSequences`' own, and `samplePriorityOwner` points at `combatTones.sampleSoundPriority`, as it pointed at the global.
- **The check.** `tools/phase5/owner_structs.py` compares 24 and 36 variables; INVENTORY.md is identical; the -Wshadow list is the same before and after; no unprefixed use is left outside comments and strings.

**From B4a** (the campaign's text as bounded references; no golden re-recorded):
- **What changed.** The (view, offset) pair every campaign text call took is an `MwText` (campaign_text.h): the first byte and the storage's end. The drawing reads to the NUL, refuses to read past the end as before, and returns a pointer to the NUL. The event pages take their next page from it, and the offsets the news, options, save and event text were given beside their view's base are gone with it. The launch's identity is the launcher's eleven bytes themselves.
- **The month names.** Their view was 0x10000 bytes wide. The reference ends with `calendarMonthText` (0x6E bytes). The import refuses a name with no NUL inside those bytes, so nothing a script can reach reads further. The importer's pointer is formed after the subtraction, one of the places the bounds sanitizer reported.
- **A notification's text offset** is checked against its storage before the pointer is formed; the view refused the same offset at the first read.
- **The inventory tool** reads an `MwText`'s cursor as it read a plain pointer; its output is unchanged.

**From B4b** (the campaign's picture and music buffers; no golden re-recorded):
- **What changed.** The eight 64 KB buffers were reached by a selector through a 65,536-entry table. They are `MwCampaignBuffer`, a whole window each, owned by the host; the pointers that name them replace the selectors, and the bind and resolve calls and `NativeAddress.cpp` are gone. A `uint16_t` index is always inside a window and wraps as before, in the packed sprite's and the MIDI readers too. The failure indices of the startup's allocations are unchanged.
- **After a program ends** its buffers are released and every pointer to them cleared, so a use after that (a music tick among them) still fails.
- **The buffers start zeroed,** in a commit of its own. A shadow of every byte (8 traces, 12 event scripts, 1990) found one read before a write: at a picture's end `MwReadPictureNibble` decodes the low nibble of the byte after its data. With the buffers filled with 00, FF and A5, the traces, the events and the frames (432 files) are the same. The byte decoded is not changed.
- **Bounds.** A loaded count is a word, so that byte is always in the buffer; the largest compressed file is 7F51h bytes and leaves 80AFh. The nibble it may write lands after the picture, and the largest decoded bank, FB70h bytes, leaves 490h. Both revisions' files have the same sizes.

**From B3c** (combat's video, render, polygons, scene, pose and cockpit state as owner structs; a pure move, no golden re-recorded):
- **What changed.** 287 globals are now the fields of six structs, one per memory file: `combatVideo` (101 fields, combat_video.h), `combatRender` (63, combat_render.h), `combatPolygons` (26, combat_polygons.h), `combatScene` (8, combat_scene.h), `combatPose` (18, combat_pose.h), `combatCockpit` (71, combat_cockpit.h). Every name is kept, and the macros that name fields point at the instances. No field has an initialiser.
- **Left outside.** The constants `glyphTailMasks`, `errorCardNotFound` and `errorInspectionShapeMemory`, and the cockpit importer's six private text buffers.
- **2FEB stays with B3b**: its 24 globals are the tone channels' state, declared in combat_sound.h. The rows below are corrected.
- **Fields whose names read like another owner's, for B4 or later.** In `combatVideo`: the picture lists (`promptBitmaps`, `pointPictures`, `smallMechPictures`, `strutPictures`, `cockpitPictures`, `hudGlyphPictures`, `inspectionDamagePictures`, `bitmapRegistryLists` and its counts), the landscape colours (`landscapeCameraColourRows`, `landscapePaletteSlots`, `playerCameraFillASuppressed`, `damagePaletteSlot`) and the inspection flags (`inspectionReloadPending`, `inspectionSetupComplete`). In `combatCockpit`: the radar marker caches (`radarMarkerX`, `radarMarkerY`, `radarMarkerCounts`, `radarMarkerBackground`, `radarCachePage`).
- **Reads across the layout** (risk 6). Searched for memory calls, casts, pointer arithmetic, and addresses taken of a moved global, and read each hit: none depends on the order of globals. The row table and the Hercules row table, adjacent in the original, are read by name with an explicit test (1BF5's ellipse and shapes). `polygonScanlineSpans` and `polygonScanlineHeader` stay inside `polygonSpanStorage`. The decimal field's overlap with the target cache is the union it was, and the pilot and mech name tables' offsets stay inside their own arrays. This is a search over the text: it can't see an index run past an array at run time, so combat_cockpit was also run on 1990 with clang's address and bounds sanitizers (the bounds one sees an index past one array of a struct into the next, which the address one can't; neither sees past an array reached through a pointer). The address sanitizer reported nothing. The bounds sanitizer reported 18 places, all `array+offset-base` in the importers (BTECH_304E_COCKPIT's six private buffers' pointers, BTECH_304E_ORDERS, MW_MAIN_1090): the sum leaves the array before the subtraction brings it back. Those lines are main's, on buffers outside the six structs, and are left for B4, listed under "B4 takes these" below.
- **B4 takes these (undefined behaviour by the letter, and harmless as it stands).** Importer lines that form a pointer past its array before subtracting the base, `array+offset-base`, so the bounds sanitizer reports them. BTECH_304E_COCKPIT.cpp, `BtechImportCockpitInitialData`: 57 (inspection bitmap names), 107 (weapon labels), 113 and 117 (range class labels, the energy placeholder), 123 (radar range labels), 214 (cockpit screen names). BTECH_304E_ORDERS.cpp, `BtechImportOrderInitialData`: 72 and 75 to 82 (the order panel's texts). MW_MAIN_1090.cpp: 640 in `MwImportCampaignInitialData`, 954 and 955 in `ImportContractData`. Line numbers are at 2ec4e00.
- **The check.** `tools/phase5/owner_structs.py BASE` (python3 -I, clang++ on the path) reads each changed memory file at BASE and now through clang's syntax tree, an owner struct as its fields: the same names, types with their array bounds, linkage and initial values, and no dynamic initialiser. It was tried on deliberate mistakes (a bound, a type, an extra field, an initial value, a dynamic initialiser), and on B3a's own commits, and a dropped initialiser in B3a's runtime state. B3d and B3e use it, with `python3 -I tools/phase2/inventory.py src/Original/Memory` identical to INVENTORY.md at each commit.

**From B3d** (combat's world, units, orders and import state as owner structs; a pure move, no golden re-recorded):
- **What changed.** 216 globals are now the fields of thirteen structs: `combatWorld` (36), `combatResources` (25), `combatBallistics` (16), `combatPlatform` (11), `combatNavigation` (9), `combatTactics` (6), `combatProjectiles` (6), `combatEffects` (5) and `combatMovement` (2) from BTECH_304E_WORLD; `combatImport` (40), `combatMechs` (14) and `combatDamage` (4) from BTECH_304E; `combatOrders` (42) from BTECH_304E_ORDERS. Each is declared in its header, names kept. The macros that name fields of the unions point at the instances. No field has an initialiser; `effectResourceSequences`, a constant array of pointers to two `combatEffects` fields, stays outside and is still constant-initialised.
- **Which slice has each combat memory file** (BTECH_304E_*, and the others):
  - B3a: BTECH_304E_RUNTIME, BTECH_304E_STARTUP, BTECH_1ADF_STATE, BTECH_1BF5_STATE.
  - B3b: BTECH_304E_SOUND (36 globals) and BTECH_2FEB (24).
  - B3c: BTECH_304E_VIDEO, _RENDER, _POLYGONS, _SCENE, _POSE, _COCKPIT.
  - B3d: BTECH_304E, BTECH_304E_WORLD and BTECH_304E_ORDERS.
  - B3e: BTECH_22B6_DATA (15 globals, combat_world.h 4 and combat_render.h 11). BtechStartImage's 53 constant tables are in no slice: B4 owns how the image is read.
  - No slice: NativeStart (six constants), as the other slices left constants outside.
- **Left outside.** `bodyDestructionEmitters` (one field of combat_damage.h in BTECH_304E_WORLD, one variable and not worth a struct); the six file-static beam tables of BTECH_304E_WORLD; BTECH_304E_ORDERS's `orderPanelText`.
- **Fields whose names read like another owner's, for B4 or later.** In `combatWorld`: the player's bindings (`playerUnitBinding`, `playerMechState`, `playerMechTemplate`, `playerPoseObject`, `playerPoseCameraContext`) and the order map's (`orderMapCameraObject`, `orderMapCameraContext`). In `combatImport`: the roster's and the damage rules' tables, `hudColourSchemeIndex` and `combatSoundEnabled`.
- **Reads across the layout** (risk 6). The same search as B3c's, each hit read: none depends on the order of globals. Two cases name a smaller extent than the array on purpose and are unchanged (`sceneChassisResourcePresent`, ten long, cleared over eight by 0810's world code); the formation positions are indexed inside their eight. No local or parameter shadows a moved name: clang's -Wshadow warnings over all 209 .cpp files are the same as main's, and no unprefixed use of a moved name is left outside comments and strings (src/Rules' `projectiles` is its own).

**From B3e** (22B6's data as owner structs; a pure move, no golden re-recorded):
- **What changed.** 15 globals are now the fields of two structs, one per header, both defined in BTECH_22B6_DATA: `combatGeometryTables` (4, combat_world.h: the orientation cosines, the coefficient angles, the heading ratio angles and the near-plane scratch `vertexNearPlaneState`) and `combatGeometry` (11, combat_render.h: the geometry coordinate caches, the shade deltas, the polygon vertex indices and the wide polygon's vertices), beside `combatRender`, which is 304E's. No field has an initialiser.
- **The start image.** Its 53 tables are static constants, private to BtechStartImage.cpp and each used once, as its row's bytes; the other 45 rows have no table (24 rules, 14 zero, 3 generated, 2 story and 2 UI pieces). A struct would only put a name between a table and its one use, so they are left. B4 owns how the image is read.
- **A field whose name reads like another owner's, for B4 or later.** `vertexNearPlaneState`, in `combatGeometryTables`, is scratch, not a table.
- **Reads across the layout** (risk 6). The same search as B3c's: each copy is of one field, sized by that field, and the arrays of coordinate caches name the fields by address.

**From B4** (native addressing; seven sub-slices, each landing on its own, no golden re-recorded):

| Sub-slice | Content | Check |
|---|---|---|
| B4a | Campaign text: the (view, offset) pairs become a bounded text reference. `MW_MAIN_081B_TEXT`'s month name is bounded by its string, where its view was 10000h bytes | campaign traces, event scripts and frames, both revisions |
| B4b | Campaign picture banks and music buffer: owned buffers for the 8 segment allocations; the bind and resolve calls go. Uninitialised bytes are not zeroed until shown unread | as B4a, and the music and travel scripts' note logs |
| B4c | Combat's pages: a page type | combat traces, frames and event logs, both revisions |
| B4d | Combat's DOS identities outside sound: the free call, the archive's workspace and source selectors, the ROM-byte hook's selector | combat traces, runtime_test |
| B4e | File records: scenario and formations, the campaign transfer, the resource banks, the archive tags; a reader that refuses past the end | combat traces and frames; campaign_contract |
| B4f | Importers and the start image: rows read by id, the built image gone. Combat and campaign each on their own | start_exe, btech_revisions, the rules, ui and words tests, the owner check, combat traces on both revisions, a bounds-sanitizer run with no report at the 18 places |
| B4g | Sound: the tone and MIDI views, the sound allocations' selector, the tone fallback's bytes (INT 0's owner stays, as a named source); native_address.h goes with the last slice to use it | after B3b |

Names such as planetSelector, houseSelector, scenarioSelectors and the trace schema's combatPlanetSelector are game indices, not addresses: they stay.

**From B4c** (`CombatPage`, combat_page.h):
- **What changed.** A combat page was a segment number (A000h, A200h, A400h, A600h); it is now `CombatPage`: None, First (A000h), Second (A200h) and BitmapScratch (A600h). `combatVideo.backPage`, `frontPage`, `drawPage`, `copySourcePage` and `ellipsePage` (they were `...PageSelector`), and `combatCockpit.savedCopySourcePage` and `savedDrawPage`, hold one, and so do the line, glyph and span hooks' arguments. `BtechEgaCanvasPage` takes the page: its segment division is gone.
- **None** is the state before the video starts (the original's zero). A page is still resolved at the first byte drawn, not when a routine is called, so a routine that returns early checks nothing.
- **The video scratch page is gone.** `videoScratchPageSelector` (304E:2E60) was written and never read; its start-image row keeps its bytes.
- **The captured word at cursor FFFF.** The original's second byte lay 1000h paragraphs on from the page's segment. That is past the canvas's eight pages whichever page it is, so it was refused as a selector off the canvas. It is refused by name now (2A36's capture and restore).
- **Kept as it was.** The debug capture's JSON prints the segment numbers it printed before (A000h and up, 0 for none). MW_EGA's own page numbers (egaDisplayPage and egaDrawPage) are campaign state and not in this slice.
- **What stores a page as a number.** Only the places above. Neither the trace schema, nor the state tool, nor the event log holds one.

**From B4d** (combat's DOS identities outside sound):
- **The archive's selectors.** `workspaceSelector` was written once and never read, `sourceSelector` neither, so both fields are gone. The descriptor isn't copied or compared as bytes, and the near heap accounts it by its native 21h bytes.
- **The free call** took the allocation, not a selector: `BtechFreeDosMemorySelector` (1BF5:1F10) is now `BtechFreeDosMemoryBlock`. The calls that really free by selector, and the hook that looks a selector up, are the sound allocations' (B4g).
- **The ROM bytes.** `btechHostReadRomByte` took a selector and an offset. The four addresses its callers gave are two bytes, the model and the Tandy signature (`CombatRomByte`), and the host answers for those.
- **Also.** 14 unused EGA register constants and the end of the graphics dispatch table are gone from combat_video.h. ega_canvas_writer_test covers B4c's two refusals (a drawing on no page, a captured or restored word at cursor FFFF).
- **Kept as it was.** DOS's memory list itself (first fit, one paragraph a block): how much memory is free decides what the game loads, so its sizes stay.

**From B4e** (combat's file records, read to their end and never past it):
- **What changed.** `CombatRecord` and `BtechReadFileRecord` (combat_record.h) read a record whole or stop, with the file's name and what to do (run the setup again with a good copy). The shipped files of both releases are never short, except one damaged bank (below), so a good install sees no difference.
- **What a file that ended early did before.** Combat's world state is cleared at each EXEC, so "what was there" is zero.

| Read | Before | Now |
|---|---|---|
| SNARIO.DAT, a scenario's selectors (16 bytes) | zeros: up to four copies of tile 0 as the map, deployment from formation 0, no edge to withdraw by | refused |
| SNARIO.DAT, a side's formation (32) | the missing units at the map's origin | refused |
| SNARIO.DAT, a special target's position (8) | refused (the original used uninitialised stack) | refused, named |
| TILE*.WLD, an object (14) | the load's first: refused (stack again). A later one: an extra object, its missing bytes the previous record's, which can be another tile's. The end between two records is the list's end | a cut record refused; the list's end the same |
| TILE*.GRD, a row (47 bytes, 87 rows) | height 0 from there on: flat ground, on the radar too | refused |
| Shape and surface banks | every access checked and refused | the same checks, named |
| A packed file | refused when its length doesn't match its header; a decoder can pass a cut off in its last run (see below) | the same, named, and one shorter than its 5-byte header |
| The campaign transfer | a fixed 718h bytes in memory, every read inside | by its fields; nothing to refuse |
| The archive tags | a short tag or length ends the search: not found, a rule of the game's | the same |

- **A packed file cut.** Method 1's literal run counts the bytes it asked for, not those it got, and its repeat takes the end of the file as its value, so a file cut in its last run could pass the length check. The shipped banks are all method 2 (LZW). combat_records_test cuts them at their header, their last 64 bytes and every 257th byte: 1,873 cuts of 11 banks (1990) and 1,705 of 10 (1989), all refused.
- **The damaged bank.** Our 1989 copy's PHAPCK.TBL holds MARPCK.TBL's bytes from 1800h to 5FFFh and one changed bit at 166Dh, and doesn't unpack to its length: the port already refused a Phoenix Hawk with it, without a name (SUPPORTED_VERSIONS.md). No trace loads one: the combat traces have a Locust and a Jenner. The test knows the file by its hash.
- **1A2E.** Its formation and target reads are functions of their own (for the test); its formation buffer is no longer prefilled with the current positions.

**From B4g** (combat's sound data; no golden re-recorded):
- **The views.** `BtechSoundBytes` is a bank's borrowed bytes and their count, for the events, the tone channels and the MIDI stream. A read past the end is refused, and an offset is still a word that wraps. The view had a base too, always 0 here.
- **The tone fallback.** A bank with no ITM reads its modulation from 0000: of the DOS machine. `BtechToneFallback` says which of the two a channel reads, and DOS memory has one source, `btechHostReadToneDosByte`: INT 0's owner live at each read, as before. A selector other than 0 is refused when the channel starts.
- **The allocations** are freed by their bytes, not a selector, so the two selector hooks and the host's control block segments are gone. DOS's memory list and its sizes stay. Two differences, neither reachable, as nothing calls the unloads: a second release of a chain no longer frees a node's old selector again, and freeing a node whose append failed is refused, where DOS ignored it.
- **`midiStreamPresent`** (was `midiStreamSelectorPresent`) is whether the track has bytes, which is when its selector was not 0.
- **native_address.h** keeps `MwReadNativeWord` and two constants. `MwCampaignBuffer` is in campaign_buffer.h. The files that read words include native_address.h themselves.
- **Also.** `ReleaseChildResources` clears its pointers before it frees the buffers again (B4b had freed first). The music tick runs on the main thread, so neither order could show. A start with an unknown option writes no launcher record.

## 3. How we check "play identical"

### Sound

1. **The note log, exact.** A new log at the sequencer's output records each event as (game ms, channel, event, key, velocity).
   - Goldens are recorded from today's port, through its MPU path, before anything changes. They cover all five songs: the title (MECH), the bar (MECHBAR), the LAUNCH and DEATH cues, and VICTORY.
   - Level 3 of the audio plan showed the port's MIDI byte stream already matches the original's under DOSBox-X: 3,003 title bytes and 4,730 bar bytes, exactly. So these goldens are anchored to the original.
   - Every sound slice keeps the note log identical. The instrument table changes the sound, never the notes or their times.
2. **The tone log, exact.** Combat's effects as (game ms, effect, frequency steps). The goldens are converted from today's port-event log, whose PIT divisors give the frequencies.
3. **The traces, unchanged.** Sound doesn't touch game state, so check.py stays at 8/8 for 1990 and 6/8 for 1989 (combat_cockpit and combat_retreat are the known two), and exe_free stays 8/8.
   - **Why those two fail on 1989.** The goldens were recorded with the 1990-01-22 BTECH.EXE, first on 2026-11-20 (5bc567b, and combat_retreat as its own in 1d5d0e1); there has never been a 1989 one. The two BTECH revisions wait for keys differently, by design: after a press, the 1990 one waits for the release too (7d4bb10, `btechLoadedRevision` in `BTECH_0810_INPUT.cpp`). The 1990 game starts combat one poll later for it, so the 1989 game's tick is one ahead of the goldens at all 20 combat snaps of both traces.
   - **Measured 2026-11-24 at 8ccc109.** Besides the tick: in combat_cockpit unit 4's position, orientation and step; in both, unit 0's armor from orders_command and the screen at the two order snaps. In a throwaway build with the 1990 waits forced on the 1989 game, both traces match, so the waits are the whole cause.
   - **For R.** 6 of 8 is the expected result on 1989, with these two names. Any other name, or a tick not exactly one ahead, is new. 1989 goldens of those two would make it 8 of 8.
4. **Listening checks.**
   - A fixed list is rendered headless to files, never played aloud by automation: each song (MECH, MECHBAR, LAUNCH, DEATH, VICTORY) and the thirteen effects the game requests, on the soundfont and on speaker-style.
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
- **The sound-port log** (OPL, MPU, PIT channel 2 and 61h writes; since H5 the tone log, its tones and snaps kept) is replaced by the **note log and tone log** (section 3). These are game events: what the score and the effect player asked to sound, and when.
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
| TinySoundFont (a3621a1) | MIT | music synthesis (new), `extern/TinySoundFont` | yes |
| FluidR3 GM 3.1, subset | MIT | music instruments (new): cut at build time from the pinned SF2, fetched once (`tools/deps/fetch_soundfont.sh`) | yes |
| *FluidR3 Mono GM* | *MIT* | *not used: SF3 only; its SF2 can't be pinned* | yes |
| *miniaudio* | *public domain or MIT-0* | *not used: SDL3 covers output* | yes |
| *GeneralUser GS* | *its own licence; sample provenance uncertain* | *not used* | *not chosen* |

These are build and test tools only; none is linked in, and only the installer builder's runtime ends up in a package:
- **DOSBox-X (GPL-2.0):** the original's reference captures (`tools/capture`); never shipped.
- **Python:** the tests and packaging scripts.
- **Inno Setup:** builds Setup.exe. Its licence allows distributing the installers it makes.
- **The AppImage's tools:** linuxdeploy and appimagetool, pinned (`tools/deps/get_appimage_tools.sh`), not shipped. The runtime ships in the AppImage: MIT, with libfuse (LGPL 2.1) and other libraries linked in statically; its notice and licence texts go with it (`tools/package/appimage-runtime`).

## 5. The path to mw1r-v1.0.0

Every slice below is its own branch with the full gate green: both setups, the traces, exe_free and packaging. Once a check is recorded, it stays exact on every later slice unless that slice says otherwise. Those checks are the note log, the tone log, the game-event log and the frame hashes. A graphics or input slice must leave the sound logs exact, and a sound slice the frames.

### Group A, the hardware layer: before the tag

**The order, and why:**
1. **Measure first (T1).** Every golden is recorded from today's port before any code it observes changes: the game-event log, the note log, the tone log.
2. **Music with provisional instruments next (S2-S3),** so the listening material goes to the project early. Listening is the one step a person has to do.
3. **Graphics and input while the listening happens (G1-G5, H2-H3).** These are checked mechanically (frames pixel-exact, game events exact), so they need no one's time to sign off.
4. **Sound finished, timers and interrupts, ports retired (S4-S9, H4-H5).** The ports go last because every device has to move off them first.
5. **Group B after H5 (below), then the release gate (R),** then the tag. R stays in this table but runs last, so it gates both groups.

| # | Slice | Check | Days |
| --- | --- | --- | --- |
| T1 | The game-event log beside each snap; the note log at the sequencer's output and the tone log at 2134's output; check.py compares all three. Goldens from today's port: the six campaign scripts, the combat scripts, every song (MECH, MECHBAR, LAUNCH, DEATH, VICTORY), the thirteen effects the game requests (0–8, 10, 11, 13, 14; 9 and 12 are in the bank but never requested). The songs' channel layout and rhythm channel confirmed | goldens reproduce run to run | 4 |
| S2 | TinySoundFont vendored with its licence. The soundfont subset tool (our own) with tests: a subset holds exactly the listed programs and plays them the same as the full bank. The pinned source is fetched once per machine; the build cuts the subset and refuses one that doesn't hash as pinned | tool tests; the subset's hash and render check; package licences | 2 |
| S3 | `Music` behind `music=soundfont`, with a first instrument table. The listening list rendered to files in `outbox/<date>/mech1-re-listening/` with a one-page sheet (below) | note log identical | 3 |
| G1 | `Canvas` and the RGB palette; the presenter takes a canvas page. The game still draws through the EGA model into it | frames identical | 2 |
| G2 | Combat text and single pixels (2A36's glyph and pixel paths) through the canvas | frames identical | 2 |
| G3 | Combat spans and lines (2A36's spans, lines, logical functions) through the canvas | frames identical | 3 |
| G4 | Combat bitmaps, blits and polygons through the canvas; then the EGA register, latch and plane model removed; flips become `present` | frames identical | 3-4 |
| G5 | The campaign overlay addressed by pixel; palette registers become palette entries; INT 10h mode calls become canvas set-up. The 3DAh status timing removed | frames identical; sound logs exact | 4 |
| H2 | Keyboard: SDL3 key events into a typed input queue. The BIOS keyboard words, shift flags and the INT 9 scancode ring removed, along with the IRQ1 handler | game-event log identical; traces 8/8 and 6/8 | 4 |
| H3 | Mouse and joystick: SDL3 mouse scaled to the frame; SDL3 axes for the joystick; the calibration screen reads axes. Game-port read counting and the delay loops that only timed hardware removed | game-event log identical | 3 |
| H3b | Controllers through the real SDL host: SDL3 virtual gamepads and joysticks plugged in, moved and taken out, read by both games. A joystick taken out mid-battle reads centred | `sdl_joystick` test; traces and event logs identical | 1 |
| S4 | The instrument table from the listening notes, per song: programs, levels, pans, the drum map. Re-rendered for a second listen if asked | note log identical; the project owner's sign-off. Signed off as it stands, against the AdLib and speaker versions; MT-32 not comparable (no ROMs) | 2, plus listening |
| S5 | `Effects`: 2134 outputs tone events to the tone voice, for the thirteen effects the game requests (9 and 12 never are) | tone log identical | 3 |
| S6 | Speaker-style music on the tone voice. The PIT channel 2 and 61h decoding removed | note log identical | 2 |
| H4a | The test clock's charges removed: the per-port-read 1 µs, combat's game-port waits, the key-event reads, the effect tick's read (the PIT latch window went in H3). The one deliberate re-record | traces identical; note and game streams identical; the tone goldens and the sound goldens re-recorded, every move attributed to a removed charge | 1 |
| H4b | Timers: PIT channel 0's programming becomes a host timer period, which the host's clock runs at the games' own rates; the start-world check (the 18.2 Hz count, the random words and the world drawn at a new campaign), recorded on H4a's timers first | traces, events (tones included) and start worlds identical to H4a's; the sound logs H4a's with the 43h/40h writes taken out, times included | 1 |
| H4c | Interrupts: first, combat's CLI pumps become explicit delivery points at its two key reads, and its waits for a key idle by name (the one re-record, every move attributed); then the interrupt flag removed, vectors as typed owners, the PIC's end-of-interrupt and mask gone (a set of device IRQs), and the BIOS chain an explicit host event at its own cadence | the re-record: traces, game and note streams and start worlds identical, tone moves attributed; then every log identical to it | 2 |
| H4d | Idle: under the real clock, a host wait sleeps until the next due tick instead of 100 µs steps; the test clock keeps its steps, so no log moves | every log identical; CPU of a held combat pause and of the title's wait, measured before and after (a combat pause: 27% of a core at H4b and H4c's first commit) | 1 |
| S8 | The campaign's AdLib and MPU backends and combat's 21CA and 2176 removed. 1ADF reports the rebuild's sound (see "From S8") | event logs and traces exact but for the one named re-record; done | 2 |
| S9 | Nuked-OPL3, Munt and MPU401/Adlib/MT32 removed from the build. ROM settings dropped; `sound_system` replaced by `music`. Notices, AUDIO.md and packaging (no nukedopl or mt32emu library) updated. The AppImage tooling's licence confirmed | packaging tests; the game's own libraries hold no LGPL; the AppImage's runtime does (libfuse, LGPL 2.1), with its notice (see "From S9") | 2 |
| H5 | The port bus retired: `MwSdlWriteDevicePort`, `ReadDevicePort` and the port-word hooks removed with their last callers; the sound-port event log keeps its tones and snaps as the tone log (its name is historical), and its port lines are gone. Its first part (ports 0 and 1) went ahead, after H4d; the rest follows S4, S8 and S9, since its last callers are the MT-32 and AdLib backends S8 removes | everything above exact | 2 |
| R | The release gate: every gate on Windows, macOS and Linux from fresh builds. Packages from a local tag. The listening list and the visual acceptance list signed off on the packaged builds. The controller path is covered by the SDL virtual-device test (H3b); no physical controller is available | all of the above | 2 |

**For R** (found by earlier slices, to be done or decided there):
- A refused start in a Windows GUI build: does the player see the reason anywhere but a log? (The missing soundfont's message was only ever printed to stderr.)
- A port failsafe's message: started with no arguments, the program writes its log, with the message, to mechwarrior.log beside the saves. Started with a game folder, a Windows GUI build has no stderr, so it is lost. There is no message box (B4e).
- The setup on a finished install (`mw1r --setup`): FIND THE GAME offers the copy in use first, even a damaged one, and INSTALL on it copies nothing. It should verify the game's files (today only BTECH.EXE and MW_MAIN.EXE are looked at) and offer "choose another" plainly.
- ega_canvas_writer_test and combat_records_test define their own MwAbortWithReason and rely on the linker not taking the library's object. MSVC may report a duplicate symbol: link and run both on Windows before trusting them there.
- A package built on each platform with hidapi's notice (`licenses/hidapi-LICENSE-bsd.txt`), and what the Mac and Windows packages link and ship: the MSVC runtime DLLs and Inno Setup's parts among them. Only the Linux package has been measured (S9).
- The Linux tar.gz holds libSDL3 three times (.so, .so.0, .so.0.2.0); the AppImage holds .so.0 alone.
- "The C17 port" in `docs/THIRD_PARTY_NOTICES.md`, and the README's stale passages.
- Packaging with a release's label should refuse a tree that isn't at that tag, or has uncommitted changes.
- On Windows: the program's path and the player's folder in the launcher's record, with a path that isn't plain ASCII (the UTF-8 manifest, and the kit's `self_path`).
- The kit's recognise rules on a real copy of MechWarrior 2, MechWarrior 3 and Pirate's Moon, `DATA1.HDR` included.
- The project owner's word on the AppImage (its runtime holds libfuse, with its notice).
- In the release notes: an older build imports the story again on a folder made by this one; an old test package's own `MechWarrior.ini` beside the program keeps its `sound_system=speaker`.

**Group A total:** about 51-52 developer days plus the listening sessions. That is roughly 10 weeks, planned for **one developer, in order**.

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

### Group B, memory images to typed state: before the tag, after H5

**On the path to the tag** (a project decision). Group B runs after group A's H5 and before R, so R gates both groups. B1 went ahead, after H5's first part, while S4 waited for the listening: it touches no sound path, so it doesn't meet S8. The rest of group B keeps its place after H5. It starts the work the fork plan's phases 3-6 would have done (giving the game's state its owners), so it is sized here, by the code each slice touches on main at 786fc8e.

**The order, and why:**
1. **Snaps by name first (B1),** so every later B slice is compared by field name, and a mismatch names its field.
2. **Campaign state, then combat state (B2, B3).** The campaign's state is most of what the traces compare. Combat's goes one subsystem per slice (B3a-B3e).
3. **Native addressing (B4)** goes once every image it points into is typed.
4. **The C runtime and DOS files (B5, B6)** go last: their callers are B2-B4's structs and references.

Every B slice keeps the group A logs exact (the note, tone and game-event logs, the frames and the traces), as well as the check in its row.

| # | Slice | Check | Days |
| --- | --- | --- | --- |
| B1 | Snaps by name: a snapshot schema of named fields. `state.py` maps the original's offsets to those names; `check.py` compares by name. The schema covers what `record.py`'s state lines hold today, and the campaign exchange block (0x718 bytes, compared as one hash today) by the save format's fields | traces identical, now by name | 3 |
| B2 | Campaign state (MW_MAIN's 1090, 0F41, 28BE, 1AA5 images; MW_0806's and MW_EGA's) as typed structs with owners. About 530 lines of named globals today, 438 of them 1090's. One commit per owner, each gated: menus and UI, economy and market, pilots and mechs, travel and contracts, the music driver, the EGA overlay | traces by name | 6 |
| B3a | Combat state, runtime and installation: 0810's, 1ADF's and 1BF5's state, and 304E's runtime and start-up, as typed structs | traces and frames | 2 |
| B3b | Combat state, sound: 304E's sound state, what S8 leaves of it, and 2FEB's (the tone channels' state) | traces and frames | 1 |
| B3c | Combat state, video, render and cockpit: 304E's video, render, polygons, scene, pose and cockpit state | traces and frames | 4 |
| B3d | Combat state, world, units and orders: 304E's world and orders state, and the rest of 304E (BTECH_304E, BTECH_304E_WORLD, BTECH_304E_ORDERS) | traces and frames | 4 |
| B3e | Combat state, geometry: 22B6's data. The start image's tables are constants with one use each, so they stay as they are; B4 owns how the image is read | traces and frames | 2 |
| B4 | Native addressing removed: references and spans in place of selector bindings and byte windows. At 343eb4c, 50 files use the byte views, the word reader or a native segment, and 87 hold a word with "selector" in it (many are game indices). In seven sub-slices, below | traces and frames | about 8 |
| B5 | The C runtime: the standard library in place of the 1989 heap (0810's allocator and combat's near heap), streams (0810's) and divide (0810's and 1BF5's), keeping any game-visible limit as an explicit rule with a test. The near heap's native sizes decide when it runs out, and that is the limit to keep. **As built:** the kept rules, each in `tests/runtime_test.cpp`, are (1) the near heap's exhaustion, 0810's allocator as accounting over 16-bit offsets with no bytes behind them (its fragmentation decided no failure in the traces or sound scripts, but can decide one); (2) the 1989 long divide (`runtime_1989::signed_long_divide`) and the overflow fallback (`runtime_1989::horizon_divide_fallback`); (3) the stream table's slots, because a slot's address goes into the checksum that seeds the combat random mix (1ADF:02F8, 0810:2C40); (4) a stream's 512-byte buffer charged at the first read. A released near-heap object keeps its own bytes (the stale sound link was never read), until a request of the same native and host size, kind and place takes it over with those bytes: a combat's object count stays bounded however many times the map and inspection screens are opened (a 500-visit test; the combat_cockpit trace ends with 78 objects, 137 without the takeover). **Difference from the earlier slices:** an object first allocated without a zeroing request starts as zeros; before B5 it started as whatever `malloc` returned. Each caller writes what it later reads (checked by reading each, and by filling new objects with 0xA5: two traces with screens and three event logs unchanged), with two cases to name: the LZW workspace's last 12 bytes, which 1BF5:5334's clear leaves out (DOS far memory or this heap's scratch; a phrase byte is always written before it is read), and the screen buffer (1BF5:14BE), whose read counts are ignored, so a short first read would show zeros where it showed stale bytes; later short reads show the previous chunk, as before. Combat's INT00 owner stays, because the missing-ITM tone fallback reads the vector. `MW_MAIN_0800.cpp` is S8's; the exit message table and checksum stay | traces and frames | 4 |
| B6 | DOS files: `std::filesystem` in place of the INT 21h handle layer and 8.3 names: `dos_files.h` and its three campaign callers (resources, saves, start-up), and combat's file streams. The save test is new in this slice: a save written and read back gives the same exchange block, field by field. **As built:** `game_files.h` and `src/Support/GameFiles.cpp` (`GameFile`, closed by its scope) replace `dos_files.h`; the kept rules are named there and tested in `tests/game_files_test.cpp` and `save_roundtrip`. Files open read only, so a read-only installation plays (`read_only_install`). A save on a full disk is reported as one. No golden changed: no trace held a handle. The overlay's font-handle word (0AD2) stays as overlay bytes, value 5 | traces; the setup and save tests; done | 3 |

**Group B total:** about 35 developer days, roughly 7 weeks. **With group A:** about 86-87 developer days plus the listening sessions, roughly 17 weeks, for one developer, in order.

For later, when the callers become ours (group B, or H5 at the latest):
- **The mode calls by what they do.** `MwSetVideoMode` and `MwGetVideoMode` (G5) still take the BIOS mode numbers, 0Dh and 3, because their callers are line-for-line conversions. Once those callers are ours, they set up the canvas and close it by name. The numbers stay only where the game's data stores them (the campaign's saved mode).

Candidates, not scheduled:
- **Present only a changed frame (after H4d).** Every pump presents, and a present compares all 64,000 indices of the shown page with the last frame (and range-checks each one) even when nothing changed: a remaining per-pump cost, not yet measured. A dirty mark would skip the comparison: the canvas's writers, page flips and palette changes set it. The mark must never miss a write, because the frame observer, and so the traces, see a frame only when this comparison finds a change. Until B4, a writer that reaches the canvas through a byte window has to set it too. The index range check moves to the writers, or stays on the changed path. The check: every log identical, and the CPU of a held pause measured before and after.

## 6. Risks

1. **Instrument choice is taste.** No GM program is exactly an MT-32 timbre. The table is chosen by ear, and the listening pass signs it off. Until then, S3's first table plays the right notes at the right times with provisional instruments.
2. **Soundfont size.** The full FluidR3 GM bank is 148 MB; S2's subset is 34.6 MiB, stereo throughout. It is generated at build time, so the repo stays small, but the package carries it. If the package's size matters, R revisits key-range pruning or a mono downmix, with listening as the check.
3. **Tone voice and the original speaker.** The original speaker's timbre came from the PC's speaker cone, not from a square-wave formula. The bar is the same pitches and timing (the tone log), not that hardware's sound.
4. **Clock-derived BIOS ticks.** Game logic that reads the 18.2 Hz count (the title's length rule picks the start world) must see the same counts from the millisecond clock. The game-event log catches any difference exactly. Likewise the music clock depends on the combat tick's timing. The note log's timestamps catch any tempo drift exactly.
5. **Players who used the MT-32 or AdLib options** lose them. That is the decision. MIDI out to a real device isn't emulation and could return later as an option, but it isn't in this plan.
6. **The original reads across its own layout.** Some code reads past one field into the next, or at an offset another routine left behind (the campaign's MPU install copies a byte at the offset DOS's GET VECTOR left, H4c). Typed structs end that layout. Each such read becomes an explicit rule with a comment, and a test where the effect can be seen; B1's by-name check names any field that moves. A read that can't be made explicit stops the slice and goes to the lead.
