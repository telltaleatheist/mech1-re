# Building the port

The public project needs a C++20 compiler and CMake 3.20 or later. A playable build
also needs SDL3 3.2 or later, supplied as an installed CMake package or an existing
source checkout. Dependencies are never downloaded automatically. Windows x64
is the currently qualified runtime target.

## Windows

Install Visual Studio with Desktop development with C++ and CMake tools. From
the repository root:

```powershell
./src/Build.ps1 -Configuration Release -SDL3Directory 'D:/Libraries/SDL3/cmake'
# Alternative: compile your existing SDL3 checkout with the game.
./src/Build.ps1 -Configuration Release -SDLSourceDirectory 'D:/Libraries/SDL'
./src/Run-MechWarrior.ps1 -GameDirectory 'D:/Games/MechWarrior'
```

The executable and SDL3.dll are written to `build-visual-studio/Release`.
For source navigation and debugging, open the repository folder in Visual Studio
(File > Open > Folder): it reads `CMakeLists.txt` as a CMake project, so it builds
the same targets as `src/Build.ps1` (set SDL3_DIR or MECHWARRIOR_SDL_SOURCE_DIR in
its CMake settings when needed). To debug `mechwarrior`, give it your game folder
as its argument, for example your private `assets/original-game`; the launcher
supports an external installation instead. See
[original-game requirements](ORIGINAL_GAME.md).

## The music soundfont

A playable build cuts its music soundfont from FluidR3 GM, whose pinned
source (135 MB) is fetched once per machine before the first configure:

```sh
tools/deps/fetch_soundfont.sh            # macOS, Linux, WSL
python tools\soundfont\subset.py fetch   # Windows
```

It lands in `build-deps/soundfont`; `MW1O_SOUNDFONT_SOURCE` can name a copy
instead. Configure refuses without it, and the build itself never downloads.
See `tools/soundfont/subset.py`.

## Direct CMake

```sh
cmake -S . -B build -DMECHWARRIOR_BUILD_SDL=ON -DSDL3_DIR=/path/to/SDL3/cmake
cmake --build build --config Release
```

To compile only the platform-independent library without SDL or original assets:

```sh
cmake -S . -B build-headless -DMECHWARRIOR_BUILD_SDL=OFF
cmake --build build-headless --config Release
```

The equivalent Windows command is `./src/Build.ps1 -Headless`.
Headless compilation does not produce a playable game. Non-Windows gameplay
has not yet been qualified.

Research tests, native comparisons, asset extraction and code-generation tools
are not included here. The reverse-engineering repository owns those workflows.
This tree builds without them; compilation alone does not certify a mission or
complete campaign. See [runtime status](RUNTIME_STATUS.md).
