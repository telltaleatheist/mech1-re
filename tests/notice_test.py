"""What the player is told when the program can't start or has to stop (src/SDL/SDLNotice.h), on
notice_probe: the text a box would show goes to MECHWARRIOR_NOTICE_FILE, so nothing is ever shown.
Windows: an abort shows its reason too, and output that goes nowhere goes to the log.

    notice_test.py --probe build/notice_probe --work build/notice_work
"""
import argparse
import os
import shutil
import subprocess
import sys

failures = 0
WINDOWS = os.name == "nt"
SCRIPTED = ("MW1O_HIDDEN", "MECHWARRIOR_DEBUG_CONTROL_DIRECTORY", "MECHWARRIOR_DEBUG_VIRTUAL_CLOCK", "MECHWARRIOR_SETUP_PICK",
            "REKIT_SETUP_FRAMES", "MECHWARRIOR_NOTICE_FILE")


def check(ok, what):
    global failures
    print(("ok   " if ok else "FAIL ") + what)
    if not ok:
        failures += 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--probe", required=True)
    ap.add_argument("--work", required=True)
    a = ap.parse_args()
    work = os.path.abspath(a.work)
    shutil.rmtree(work, ignore_errors=True)  # the test's own folder
    os.makedirs(work)
    count = 0

    def run(*args, nowhere=False, **more):
        nonlocal count
        count += 1
        notice = os.path.join(work, "notice%d.txt" % count)
        env = {k: v for k, v in os.environ.items() if k not in SCRIPTED}
        env.update(MECHWARRIOR_NOTICE_FILE=notice, SDL_MAC_BACKGROUND_APP="1", **more)
        to = subprocess.DEVNULL if nowhere else subprocess.PIPE
        p = subprocess.run([a.probe] + list(args), env=env, stdin=subprocess.DEVNULL, stdout=to, stderr=to, timeout=60)
        err = "" if nowhere else p.stderr.decode("utf-8", "replace")
        return p.returncode, err, open(notice, encoding="utf-8").read() if os.path.exists(notice) else None

    code, err, text = run("attended", "refuse")
    check(code == 2 and "the refusal's reason" in err, "a refused start prints its reason")
    check(text is not None and "couldn't start" in text and "the refusal's reason" in text, "and shows it")
    check(run("hidden", "refuse")[2] is None, "break: a hidden run shows nothing")
    for name in SCRIPTED[1:5]:
        check(run("attended", "refuse", **{name: "x"})[2] is None, "break: a scripted run shows nothing (%s)" % name)

    for how, reason in (("failsafe", "the failsafe's reason"), ("fail", "The host's operation")):
        code, err, text = run("attended", how)
        check(code not in (0, 2) and reason in err, "%s prints its reason" % how)
        if WINDOWS:
            check(code == 3 and "fatal abort" in err, "%s: the stack is in the log" % how)
            check(text is not None and "had to stop" in text and reason in text, "%s shows it" % how)
        else:
            check(text is None, "%s: off Windows stderr is all" % how)
        check(run("hidden", how)[2] is None, "break: %s in a hidden run shows nothing" % how)

    log = os.path.join(work, "log é日.txt")
    code, err, text = run("attended", "refuse", log)
    check(not os.path.exists(log) and "The log is" not in text, "break: output that is read opens no log")
    code, err, text = run("attended", "refuse", log, nowhere=True)
    if WINDOWS:
        check(os.path.exists(log) and "the refusal's reason" in open(log, encoding="utf-8").read(), "output that goes nowhere goes to the log")
        check(text is not None and "The log is " + log in text, "and the notice names the log")
        os.remove(log)
        run("attended", "failsafe", log, nowhere=True)
        check(os.path.exists(log) and "the failsafe's reason" in open(log, encoding="utf-8").read(), "a failsafe's too")
        new = os.path.join(work, "not there é日", "deeper", "mechwarrior.log")
        code, err, text = run("attended", "refuse", new, nowhere=True)
        check(os.path.exists(new) and "The log is " + new in text, "a save directory that isn't there yet is made for the log")
    else:
        check(not os.path.exists(log), "off Windows a log is opened only by a start without a folder")
    both = os.path.join(work, "both.log")
    with open(both, "w") as f:
        f.write("a line of the start before\n" * 20)
    code, err, text = run("hidden", "lines", both)
    wanted = [line % n for n in range(3) for line in ("out %d: a line of the output, longer than the other", "err %d: a line of the errors")]
    check(code == 0 and open(both, encoding="utf-8").read().split("\n") == wanted + [""],
          "the log holds both streams' lines whole, in turn, and nothing of the start before")
    return 1 if failures else 0


sys.exit(main())
