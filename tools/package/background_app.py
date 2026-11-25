"""A Mac app's hidden runs without a foreground app.

A program started from inside an app is a foreground app by the app's Info.plist, whatever SDL_MAC_BACKGROUND_APP
says (that only keeps SDL from asking for it). So a check runs a temporary copy of the app whose Info.plist says
LSBackgroundOnly, signed ad hoc again. The copy differs from the app in Info.plist and the signature, no more. The app's
own signature is checked first, and the app is only read.

    background_app.py APP FOLDER     # the copy in FOLDER; prints its program
"""
from contextlib import contextmanager
from pathlib import Path
import plistlib
import subprocess
import sys
import tempfile


def app_of(executable):
    """The app `executable` is the program of, or None."""
    contents = Path(executable).parent.parent
    inside = Path(executable).parent.name == "MacOS" and contents.name == "Contents" and (contents / "Info.plist").is_file()
    return contents.parent if inside else None


def command(*args):
    done = subprocess.run([str(a) for a in args], capture_output=True, text=True)
    if done.returncode != 0:
        raise RuntimeError(f"{args[0]}: {(done.stderr or done.stdout).strip()}")


def background_copy(app, folder):
    """The copy of `app` in `folder`; returns its app."""
    app = Path(app)
    command("codesign", "--verify", "--deep", "--strict", app)
    copy = Path(folder) / app.name
    if copy.exists():
        raise RuntimeError(f"{copy} is there already")
    command("ditto", app, copy)
    info = copy / "Contents/Info.plist"
    plist = plistlib.loads(info.read_bytes())
    plist["LSBackgroundOnly"] = True
    info.write_bytes(plistlib.dumps(plist))
    command("codesign", "--force", "--sign", "-", "--preserve-metadata=identifier,entitlements,flags", copy)
    command("codesign", "--verify", "--deep", "--strict", copy)
    return copy


@contextmanager
def runnable(executable):
    """The program a check runs for `executable`: an app's is its background copy's, any other is itself."""
    app = app_of(executable) if sys.platform == "darwin" else None
    if app is None:
        yield executable
        return
    with tempfile.TemporaryDirectory(prefix="mw1-background-") as folder:
        yield (background_copy(app, folder) / "Contents/MacOS" / Path(executable).name).resolve()


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    copy = background_copy(Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve())
    print(copy / "Contents/MacOS" / plistlib.loads((copy / "Contents/Info.plist").read_bytes())["CFBundleExecutable"])
