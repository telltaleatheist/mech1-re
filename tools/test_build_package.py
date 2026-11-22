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
                     "data/rules/units.json", "data/ui/hub.json"]:
            file = root / name
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_text("fixture")
        exe = root / "build/mechwarrior"
        exe.write_text('#!/bin/sh\necho "Usage: mechwarrior <game-directory>" >&2\nexit 2\n')
        exe.chmod(0o755)
        for name in ["libSDL3.so.0", "libmt32emu.so.2", "libnukedopl.so"]:
            (root / "build" / name).write_bytes(b"fixture library")
        (root / "build/libSDL3.so").symlink_to("libSDL3.so.0")

    def test_extracted_archive_starts_and_libraries_are_portable(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            with patch.object(build_package, "run"):
                build_package.package(root, root / "build", root / "sdl", root / "dist", "v0.1.0-test")
            extracted = root / "dist/smoke/MechWarrior-v0.1.0-test-linux-x86_64"
            self.assertFalse((extracted / "libSDL3.so").is_symlink())
            self.assertEqual((extracted / "libSDL3.so").read_bytes(), b"fixture library")
            self.assertTrue(os.access(extracted / "mechwarrior", os.X_OK))
            self.assertEqual(subprocess.run([str(extracted / "mechwarrior")], capture_output=True).returncode, 2)
            self.assertTrue((extracted / "licenses/Munt-COPYING.LESSER.txt").exists())
            self.assertTrue((extracted / "data/rules/units.json").exists())
            self.assertTrue((extracted / "data/ui/hub.json").exists())
            self.assertEqual(len(list((root / "dist").glob("*.sha256"))), 1)

    def test_missing_audio_library_blocks_packaging(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            (root / "build/libnukedopl.so").unlink()
            with patch.object(build_package, "run"), self.assertRaisesRegex(RuntimeError, "Missing runtime dependency"):
                build_package.package(root, root / "build", root / "sdl", root / "dist", "test")
            self.assertFalse(list((root / "dist").glob("*.tar.gz")))


    def test_missing_data_blocks_packaging(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            (root / "data/ui/hub.json").unlink()
            with patch.object(build_package, "run"), self.assertRaisesRegex(RuntimeError, "Missing game data: data/ui"):
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
