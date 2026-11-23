#!/usr/bin/env python3
"""song: and sound: stay out of the game checks (tools/trace/events/README.md).

They call the game, so game-state traces must never use them:
- record.py refuses them without --feed-calls, and with it outside
  tools/trace/events/scripts;
- only events.py passes --feed-calls; check.py, exe_free.py, repeat.py and
  audio.py never do;
- no trace or sound script holds one;
- the port refuses them unless MECHWARRIOR_EVENT_LOG is set (with a port build
  and a game folder: MW1O_FEED_PORT and MW1O_GAME; skipped otherwise).

    python3 -I tools/trace/test_feed_calls.py
"""
import importlib.util, os, subprocess, sys, tempfile, unittest

HERE = os.path.dirname(os.path.abspath(__file__))
FEED_SCRIPTS = os.path.join(HERE, "events", "scripts")


def load_record():
    spec = importlib.util.spec_from_file_location("record", os.path.join(HERE, "record.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class FeedCalls(unittest.TestCase):
    def setUp(self):
        self.record = load_record()

    def script(self, folder, text):
        fd, path = tempfile.mkstemp(suffix=".keys", dir=folder)
        with os.fdopen(fd, "w") as f:
            f.write(text)
        return path  # the caller's temporary folder removes it

    def test_refused_without_the_flag(self):
        with tempfile.TemporaryDirectory() as folder:
            for token in ("song:bar", "sound:3"):
                with self.assertRaises(SystemExit) as raised:
                    self.record.read_script(self.script(folder, f"Return {token} @snap:a\n"))
                self.assertIn("refused", str(raised.exception))

    def test_refused_outside_the_event_scripts(self):
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaises(SystemExit) as raised:
                self.record.read_script(self.script(folder, "Return song:bar @snap:a\n"), feed_calls=True)
            self.assertIn("allowed only", str(raised.exception))

    def test_event_scripts_read_with_the_flag(self):
        names = sorted(f for f in os.listdir(FEED_SCRIPTS) if f.endswith(".keys"))
        self.assertTrue(names)
        for name in names:
            tokens, snaps = self.record.read_script(os.path.join(FEED_SCRIPTS, name), feed_calls=True)
            self.assertTrue(any(t.startswith(("song:", "sound:")) for t in tokens), name)

    def test_no_trace_or_sound_script_calls_the_game(self):
        for folder in (os.path.join(HERE, "scripts"), os.path.join(HERE, "audio", "scripts")):
            for name in sorted(os.listdir(folder)):
                if name.endswith(".keys"):
                    self.record.read_script(os.path.join(folder, name))  # exits if one does

    def test_only_events_passes_the_flag(self):
        for name in ("check.py", "exe_free.py", "repeat.py", "audio.py"):
            with open(os.path.join(HERE, name), encoding="utf-8") as f:
                self.assertNotIn("--feed-calls", f.read(), name)
        with open(os.path.join(HERE, "events.py"), encoding="utf-8") as f:
            self.assertIn("--feed-calls", f.read())

    def test_record_cli_refuses(self):
        script = os.path.join(FEED_SCRIPTS, "songs.keys")
        with tempfile.TemporaryDirectory() as work:
            run = subprocess.run([sys.executable, "-I", os.path.join(HERE, "record.py"), "--exe", "none",
                                  "--game", work, "--script", script, "--out", os.path.join(work, "t.jsonl"),
                                  "--frames", os.path.join(work, "frames")], capture_output=True, text=True)
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("refused", run.stderr)

    @unittest.skipUnless(os.environ.get("MW1O_FEED_PORT") and os.environ.get("MW1O_GAME"),
                         "needs MW1O_FEED_PORT (a port build) and MW1O_GAME")
    def test_port_refuses_without_the_event_log(self):
        env = {k: v for k, v in os.environ.items() if k != "MECHWARRIOR_EVENT_LOG"}
        with tempfile.TemporaryDirectory() as work:
            run = subprocess.run([sys.executable, "-I", os.path.join(HERE, "record.py"), "--feed-calls",
                                  "--exe", os.environ["MW1O_FEED_PORT"], "--game", os.environ["MW1O_GAME"],
                                  "--script", os.path.join(FEED_SCRIPTS, "songs.keys"),
                                  "--out", os.path.join(work, "t.jsonl"), "--frames", os.path.join(work, "frames"),
                                  "--work", os.path.join(work, "run"), "--timeout", "120"],
                                 env=env, capture_output=True, text=True)
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("MECHWARRIOR_EVENT_LOG", run.stdout + run.stderr)


if __name__ == "__main__":
    unittest.main()
