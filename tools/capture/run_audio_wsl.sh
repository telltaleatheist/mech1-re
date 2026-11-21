#!/bin/bash
# Runs a key script against the ORIGINAL under DOSBox-X, headless, like
# run_wsl.sh, and captures what it sends to the sound hardware with DOSBox-X's
# own captures (Phase 5 level 3, docs/phase5/AUDIO_PLAN.md):
#   A: AdLib. DX-CAPTURE /O wraps the game: the OPL register writes as DROv2
#      (cap/*.dro), from the first key-on, with millisecond delays.
#   R: Roland. The MPU in intelligent mode, its MIDI out on DOSBox-X's OSS
#      handler pointed at a plain file (cap/midi.oss): every MIDI byte as a
#      4-byte SEQ_MIDIPUTC record, written unbuffered. No timestamps.
#   S: speaker. DX-CAPTURE /A: the mixer's output as WAV (cap/*.wav).
# A and R also write the WAV with WAV=1.
#
#   run_audio_wsl.sh NAME KEYS.BIN A|R|S [LIMIT_SECONDS [HOLD_SECONDS]]
#
# HOLD: seconds to leave the game running after the script is used up (the
# last screen's music keeps playing). Within a script, a screen is held by
# wait:MS (emulated time); idle: can't hold one on the original, whose empty
# polls take almost no emulated time (see tools/trace/audio/README.md).
#
# Shutdown is SIGTERM, which DOSBox-X answers by closing its captures (the
# DRO and WAV headers get their lengths); SIGKILL only if it hangs.
# Environment: MWSNAP_HOME, DOSBOX_X as for run_wsl.sh.
set -u
NAME=$1; KEYS=$2; DEV=$3; LIMIT=${4:-300}; HOLD=${5:-0}
M=${MWSNAP_HOME:-$HOME/mwsnap}; X=${DOSBOX_X:-dosbox-x}; RUN=$M/run-$NAME
WAVSW=; [ "${WAV:-0}" = 1 ] && WAVSW=" /A"
case $DEV in
  A) GAME="DX-CAPTURE /O$WAVSW MW 1 A"; MIDI="mpu401=none" ;;
  R) GAME="MW 1 R"; [ -n "$WAVSW" ] && GAME="DX-CAPTURE$WAVSW MW 1 R"
     MIDI="mpu401=intelligent
mididevice=oss
midiconfig=$RUN/cap/midi.oss" ;;
  S) GAME="DX-CAPTURE /A MW 1 S"; MIDI="mpu401=none" ;;
  *) echo "device: A, R or S" >&2; exit 2 ;;
esac
mkdir -p "$RUN/cap" && cd "$RUN" || exit 1
cp -r "$M/game/." "$RUN/" && cp "$M/in/MWSNAP.COM" "$RUN/" && cp "$M/in/$KEYS" "$RUN/KEYS.BIN" || exit 1
: > "$RUN/cap/midi.oss"
cat > "$M/run-$NAME.conf" <<EOC
[sdl]
autolock=false
[dosbox]
machine=svga_s3
memsize=16
quit warning=false
captures=$RUN/cap
[cpu]
core=normal
cputype=386
cycles=fixed 3000
[sblaster]
sbtype=sb16
oplmode=opl2
[midi]
$MIDI
[speaker]
pcspeaker=true
[autoexec]
mount c "$RUN"
c:
MWSNAP K T00010000
$GAME
EOC
env -u DISPLAY -u WAYLAND_DISPLAY SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    nice -n 10 "$X" -conf "$M/run-$NAME.conf" -nopromptfolder -fastlaunch < /dev/null > "$M/run-$NAME.log" 2>&1 &
pid=$!; start=$SECONDS
while kill -0 $pid 2>/dev/null; do
  [ -e "$RUN/MWSNAPEN.TXT" ] && { sleep $((3 + HOLD)); echo "script used up; held to $((SECONDS-start)) s"; break; }
  [ $((SECONDS-start)) -ge "$LIMIT" ] && { echo "deadline $LIMIT s"; break; }
  sleep 2
done
kill -TERM $pid 2>/dev/null
for i in 1 2 3 4 5 6 7 8 9 10; do kill -0 $pid 2>/dev/null || break; sleep 1; done
kill -0 $pid 2>/dev/null && { echo "SIGTERM ignored; SIGKILL (captures may be unfinished)"; kill -9 $pid; }
wait $pid 2>/dev/null
echo "captures: $(ls "$RUN" | grep -c MW1S); flags: $(ls "$RUN" | grep -E '^(NOPATCH|KEYFAIL|ITEMFAIL)' | tr '\n' ' ')"
ls -l "$RUN/cap" | tail -n +2
