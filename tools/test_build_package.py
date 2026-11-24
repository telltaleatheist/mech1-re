"""Exercise archive extraction/startup without downloading build dependencies."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import build_package


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
            with patch.object(build_package, "run"), patch.object(build_package, "platform_name", return_value="linux-x86_64"):
                build_package.package(root, root / "build", root / "sdl", root / "dist", "v0.1.0-test")
            extracted = root / "dist/smoke/MW1-Rebuild-v0.1.0-test-linux-x86_64"
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


if __name__ == "__main__":
    unittest.main()
