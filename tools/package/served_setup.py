"""The setup served to the launcher (docs/RELEASING.md, "The setup served to the launcher"), asked of a built or
packaged program as the launcher asks it: stdin and stdout are pipes. Every run has its own player's folder
(MECHWARRIOR_SAVE_DIRECTORY) and home under `scratch`. A copy of the game can't be made up (BTECH.EXE is known by its
bytes), so the checks that need one take the game's folder, and are named as skipped without it. Nothing of the game is
kept; names and counts only are printed.

tests/status_setup_test.py runs these on the build and tools/build_package.py on each package.
"""
import json
import os
from pathlib import Path
import queue
import re
import shutil
import subprocess
import sys
import threading
import time

SECONDS = 120
MANY = 6000  # files in the copy that is stopped: enough that it can't end before the stop arrives
HOOKS = ("REKIT_SETUP_", "MECHWARRIOR_SETUP_", "MECHWARRIOR_NOTICE_")


def asking_env(env, user):
    """`env` with the player's folder `user` and a home of its own. The video driver doesn't exist, so a run that tried
    to open a window would fail, and say so."""
    out = {k: v for k, v in env.items() if not k.startswith(HOOKS)}
    home = str(Path(user).parent / "home")
    out.update(MW1O_HIDDEN="1", MECHWARRIOR_SAVE_DIRECTORY=str(user), SDL_VIDEO_DRIVER="none-of-them", SDL_AUDIO_DRIVER="dummy",
               SDL_MAC_BACKGROUND_APP="1", HOME=home, APPDATA=home, XDG_DATA_HOME=os.path.join(home, "share"))
    return out


class Answer:
    """What one ask answered: its exit, its stdout's lines as JSON (None for a line that isn't), its stderr."""

    def __init__(self, code, lines, stderr):
        self.code, self.lines, self.stderr = code, lines, stderr
        self.messages = []
        for line in lines:
            try:
                value = json.loads(line)
            except ValueError:
                value = None
            self.messages.append(value if isinstance(value, dict) and "done" in value else None)
        self.last = self.messages[-1] if self.messages and self.messages[-1] else {}

    def clean(self):
        """Nothing but messages on stdout, `done` on the last and only there."""
        return bool(self.messages) and None not in self.messages and [m["done"] for m in self.messages] == \
            [False] * (len(self.messages) - 1) + [True]

    def refused(self):
        """One line saying why, exit 2."""
        return self.clean() and len(self.lines) == 1 and self.code == 2 and self.last.get("ok") is False and bool(self.last.get("summary"))

    def __str__(self):
        return "exit %s, %d lines, last %s, stderr %s" % (self.code, len(self.lines), self.last, self.stderr[-300:])


def ask(program, args, env, cancel_at=None, gone_at=None, kill_at=None, during=None):
    """`program --setup args`, both ends pipes. With a stage's first word: when that stage's first line comes, the
    "cancel" line is sent (cancel_at), stdout closed (gone_at), the program killed (kill_at) or a function called
    (during, at "Copying"). cancel_at "" is at once."""
    p = subprocess.Popen([str(program), "--setup"] + [str(a) for a in args], env=env, stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    errors = []
    reader = threading.Thread(target=lambda: errors.append(p.stderr.read()), daemon=True)
    reader.start()
    got = queue.Queue()

    def read():
        try:
            for raw in p.stdout:
                got.put(raw.decode("utf-8", "replace").rstrip("\r\n"))
        except ValueError:
            pass  # closed here
        got.put(None)

    lines, acted = [], False
    at = cancel_at if cancel_at is not None else kill_at if kill_at is not None else "Copying" if during else None

    def act():
        if cancel_at is not None:
            try:
                p.stdin.write(b"cancel\n")
                p.stdin.flush()
            except OSError:
                pass  # already ended: the answer's checks say what that means
        elif kill_at is not None:
            p.kill()
        else:
            during()

    if at == "":
        act()
        acted = True
    if gone_at is not None:
        # Lines read by hand: a reader thread would hold the pipe open.
        while True:
            raw = p.stdout.readline()
            if not raw:
                break
            lines.append(raw.decode("utf-8", "replace").rstrip("\r\n"))
            if '"stage": "%s' % gone_at in lines[-1]:
                break
        p.stdout.close()
    else:
        threading.Thread(target=read, daemon=True).start()
        end = time.monotonic() + SECONDS
        while True:
            try:
                line = got.get(timeout=max(0.1, end - time.monotonic()))
            except queue.Empty:
                p.kill()
                raise RuntimeError("--setup %s did not end" % args)
            if line is None:
                break
            lines.append(line)
            if at and not acted and '"stage": "%s' % at in line:
                act()
                acted = True
    code = p.wait(timeout=SECONDS)
    reader.join(5)
    try:
        p.stdin.close()
    except OSError:
        pass
    return Answer(code, lines, (errors[0] if errors else b"").decode("utf-8", "replace"))


def any_case(folder, name):
    for f in os.listdir(str(folder)):
        if f.lower() == name.lower():
            return Path(folder) / f
    raise RuntimeError("The game's folder has no " + name)


def copy_of(game, folder, files=0, main=True):
    """The game's two programs, without the rest of its files, and `files` made-up files more."""
    folder = Path(folder)
    folder.mkdir(parents=True)
    shutil.copyfile(str(any_case(game, "BTECH.EXE")), str(folder / "BTECH.EXE"))
    if main:
        shutil.copyfile(str(any_case(game, "MW_MAIN.EXE")), str(folder / "MW_MAIN.EXE"))
    for i in range(files):
        (folder / ("F%05d.DAT" % i)).write_bytes(b"made up %d\n" % i * 60)
    return folder


def tree(folder):
    """The files under a folder, but the served runs' own log."""
    out = []
    for d, _, files in os.walk(str(folder)):
        out += [os.path.relpath(os.path.join(d, f), str(folder)) for f in files]
    return sorted(f for f in out if f != "setup.log")


def same(a, b):
    return bool(a) and bool(b) and os.path.exists(str(a)) and os.path.exists(str(b)) and os.path.samefile(str(a), str(b))


def remembered(user):
    ini = Path(user) / "MechWarrior.ini"
    found = re.findall(r"(?m)^game_folder=(.*?)\r?$", ini.read_text(encoding="utf-8")) if ini.is_file() else []
    return found[-1] if found else ""


def remember(user, folder):
    (Path(user) / "MechWarrior.ini").write_text("game_folder=%s\n" % folder, encoding="utf-8")


# What needs a copy of the game: named as skipped without one.
COPY_PARTS = ("look and install of a copy, its words and what is amiss with its files, remembering the folder",
              "a cancel, the launcher gone, two installs at once, a kill in mid copy",
              "a copy without MW_MAIN.EXE, and what --search answers when a copy is in use",
              "--folder, ours and other people's folders",
              "folder names with accents and Japanese, through --look, --install and --status")


def checks(program, env, scratch, game=None):
    """Yields (passed, what) for each check; (None, what) for a part that was skipped."""
    scratch = Path(scratch)
    scratch.mkdir(parents=True, exist_ok=True)
    (scratch / "home").mkdir()

    asked = []  # every ask and its answer

    def run(user, args, **more):
        a = ask(program, args, asking_env(env, scratch / user), **more)
        asked.append((args, a, more))
        return a

    def stray():
        """The asks with anything on stdout but messages, `done` last and only there. One that was killed or whose
        pipe was closed ends early: its whole lines are messages."""
        out = []
        for args, a, more in asked:
            early = "gone_at" in more or "kill_at" in more
            lines = a.messages[:-1] if "kill_at" in more else a.messages
            if (None in lines) if early else (not a.clean() and args[0] in ("--describe", "--search", "--look", "--install")):
                out.append(" ".join(str(w) for w in args))
        return out

    def status(user):
        p = subprocess.run([str(program), "--status"], env=asking_env(env, scratch / user), stdout=subprocess.PIPE, timeout=SECONDS)
        try:
            st = json.loads(p.stdout.decode("utf-8"))
        except ValueError:
            st = {}
        return st if "files" in st else {"files": {"state": "no answer", "where": ""}}

    yield status("empty").get("setup") == 1, "--status says the setup is served"
    d = run("empty", ["--describe"])
    yield d.clean() and len(d.lines) == 1 and d.code == 0 and d.last.get("ok") and d.last.get("title") == "MechWarrior 1" and \
        d.last.get("pick_words") and d.last.get("find_sub") and d.last.get("find_words"), "describe: the words on stdout alone (%s)" % d
    yield d.last.get("into", "").endswith("install"), "describe: where it would go"
    s = run("empty", ["--search"])
    yield s.clean() and len(s.lines) == 1 and s.code == 0 and s.last.get("ok") and \
        (not s.last.get("path") or os.path.exists(s.last["path"])), "search: an empty home: a copy that is there, or none (%s)" % s
    yield not (scratch / "empty").exists(), "break: --status, --describe and --search make nothing in the player's folder"
    # As the launcher starts it: not a hidden run.
    shown = asking_env(env, scratch / "empty")
    del shown["MW1O_HIDDEN"]
    for args in (["--describe"], ["--search"], ["--look"]):
        a = ask(program, args, shown)
        yield a.clean() and len(a.lines) == 1, "%s with a player at the run: one line on stdout (%s)" % (args[0], a)

    notgame = scratch / "notgame"
    notgame.mkdir()
    (notgame / "README.TXT").write_text("x")
    for bad in (["--look"], ["--look", ""], ["--look", notgame, "b"], ["--install"], ["--install", ""], ["--install", notgame, "--folder"],
                ["--install", notgame, "--folder", ""], ["--install", notgame, "b"],
                ["--install", notgame, "--folder", scratch / "x", "y"], ["--describe", "a"], ["--search", "a"]):
        a = run("empty", bad)
        yield a.refused(), "break: %s: one line saying why, exit 2 (%s)" % (" ".join(str(w) for w in bad).replace(str(notgame), "PICK"), a)
    yield not (scratch / "empty").exists() and not (scratch / "x").exists(), "break: nor do wrong arguments"
    for other in (["--bogus"], ["x"], ["--describe2"]):
        a = run("empty", other)
        yield a.code == 2 and not a.lines, "break: --setup %s is the usage, exit 2, as before (%s)" % (other[0], a)
    for hook in ("MECHWARRIOR_SETUP_PICK", "REKIT_SETUP_GO"):
        own = asking_env(env, scratch / "empty")
        del own["MECHWARRIOR_SAVE_DIRECTORY"]
        a = ask(program, ["--describe"], dict(own, **{hook: "1"}))
        yield a.refused() and hook in a.last["summary"], "break: %s without a player's folder of its own: refused (%s)" % (hook, a)

    a = run("look", ["--look", notgame])
    yield a.clean() and a.code == 2 and "No copy of" in a.last.get("summary", ""), "look: not the game (%s)" % a
    fake = scratch / "fake"
    fake.mkdir()
    (fake / "BTECH.EXE").write_bytes(b"x" * 1000)
    a = run("look", ["--look", fake / "BTECH.EXE"])
    yield a.clean() and a.code == 2 and "a revision this program doesn't know" in a.last.get("summary", ""), \
        "look: an unknown BTECH.EXE is named (%s)" % a
    a = run("look", ["--install", notgame])
    yield a.clean() and a.code == 2 and not a.last.get("ok"), "break: install: not the game"
    a = run("look", ["--look", scratch / "nowhere"])
    yield a.clean() and a.code == 2 and a.last.get("summary"), "look: nothing is there (%s)" % a
    yield tree(scratch / "look") == [], "break: nothing made in the player's folder but the log"

    if not game:
        yield not stray(), "break: every ask: nothing on stdout but its messages %s" % stray()
        for part in COPY_PARTS:
            yield None, part
        return

    copy = copy_of(game, scratch / "copy" / "MECHWAR")
    before = tree(copy)
    user = scratch / "user"
    a = run("user", ["--look", copy / "BTECH.EXE"])
    yield a.clean() and a.code == 0 and a.last.get("ok") and not a.last.get("in_place") and same(a.last.get("path"), copy) and \
        same(Path(a.last.get("into", "")).parent, user), "look: the pick of BTECH.EXE (%s)" % a
    yield "files missing" in a.last.get("words", "") and "You can continue." in a.last.get("words", "") and "\n" not in a.last["words"], \
        "look: its words are the short ones, with what is missing (%s)" % a.last.get("words")
    log = (user / "setup.log").read_text(encoding="utf-8")
    yield "setup: files: missing: " in log and not a.stderr, "look: the file check's line is in setup.log, not on stdout or stderr"
    yield tree(user) == [], "break: a look makes nothing but the log"
    a = run("user", ["--install", copy])
    install = user / "install"
    yield a.clean() and a.code == 0 and a.last.get("ok") and same(a.last.get("where"), install), "install: done (%s)" % a
    yield "files copied into" in a.last.get("summary", "") and "Missing: MW_EGA.EXE, 6X6.FNT, FOX88.FNT and" in a.last.get("summary", ""), \
        "install: its summary says what is missing"
    yield (install / "BTECH.EXE").is_file() and tree(copy) == before, "install: copied, the original left as it was"
    yield same(remembered(user), install), "install: remembered in MechWarrior.ini"
    yield sorted(os.listdir(str(user))) == ["MechWarrior.ini", "install", "setup.log"], "install: nothing else left"
    st = status("user")
    yield st["files"]["state"] == "ready" and same(st["files"]["where"], install) and st.get("setup") == 1, "install: --status is ready there"
    written = (user / "MechWarrior.ini").stat().st_mtime_ns
    a = run("user", ["--look", install])
    yield a.clean() and a.last.get("in_place") and a.code == 0 and same(a.last.get("where"), install), "look: the install itself is in place (%s)" % a
    a = run("user", ["--install", install / "BTECH.EXE"])
    yield a.clean() and a.code == 0 and a.last.get("ok") and a.last.get("in_place") and (install / "BTECH.EXE").is_file(), \
        "install: of the copy in place: nothing is copied, and it stays (%s)" % a
    yield (user / "MechWarrior.ini").stat().st_mtime_ns == written, "break: the folder remembered already isn't written again"
    (user / "MechWarrior.ini").write_text("# mine\nmusic=off\n", encoding="utf-8")
    a = run("user", ["--look", install])
    kept = (user / "MechWarrior.ini").read_text(encoding="utf-8")
    yield same(remembered(user), install) and "# mine" in kept and "music=off" in kept, "look: in place is remembered, the other lines kept"

    many = copy_of(game, scratch / "many", MANY)
    a = run("stopped", ["--install", many], cancel_at="Copying")
    yield a.clean() and a.code == 1 and a.last.get("cancelled") and not a.last.get("ok") and a.last.get("summary"), \
        "break: a cancel line on stdin stops the copy (%s)" % a
    left = sorted(os.listdir(str(scratch / "stopped")))
    yield left == ["setup.log"], "break: the stop left nothing behind (%s)" % left
    a = run("stopped", ["--install", many], cancel_at="")
    yield a.clean() and a.code == 1 and a.last.get("cancelled") and sorted(os.listdir(str(scratch / "stopped"))) == ["setup.log"], \
        "break: a cancel sent at once (%s)" % a
    a = run("gone", ["--install", many], gone_at="Copying")
    yield (scratch / "gone" / "install" / "MW_MAIN.EXE").is_file() and a.code == 0 and same(remembered(scratch / "gone"), scratch / "gone" / "install"), \
        "break: the launcher gone in mid copy: the copy goes on and is remembered (%s)" % a
    second = []
    a = run("two", ["--install", many], during=lambda: second.append(run("two", ["--install", copy])))
    yield a.clean() and a.code == 0 and a.last.get("ok"), "two installs at once: the first goes in (%s)" % a
    yield len(second) == 1 and second[0].clean() and second[0].code == 2 and "Another setup is copying" in second[0].last.get("summary", ""), \
        "break: the second is refused (%s)" % (second[0] if second else "not run")
    yield not (scratch / "two" / "install.lock").exists(), "two installs at once: no lock file after"
    run("killed", ["--install", many], kill_at="Copying")
    yield not (scratch / "killed" / "install").exists(), "break: killed in mid copy: no install"
    a = run("killed", ["--install", copy])
    yield a.code == 0 and a.last.get("ok") and not (scratch / "killed" / "install.new").exists() and \
        not (scratch / "killed" / "install.lock").exists(), "break: and the next install clears up and goes in (%s)" % a

    # A copy without MW_MAIN.EXE goes in and is incomplete; --search then never answers the copy in use.
    half = copy_of(game, scratch / "half", main=False)
    a = run("half-user", ["--install", half])
    st = status("half-user")
    yield a.code == 0 and a.last.get("ok") and st["files"]["state"] == "incomplete" and st["files"].get("missing"), \
        "install: a copy without MW_MAIN.EXE goes in, and --status says it is incomplete (%s)" % a
    a = run("half-user", ["--search"])
    yield a.clean() and a.code == 0 and not same(a.last.get("path"), scratch / "half-user" / "install"), \
        "search: an incomplete install of the player's own isn't offered again (%s)" % a
    a = run("user", ["--search"])
    yield a.clean() and not same(a.last.get("path"), install), "search: nor a ready one in use (%s)" % a
    remember(user, copy)
    a = run("user", ["--search"])
    yield not same(a.last.get("path"), copy) and not same(a.last.get("path"), install), \
        "search: another copy in use and ready: neither it nor the player's own (%s)" % a
    remember(user, half)
    st = status("user")
    yield st["files"]["state"] == "incomplete" and same(st["files"]["where"], half), "another copy in use and incomplete: --status says so"
    a = run("user", ["--search"])
    yield same(a.last.get("path"), install), "search: then the player's own ready install is the answer (%s)" % a
    a = run("user", ["--look", a.last.get("path") or scratch / "nowhere"])
    st = status("user")
    yield a.last.get("in_place") and st["files"]["state"] == "ready" and same(st["files"]["where"], install), \
        "look: it is in place and in use from then on (%s)" % a

    # --folder: the game's own folder; one of somebody else's, and an install that isn't ours, stay as they are.
    own = scratch / "drive" / "MechWarrior 1"
    a = run("user2", ["--install", copy, "--folder", own])
    yield a.code == 0 and a.last.get("ok") and (own / "install" / "BTECH.EXE").is_file() and same(remembered(scratch / "user2"), own / "install"), \
        "folder: into one of its own, and remembered (%s)" % a
    yield "game=mw1r" in (own / "rebuild-game-folder.txt").read_text(), "folder: marked as the game's"
    a = run("user2", ["--install", copy, "--folder", own])
    yield a.code == 0 and a.last.get("ok"), "folder: our own install there is replaced"
    (own / "install" / "mechwarrior-install.txt").unlink()
    (own / "install" / "LETTER.TXT").write_text("the player's")
    a = run("user3", ["--install", copy, "--folder", own])
    yield a.code == 2 and not a.last.get("ok") and (own / "install" / "LETTER.TXT").read_text() == "the player's", \
        "break: folder: an install without our marker is never replaced (%s)" % a
    theirs = scratch / "theirs"
    theirs.mkdir()
    (theirs / "LETTER.TXT").write_text("the player's")
    a = run("user3", ["--install", copy, "--folder", theirs])
    yield a.code == 2 and not a.last.get("ok") and tree(theirs) == ["LETTER.TXT"], \
        "break: folder: somebody else's folder is left alone (%s)" % a
    yield tree(scratch / "user3") == [], "break: nothing remembered from the refused ones"

    # Names with any character in them: the pick's folder, the player's, the one it goes to.
    far = copy_of(game, scratch / "Spiel éß" / "戦闘日本" / "MECHWAR")
    far_user = "Spieler é日本"
    a = run(far_user, ["--look", far / "BTECH.EXE"])
    yield a.clean() and a.code == 0 and same(a.last.get("path"), far), "other characters: look: the path says the folder (%s)" % a
    a = run(far_user, ["--install", far])
    yield a.clean() and a.code == 0 and same(a.last.get("where"), scratch / far_user / "install"), "other characters: install: where (%s)" % a
    st = status(far_user)
    yield st["files"]["state"] == "ready" and same(st["files"]["where"], scratch / far_user / "install"), "other characters: --status where"
    drive = scratch / "Laufwerk üメカ"
    a = run(far_user, ["--install", far, "--folder", drive])
    yield a.clean() and a.code == 0 and same(a.last.get("where"), drive / "install") and same(remembered(scratch / far_user), drive / "install"), \
        "other characters: --folder, and it is remembered (%s)" % a
    st = status(far_user)
    yield st["files"]["state"] == "ready" and same(st["files"]["where"], drive / "install"), "other characters: --status says that folder"
    yield not stray(), "break: every ask (%d): nothing on stdout but its messages %s" % (len(asked), stray())


if __name__ == "__main__":
    import tempfile
    failed = skipped = 0
    with tempfile.TemporaryDirectory(prefix="mw1-served-") as scratch_dir:
        for passed, what in checks(Path(sys.argv[1]).resolve(), os.environ, scratch_dir, os.environ.get("MW1O_GAME")):
            print(("skip " if passed is None else "ok   " if passed else "FAIL ") + what)
            skipped += passed is None
            failed += passed is not None and not passed
    print("%d failed, %d parts skipped" % (failed, skipped))
    sys.exit(1 if failed else 0)
