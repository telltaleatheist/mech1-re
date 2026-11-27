**MechWarrior 1 Rebuild** is a fan rebuild of MechWarrior (Activision, 1989, developed by Dynamix) for today's Windows,
macOS and Linux. It is built on [Elthial's MechWarrior SDL port](https://github.com/Elthial/Mechwarrior-sdl-port), the
readable reconstruction of the original program, whose research and code it continues. It is not an emulator and not a
patched copy. It is unofficial and free, and not affiliated with or endorsed by any holder of rights in the game.

## New in 1.0.2

- **No copy of the game? It can fetch one.** When the first start finds no copy on your computer, it offers to download
  the 1989 release from the BattleTech launcher's own list, and installs it at once. A copy found on your computer is
  always offered first, and then the download isn't asked about. The list is used only when its signature checks, and
  the download only when it is exactly the file the list names. You can still pick your own copy instead.
- **A download that breaks off goes on from where it stopped** when it is tried again.
- **Only the game is copied in.** Some copies, the downloaded one among them, carry a small tool beside the game for
  removing the original's copy protection. The rebuild has no copy protection to remove, so the install leaves that tool
  out. Your own copy is still never changed.
- **The Mac app is signed and notarized.** It opens like any other app: no right-click, Open, and no trip to Privacy &
  Security.
- **The credit to Elthial's SDL port** now also shows where the system describes the program: the Mac app's Get Info,
  the Windows installer and its shortcuts, and the Linux desktop entry.

Nothing in the game itself has changed. Your saves, settings and copied game stay as they are.

## What it is

- **The campaign and combat of the 1989 game**, rebuilt function by function: the company and its money, planets, travel,
  contracts, the bar and its pilots, buying, repairing and rearming Mechs, the story, and the battles from the cockpit.
- **The same speed on any computer.** A combat tick has a fixed length. The - and + keys make combat up to four times
  as fast, and the game remembers your choice.
- **The campaign's music on a bundled soundfont.** `music=` in `MechWarrior.ini` plays it as the PC speaker did, or turns
  it off. Combat's sounds are the original's speaker effects.
- **Up to two game controllers** as the original's joysticks.
- **A resizable window** that the picture fills.
- **No copy protection.** The original's is left out.

## Bring your own copy of the game

None of the game's files are in these downloads. You need your own copy of MechWarrior for DOS: the 1989 release or the
one of 1990-01-22. The first start looks for it on your computer, or asks you to pick its `BTECH.EXE`, any file in its
folder, or the archive it came in, then copies it into the rebuild's own folder. Your copy is never changed. If files are
missing, damaged or unlike the ones this project knows, it says so first. If none is found, it can download the 1989
release for you (above).

The BattleTech launcher (1.5.0 or later) can install, set up and start the game too.

## Downloads

- **Windows (64-bit):** `MW1-Rebuild-mw1r-v1.0.2-Setup.exe` installs it with a Start-menu shortcut.
  `MW1-Rebuild-mw1r-v1.0.2-win64.zip` is the same program to unpack anywhere and run `mw1r.exe`. Neither is signed, so
  Windows may warn first: choose "More info", then "Run anyway".
- **macOS 12 or later (Apple silicon and Intel):** `MW1-Rebuild-mw1r-v1.0.2-mac.dmg`. Open it and drag MechWarrior 1
  Rebuild to Applications.
- **Linux (x86-64; needs glibc 2.38 or later and GCC 13's libstdc++; tested on Ubuntu 24.04):**
  `MW1-Rebuild-mw1r-v1.0.2-x86_64.AppImage`: make it executable and run it. An AppImage needs FUSE's `fusermount`; on a
  system without it, start it with `APPIMAGE_EXTRACT_AND_RUN=1` set. `MW1-Rebuild-mw1r-v1.0.2-linux-x86_64.tar.gz` is the
  same program to unpack and run as `./mw1r`.
- **`SHA256SUMS`** lists every file's SHA-256.

## If you played an earlier test version

- A `MechWarrior.ini` from a test version that says `sound_system=speaker` still plays the music as the PC speaker
  did. Change that line to `music=soundfont` for the new music, or use the new `MechWarrior.ini`.
- Your folder from a test version (`MicroProse/MechWarrior 1 Rebuild`) is moved to `BattleTech/MechWarrior 1 Rebuild`
  once. Saves from the first previews (`BattleTechReverse/MechWarrior`) are copied there once.
- If you go back to an earlier test version, it reads your copy of the game again the first time you start it.
- The installer removes what the test versions left in its folder.

## Not yet played or heard by a person

Nobody has gone through these yet. Automated tests and recorded runs check parts of them:

- The whole story, from start to end, and every kind of contract with every Mech.
- The Linux package, which is built and smoke-tested automatically.
- The Intel half of the Mac package. Its setup checks, a first start on each edition and one recorded battle passed under
  Rosetta, not on a real Intel Mac.
- Real game controllers: only simulated ones have been tested.
- The new window on a real screen, on Windows, Linux or a Mac: resized, maximised or in full screen. Its drawing and its
  mouse were tested at many sizes, with hidden windows or none.
- A real reset or loss of a graphics device. The tests send what SDL would say, and refuse to draw.
- The message box that says why a start was refused: its text was checked, the box was not seen.
- Setting the game up on the launcher's screens.
- The download of the original files on the setup screen. It has been tested with the launcher's own file and with a test
  list, not yet by a player.

Please report anything that looks or sounds wrong.

## Known differences from the original

- Combat doesn't run at the old PC's own speed but at the fixed one above, which you can change.
- The graphics are the EGA version's. CGA and Tandy graphics aren't there, nor the AdLib, MT-32 and Tandy sound.
- The music's instruments are the soundfont's, chosen to be near the MT-32 version's.
- Saved games of the DOS original aren't copied in.

## Known limits

- A battle that needs a file that is damaged in your copy stops with a message naming the file. The setup names the one
  damaged file this project knows of, and it can be set up again from a good copy.
- Only the two editions above play.
