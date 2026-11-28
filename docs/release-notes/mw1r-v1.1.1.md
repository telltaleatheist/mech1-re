**MechWarrior 1 Rebuild** is a fan rebuild of MechWarrior (Activision, 1989, developed by Dynamix) for today's Windows,
macOS and Linux. It is built on [Elthial's MechWarrior SDL port](https://github.com/Elthial/Mechwarrior-sdl-port), the
readable reconstruction of the original program, whose research and code it continues. It is not an emulator and not a
patched copy. It is unofficial and free, and not affiliated with or endorsed by any holder of rights in the game.

This version changes only updating from the menu (below). Everything else is as in 1.1.0, which added a menu over the
game and a first start that sets itself up. The game itself plays as in 1.0.2.

## New in 1.1.1

- **Updating from the menu now works for the Windows zip.** A copy unpacked from the zip and started from its own folder
  now puts the new version in place and starts it again. If you play the Windows zip of 1.1.0, download this version by
  hand once; after that, updates work from the menu.
- **A found update is a small notice in the corner.** "UPDATE AVAILABLE" and the version show in the corner for a few
  seconds, without stopping the game or covering it. Press the menu's keys (Shift+Alt+F1, Cmd+Shift+F1 on a Mac) to see
  the update and start it.
- **The log says what an update did**: where it was downloaded from, that its list of files and its signature checked,
  that it was put in place, and, on the next start, the version it went from and to. This helps if an update goes wrong
  and you report it.

## New in 1.1.0 (since 1.0.2)

- **A menu over the game.** Press Shift+Alt+F1 (Cmd+Shift+F1 on a Mac) at any moment, in the campaign or in a battle.
  The game waits while it is open, and Esc takes you back to it. For the first few starts a small note in the corner
  says so. In the menu:
  - **Game files**: the copy of the game in use. CHANGE sets the game up again with another copy and starts the game
    again with it; if you cancel, the game starts again with the copy it had.
  - **Updates**: whether a newer version is out, checked against the BattleTech launcher's signed list. UPDATE NOW
    downloads it, checks it against the release's signed list of files, copies your saves, the story and your settings
    to a Backups folder first, puts the new version in place and starts it. A start checks once a day by itself, unless
    you turn that off; the launcher does its own checking, so a game it starts never asks.
  - **Check my files**: the program's files and the game's, each checked against the record made when they were put in
    place, naming any that are missing or changed.
  - **Reset settings**: your `MechWarrior.ini` back to the packaged settings (a copy goes to Backups first). The copy of
    the game you use is kept.
  - **Report a problem**: the page to report it on, and the log to attach (open it, or copy its path).
  - **Open my data folder**: the folder that holds your saves, settings and log.
- On Windows, if your keyboard-layout hotkey Alt+Shift is turned on, the menu's keys may also switch the layout.
- **The first start sets itself up.** A copy of the game found on your computer is checked and copied in at once, in a
  small window, with no question. If the check finds something to say, such as the one damaged file this project knows
  of, it shows that first. With no copy found, the download of the 1990-01-22 revision starts at once in the same small
  window: from the BattleTech launcher's signed list, only the exact file the list names, and going on from where it
  stopped if it breaks off. CANCEL and USE MY OWN COPY INSTEAD are there all along. A failure or a cancel opens the setup
  window, where you pick a copy yourself or start the download again.

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

None of the game's files are in these downloads. The game needs a copy of MechWarrior for DOS: the 1989 release or the
one of 1990-01-22. The first start looks for one on your computer and copies it into the rebuild's own folder. If files
are missing, damaged or unlike the ones this project knows, it says so first. If none is found, it downloads the
1990-01-22 revision (above). You can instead pick your own copy: its `BTECH.EXE`, any file in its folder, or the archive
it came in. Your copy is never changed.

The BattleTech launcher (1.5.0 or later) can install, set up and start the game too.

## Downloads

- **Windows (64-bit):** `MW1-Rebuild-mw1r-v1.1.1-Setup.exe` installs it with a Start-menu shortcut.
  `MW1-Rebuild-mw1r-v1.1.1-win64.zip` is the same program to unpack anywhere and run `mw1r.exe`. Neither is signed, so
  Windows may warn first: choose "More info", then "Run anyway".
- **macOS 12 or later (Apple silicon and Intel):** `MW1-Rebuild-mw1r-v1.1.1-mac.dmg`. Open it and drag MechWarrior 1
  Rebuild to Applications. The app is signed and notarized, so it opens like any other app.
- **Linux (x86-64; needs glibc 2.38 or later and GCC 13's libstdc++; tested on Ubuntu 24.04):**
  `MW1-Rebuild-mw1r-v1.1.1-x86_64.AppImage`: make it executable and run it. An AppImage needs FUSE's `fusermount`; on a
  system without it, start it with `APPIMAGE_EXTRACT_AND_RUN=1` set. `MW1-Rebuild-mw1r-v1.1.1-linux-x86_64.tar.gz` is the
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
- The first start's download of the original files. It has been tested with the launcher's own file and with a test list,
  not yet by a player.
- The menu over the game on a real screen, and its keys on real keyboards. It was tested on hidden windows, with its keys
  sent by the tests.
- Updating from the menu with a real release. The same updater has been tried on Windows (zip and installer) with
  Inception Rebuild's releases; on a Mac and on Linux it has not yet been tried with a real release.

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
