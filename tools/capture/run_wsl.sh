#!/bin/bash
# Runs MWSNAP and a key script against the original under DOSBox-X, headless
# (no window: dummy SDL video, DISPLAY and WAYLAND_DISPLAY unset), on Linux
# or WSL. One run at a time; nice.
#
#   run_wsl.sh NAME KEYS.BIN [CYCLES [LIMIT_SECONDS]]
#
# Environment:
#   MWSNAP_HOME    the work folder (default ~/mwsnap): game/ holds a copy of
#                  the game (only read), in/ holds MWSNAP.COM and the KEYS.BIN
#                  files; each run gets run-NAME/ and run-NAME.conf/.log
#   DOSBOX_X       the dosbox-x binary (default: dosbox-x on PATH). An
#                  unpacked Debian package works without root if
#                  LD_LIBRARY_PATH names its library folders.
#
# The captures are run-NAME/MW1Snnnn.BIN, in keys.py --names order.
# MWSNAPEN.TXT marks the end of the script; NOPATCH.TXT, KEYFAIL.TXT and
# ITEMFAIL.TXT report problems (see mwsnap.asm).
set -u
NAME=$1; KEYS=$2; CYC=${3:-3000}; LIMIT=${4:-240}
M=${MWSNAP_HOME:-$HOME/mwsnap}; X=${DOSBOX_X:-dosbox-x}; RUN=$M/run-$NAME
mkdir -p "$RUN" && cd "$RUN" || exit 1
cp -r "$M/game/." "$RUN/" && cp "$M/in/MWSNAP.COM" "$RUN/" && cp "$M/in/$KEYS" "$RUN/KEYS.BIN" || exit 1
cat > "$M/run-$NAME.conf" <<EOC
[sdl]
autolock=false
[dosbox]
machine=svga_s3
memsize=16
quit warning=false
[cpu]
core=normal
cputype=386
cycles=fixed $CYC
[mixer]
nosound=true
[speaker]
pcspeaker=false
[autoexec]
mount c "$RUN"
c:
MWSNAP K T00010000
MW 1 S
EOC
env -u DISPLAY -u WAYLAND_DISPLAY SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    nice -n 10 "$X" -conf "$M/run-$NAME.conf" -nopromptfolder -fastlaunch < /dev/null > "$M/run-$NAME.log" 2>&1 &
pid=$!; start=$SECONDS
while kill -0 $pid 2>/dev/null; do
  [ -e "$RUN/MWSNAPEN.TXT" ] && { sleep 3; echo "script used up at $((SECONDS-start)) s"; break; }
  [ $((SECONDS-start)) -ge "$LIMIT" ] && { echo "deadline $LIMIT s"; break; }
  sleep 2
done
kill -9 $pid 2>/dev/null; wait $pid 2>/dev/null   # SIGTERM would wait at a y/n prompt
echo "captures: $(ls "$RUN" | grep -c MW1S); flags: $(ls "$RUN" | grep -E '^(NOPATCH|KEYFAIL|ITEMFAIL)' | tr '\n' ' ')"
