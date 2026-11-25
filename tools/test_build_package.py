"""Exercise archive extraction/startup without downloading build dependencies."""
import hashlib
import os
from pathlib import Path
import plistlib
import re
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import DEFAULT, patch
import build_package
from build_package import background_app


@unittest.skipIf(os.name == "nt", "Native Linux archive layout test")
class PackageTests(unittest.TestCase):
    def fixture(self, root):
        for name in ["LICENSE", "MechWarrior.ini", "docs/LICENSES/MIT.txt", "docs/NOTICE.md",
                     "docs/THIRD_PARTY_NOTICES.md", "sdl/LICENSE.txt", "sdl/src/hidapi/LICENSE-bsd.txt",
                     "data/rules/units.json", "data/ui/hub.json", "data/music/soundfont_programs.txt", "extern/re-setup-kit/LICENSE",
                     "extern/re-setup-kit/third_party/stb/LICENSE", "extern/re-setup-kit/third_party/miniz/LICENSE",
                     "extern/TinySoundFont/LICENSE"]:
            file = root / name
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_text("fixture")
        prefix = root / "deps/prefix"
        for name in ["libarchive-3.7.9/COPYING", "zlib-1.3.1/LICENSE", "xz-5.8.1/COPYING", "xz-5.8.1/COPYING.0BSD"]:
            (prefix / "share/licenses" / name).parent.mkdir(parents=True, exist_ok=True)
            (prefix / "share/licenses" / name).write_text("fixture licence")
        music = root / "data/music"
        for name in build_package.SOUNDFONT_LICENCES:
            (music / name).write_text("fixture licence")
        (root / "build/soundfont").mkdir(parents=True)
        (root / "build/soundfont" / build_package.SOUNDFONT).write_bytes(b"fixture bank")
        (music / "soundfont_subset.sha256").write_text(hashlib.sha256(b"fixture bank").hexdigest() + "  x\n")
        (root / "build/CMakeCache.txt").write_text(f"REKIT_LIBARCHIVE_LIB:FILEPATH={prefix / 'lib/libarchive.a'}\n")
        exe = root / "build/mechwarrior"
        exe.write_text('#!/bin/sh\n[ "$1" = --help ] && { echo "Usage: mw1r [<game folder>]"; exit 0; }\nexit 2\n')
        exe.chmod(0o755)
        # SDL's build has the library with its links; the build copies the library itself beside the program.
        (root / "build/sdl3").mkdir()
        (root / "build/sdl3/libSDL3.so.0.2.0").write_bytes(b"fixture library")
        (root / "build/sdl3/libSDL3.so.0").symlink_to("libSDL3.so.0.2.0")
        (root / "build/sdl3/libSDL3.so").symlink_to("libSDL3.so.0")
        (root / "build/libSDL3.so.0.2.0").write_bytes(b"fixture library")

    def test_extracted_archive_starts_and_libraries_are_portable(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            # The fixture's program answers nothing but --help: what it is asked is checked, on the extracted one.
            with patch.object(build_package, "run"), patch.object(build_package, "platform_name", return_value="linux-x86_64"), \
                    patch.object(build_package, "check_served_setup") as served:
                build_package.package(root, root / "build", root / "sdl", root / "dist", "v0.1.0-test")
            extracted = root / "dist/smoke/MW1-Rebuild-v0.1.0-test-linux-x86_64"
            served.assert_called_once()
            self.assertTrue(os.path.samefile(served.call_args[0][0], extracted / "mw1r"))
            self.assertEqual([f.name for f in extracted.glob("libSDL3*")], ["libSDL3.so.0"])
            self.assertFalse((extracted / "libSDL3.so.0").is_symlink())
            self.assertEqual((extracted / "libSDL3.so.0").read_bytes(), b"fixture library")
            self.assertTrue(os.access(extracted / "mw1r", os.X_OK) and not (extracted / "mechwarrior").exists())
            self.assertEqual(subprocess.run([str(extracted / "mw1r"), "--help"], capture_output=True).returncode, 0)
            self.assertFalse([f for f in extracted.rglob("*") if any(n in f.name.lower() for n in build_package.REMOVED_LIBRARIES)])
            self.assertTrue((extracted / "licenses/stb-LICENSE").exists() and (extracted / "licenses/miniz-LICENSE").exists())
            self.assertTrue((extracted / "licenses/TinySoundFont-LICENSE").exists())
            self.assertTrue((extracted / "licenses/hidapi-LICENSE-bsd.txt").exists())
            for name in ["libarchive-COPYING", "zlib-LICENSE", "xz-COPYING", "xz-COPYING.0BSD"]:
                self.assertTrue((extracted / "licenses" / name).exists(), name)
            self.assertTrue((extracted / "data/rules/units.json").exists())
            self.assertTrue((extracted / "data/ui/hub.json").exists())
            self.assertEqual((extracted / "data/music" / build_package.SOUNDFONT).read_bytes(), b"fixture bank")
            for name in build_package.SOUNDFONT_LICENCES:
                self.assertTrue((extracted / "licenses" / name).exists(), name)
            self.assertEqual(len(list((root / "dist").glob("*.sha256"))), 1)

    def test_a_game_file_blocks_packaging(self):
        # A build's leftover in data/, which the package takes whole: the check itself runs, the build doesn't.
        check = lambda *args, **kwargs: subprocess.run(args, check=True, capture_output=True) if any("check_paths.py" in a for a in args) else None
        real = Path(build_package.__file__).resolve().parent / "package/check_paths.py"
        for name in ("", "data/rules/SNARIO.DAT", "data/mw_main.exe", "data/ui/text.json"):
            with tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                self.fixture(root)
                (root / "tools/package").mkdir(parents=True)
                (root / "tools/package/check_paths.py").write_bytes(real.read_bytes())
                if name:
                    (root / name).write_bytes(b"x")
                package = lambda: build_package.package(root, root / "build", root / "sdl", root / "dist", "test")
                with patch.object(build_package, "run", check), patch.object(build_package, "platform_name", return_value="linux-x86_64"), \
                        patch.object(build_package, "check_served_setup"):
                    if name:
                        with self.assertRaises(subprocess.CalledProcessError) as refused:
                            package()
                        self.assertIn(Path(name).name.encode() + b": a file of the original game", refused.exception.stdout)
                    else:
                        package()
                self.assertEqual(len(list((root / "dist").glob("*.tar.gz"))), 0 if name else 1)

    def test_missing_or_removed_library_blocks_packaging(self):
        for broken in ("missing", "libmt32emu.so.2", "_deps/libnukedopl.so"):
            with tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                self.fixture(root)
                if broken == "missing":
                    for lib in (root / "build").rglob("libSDL3.so*"):
                        lib.unlink()
                else:
                    (root / "build" / broken).parent.mkdir(exist_ok=True)
                    (root / "build" / broken).write_bytes(b"an old build's library")
                with patch.object(build_package, "run"), patch.object(build_package, "platform_name", return_value="linux-x86_64"), self.assertRaisesRegex(RuntimeError, "Missing runtime dependency" if broken == "missing" else "Removed libraries"):
                    build_package.package(root, root / "build", root / "sdl", root / "dist", "test")
                self.assertFalse(list((root / "dist").glob("*.tar.gz")))

    def test_windows_imports_are_in_the_package_or_windows(self):
        with tempfile.TemporaryDirectory() as directory:
            folder, system = Path(directory) / "dist", Path(directory) / "System32"
            folder.mkdir(), system.mkdir()
            for name in ("mw1r.exe", "SDL3.dll", "msvcp140.dll", "vcruntime140.dll", "README.txt"):
                (folder / name).write_bytes(b"")
            for name in ("kernel32.dll", "vcruntime140_1.dll", "concrt140.dll"):  # a machine with the runtime installed
                (system / name).write_bytes(b"")
            imports = {"mw1r.exe": ["SDL3.dll", "KERNEL32.dll", "MSVCP140.dll", "api-ms-win-crt-heap-l1-1-0.dll"],
                       "SDL3.dll": ["VCRUNTIME140.dll"], "msvcp140.dll": ["VCRUNTIME140.dll"], "vcruntime140.dll": []}
            check = lambda: build_package.check_windows_imports(folder, lambda file: imports[file.name], system)
            check()
            for file, dll in (("mw1r.exe", "VCRUNTIME140_1.dll"), ("msvcp140.dll", "concrt140.dll"), ("SDL3.dll", "other.dll")):
                imports[file].append(dll)
                with self.assertRaisesRegex(RuntimeError, f"{file} needs {dll.lower()}"):
                    check()
                imports[file].pop()

    def test_mac_binaries_are_universal(self):
        exe, lib = Path("mw1r"), Path("libSDL3.0.dylib")
        both = {exe: ["arm64", "x86_64"], lib: ["x86_64", "arm64"]}
        build_package.check_architectures([exe, lib], both.get)
        for one in (["arm64"], ["x86_64"], []):
            with self.assertRaisesRegex(RuntimeError, r"Not universal: libSDL3.0.dylib"):
                build_package.check_architectures([exe, lib], {**both, lib: one}.get)

    def test_version_stamp(self):
        stamp = b"MW3-PROGRAM-" + b"VERSION:"
        self.assertEqual(build_package.release_tag("mw1r-v1.2.3"), "mw1r-v1.2.3")
        for label in ("v1.2.3", "mw1r-v1.2.3-test", "mw1r-v1.2", "mw1-v1.2.3", "1.2.3"):
            self.assertEqual(build_package.release_tag(label), "")
        for label, held, good in (("mw1r-v1.2.3", b"mw1r-v1.2.3", True), ("mw1r-v1.2.3", b"mw1r-v1.2.4", False), ("mw1r-v1.2.3", None, False),
                                  ("mw1r-v1.2.3-test", None, True), ("mw1r-v1.2.3-test", b"mw1r-v1.2.3", False), ("test", b"", False)):
            with tempfile.TemporaryDirectory() as directory:
                program = Path(directory) / "program"
                program.write_bytes(b"code" + (b"" if held is None else stamp + held + b"\0") + b"more")
                if good:
                    build_package.check_stamp(program, label)
                else:
                    with self.assertRaisesRegex(RuntimeError, "version stamp"):
                        build_package.check_stamp(program, label)

    def test_wrong_stamp_blocks_packaging(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            with patch.object(build_package, "run"), patch.object(build_package, "check_release_tree"), patch.object(build_package, "platform_name", return_value="linux-x86_64"), self.assertRaisesRegex(RuntimeError, "version stamp"):
                build_package.package(root, root / "build", root / "sdl", root / "dist", "mw1r-v1.2.3")
            self.assertFalse(list((root / "dist").glob("*.tar.gz")))

    def test_a_release_is_packaged_only_from_its_tag(self):
        def git(tree, *args):
            subprocess.run(["git", "-C", str(tree), "-c", "user.name=t", "-c", "user.email=t@example.invalid", "-c", "commit.gpgsign=false",
                            "-c", "tag.gpgsign=false", *args], check=True, capture_output=True)

        tag = "mw1r-v1.2.3"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "tree"
            root.mkdir()
            refused = lambda why: self.assertRaisesRegex(RuntimeError, why)
            with refused("git tree"):
                build_package.check_release_tree(root, tag)  # no git tree at all
            git(root, "init", "-q")
            (root / ".gitignore").write_text("/dist/\n")
            (root / "a.txt").write_text("one")
            git(root, "add", "-A")
            git(root, "commit", "-qm", "one")
            with refused("git tree"):
                build_package.check_release_tree(root, tag)  # no such tag
            git(root, "tag", tag)
            build_package.check_release_tree(root, tag)
            (root / "dist").mkdir()
            (root / "dist/x").write_text("ignored")
            build_package.check_release_tree(root, tag)
            with refused("not the top"):
                build_package.check_release_tree(root / "dist", tag)
            (root / "a.txt").write_text("two")
            with refused("with changes"):
                build_package.check_release_tree(root, tag)
            build_package.check_release_tree(root, tag + "-test")  # any other label takes the tree as it is
            git(root, "checkout", "-q", "a.txt")
            (root / "new.txt").write_text("untracked")
            with refused("with changes"):
                build_package.check_release_tree(root, tag)
            git(root, "add", "-A")
            with refused("with changes"):
                build_package.check_release_tree(root, tag)
            git(root, "commit", "-qm", "two")
            with refused("but the tree is at"):
                build_package.check_release_tree(root, tag)
            git(root, "tag", "-a", "mw1r-v1.2.4", "-m", "annotated")
            build_package.check_release_tree(root, "mw1r-v1.2.4")

    def test_a_release_label_is_checked_before_the_build(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            for platform in ("linux-x86_64", "mac", "win64"):
                with patch.object(build_package, "configure_and_build") as built, patch.object(build_package, "platform_name", return_value=platform), self.assertRaisesRegex(RuntimeError, "git tree"):
                    build_package.package(root, root / "build", root / "sdl", root / "dist", "mw1r-v1.2.3")
                built.assert_not_called()

    def test_a_notice_missing_from_the_package_is_refused(self):
        with tempfile.TemporaryDirectory() as directory:
            licenses = Path(directory)
            names = [*build_package.NOTICES, *build_package.ARCHIVE_LICENCES.values(), *build_package.SOUNDFONT_LICENCES]
            for name in names:
                (licenses / name).write_text("text")
            build_package.check_notices(licenses)
            for name in names:
                (licenses / name).write_text("")
                with self.assertRaisesRegex(RuntimeError, name.replace(".", r"\.")):
                    build_package.check_notices(licenses)
                (licenses / name).unlink()
                with self.assertRaisesRegex(RuntimeError, name.replace(".", r"\.")):
                    build_package.check_notices(licenses)
                (licenses / name).write_text("text")

    def test_missing_kit_or_hidapi_licence_blocks_packaging(self):
        for licence in ("extern/re-setup-kit/third_party/stb/LICENSE", "sdl/src/hidapi/LICENSE-bsd.txt"):
            with tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                self.fixture(root)
                (root / licence).unlink()
                with patch.object(build_package, "run"), patch.object(build_package, "platform_name", return_value="linux-x86_64"), self.assertRaisesRegex(RuntimeError, "Missing licence"):
                    build_package.package(root, root / "build", root / "sdl", root / "dist", "test")
                self.assertFalse(list((root / "dist").glob("*.tar.gz")))

    def test_missing_archive_licence_or_library_blocks_packaging(self):
        for broken in ("licence", "library"):
            with tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                self.fixture(root)
                if broken == "licence":
                    (root / "deps/prefix/share/licenses/xz-5.8.1/COPYING.0BSD").unlink()
                else:
                    (root / "build/CMakeCache.txt").write_text("REKIT_LIBARCHIVE_LIB:FILEPATH=REKIT_LIBARCHIVE_LIB-NOTFOUND\n")
                with patch.object(build_package, "run"), patch.object(build_package, "platform_name", return_value="linux-x86_64"), self.assertRaisesRegex(RuntimeError, "Missing (licence|libarchive)"):
                    build_package.package(root, root / "build", root / "sdl", root / "dist", "test")
                self.assertFalse(list((root / "dist").glob("*.tar.gz")))

    def test_missing_or_wrong_soundfont_blocks_packaging(self):
        for broken in ("missing", "wrong", "licence"):
            with tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                self.fixture(root)
                bank = root / "build/soundfont" / build_package.SOUNDFONT
                if broken == "missing":
                    bank.unlink()
                elif broken == "wrong":
                    bank.write_bytes(b"another bank")
                else:
                    (root / "data/music" / build_package.SOUNDFONT_LICENCES[0]).unlink()
                with patch.object(build_package, "run"), patch.object(build_package, "platform_name", return_value="linux-x86_64"), self.assertRaisesRegex(RuntimeError, "Missing soundfont|Soundfont .* hashes|Missing licence"):
                    build_package.package(root, root / "build", root / "sdl", root / "dist", "test")
                self.assertFalse(list((root / "dist").glob("*.tar.gz")))

    def test_appimage_tools_are_pinned(self):
        with tempfile.TemporaryDirectory() as directory:
            tools = Path(directory)
            for name in build_package.appimage_tools():
                (tools / name).write_bytes(name.encode())
            pins = {name: hashlib.sha256(name.encode()).hexdigest() for name in build_package.appimage_tools()}
            with patch.object(build_package, "appimage_tools", return_value=pins):
                build_package.check_appimage_tools(tools)
                (tools / "runtime-x86_64").write_bytes(b"another runtime")
                found = hashlib.sha256(b"another runtime").hexdigest()
                with self.assertRaisesRegex(RuntimeError, f"runtime-x86_64 hashes {found}, wanted {pins['runtime-x86_64']}"):
                    build_package.check_appimage_tools(tools)
                (tools / "runtime-x86_64").unlink()
                with self.assertRaisesRegex(RuntimeError, "Missing AppImage tool: .*runtime-x86_64"):
                    build_package.check_appimage_tools(tools)

    def test_program_version_from_the_label(self):
        self.assertEqual(build_package.program_version("mw1r-v1.0.0"), "1.0.0")
        self.assertEqual(build_package.program_version("1.0.0-test.436522e"), "1.0.0-test.436522e")
        self.assertEqual(build_package.program_version("mw1r-v1.0"), "mw1r-v1.0")  # not a release tag: as given

    def test_missing_data_blocks_packaging(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            (root / "data/ui/hub.json").unlink()
            with patch.object(build_package, "run"), patch.object(build_package, "platform_name", return_value="linux-x86_64"), self.assertRaisesRegex(RuntimeError, "Missing game data: data/ui"):
                build_package.package(root, root / "build", root / "sdl", root / "dist", "test")
            self.assertFalse(list((root / "dist").glob("*.tar.gz")))

    def first_run_fixture(self, root, imports):
        """A stand-in program: it imports (or not), then reaches its input wait and waits."""
        for folder in ("data/rules", "data/ui"):
            (root / folder).mkdir(parents=True)
            (root / folder / "x.json").write_text("{}")
        story = 'mkdir -p "$MECHWARRIOR_SAVE_DIRECTORY/story" && echo "{}" > "$MECHWARRIOR_SAVE_DIRECTORY/story/import.json"\n'
        exe = root / "mechwarrior"
        exe.write_text("#!/bin/sh\n" + (story if imports else "")
                       + 'echo "{}" > "$MECHWARRIOR_DEBUG_CONTROL_DIRECTORY/status.json"\nexec sleep 30\n')
        exe.chmod(0o755)
        (root / "game").mkdir()
        return exe

    def test_first_run_imports_and_reaches_the_game(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            exe = self.first_run_fixture(root, imports=True)
            build_package.check_first_run(exe, root / "game", timeout=20)

    def test_first_run_without_the_import_fails(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            exe = self.first_run_fixture(root, imports=False)
            with self.assertRaisesRegex(RuntimeError, "did not import the story"):
                build_package.check_first_run(exe, root / "game", timeout=2)


class VersionTests(unittest.TestCase):
    """The version a player is shown: a release's number, and any other label as it is."""
    labels = (("mw1r-v1.0.0", "1.0.0"), ("mw1r-v12.3.45", "12.3.45"), ("1.0.0-test.436522e", "1.0.0-test.436522e"), ("mw1r-v1.0.0-test", "mw1r-v1.0.0-test"))

    def test_mac_app(self):
        for label, shown in self.labels:
            plist = plistlib.loads(plistlib.dumps(build_package.mac_plist(label, "12.0")))
            self.assertEqual((plist["CFBundleShortVersionString"], plist["CFBundleVersion"], plist["LSMinimumSystemVersion"]), (shown, shown, "12.0"))
            self.assertNotIn("LSBackgroundOnly", plist)  # the shipped app is one the player sees

    def test_windows_setup(self):
        for label, shown in self.labels:
            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory)
                setup = output / f"MW1-Rebuild-{label}-Setup.exe"  # the file keeps the label: the launcher's name for it
                with patch.object(build_package, "find_iscc", return_value="iscc"), \
                        patch.object(build_package.subprocess, "run", return_value=subprocess.CompletedProcess([], 0, "Inno Setup", "")), \
                        patch.object(build_package, "run", side_effect=lambda *args: setup.write_bytes(b"setup")) as iscc:
                    build_package.windows_installer(output, output / "stage", output, label, output)
                given = iscc.call_args[0]
                self.assertIn(f"/DAppVersion={shown}", given)
                self.assertIn(f"/DStem=MW1-Rebuild-{label}", given)
                self.assertTrue(Path(str(setup) + ".sha256").is_file())


class BackgroundAppTests(unittest.TestCase):
    """A check's runs of a Mac app are its background copy's."""

    def test_a_program_outside_an_app_runs_itself(self):
        with tempfile.TemporaryDirectory() as directory:
            for program in (Path(directory) / "mw1r", Path(directory) / "Contents/MacOS/mw1r"):  # the second: no Info.plist
                program.parent.mkdir(parents=True, exist_ok=True)
                program.write_bytes(b"")
                with background_app.runnable(program) as runs:
                    self.assertEqual(runs, program)

    def app(self, folder):
        app = Path(folder) / "A game.app"
        for name in ("MacOS/mw1r", "Resources/data/rules/x.json"):
            (app / "Contents" / name).parent.mkdir(parents=True, exist_ok=True)
            (app / "Contents" / name).write_text("#!/bin/sh\nexit 0\n" if name == "MacOS/mw1r" else "{}")
        (app / "Contents/MacOS/mw1r").chmod(0o755)
        (app / "Contents/Info.plist").write_bytes(plistlib.dumps(build_package.mac_plist("mw1r-v1.2.3", "12.0")))
        subprocess.run(["codesign", "--force", "--sign", "-", str(app)], check=True, capture_output=True)
        return app

    def files(self, app):
        return {str(f.relative_to(app)): f.read_bytes() for f in Path(app).rglob("*") if f.is_file()}

    @unittest.skipIf(sys.platform != "darwin", "A Mac's apps and signatures")
    def test_the_smoke_runs_the_copy_and_leaves_the_app(self):
        with tempfile.TemporaryDirectory() as directory:
            app = self.app(directory)
            before, ran = self.files(app), []

            def seen(program, *args):
                copy = program.parents[2]
                ran.append(copy)
                self.assertEqual(program.name, "mw1r")
                self.assertFalse(os.path.samefile(copy, app))
                subprocess.run(["codesign", "--verify", "--deep", "--strict", str(copy)], check=True)
                now = self.files(copy)
                self.assertEqual(plistlib.loads(now["Contents/Info.plist"]), {**plistlib.loads(before["Contents/Info.plist"]), "LSBackgroundOnly": True})
                self.assertEqual({k: v for k, v in now.items() if k in before and before[k] != v}.keys() - {"Contents/Info.plist"},
                                 {k for k in before if "_CodeSignature" in k and before[k] != now.get(k)})
                self.assertEqual(now.keys(), before.keys())

            with patch.multiple(build_package, check_usage=DEFAULT, check_served_setup=DEFAULT, check_first_run=DEFAULT) as checks:
                for check in checks.values():
                    check.side_effect = seen
                build_package.smoke(app / "Contents/MacOS/mw1r", Path(directory))
                for check in checks.values():
                    check.assert_called_once()
            self.assertEqual(len(set(ran)), 1)
            self.assertFalse(ran[0].exists())
            self.assertEqual(self.files(app), before)

    @unittest.skipIf(sys.platform != "darwin", "A Mac's apps and signatures")
    def test_an_app_with_a_broken_signature_is_not_run(self):
        with tempfile.TemporaryDirectory() as directory:
            app = self.app(directory)
            (app / "Contents/Resources/data/rules/x.json").write_text("changed")
            with patch.multiple(build_package, check_usage=DEFAULT, check_served_setup=DEFAULT, check_first_run=DEFAULT) as checks:
                with self.assertRaisesRegex(RuntimeError, "codesign"):
                    build_package.smoke(app / "Contents/MacOS/mw1r", Path(directory))
                for check in checks.values():
                    check.assert_not_called()

    @unittest.skipIf(sys.platform != "darwin", "A Mac's apps and signatures")
    def test_by_hand(self):
        with tempfile.TemporaryDirectory() as directory:
            app, out = self.app(directory), Path(directory) / "out"
            out.mkdir()
            tool = [sys.executable, "-I", background_app.__file__, str(app), str(out)]
            found = subprocess.run(tool, capture_output=True, text=True)
            self.assertEqual(found.returncode, 0, found.stderr)
            self.assertTrue(os.path.samefile(found.stdout.strip(), out / app.name / "Contents/MacOS/mw1r"))
            self.assertNotEqual(subprocess.run(tool, capture_output=True).returncode, 0)  # a copy is there already
            self.assertTrue(plistlib.loads((out / app.name / "Contents/Info.plist").read_bytes())["LSBackgroundOnly"])


class FolderToolTests(unittest.TestCase):
    """check_paths.py and sums.py, run on a folder of their own."""
    tools = Path(build_package.__file__).resolve().parent / "package"

    def tool(self, name, folder):
        return subprocess.run([sys.executable, "-I", str(self.tools / name), str(folder)], capture_output=True, text=True)

    def test_check_paths_refuses_every_game_file_by_name(self):
        table = (self.tools.parent.parent / "src/Setup/GameFilesTable.cpp").read_text(encoding="utf-8")
        played = sorted(set(re.findall(r'^\s*\{"([A-Z0-9_]+\.[A-Z]+)"', table, re.MULTILINE)))
        self.assertGreater(len(played), 100)
        others = ["MW.EXE", "MW_MAIN.EXE", "MW_TANDY.EXE", "MW_CPICS.EXE", "BTECH.EXE", "BIG.GAM", "DATA.DEF",
                  "import.json", "text.json", "words.json", "event_text.json", "missions.json", "native.json", "pilots.json", "planets.json"]
        ours = ["mw1r", "mw1r.exe", "SDL3.dll", "libSDL3.so.0", "events.json", "units.json", "FluidR3_GM-subset.sf2", "MechWarrior.ini",
                "SairaCondensed-Bold.ttf", "badge.png", "mechwarrior.icns", "README.txt", "soundfont_subset.sha256", "CodeResources"]
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory) / "package" / "data" / "deeper"
            folder.mkdir(parents=True)
            for name in ours:
                (folder / name).write_bytes(b"ours")
            found = self.tool("check_paths.py", folder.parent.parent)
            self.assertEqual(found.returncode, 0, found.stdout + found.stderr)
            for names in (played + others, [n.lower() for n in played + others]):
                for name in names:
                    (folder / name).write_bytes(b"")
                found = self.tool("check_paths.py", folder.parent.parent)
                self.assertEqual(found.returncode, 1)
                refused = re.findall(r"([^/\\]+): a file of the original game", found.stdout)
                self.assertEqual(sorted(refused), sorted(names))
                for name in names:
                    (folder / name).unlink()
            try:
                (folder / "BTECH.EXE").symlink_to("units.json")
            except OSError:
                return  # Windows without the right to make links
            self.assertEqual(self.tool("check_paths.py", folder.parent.parent).returncode, 1)  # a link by that name too

    def test_the_digest_ends_its_line_with_a_newline_alone(self):
        # Text is written as on Windows here, so a digest written as text is caught on every system.
        real = Path.write_text
        windows = lambda path, data, encoding=None, errors=None, newline=None: real(path, data, encoding, errors, newline or "\r\n")
        with tempfile.TemporaryDirectory() as directory, patch.object(Path, "write_text", windows):
            file = Path(directory) / "MW1-Rebuild-test-Setup.exe"
            file.write_bytes(b"setup")
            build_package.write_digest(file)
            written = (Path(directory) / (file.name + ".sha256")).read_bytes()
            self.assertNotIn(b"\r", written)
            self.assertEqual(written, hashlib.sha256(b"setup").hexdigest().encode() + b"  " + file.name.encode() + b"\n")

    def test_sums_leaves_out_itself_and_the_single_sums(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            for name in ("b.zip", "a.dmg"):
                (folder / name).write_bytes(name.encode())
                (folder / (name + ".sha256")).write_text("x")
            for _ in range(2):  # a second run finds the SHA256SUMS of the first
                found = self.tool("sums.py", folder)
                self.assertEqual(found.returncode, 0, found.stdout + found.stderr)
                self.assertEqual((folder / "SHA256SUMS").read_bytes(), b"".join(
                    hashlib.sha256(n.encode()).hexdigest().encode() + b"  " + n.encode() + b"\n" for n in ("a.dmg", "b.zip")))
        with tempfile.TemporaryDirectory() as directory:
            (Path(directory) / "only.sha256").write_text("x")
            found = self.tool("sums.py", directory)
            self.assertNotEqual(found.returncode, 0)
            self.assertFalse((Path(directory) / "SHA256SUMS").exists())


if __name__ == "__main__":
    unittest.main()
