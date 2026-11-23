"""Exercise archive extraction/startup without downloading build dependencies."""
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
                     "docs/THIRD_PARTY_NOTICES.md", "sdl/LICENSE.txt",
                     "build/_deps/munt-src/mt32emu/COPYING.LESSER.txt", "build/_deps/nukedopl-src/LICENSE",
                     "data/rules/units.json", "data/ui/hub.json", "extern/re-setup-kit/LICENSE",
                     "extern/re-setup-kit/third_party/stb/LICENSE", "extern/re-setup-kit/third_party/miniz/LICENSE"]:
            file = root / name
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_text("fixture")
        prefix = root / "deps/prefix"
        for name in ["libarchive-3.7.9/COPYING", "zlib-1.3.1/LICENSE", "xz-5.8.1/COPYING", "xz-5.8.1/COPYING.0BSD"]:
            (prefix / "share/licenses" / name).parent.mkdir(parents=True, exist_ok=True)
            (prefix / "share/licenses" / name).write_text("fixture licence")
        (root / "build/CMakeCache.txt").write_text(f"REKIT_LIBARCHIVE_LIB:FILEPATH={prefix / 'lib/libarchive.a'}\n")
        exe = root / "build/mechwarrior"
        exe.write_text('#!/bin/sh\n[ "$1" = --help ] && { echo "Usage: mw1r [<game folder>]"; exit 0; }\nexit 2\n')
        exe.chmod(0o755)
        for name in ["libSDL3.so.0", "libmt32emu.so.2", "libnukedopl.so"]:
            (root / "build" / name).write_bytes(b"fixture library")
        (root / "build/libSDL3.so").symlink_to("libSDL3.so.0")

    def test_extracted_archive_starts_and_libraries_are_portable(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            with patch.object(build_package, "run"), patch.object(build_package, "platform_name", return_value="linux-x86_64"):
                build_package.package(root, root / "build", root / "sdl", root / "dist", "v0.1.0-test")
            extracted = root / "dist/smoke/MW1-Rebuild-v0.1.0-test-linux-x86_64"
            self.assertFalse((extracted / "libSDL3.so").is_symlink())
            self.assertEqual((extracted / "libSDL3.so").read_bytes(), b"fixture library")
            self.assertTrue(os.access(extracted / "mw1r", os.X_OK) and not (extracted / "mechwarrior").exists())
            self.assertEqual(subprocess.run([str(extracted / "mw1r"), "--help"], capture_output=True).returncode, 0)
            self.assertTrue((extracted / "licenses/Munt-COPYING.LESSER.txt").exists())
            self.assertTrue((extracted / "licenses/stb-LICENSE").exists() and (extracted / "licenses/miniz-LICENSE").exists())
            for name in ["libarchive-COPYING", "zlib-LICENSE", "xz-COPYING", "xz-COPYING.0BSD"]:
                self.assertTrue((extracted / "licenses" / name).exists(), name)
            self.assertTrue((extracted / "data/rules/units.json").exists())
            self.assertTrue((extracted / "data/ui/hub.json").exists())
            self.assertEqual(len(list((root / "dist").glob("*.sha256"))), 1)

    def test_missing_audio_library_blocks_packaging(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            (root / "build/libnukedopl.so").unlink()
            with patch.object(build_package, "run"), patch.object(build_package, "platform_name", return_value="linux-x86_64"), self.assertRaisesRegex(RuntimeError, "Missing runtime dependency"):
                build_package.package(root, root / "build", root / "sdl", root / "dist", "test")
            self.assertFalse(list((root / "dist").glob("*.tar.gz")))


    def test_missing_kit_licence_blocks_packaging(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            (root / "extern/re-setup-kit/third_party/stb/LICENSE").unlink()
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
