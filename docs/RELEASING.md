# Building a package

Nothing publishes from this repository: there is no release workflow, and a
release (a preview included) is made only when the project decides to. The
workflows build and check packages; a person publishes them.

## The workflows

- `linux.yml` builds the Linux package on every push to main, and by hand.
  It runs the packaging unit tests first.
- `windows.yml` builds the Windows package by hand only (Windows runners
  cost twice as much). Run it before a milestone.
- `build.yml` builds the headless Windows configuration, by hand only.

Each package workflow uploads its archive as a workflow artifact
(`release-linux`, `release-windows`); none creates a tag or a release.

Windows uses MSVC x64 on Windows Server 2022. Linux uses GCC on Ubuntu 24.04
x86-64 (glibc 2.39+). SDL 3.2.0 is pinned by commit, matching the supported CMake
minimum; CMake 3.31.6 is pinned for the older dependency build scripts. Munt and
Nuked-OPL3 use the project's existing pinned commits. SDL and the LGPL audio
libraries are bundled as shared libraries. Windows also bundles the MSVC CRT.
Linux requires compatible system C/C++ runtime, desktop and audio libraries.

## What a package holds

`tools/build_package.py` builds the program and stages:
- the executable and its shared libraries;
- `data/`: our rules (`data/rules`) and screen layout (`data/ui`), which the
  game reads beside the program; packaging refuses a tree without them;
- `MechWarrior.ini`, `LICENSE`, a `README.txt` and the licence notices.

No original game files, saves, story files or synthesizer ROMs are packaged.
On the first run the game reads the player's copy's programs once (MW_MAIN.EXE,
BTECH.EXE and MW_EGA.EXE, in a revision `SUPPORTED_VERSIONS.md` lists) and
writes a `story/` folder into the save directory; from then on it plays from
`story/`, `data/` and the game's resource files, and never opens a program
(`docs/phase2/PLAN.md` section 9).

## Starting without a game folder

An installed copy is started with no arguments (a shortcut, a double-click).
The game folder is then the `game_folder` line of the player's own
`MechWarrior.ini` (in the save directory), or, the first time, the one the
player picks in a folder dialog: it must hold `MW_MAIN.EXE` until the story
is imported, and the game remembers it. The log (`mechwarrior.log`) and F12
captures go beside the saves. Deleting the `game_folder` line asks again.
With a game folder on the command line nothing changes.

The save directory is `MECHWARRIOR_SAVE_DIRECTORY`, or SDL's per-user folder:
`%APPDATA%\BattleTechReverse\MechWarrior` on Windows,
`~/Library/Application Support/BattleTechReverse/MechWarrior` on macOS and
`~/.local/share/BattleTechReverse/MechWarrior` on Linux. Nothing is written
into the install folder.

## Names and installers

A release is tagged `mw1r-vX.Y.Z`, only when the project decides to; the
label below is that tag (or any other name for a test build). Each platform's
files are named `MW1-Rebuild-<label>-...`:

| Platform | Made on | Files |
|---|---|---|
| Windows x64 | Windows, MSVC | `-win64.zip`, and with `--installer` `-Setup.exe` (Inno Setup) |
| macOS | a Mac | `-mac.dmg` (`MechWarrior 1 Rebuild.app`, signed ad hoc), this Mac's architecture |
| Linux x86-64 | Linux or WSL | `-linux-x86_64.tar.gz`, and with `--installer` `-x86_64.AppImage` |

    python tools/build_package.py --sdl SDL_SOURCE --label LABEL [--game GAME] [--installer] [--appimage-tools DIR]

- **Windows**: the program is linked as a GUI program
  (`MECHWARRIOR_WINDOWS_GUI`), so no console window opens, with its icon
  (`tools/package/mechwarrior.ico`). `--installer` runs Inno Setup's `iscc`
  (on the PATH, scoop's `extras/inno-setup`, or Program Files) on
  `tools/package/installer.iss`. Setup installs to
  `Program Files\MechWarrior 1 Rebuild` (or "only for me"), with Start-menu
  and desktop shortcuts and an uninstaller that leaves the save directory. Its
  AppId is this project's own and stays the same every version, so a new
  Setup upgrades in place.
- **macOS**: the libraries go to `Contents/Frameworks` named `@rpath`, `data/`
  and `MechWarrior.ini` to `Contents/Resources` (SDL's base path); the
  libraries, the program and the app are signed ad hoc with
  `tools/package/mechwarrior.entitlements` and checked with
  `codesign --verify --deep --strict`. The dmg holds the app, a link to
  Applications and the read-me. Unsigned by a developer identity: the first
  start needs right-click, Open.
- **Linux**: `--appimage-tools DIR` names the folder with
  `linuxdeploy-x86_64.AppImage` and `appimagetool-x86_64.AppImage`. The
  AppImage holds `usr/bin/mechwarrior` with `data/` and `MechWarrior.ini`
  beside it, and the notices in `usr/share/doc/mechwarrior`.

Every installer's contents pass `tools/package/check_paths.py`: no local path
or user name in any file. Each file gets a `.sha256` beside it; for a release,
`python3 -I tools/package/sums.py <folder>` writes one `SHA256SUMS` over all
the platforms' files.

## The smoke test

Packaging ends by extracting the archive and starting the program from it:
- with `--help` it must print its usage (a start with no arguments asks for
  the game folder);
- with a game folder (`--game DIR`, or `MW1O_GAME`), it also runs the first
  run headless from an empty save directory and must import the story and
  reach the game's first input wait before it is stopped. The game folder is
  only read and is never packaged. CI has no game folder, so this half runs
  only where one is given.

To run the same checks on a package already extracted (on any platform with a
game folder):

    python3 tools/build_package.py --smoke DIR/MechWarrior-LABEL-PLATFORM --game "$GAME"

The packaging unit tests (no build, no game files):

    python -m unittest discover -s tools -p 'test_build_package.py' -v

## Before calling a package validated

The smoke test establishes loading, the first-run import and startup only.
Before a package goes to anyone, check by hand on each platform: SDL
initialisation, the campaign, a combat and its return, input and audio. macOS
and Linux playable builds are not yet qualified (`RUNTIME_STATUS.md`).
