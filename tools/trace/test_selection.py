#!/usr/bin/env python3
"""An --only name that isn't one is refused (exit 2) by each runner, before it runs or skips anything."""
import os, subprocess, sys, tempfile, unittest

HERE = os.path.dirname(os.path.abspath(__file__))


class Selection(unittest.TestCase):
    def refused(self, tool, *args):
        with tempfile.TemporaryDirectory() as work:
            run = subprocess.run([sys.executable, "-I", os.path.join(HERE, tool), *args, "--only", "combat_cockpit", "no_such"]
                                 if tool != "audio.py" else
                                 [sys.executable, "-I", os.path.join(HERE, tool), *args, "--only", "combat", "no_such"],
                                 cwd=work, capture_output=True, text=True, env={k: v for k, v in os.environ.items()
                                                                                if not k.startswith("MW1O_")})
        self.assertEqual(run.returncode, 2, run.stdout + run.stderr)
        self.assertIn("no such name: no_such\n", run.stderr)
        self.assertNotIn("skipped", run.stdout)
        self.assertIn("combat", run.stderr.split("names:")[1])

    def test_events(self):
        self.refused("events.py", "check", "--work", "w")

    def test_audio(self):
        self.refused("audio.py", "check", "--work", "w")

    def test_check(self):
        self.refused("check.py", "--port", "p", "--work", "w")

    def test_exe_free(self):
        self.refused("exe_free.py", "--port", "p", "--work", "w")

    def test_run(self):
        self.refused("run.py", "--home", "h", "--out", "o")

    def test_known_names_pass(self):
        with tempfile.TemporaryDirectory() as work:
            run = subprocess.run([sys.executable, "-I", os.path.join(HERE, "events.py"), "check", "--work", "w",
                                  "--only", "audio-combat", "feed-effects"], cwd=work, capture_output=True, text=True,
                                 env={k: v for k, v in os.environ.items() if not k.startswith("MW1O_")})
        self.assertEqual(run.returncode, 77, run.stdout + run.stderr)


if __name__ == "__main__":
    unittest.main()
