"""Build and smoke-test a redistributable runtime, without original game data.

    build_package.py --sdl SDL_SOURCE --label LABEL [--game GAME_DIR]
    build_package.py --smoke EXTRACTED_PACKAGE_DIR [--game GAME_DIR]

The package holds the program, its libraries, our data/ (rules and screen
layout, which play reads beside the program), the configuration and the
notices; never the original game's files. The smoke test starts the
extracted archive: without arguments it must print its usage. With a game
folder (--game, or MW1O_GAME) it also runs the first run headless from a
scratch save directory and must import the story and reach the game's first
input wait (the debug control's status.json) before it is stopped. --smoke
runs the same checks on a package already extracted.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import tarfile
import tempfile
import time
import zipfile

# Our data beside the program: each folder must hold JSON files.
DATA_FOLDERS = ("data/rules", "data/ui")
FIRST_RUN_SECONDS = 120


def stage_data(root, stage):
    """Copies root/data into the package, refusing a tree without the rules or the layout."""
    for folder in DATA_FOLDERS:
        if not list((root / folder).glob("*.json")):
            raise RuntimeError(f"Missing game data: {folder}/*.json")
    shutil.copytree(root / "data", stage / "data")


def check_usage(executable):
    # --help, not a bare start: without a game folder an installed copy opens a folder dialog.
    result = subprocess.run([str(executable), "--help"], cwd=executable.parent, capture_output=True, text=True, timeout=30)
    if result.returncode != 0 or "Usage: mechwarrior" not in result.stdout:
        raise RuntimeError(f"Packaged executable failed to load: {result.returncode}\n{result.stdout}\n{result.stderr}")


def check_first_run(executable, game, timeout=FIRST_RUN_SECONDS):
    """Starts the package on `game` with an empty save directory: it must import
    the story (story/import.json) and reach its first input wait (status.json)."""
    for folder in DATA_FOLDERS:
        if not list((executable.parent / folder).glob("*.json")):
            raise RuntimeError(f"The package has no {folder}")
    with tempfile.TemporaryDirectory(prefix="mw1-first-run-") as scratch:
        scratch = Path(scratch)
        saves, control, captures = (scratch / d for d in ("saves", "control", "captures"))
        for d in (saves, control, captures):
            d.mkdir()
        headless = {"MW1O_HIDDEN": "1"} if sys.platform == "darwin" else {"SDL_VIDEO_DRIVER": "offscreen"}
        env = dict(os.environ, **headless, SDL_AUDIO_DRIVER="dummy",
                   MECHWARRIOR_SAVE_DIRECTORY=str(saves), MECHWARRIOR_DEBUG_CONTROL_DIRECTORY=str(control))
        log_path = scratch / "run.log"
        with open(log_path, "wb") as log:
            game_process = subprocess.Popen([str(executable), str(Path(game).resolve()), str(captures)],
                                            cwd=executable.parent, env=env, stdout=log, stderr=subprocess.STDOUT)
            reached = False
            deadline = time.monotonic() + timeout
            try:
                while time.monotonic() < deadline and game_process.poll() is None:
                    try:
                        json.loads((control / "status.json").read_text(encoding="latin-1"))
                        reached = (saves / "story" / "import.json").is_file()
                    except (OSError, ValueError):
                        pass
                    if reached:
                        break
                    time.sleep(0.1)
            finally:
                if game_process.poll() is None:
                    if os.name == "nt":
                        game_process.terminate()
                    else:
                        game_process.send_signal(signal.SIGTERM)
                    try:
                        game_process.wait(10)
                    except subprocess.TimeoutExpired:
                        game_process.kill()
                        game_process.wait()
        if not reached:
            tail = log_path.read_text(encoding="latin-1", errors="replace").splitlines()[-10:]
            what = "exited with code {}".format(game_process.returncode) if game_process.returncode not in (None, -signal.SIGTERM) else "timed out"
            raise RuntimeError("Packaged first run did not import the story and reach the game ("
                               + what + "):\n" + "\n".join(tail))
    print(f"First run from the package: story imported, the game reached its first input wait")


def smoke(executable, game):
    check_usage(executable)
    if game:
        check_first_run(executable, game)


def run(*args, **kwargs):
    return subprocess.run(args, check=True, **kwargs)


def package(root, build, sdl, output, label, game=None):
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]*", label):
        raise ValueError("Invalid package label")
    windows = os.name == "nt"
    platform = "windows-x64" if windows else "linux-x86_64"
    name = f"MechWarrior-{label}-{platform}"
    stage = output / name
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    args = ["cmake", "-S", str(root), "-B", str(build),
            "-DMECHWARRIOR_BUILD_SDL=ON", f"-DMECHWARRIOR_SDL_SOURCE_DIR={sdl}"]
    if windows:
        args += ["-A", "x64"]
    else:
        args += ["-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_BUILD_RPATH=$ORIGIN",
                 "-DCMAKE_BUILD_RPATH_USE_ORIGIN=ON"]
    run(*args)
    run("cmake", "--build", str(build), "--config", "Release", "--target", "mechwarrior", "--parallel", "4")
    binary = build / "Release/mechwarrior.exe" if windows else build / "mechwarrior"
    shutil.copy2(binary, stage / binary.name)
    libraries = list(build.rglob("*.dll" if windows else "*.so*"))
    for lib in libraries:
        if lib.is_file():
            destination = stage / lib.name
            if destination.exists() and destination.read_bytes() != lib.read_bytes():
                raise RuntimeError(f"Conflicting runtime libraries: {lib.name}")
            shutil.copy2(lib.resolve(), destination)
    if windows:
        vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
        vs = Path(subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
                  "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip())
        crt_dirs = sorted((vs / "VC/Redist/MSVC").glob("*/x64/Microsoft.VC*.CRT"))
        if not crt_dirs:
            raise RuntimeError("Cannot locate redistributable MSVC runtime")
        for dll in crt_dirs[-1].glob("*.dll"):
            shutil.copy2(dll, stage / dll.name)
    for required in (["SDL3.dll", "mt32emu-*.dll", "nukedopl.dll"] if windows else ["libSDL3.so*", "libmt32emu.so*", "libnukedopl.so*"]):
        if not list(stage.glob(required)):
            raise RuntimeError(f"Missing runtime dependency: {required}")
    for source in [root / "LICENSE", root / "MechWarrior.ini"]:
        shutil.copy2(source, stage / source.name)
    stage_data(root, stage)
    notices = stage / "licenses"
    notices.mkdir()
    shutil.copytree(root / "docs/LICENSES", notices / "project")
    for source in [root / "docs/NOTICE.md", root / "docs/THIRD_PARTY_NOTICES.md", sdl / "LICENSE.txt",
                   build / "_deps/munt-src/mt32emu/COPYING.LESSER.txt", build / "_deps/nukedopl-src/LICENSE"]:
        prefix = "SDL-" if source.parent == sdl else "Munt-" if source.name == "COPYING.LESSER.txt" else "Nuked-" if source.name == "LICENSE" else ""
        shutil.copy2(source, notices / (prefix + source.name))
    (stage / "README.txt").write_text(
        "MechWarrior (1989) experimental SDL preservation port\n\n"
        "An original game installation is required and is not included.\n"
        "No game data, saves or MT-32 ROMs are included.\n"
        "Keep the configuration, the data folder and the shared libraries beside the executable.\n"
        "The first run reads the game's programs once and writes a story folder into the save\n"
        "directory; after that the game plays from it and never opens the programs.\n"
        "Run from a writable directory with: mechwarrior <game-directory> [capture-directory]\n"
        "Windows: use mechwarrior.exe. Linux: use ./mechwarrior.\n"
        "Linux target: Ubuntu 24.04 x86-64 (glibc 2.39+) with desktop/audio services.\n"
        "Support pilot is disabled by default. See the repository docs for controls.\n"
        "CI verifies compilation, linking and executable loading, not gameplay.\n",
        encoding="utf-8")
    archive = output / (name + (".zip" if windows else ".tar.gz"))
    if windows:
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as target:
            for file in sorted(stage.rglob("*")):
                if file.is_file():
                    target.write(file, file.relative_to(output))
    else:
        with tarfile.open(archive, "w:gz") as target:
            target.add(stage, arcname=name)
    # Exercise the archive's actual layout rather than the build directory.
    extracted = output / "smoke"
    if extracted.exists():
        shutil.rmtree(extracted)
    extracted.mkdir()
    if windows:
        with zipfile.ZipFile(archive) as source:
            source.extractall(extracted)
    else:
        with tarfile.open(archive) as source:
            source.extractall(extracted, filter="data")
    executable = extracted / name / binary.name
    if not windows:
        run("ldd", str(executable))
    smoke(executable, game)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    (output / (archive.name + ".sha256")).write_text(f"{digest}  {archive.name}\n", encoding="utf-8")
    print(f"Packaged and startup-checked {archive}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--sdl", type=Path)
    parser.add_argument("--label")
    parser.add_argument("--smoke", type=Path, help="an extracted package directory: run the smoke test only")
    parser.add_argument("--game", type=Path, default=os.environ.get("MW1O_GAME"),
                        help="an original game folder for the first-run check (default MW1O_GAME; never packaged)")
    options = parser.parse_args()
    if options.smoke:
        names = ["mechwarrior.exe", "mechwarrior"]
        executable = next((options.smoke / n for n in names if (options.smoke / n).is_file()), None)
        if not executable:
            parser.error(f"no mechwarrior program in {options.smoke}")
        smoke(executable.resolve(), options.game)
    else:
        if not options.sdl or not options.label:
            parser.error("--sdl and --label are required to package")
        root = Path(__file__).resolve().parents[1]
        package(root, root / "build-package", options.sdl.resolve(), root / "dist", options.label, options.game)
