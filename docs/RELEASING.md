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
The player supplies their own copy of the game as the first argument. On the
first run the game reads that copy's programs once (MW_MAIN.EXE, BTECH.EXE and
MW_EGA.EXE, in a revision `SUPPORTED_VERSIONS.md` lists) and writes a
`story/` folder into the save directory; from then on it plays from `story/`,
`data/` and the game's resource files, and never opens a program
(`docs/phase2/PLAN.md` section 9).

## The smoke test

Packaging ends by extracting the archive and starting the program from it:
- with no arguments it must print its usage;
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
