"""--only: a name that isn't one is refused, so a run never tests less than was asked for."""
import sys


def check_only(only, valid):
    """Exits (status 2), naming them, if any of `only` is not in `valid`."""
    unknown = sorted(set(only or ()) - set(valid))
    if unknown:
        print(f"--only: no such name: {', '.join(unknown)}\nnames: {', '.join(sorted(valid))}", file=sys.stderr)
        sys.exit(2)
