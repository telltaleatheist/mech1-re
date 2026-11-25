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
x86-64; what it builds needs glibc 2.38 and GCC 13's libstdc++, by the symbol
versions in the files. SDL 3.2.0 is pinned by commit, matching the supported CMake
minimum; CMake 3.31.6 is pinned for the older dependency build scripts. SDL is
bundled as a shared library. Windows also bundles the MSVC CRT.
Linux requires compatible system C/C++ runtime, desktop and audio libraries.

## What a package holds

`tools/build_package.py` builds the program and stages:
- the program, `mw1r` (`mw1r.exe`; `Contents/MacOS/mw1r` in the Mac app;
  `usr/bin/mw1r` in the AppImage), and its shared libraries. The build's
  target and file stay `mechwarrior`, which the tools and traces run; the
  package names it;
- `data/`: our rules (`data/rules`) and screen layout (`data/ui`), which the
  game reads beside the program; packaging refuses a tree without them;
- `MechWarrior.ini`, `LICENSE`, a `README.txt` and the licence notices.

No original game files, saves or story files are packaged.
On the first run the game reads the player's copy's programs once (MW_MAIN.EXE,
BTECH.EXE and MW_EGA.EXE, in a revision `SUPPORTED_VERSIONS.md` lists) and
writes a `story/` folder into the save directory; from then on it plays from
`story/`, `data/` and the game's resource files, and never opens a program
(`docs/phase2/PLAN.md` section 9).

## Starting without a game folder

An installed copy is started with no arguments (a shortcut, a double-click).
It plays from the `game_folder` line of the player's own `MechWarrior.ini`
while that folder is still the game, else from the player's own copy
(`install/` in the save directory). With neither, re-setup-kit's setup
screen opens first (`src/SDL/SDLSetup.cpp`, `src/Setup`), in the rebuild's
theme (`data/setup`): FIND THE GAME looks on the computer, then asks; the
player picks their copy of MechWarrior (its `BTECH.EXE` or `MW_MAIN.EXE`, its
folder through one of its files, or the archive it came in, such as a .zip);
"Use these game files?" copies it into `install/` in the save directory, or
with CHANGE FOLDER into a folder of the game's own (marked `game=mw1r`). The
original, folder or archive, is only read. A copy needs `MW_MAIN.EXE` until
the story is imported. The log (`mechwarrior.log`) and F12 captures go beside
the saves. With a game folder on the command line nothing changes. A hidden
run never opens the screen, except a check's (`REKIT_SETUP_FRAMES`, driven by
the kit's `REKIT_SETUP_*` hooks, still with no picker).

The launcher asks the program through the kit's contract:
`mw1r --status` prints one JSON line (game `mech1`; files `ready`,
`missing` or `incomplete`; `story import` pending until done), exits 0 and
writes nothing; `mw1r --setup` sets the game up and exits 0 ready,
1 cancelled or 2 failed (the reason as `{"error": ...}` on stdout).
`tests/status_setup_test.py` checks both on the built program.

### The setup served to the launcher

`mw1r --setup` followed by `--describe`, `--search`, `--look <pick>` or
`--install <pick> [--folder <dir>]` is the setup for the launcher's own
screens (the kit's `rekit/sdl/setup_serve.h`; `--status` says `"setup": 1`).
It opens no window and no picker. It answers on stdout with JSON lines and
nothing else, the last one with `done`, and exits 0 (ready), 1 (cancelled) or
2 (failed). `MwSetupServe` (`src/SDL/SDLSetup.cpp`) answers with the setup
screen's recipe, so what is found, the words for it, the sentence for a pick
that can't be used, the file check's short note and the install are the
screen's.

- The folder a served install went to is remembered in `MechWarrior.ini`, as
  after the screen. So is a looked-at copy that is in place already.
- `--search` passes over the copy in use and the player's own install. It
  answers the player's own when that is ready and the copy in use isn't.
- `--folder` names the game's own folder: the kit marks it, refuses one that
  holds other things or is another game's, and installs to its `install`.
- A `cancel` line on stdin stops a look or an install at its next progress
  step. What was copied is removed; a folder named with `--folder` keeps the
  kit's mark. A launcher that closes the pipe stops nothing.
- Wrong arguments get one `done` line saying what is wrong, and exit 2.
  Anything else after `--setup` is the usage and exit 2. So is an ask with a
  test's hook (`MECHWARRIOR_SETUP_PICK`, `REKIT_SETUP_GO`) set and
  `MECHWARRIOR_SAVE_DIRECTORY` not: one line, exit 2.
- What a look or an install says besides its answer (the file check's line)
  goes to `setup.log` in the save directory, anew each time.
- One install at a time into a folder: `install.lock` beside it is locked
  while the copy is made, and removed after. A second one is refused. A game
  that is playing from the folder isn't asked.
- No program record is written.

`tools/package/served_setup.py` asks a built or packaged program through
pipes, as the launcher does, with a video driver that doesn't exist.
`status_setup` and every package's smoke test run it. Without the game's
folder, the parts that need a copy are skipped and named.

The save directory is `MECHWARRIOR_SAVE_DIRECTORY`, or SDL's per-user folder:
`%APPDATA%\BattleTech\MechWarrior 1 Rebuild` on Windows,
`~/Library/Application Support/BattleTech/MechWarrior 1 Rebuild` on macOS and
`~/.local/share/BattleTech/MechWarrior 1 Rebuild` on Linux (or under
`$XDG_DATA_HOME`): the family's folder, re-setup-kit's `rekit::kFamilyOrg`.
Nothing is written into the install folder. Each start settles it in this
order (`setup::settle_user_folder`):

1. The BattleTech folder has something in it: used as it is.
2. Else a test version's `MicroProse/MechWarrior 1 Rebuild` has: renamed
   across once (`rekit::move_once`, never a copy), and `MicroProse` removed
   only if that leaves it empty. If the rename fails (another drive, a file in
   use), that start uses the MicroProse folder and the next start tries again.
3. Else the previews' `BattleTechReverse/MechWarrior` is copied in once (a
   marker file; nothing overwritten, the old folder left as it was).

Packages link libarchive, zlib and liblzma statically, built from pinned
sources: `tools/deps/build_archive.sh mac|linux` (into `build-deps/`) and
`tools/windows/build_deps_archive.cmd` (into `deps\archive`, clang-cl).

The music soundfont is cut from FluidR3 GM at build time. Its pinned source
is fetched once per machine, beside the libarchive step:
`tools/deps/fetch_soundfont.sh` (Windows: `python tools\soundfont\subset.py
fetch`), into `build-deps/soundfont`, or `MW1O_SOUNDFONT_SOURCE` names a copy.
Configure refuses without it; the build never downloads. A package without
the subset hashing as `data/music/soundfont_subset.sha256` pins, or without
its `COPYING` and `README`, is refused.

## Names and installers

A release is tagged `mw1r-vX.Y.Z`, only when the project decides to; the
label below is that tag (or any other name for a test build: a name with
`-test` in it, so it is never taken for a release). The label is the program's
version too (`MECHWARRIOR_VERSION`, the `version` `--status` reports), a
release tag's number alone (`mw1r-v1.0.0` reports `1.0.0`); the Mac app's
Info.plist and the Windows Setup show the same. Packaging with a
release's label is refused unless the tree is a git checkout at that tag with
nothing changed, added or missing (ignored files don't count). Each
platform's files are named `MW1-Rebuild-<label>-...`:

| Platform | Made on | Files |
|---|---|---|
| Windows x64 | Windows, MSVC | `-win64.zip`, and with `--installer` `-Setup.exe` (Inno Setup) |
| macOS | a Mac | `-mac.dmg` (`MechWarrior 1 Rebuild.app`, signed ad hoc), universal (x86_64 and arm64), macOS 12 and later |
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
  Setup upgrades in place. It first deletes what earlier previews and test
  versions left there and no version ships now, by exact name; a player who
  installs into a folder holding another program's file of such a name would
  lose that file.
- **macOS**: the libraries go to `Contents/Frameworks` named `@rpath`, `data/`
  and `MechWarrior.ini` to `Contents/Resources` (SDL's base path); the
  libraries, the program and the app are signed ad hoc with
  `tools/package/mechwarrior.entitlements` and checked with
  `codesign --verify --deep --strict`. The dmg holds the app, a link to
  Applications and the read-me. Unsigned by a developer identity: the first
  start needs right-click, Open.
- **Linux**: `--appimage-tools DIR` names the folder with
  `linuxdeploy-x86_64.AppImage`, `appimagetool-x86_64.AppImage` and
  `runtime-x86_64`, which `tools/deps/get_appimage_tools.sh DIR` fetches; each
  is refused unless it has the hash pinned there. The
  AppImage holds `usr/bin/mw1r` with `data/` and `MechWarrior.ini`
  beside it, and the notices in `usr/share/doc/mw1r`, the AppImage runtime's
  (`tools/package/appimage-runtime`) among them. Packaging unpacks it with
  `--appimage-extract` and starts the program from the unpacked folder, as the
  launcher does.

For the BattleTech launcher, a program built with a release's tag
(`MECHWARRIOR_RELEASE_TAG`, which packaging sets from the label) holds
`MW3-PROGRAM-VERSION:<tag>` in its file, and each start with a player at it
writes `rebuild-program.txt` (game, program, version) into the player's folder,
so the launcher finds a copy installed without it. Any other label's program
holds no such string and writes nothing; packaging refuses a program whose
stamp is not its label's.

Every installer's contents pass `tools/package/check_paths.py`: no local path
or user name in any file, and no file of the game, by name. Each file gets a `.sha256` beside it; for a release,
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

On a Mac these runs start a temporary copy of the app
(`tools/package/background_app.py`): a program inside an app comes up in the
foreground, whatever the hidden run's settings say, so the copy's Info.plist
says `LSBackgroundOnly`, and it is signed ad hoc again. The app's own signature
is checked first, and the app is only read. For a run by hand, make the copy and
start its program; `x86_64` after the folder makes it the Intel half alone,
which Rosetta runs on an Apple silicon Mac:

    python3 -I tools/package/background_app.py "MechWarrior 1 Rebuild.app" FOLDER [x86_64]

To run the same checks on a package already extracted (on any platform with a
game folder):

    python3 tools/build_package.py --smoke DIR/MechWarrior-LABEL-PLATFORM --game "$GAME"

On a Mac, `--smoke "MechWarrior 1 Rebuild.app" --arch x86_64` runs the Intel half.

The packaging unit tests (no build, no game files):

    python -m unittest discover -s tools -p 'test_build_package.py' -v

## Making a release

By hand, when the project owner has cleared it and not before. The gate
(`phase5/GATE_PLAN.md`) is run on the candidate, one commit of main, before
any of this. What fails a step stops the release.

1. Choose the version, `mw1r-vX.Y.Z`, and the commit. Finish its notes
   (`RELEASE_NOTES_DRAFT.md` for the first one) on main, with every line
   marked CHECK in them confirmed or taken out. In `README.md`, the status
   line and the launcher's sentence say what is true then. Copy `DEPLOY.md` if the family's has changed. The project owner reads the notes
   and the README. If main moved, the steps of the gate it can affect run again.
2. Tag that commit on each building machine, locally: `git tag mw1r-vX.Y.Z`.
   Nothing is pushed yet. Packaging refuses this label in any other tree.
3. Build the packages from the tag, each in a clean checkout with its
   submodules and with the game given, so the smoke test does its first run:

       # macOS
       tools/deps/build_archive.sh mac
       python3 -I tools/build_package.py --sdl SDL --label mw1r-vX.Y.Z --game GAME
       # Linux (or WSL)
       tools/deps/build_archive.sh linux
       tools/deps/get_appimage_tools.sh DIR
       python3 -I tools/build_package.py --sdl SDL --label mw1r-vX.Y.Z --game GAME --installer --appimage-tools DIR
       # Windows
       tools\windows\build_deps_archive.cmd
       python -I tools\build_package.py --sdl SDL --label mw1r-vX.Y.Z --game GAME --installer

   SDL is the pinned commit (`.github/workflows/linux.yml` names it). Visual
   Studio's `cmake` must be the first on Windows' PATH. Each run must end with
   its smoke test, the first run and the served setup passed, none skipped.
4. Put the five packages in one empty folder (`-win64.zip`, `-Setup.exe`,
   `-mac.dmg`, `-x86_64.AppImage` and `-linux-x86_64.tar.gz`), check each
   against its `.sha256`, then write the one list:

       python3 -I tools/package/sums.py FOLDER

5. Check each package by hand on its system ("Before calling a package
   validated", below), from the file that will be uploaded. `Setup.exe` is the
   exception: it isn't run before the release. It carries the AppId every
   version shares, so where a copy of the game is installed it would replace
   that copy, and its uninstaller would remove it. Its twin is run instead: the
   same packaged folder and label, compiled from a copy of `installer.iss` with
   a test AppId and a test name (so its folder and shortcuts are its own too).
   The twin is installed into its own folder, its files compared with the
   packaged folder's, the smoke test run on it with both copies of the game,
   and it is uninstalled; a copy that was installed before must be unchanged.
   What this leaves unproved is the release file itself, beyond its hash.
6. Push the tag: `git push origin mw1r-vX.Y.Z`.
7. Publish, with the notes as the text:

       gh release create mw1r-vX.Y.Z --verify-tag --title "MechWarrior 1 Rebuild X.Y.Z" \
         --notes-file NOTES <the five packages> SHA256SUMS

   Not a draft and not a prerelease: the launcher takes only a published, full
   release, and installs nothing without the package's line in `SHA256SUMS`.
8. Download every asset again from the release, and check it against
   `SHA256SUMS`.
9. On a clean games folder, the launcher's INSTALL must take the game, set it
   up and start it; with an older copy there, UPDATE must.

## Before calling a package validated

The smoke test establishes loading, the first-run import and startup only.
Before a package goes to anyone, check by hand on each platform: SDL
initialisation, the campaign, a combat and its return, input and audio.
What was checked on each platform for the last release, and what nobody has
played yet, is in [RUNTIME_STATUS.md](RUNTIME_STATUS.md).
