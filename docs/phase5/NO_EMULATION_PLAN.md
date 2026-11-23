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

## 3. How we check "play identical"

### Sound

1. **The note log, exact.** A new log at the sequencer's output records each event as (game ms, channel, event, key, velocity).
   - Goldens are recorded from today's port, through its MPU path, before anything changes. They cover all five songs: the title (MECH), the bar (MECHBAR), the LAUNCH and DEATH cues, and VICTORY.
   - Level 3 of the audio plan showed the port's MIDI byte stream already matches the original's under DOSBox-X: 3,003 title bytes and 4,730 bar bytes, exactly. So these goldens are anchored to the original.
   - Every sound slice keeps the note log identical. The instrument table changes the sound, never the notes or their times.
2. **The tone log, exact.** Combat's effects as (game ms, effect, frequency steps). The goldens are converted from today's port-event log, whose PIT divisors give the frequencies.
3. **The traces, unchanged.** Sound doesn't touch game state, so check.py stays at 8/8 for 1990 and 6/8 for 1989 (combat_cockpit and combat_retreat are the known two), and exe_free stays 8/8.
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
| TinySoundFont (a3621a1) | MIT | music synthesis (new), `extern/TinySoundFont` | yes |
| FluidR3 GM 3.1, subset | MIT | music instruments (new): cut at build time from the pinned SF2, fetched once (`tools/deps/fetch_soundfont.sh`) | yes |
| *FluidR3 Mono GM* | *MIT* | *not used: SF3 only; its SF2 can't be pinned* | yes |
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
| S4 | The instrument table from the listening notes, per song: programs, levels, pans, the drum map. Re-rendered for a second listen if asked | note log identical; the project owner's sign-off | 2, plus listening |
| S5 | `Effects`: 2134 outputs tone events to the tone voice, for the thirteen effects the game requests (9 and 12 never are) | tone log identical | 3 |
| S6 | Speaker-style music on the tone voice. The PIT channel 2 and 61h decoding removed | note log identical | 2 |
| H4 | Timers and interrupts: the game's millisecond clock calls the music tick at its rate and supplies the 18.2 Hz count where the game reads it. The interrupt flag, vector install and restore, the INT 8 handler's IRQ entry and BIOS chain and the per-read time charges removed (the PIT latch window went in H3) | note log identical, timestamps included; game-event log identical | 5 |
| S8 | The campaign's AdLib and MPU backends and combat's 21CA and 2176 removed. 1ADF reports the rebuild's sound | logs and traces exact | 2 |
| S9 | Nuked-OPL3, Munt and MPU401/Adlib/MT32 removed from the build. ROM settings dropped; `sound_system` replaced by `music`. Notices, AUDIO.md and packaging (no nukedopl or mt32emu library) updated. The AppImage tooling's licence confirmed | packaging tests; no LGPL in the package | 2 |
| H5 | The port bus retired: `MwSdlWriteDevicePort`, `ReadDevicePort` and the port-word hooks removed with their last callers; the sound-port event log retired in favour of the note and tone logs | everything above exact | 2 |
| R | The release gate: every gate on Windows, macOS and Linux from fresh builds. Packages from a local tag. The listening list and the visual acceptance list signed off on the packaged builds. A physical gamepad tried by hand in both games, if the project owner has one (H3's host read is otherwise untested by hand) | all of the above | 2 |

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

For later, when the callers become ours (group B, or H5 at the latest):
- **The mode calls by what they do.** `MwSetVideoMode` and `MwGetVideoMode` (G5) still take the BIOS mode numbers, 0Dh and 3, because their callers are line-for-line conversions. Once those callers are ours, they set up the canvas and close it by name. The numbers stay only where the game's data stores them (the campaign's saved mode).

## 6. Risks

1. **Instrument choice is taste.** No GM program is exactly an MT-32 timbre. The table is chosen by ear, and the listening pass signs it off. Until then, S3's first table plays the right notes at the right times with provisional instruments.
2. **Soundfont size.** The full FluidR3 GM bank is 148 MB; S2's subset is 34.6 MiB, stereo throughout. It is generated at build time, so the repo stays small, but the package carries it. If the package's size matters, R revisits key-range pruning or a mono downmix, with listening as the check.
3. **Tone voice and the original speaker.** The original speaker's timbre came from the PC's speaker cone, not from a square-wave formula. The bar is the same pitches and timing (the tone log), not that hardware's sound.
4. **Clock-derived BIOS ticks.** Game logic that reads the 18.2 Hz count (the title's length rule picks the start world) must see the same counts from the millisecond clock. The game-event log catches any difference exactly. Likewise the music clock depends on the combat tick's timing. The note log's timestamps catch any tempo drift exactly.
5. **Players who used the MT-32 or AdLib options** lose them. That is the decision. MIDI out to a real device isn't emulation and could return later as an option, but it isn't in this plan.
