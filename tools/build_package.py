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


def resources_of(executable):
    """Where the program finds data/ and MechWarrior.ini: beside it, or a Mac app's Contents/Resources."""
    if executable.parent.name == "MacOS" and executable.parent.parent.name == "Contents":
        return executable.parent.parent / "Resources"
    return executable.parent


def check_usage(executable):
    # --help, not a bare start: without a game folder an installed copy opens the setup.
    result = subprocess.run([str(executable), "--help"], cwd=executable.parent, capture_output=True, text=True, timeout=30)
    if result.returncode != 0 or f"Usage: {PROGRAM}" not in result.stdout:
        raise RuntimeError(f"Packaged executable failed to load: {result.returncode}\n{result.stdout}\n{result.stderr}")


def check_first_run(executable, game, timeout=FIRST_RUN_SECONDS):
    """Starts the package on `game` with an empty save directory: it must import
    the story (story/import.json) and reach its first input wait (status.json)."""
    home = resources_of(executable)
    for folder in DATA_FOLDERS:
        if not list((home / folder).glob("*.json")):
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


STEM = "MW1-Rebuild"
APP_NAME = "MechWarrior 1 Rebuild"
PROGRAM = "mw1r"  # the shipped program (the build's target stays mechwarrior, which the tools and traces run)
BUNDLE_ID = "io.github.mech1re.mechwarrior"  # its own, so it never replaces another game's settings
README = (
    "MechWarrior 1 Rebuild\n\n"
    "A rebuild of MechWarrior (1989)'s program. Your own copy of the game is required and is\n"
    "not included: no game data, saves or MT-32 ROMs are in this package.\n\n"
    "The first time it starts without your game, it asks for your copy of MechWarrior: its\n"
    "BTECH.EXE, any file in its folder, or the archive it came in (a .zip, for example). It\n"
    "copies the game into your own folder and plays from there; your copy is never changed.\n"
    "Your settings (MechWarrior.ini), saves, the copied game and the log (mechwarrior.log)\n"
    "live in your own folder:\n"
    "  Windows: %APPDATA%\\MicroProse\\MechWarrior 1 Rebuild\n"
    "  macOS:   ~/Library/Application Support/MicroProse/MechWarrior 1 Rebuild\n"
    "  Linux:   ~/.local/share/MicroProse/MechWarrior 1 Rebuild\n"
    "Saves from the earlier previews (BattleTechReverse/MechWarrior) are copied there once.\n\n"
    "Keys: arrows and Enter; in combat - and + change the speed (1x to 4x); F12 saves a\n"
    "screenshot beside your saves; Ctrl+Escape quits.\n\n"
    "From a command line: mw1r <game folder> [capture folder] starts on that folder.\n")


def platform_name():
    if os.name == "nt":
        return "win64"
    if sys.platform == "darwin":
        return "mac"
    return "linux-x86_64"


def program_version(label):
    """The version the program reports (--status): a release tag's X.Y.Z (mw1r-v1.0.0 -> 1.0.0), else the label."""
    found = re.fullmatch(r"mw1r-v(\d+\.\d+\.\d+)", label)
    return found.group(1) if found else label


def configure_and_build(root, build, sdl, label):
    args = ["cmake", "-S", str(root), "-B", str(build),
            "-DMECHWARRIOR_BUILD_SDL=ON", f"-DMECHWARRIOR_SDL_SOURCE_DIR={sdl}", "-DMECHWARRIOR_TRIM_PATHS=ON",
            f"-DMECHWARRIOR_VERSION={program_version(label)}"]
    if os.name == "nt":
        args += ["-A", "x64", "-DMECHWARRIOR_WINDOWS_GUI=ON"]
    elif sys.platform == "darwin":
        args += ["-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_OSX_DEPLOYMENT_TARGET=12.0"]
    else:
        args += ["-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_BUILD_RPATH=$ORIGIN",
                 "-DCMAKE_BUILD_RPATH_USE_ORIGIN=ON"]
    run(*args)
    run("cmake", "--build", str(build), "--config", "Release", "--target", "mechwarrior", "--parallel", "4")
    return build / "Release/mechwarrior.exe" if os.name == "nt" else build / "mechwarrior"


# libarchive, zlib and liblzma, linked in statically (tools/deps/build_archive.sh, tools/windows/build_deps_archive.cmd
# put their licences in <prefix>/share/licenses): <folder>/<file> -> the name in licenses/.
ARCHIVE_LICENCES = {"libarchive-3.7.9/COPYING": "libarchive-COPYING", "zlib-1.3.1/LICENSE": "zlib-LICENSE",
                    "xz-5.8.1/COPYING": "xz-COPYING", "xz-5.8.1/COPYING.0BSD": "xz-COPYING.0BSD"}


def archive_licences(build, notices):
    """The archive libraries' licences into `notices`. A build without the pinned static libarchive (none, or a
    system one) is refused: the setup could not read a download, and its licences would be missing."""
    cache = build / "CMakeCache.txt"
    lib = ""
    if cache.is_file():
        for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
            if line.startswith("REKIT_LIBARCHIVE_LIB:"):
                lib = line.split("=", 1)[1].strip()
    if not lib or lib.endswith("-NOTFOUND"):
        raise RuntimeError("Missing libarchive: build it first (tools/deps/build_archive.sh, "
                           "tools/windows/build_deps_archive.cmd) so the setup reads downloads")
    licences = Path(lib).parent.parent / "share/licenses"
    for source, name in ARCHIVE_LICENCES.items():
        if not (licences / source).is_file():
            raise RuntimeError(f"Missing licence: {licences / source} (libarchive {lib} is not the pinned static build)")
        shutil.copy2(licences / source, notices / name)


def stage_common(root, build, sdl, resources):
    """The configuration, our data, the read-me and the notices, into `resources`
    (beside the program, or a Mac app's Contents/Resources)."""
    for source in [root / "LICENSE", root / "MechWarrior.ini"]:
        shutil.copy2(source, resources / source.name)
    stage_data(root, resources)
    notices = resources / "licenses"
    notices.mkdir()
    shutil.copytree(root / "docs/LICENSES", notices / "project")
    for source in [root / "docs/NOTICE.md", root / "docs/THIRD_PARTY_NOTICES.md", sdl / "LICENSE.txt",
                   build / "_deps/munt-src/mt32emu/COPYING.LESSER.txt", build / "_deps/nukedopl-src/LICENSE"]:
        prefix = "SDL-" if source.parent == sdl else "Munt-" if source.name == "COPYING.LESSER.txt" else "Nuked-" if source.name == "LICENSE" else ""
        shutil.copy2(source, notices / (prefix + source.name))
    archive_licences(build, notices)
    # The setup kit's own third-party code (stb, miniz): a package without their licences is refused.
    kit = root / "extern/re-setup-kit"
    for name, source in [("re-setup-kit-LICENSE", kit / "LICENSE"), ("stb-LICENSE", kit / "third_party/stb/LICENSE"),
                         ("miniz-LICENSE", kit / "third_party/miniz/LICENSE")]:
        if not source.is_file():
            raise RuntimeError(f"Missing licence: {source}")
        shutil.copy2(source, notices / name)
    (resources / "README.txt").write_text(README, encoding="utf-8")


def check_paths(root, folder, *extra):
    """No local path or user name in anything shipped (tools/package/check_paths.py), nor the folders
    the build ran in (`extra`: the tree and SDL's source, which __FILE__ would name)."""
    run(sys.executable, "-I", str(root / "tools/package/check_paths.py"), str(folder),
        *[str(Path(p).resolve()) for p in extra])


def package(root, build, sdl, output, label, game=None, installer=False, tools=None):
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]*", label):
        raise ValueError("Invalid package label")
    if platform_name() == "mac":
        return package_mac(root, build, sdl, output, label, game)
    windows = platform_name() == "win64"
    name = f"{STEM}-{label}-{platform_name()}"
    stage = output / name
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    binary = configure_and_build(root, build, sdl, label)
    program = PROGRAM + (".exe" if windows else "")
    shutil.copy2(binary, stage / program)
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
    stage_common(root, build, sdl, stage)
    check_paths(root, stage, root, sdl)
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
    executable = extracted / name / program
    if not windows:
        run("ldd", str(executable))
    smoke(executable, game)
    write_digest(archive)
    print(f"Packaged and startup-checked {archive}")
    if installer:
        if windows:
            windows_installer(root, stage, output, label, sdl)
        else:
            appimage(root, stage, output, label, tools, game, sdl)
    return archive


def write_digest(file):
    digest = hashlib.sha256(file.read_bytes()).hexdigest()
    (file.parent / (file.name + ".sha256")).write_text(f"{digest}  {file.name}\n", encoding="utf-8")


def find_iscc():
    found = shutil.which("iscc")
    if found:
        return found
    for base in (os.environ.get("ProgramFiles(x86)"), os.environ.get("ProgramFiles"),
                 os.path.expandvars(r"%USERPROFILE%\scoop\apps\inno-setup\current")):
        if base:
            for candidate in (Path(base) / "Inno Setup 6/ISCC.exe", Path(base) / "ISCC.exe"):
                if candidate.is_file():
                    return str(candidate)
    raise RuntimeError("Inno Setup's ISCC.exe not found (scoop install inno-setup, or jrsoftware.org)")


def windows_installer(root, stage, output, label, sdl):
    """MW1-Rebuild-<label>-Setup.exe from the staged folder (tools/package/installer.iss)."""
    stem = f"{STEM}-{label}"
    run(find_iscc(), "/Qp", f"/DAppVersion={label}", f"/DDist={stage.resolve()}", f"/DStem={stem}",
        f"/O{output.resolve()}", str(root / "tools/package/installer.iss"))
    setup = output / f"{stem}-Setup.exe"
    if not setup.is_file():
        raise RuntimeError(f"Inno Setup made no {setup.name}")
    write_digest(setup)
    print(f"Installer {setup}")


def appimage(root, stage, output, label, tools, game, sdl):
    """MW1-Rebuild-<label>-x86_64.AppImage: usr/bin/mw1r with data/ and MechWarrior.ini beside it
    (SDL_GetBasePath), the notices in usr/share/doc, the libraries by linuxdeploy. Needs `tools` holding
    linuxdeploy-x86_64.AppImage and appimagetool-x86_64.AppImage."""
    if not tools:
        raise RuntimeError("--appimage-tools DIR (linuxdeploy and appimagetool) is needed for the AppImage")
    tools = Path(tools).resolve()
    appdir = output / "AppDir"
    if appdir.exists():
        shutil.rmtree(appdir)
    bin_dir, doc = appdir / "usr/bin", appdir / f"usr/share/doc/{PROGRAM}"
    bin_dir.mkdir(parents=True)
    doc.mkdir(parents=True)
    shutil.copy2(stage / PROGRAM, bin_dir / PROGRAM)
    shutil.copy2(stage / "MechWarrior.ini", bin_dir / "MechWarrior.ini")
    shutil.copytree(stage / "data", bin_dir / "data")
    for name in ("LICENSE", "README.txt"):
        shutil.copy2(stage / name, doc / name)
    shutil.copytree(stage / "licenses", doc / "licenses")
    env = dict(os.environ, APPIMAGE_EXTRACT_AND_RUN="1", ARCH="x86_64",
               LD_LIBRARY_PATH=str(stage.resolve()) + os.pathsep + os.environ.get("LD_LIBRARY_PATH", ""))
    run(str(tools / "linuxdeploy-x86_64.AppImage"), "--appdir", str(appdir), "-e", str(bin_dir / PROGRAM),
        "-d", str(root / "tools/package/mechwarrior.desktop"), "-i", str(root / "tools/package/mechwarrior.png"), env=env)
    smoke(bin_dir / PROGRAM, game)
    check_paths(root, appdir, root, sdl)
    image = output / f"{STEM}-{label}-x86_64.AppImage"
    run(str(tools / "appimagetool-x86_64.AppImage"), str(appdir), str(image), env=env)
    result = subprocess.run([str(image.resolve()), "--help"], capture_output=True, text=True, timeout=60, env=env)
    if result.returncode != 0 or f"Usage: {PROGRAM}" not in result.stdout:
        raise RuntimeError(f"The AppImage did not start: {result.returncode}\n{result.stderr}")
    write_digest(image)
    print(f"AppImage {image}")


def package_mac(root, build, sdl, output, label, game):
    """MechWarrior 1 Rebuild.app and MW1-Rebuild-<label>-mac.dmg: the program in Contents/MacOS, data/,
    MechWarrior.ini and the notices in Contents/Resources (SDL_GetBasePath), the libraries in
    Contents/Frameworks named @rpath, every binary signed ad hoc. This Mac's architecture."""
    binary = configure_and_build(root, build, sdl, label)
    app = output / f"{APP_NAME}.app"
    if app.exists():
        shutil.rmtree(app)
    macos, frameworks, resources = (app / "Contents" / d for d in ("MacOS", "Frameworks", "Resources"))
    for d in (macos, frameworks, resources):
        d.mkdir(parents=True)
    exe = macos / PROGRAM
    shutil.copy2(binary, exe)

    def dependencies(file):
        lines = subprocess.run(["otool", "-L", str(file)], capture_output=True, text=True, check=True).stdout.splitlines()
        return sorted({l.strip().split(" (")[0] for l in lines[1:] if l.startswith(("\t", " "))})

    def system(path):
        return path.startswith(("/System/", "/usr/lib/"))

    copied, todo = {}, [exe]
    while todo:
        file = todo.pop()
        for dep in dependencies(file):
            leaf = Path(dep).name
            if system(dep) or dep in copied or dep.startswith(("@executable_path", "@loader_path")) or leaf == file.name:
                continue  # (a library lists its own install name too)
            if dep.startswith("@rpath/"):
                found = [p for p in build.rglob(leaf) if p.is_file() or p.is_symlink()]
                if not found:
                    raise RuntimeError(f"{dep}: not in {build}")
                source = found[0].resolve()
            else:
                raise RuntimeError(f"{file.name} loads {dep} from outside the build: build its own copy")
            target = frameworks / leaf
            if not target.exists():
                shutil.copy2(source, target)
                target.chmod(0o644)
                run("install_name_tool", "-id", f"@rpath/{leaf}", str(target), stderr=subprocess.DEVNULL)
                todo.append(target)
            copied[dep] = leaf
    rpaths = subprocess.run(["otool", "-l", str(exe)], capture_output=True, text=True, check=True).stdout.splitlines()
    for i, line in enumerate(rpaths):
        if "cmd LC_RPATH" in line:
            run("install_name_tool", "-delete_rpath", rpaths[i + 2].strip().split(" ")[1], str(exe))
    run("install_name_tool", "-add_rpath", "@executable_path/../Frameworks", str(exe))
    left = [d for f in [exe, *frameworks.iterdir()] for d in dependencies(f)
            if not (system(d) or d.startswith("@"))]
    if left:
        raise RuntimeError("still outside the app: " + ", ".join(sorted(set(left))))
    for required in ("libSDL3*", "libmt32emu*", "libnukedopl*"):
        if not list(frameworks.glob(required)):
            raise RuntimeError(f"Missing runtime dependency: {required}")

    stage_common(root, build, sdl, resources)
    iconset = output / "mechwarrior.iconset"
    if iconset.exists():
        shutil.rmtree(iconset)
    iconset.mkdir()
    png = root / "tools/package/mechwarrior.png"
    for n in (16, 32, 128, 256, 512):
        for scale, suffix in ((1, ""), (2, "@2x")):
            run("sips", "-z", str(n * scale), str(n * scale), str(png), "--out",
                str(iconset / f"icon_{n}x{n}{suffix}.png"), stdout=subprocess.DEVNULL)
    run("iconutil", "-c", "icns", str(iconset), "-o", str(resources / "mechwarrior.icns"))
    shutil.rmtree(iconset)
    minos = re.search(r"minos (\S+)", subprocess.run(["otool", "-l", str(exe)], capture_output=True, text=True,
                                                      check=True).stdout)
    plist = {"CFBundleName": APP_NAME, "CFBundleDisplayName": APP_NAME, "CFBundleIdentifier": BUNDLE_ID,
             "CFBundleExecutable": PROGRAM, "CFBundleIconFile": "mechwarrior", "CFBundlePackageType": "APPL",
             "CFBundleShortVersionString": label, "CFBundleVersion": label, "CFBundleDevelopmentRegion": "en",
             "CFBundleInfoDictionaryVersion": "6.0", "LSMinimumSystemVersion": minos.group(1) if minos else "12.0",
             "LSApplicationCategoryType": "public.app-category.games", "NSHighResolutionCapable": True,
             "NSHumanReadableCopyright": "A fan rebuild; MechWarrior is a trademark of its owners. "
                                         "Needs your own copy of MechWarrior (1989)."}
    import plistlib
    with open(app / "Contents/Info.plist", "wb") as f:
        plistlib.dump(plist, f)
    # Ad hoc (no identity, so no name in the signature): the libraries, the program, then the app.
    entitlements = str(root / "tools/package/mechwarrior.entitlements")
    for lib in sorted(frameworks.iterdir()):
        run("codesign", "--force", "--sign", "-", str(lib))
    run("codesign", "--force", "--sign", "-", "--entitlements", entitlements, str(exe))
    run("codesign", "--force", "--sign", "-", "--entitlements", entitlements, str(app))
    run("codesign", "--verify", "--deep", "--strict", str(app))
    smoke(exe, game)

    dmg = output / f"{STEM}-{label}-mac.dmg"
    dmg_stage = output / "dmg"
    for old in (dmg_stage, dmg):
        if old.is_dir():
            shutil.rmtree(old)
        elif old.exists():
            old.unlink()
    dmg_stage.mkdir()
    run("ditto", str(app), str(dmg_stage / app.name))
    (dmg_stage / "Applications").symlink_to("/Applications")
    shutil.copy2(resources / "README.txt", dmg_stage / "Read me.txt")
    check_paths(root, dmg_stage, root, sdl)
    run("hdiutil", "create", "-volname", APP_NAME, "-srcfolder", str(dmg_stage), "-ov", "-format", "UDZO", str(dmg),
        stdout=subprocess.DEVNULL)
    shutil.rmtree(dmg_stage)
    write_digest(dmg)
    print(f"App: {app}\nDMG: {dmg}")
    return dmg


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--sdl", type=Path)
    parser.add_argument("--label")
    parser.add_argument("--smoke", type=Path, help="an extracted package directory: run the smoke test only")
    parser.add_argument("--installer", action="store_true",
                        help="also the installer: Setup.exe (Windows, Inno Setup) or AppImage (Linux); a Mac always makes its .dmg")
    parser.add_argument("--appimage-tools", type=Path, help="Linux: the folder with linuxdeploy and appimagetool")
    parser.add_argument("--game", type=Path, default=os.environ.get("MW1O_GAME"),
                        help="an original game folder for the first-run check (default MW1O_GAME; never packaged)")
    options = parser.parse_args()
    if options.smoke:
        names = [PROGRAM + ".exe", PROGRAM, "Contents/MacOS/" + PROGRAM]
        executable = next((options.smoke / n for n in names if (options.smoke / n).is_file()), None)
        if not executable:
            parser.error(f"no {PROGRAM} program in {options.smoke}")
        smoke(executable.resolve(), options.game)
    else:
        if not options.sdl or not options.label:
            parser.error("--sdl and --label are required to package")
        root = Path(__file__).resolve().parents[1]
        package(root, root / "build-package", options.sdl.resolve(), root / "dist", options.label, options.game,
                options.installer, options.appimage_tools)
