#!/usr/bin/env python3
"""The game plays from an installation nothing can be written to (B6).

    read_only_install_test.py --port BUILD/mechwarrior --work DIR

Copies MW1O_GAME, takes every write permission off the copy, and runs a
trace on it against the golden. Skipped (77) without MW1O_GAME, or where
this user can write a read-only file anyway.
"""
import argparse, os, shutil, stat, subprocess, sys

SKIP = 77
TRACE = "options"


def writable(root, on):
    for folder, _, files in os.walk(root, topdown=not on):
        for path in [os.path.join(folder, f) for f in files] + [folder]:
            mode = os.stat(path).st_mode
            os.chmod(path, mode | stat.S_IWUSR if on else mode & ~(stat.S_IWUSR | stat.S_IWGRP | stat.S_IWOTH))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--work", required=True)
    a = ap.parse_args()
    source = os.environ.get("MW1O_GAME")
    if not source or not os.path.isdir(source) or not os.path.exists(a.port):
        print("skipped: set MW1O_GAME and build the port")
        return SKIP
    work = os.path.abspath(a.work)
    game = os.path.join(work, "game")
    if os.path.isdir(game):
        writable(game, True)
    shutil.rmtree(work, ignore_errors=True)
    shutil.copytree(source, game)
    writable(game, False)
    try:
        for name in sorted(os.listdir(game)):
            try:
                open(os.path.join(game, name), "r+b").close()
                print("skipped: this user can write a read-only file")
                return SKIP
            except PermissionError:
                pass
        check = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "trace", "check.py")
        done = subprocess.run([sys.executable, "-I", check, "--port", a.port, "--game", game,
                               "--work", os.path.join(work, "run"), "--quiet", "--only", TRACE],
                              capture_output=True, text=True)
        print(done.stdout.strip() or done.stderr.strip())
        print("read-only installation: " + ("as the golden" if done.returncode == 0 else "FAILED"))
        return 0 if done.returncode == 0 else 1
    finally:
        # Not left behind: setup looks for the game near the program, and
        # would find this copy (status_setup expects none).
        writable(game, True)
        shutil.rmtree(game)


if __name__ == "__main__":
    sys.exit(main())
