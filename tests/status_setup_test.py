"""The launcher contract on the built program (re-setup-kit DESIGN.md):
`mechwarrior --status` prints one JSON line, exits 0, opens no window and
writes nothing; `mechwarrior --setup` exits 0 ready, 1 cancelled, 2 failed
(the reason as {"error": ...} on stdout) and never opens a picker in a hidden
run. With MW1O_GAME (and MW1O_BTECH_1990) a copy is set up from a folder and
the next --status finds it ready, and again through the kit's setup screen,
hidden and driven by its REKIT_SETUP_* hooks (MW1's theme, its log lines). Every run has its own player folder
(MECHWARRIOR_SAVE_DIRECTORY) in the work folder; nothing of the game is kept.
Prints names and counts only.

    status_setup_test.py --program build/mechwarrior --work build/status_setup_work
"""
import argparse
import json
import os
import shutil
import subprocess
import sys
import time

failures = 0


def check(ok, what):
    global failures
    print(("ok   " if ok else "FAIL ") + what)
    if not ok:
        failures += 1


def tree(root):
    out = []
    for d, _, files in os.walk(root):
        out += [os.path.relpath(os.path.join(d, f), root) for f in files]
        out.append(os.path.relpath(d, root) + "/")
    return sorted(out)


def run(program, args, env, timeout=60):
    t = time.monotonic()
    p = subprocess.run([program] + args, env=env, capture_output=True, text=True, timeout=timeout)
    return p, time.monotonic() - t


def find_any_case(folder, name):
    for f in os.listdir(folder):
        if f.lower() == name.lower():
            return os.path.join(folder, f)
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--program", required=True)
    ap.add_argument("--work", required=True)
    a = ap.parse_args()
    work = os.path.abspath(a.work)
    shutil.rmtree(work, ignore_errors=True)  # the test's own folder
    os.makedirs(work)
    base = dict(os.environ)
    for k in ("MECHWARRIOR_SAVE_DIRECTORY", "MECHWARRIOR_SETUP_PICK", "REKIT_SETUP_FRAMES", "REKIT_SETUP_GO",
              "REKIT_SETUP_FIND", "REKIT_SETUP_PICKER", "REKIT_SETUP_FILES_TO", "REKIT_SETUP_SHOT"):
        base.pop(k, None)
    base["MW1O_HIDDEN"] = "1"
    base["SDL_VIDEO_DRIVER"] = "offscreen"
    base["SDL_AUDIO_DRIVER"] = "dummy"

    # --status with an empty home: one JSON line, exit 0, nothing written, fast.
    home = os.path.join(work, "home")
    os.makedirs(home)
    env = dict(base, HOME=home, APPDATA=home, XDG_DATA_HOME=os.path.join(home, "share"))
    before = tree(work)
    p, took = run(a.program, ["--status"], env)
    lines = [l for l in p.stdout.splitlines() if l.strip()]
    check(p.returncode == 0, "--status exits 0")
    check(len(lines) == 1, "--status prints one line (%d)" % len(lines))
    st = json.loads(lines[0]) if len(lines) == 1 else {}
    check(st.get("kit") == 1 and st.get("game") == "mech1", "--status: kit 1, game mech1")
    check(st.get("files", {}).get("state") in ("missing", "ready", "incomplete"), "--status: a state")
    check("story import" in st.get("first_run", {}).get("pending", []), "--status: the story import pending")
    check(tree(work) == before, "break: --status wrote nothing (no player folder, no log)")
    check(took < 5, "--status answered in %.2f s" % took)

    # --setup in a hidden run, nothing picked: failed, the reason as JSON, no picker.
    user = os.path.join(work, "user-a")
    env = dict(base, MECHWARRIOR_SAVE_DIRECTORY=user)
    p, _ = run(a.program, ["--setup"], env)
    check(p.returncode == 2, "break: hidden --setup with nothing picked exits 2 (%d)" % p.returncode)
    check('{"error": ' in p.stdout, "hidden --setup says why as JSON")

    # A pick that is not the game: failed.
    notgame = os.path.join(work, "notgame")
    os.makedirs(notgame)
    open(os.path.join(notgame, "README.TXT"), "w").write("x")
    p, _ = run(a.program, ["--setup"], dict(env, MECHWARRIOR_SETUP_PICK=notgame))
    check(p.returncode == 2 and '{"error": ' in p.stdout, "break: a pick that is not the game exits 2")
    check(not os.path.exists(os.path.join(user, "install")), "break: nothing installed from it")

    # The setup screen, hidden: MW1's header and theme; no picker (unattended), a check's answer stands in.
    # FIND's search finds nothing; the screen on SDL's dummy driver (offscreen needs GL for a window on a Mac).
    alone = dict(HOME=home, APPDATA=home, XDG_DATA_HOME=os.path.join(home, "share"), SDL_VIDEO_DRIVER="dummy")
    screen = dict(env, **alone, REKIT_SETUP_FRAMES="5", REKIT_SETUP_FIND="1", REKIT_SETUP_GO="1",
                  REKIT_SETUP_PICKER="path:" + os.path.join(notgame, "README.TXT"))
    p, _ = run(a.program, ["--setup"], screen)
    out = p.stdout.splitlines()
    check(p.returncode == 1, "break: the screen with a pick that is not the game ends cancelled (%d)" % p.returncode)
    check("setup: screen find: FIND THE GAME, QUIT" in out and "setup: header MECHWARRIOR 1" in out,
          "the screen's log: the find screen, MW1's header")
    check("setup: the file picker asked for" in out and not any(l.startswith("setup: screen confirm") for l in out),
          "break: not the game, not offered")
    check(any(l.startswith("setup: font ") and l.endswith("SairaCondensed-Bold.ttf") for l in out),
          "the theme's face read")
    check(not os.path.exists(os.path.join(user, "install")), "break: nothing installed from the screen either")

    game = os.environ.get("MW1O_GAME")
    btech = os.environ.get("MW1O_BTECH_1990")
    if game and btech:
        pick = os.path.join(work, "pick", "MECHWAR")
        os.makedirs(pick)
        shutil.copyfile(btech, os.path.join(pick, "BTECH.EXE"))
        shutil.copyfile(find_any_case(game, "MW_MAIN.EXE"), os.path.join(pick, "MW_MAIN.EXE"))
        user = os.path.join(work, "user-b")
        env = dict(base, MECHWARRIOR_SAVE_DIRECTORY=user)
        p, _ = run(a.program, ["--setup"], dict(env, MECHWARRIOR_SETUP_PICK=os.path.join(pick, "BTECH.EXE")))
        check(p.returncode == 0, "--setup from a pick of BTECH.EXE exits 0 (%d)" % p.returncode)
        install = os.path.join(user, "install")
        check(os.path.isfile(os.path.join(install, "BTECH.EXE")), "the copy is in the player's folder's install")
        check(os.path.isfile(os.path.join(pick, "BTECH.EXE")), "the player's original is left as it was")
        ini = open(os.path.join(user, "MechWarrior.ini")).read() if os.path.exists(os.path.join(user, "MechWarrior.ini")) else ""
        check("game_folder=" in ini, "game_folder saved to MechWarrior.ini")
        before = tree(work)
        p, took = run(a.program, ["--status"], env)
        st = json.loads(p.stdout.strip().splitlines()[-1]) if p.stdout.strip() else {}
        files = st.get("files", {})
        check(files.get("state") == "ready" and os.path.normcase(os.path.normpath(files.get("where", ""))) ==
              os.path.normcase(os.path.normpath(install)), "--status after the setup: ready, where = the install")
        check(files.get("data") == "1990-01-22", "--status: the revision")
        check(tree(work) == before, "break: --status wrote nothing after a setup either")
        check(took < 1.0, "--status found the game in %.2f s (< 1 s)" % took)

        # The same copy through the setup screen (hidden): found, offered, installed; and into a picked folder.
        user = os.path.join(work, "user-c")
        env = dict(base, MECHWARRIOR_SAVE_DIRECTORY=user)
        screen = dict(env, **alone, REKIT_SETUP_FRAMES="200", REKIT_SETUP_FIND="1", REKIT_SETUP_GO="1",
                      REKIT_SETUP_PICKER="path:" + os.path.join(pick, "BTECH.EXE"))
        p, _ = run(a.program, ["--setup"], screen)
        out = p.stdout.splitlines()
        check(p.returncode == 0, "the screen: --setup exits 0 (%d)" % p.returncode)
        check("setup: screen confirm: MECHWAR, INSTALL, CHANGE FOLDER, BACK" in out and
              "setup: work: INSTALLING MECHWARRIOR 1" in out, "the screen's log: offered, then installing")
        check(os.path.isfile(os.path.join(user, "install", "BTECH.EXE")), "the screen's copy in the player's folder")
        games = os.path.join(work, "games")
        os.makedirs(os.path.join(games, "something else"))
        user = os.path.join(work, "user-d")
        p, _ = run(a.program, ["--setup"], dict(screen, MECHWARRIOR_SAVE_DIRECTORY=user, REKIT_SETUP_FILES_TO=games))
        folder = os.path.join(games, "MechWarrior 1 Rebuild")
        mark = os.path.join(folder, "rebuild-game-folder.txt")
        check(p.returncode == 0 and os.path.isfile(os.path.join(folder, "install", "BTECH.EXE")),
              "CHANGE FOLDER: a folder of the game's own made in a pick holding other things (%d)" % p.returncode)
        check(os.path.isfile(mark) and "\ngame=mw1r\n" in open(mark).read(), "its mark: game=mw1r")
        check(os.listdir(os.path.join(games, "something else")) == [], "what was there is left as it was")
    else:
        print("skip the game parts: MW1O_GAME and MW1O_BTECH_1990 not both set")

    shutil.rmtree(work, ignore_errors=True)
    print("%s (%d failed)" % ("FAILED" if failures else "passed", failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
