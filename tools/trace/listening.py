#!/usr/bin/env python3
"""The listening renders (docs/phase5/NO_EMULATION_PLAN.md, slice S3): every
song, to WAV files, with a one-page sheet. Nothing is played aloud: the runs
are headless with SDL's dummy audio driver, and the sound comes from the
port's PCM dump.

    listening.py --port BUILD/mechwarrior --game DIR --work DIR --out DIR
                 [--seconds N]

The feed script listening/songs.keys (events/scripts/songs.keys with each song
given three minutes) asks the game's own routines for each song in turn
(record.py --feed-calls), once per device:
- soundfont: music=soundfont, what the rebuild plays (src/Audio/Music);
- speaker: the port's speaker path, whose notes match the original's
  (docs/phase5/AUDIO_PLAN.md, level 3), as the reference. The AdLib and MT-32
  paths left in S8, after the listening (S4).
Each run's song marks (<dump>.songs, written beside MECHWARRIOR_AUDIO_PCM)
cut it into songs, named by data/music/instruments.json's heads. Each song is
the feed's own start of it, up to the next song or N seconds, with trailing
silence trimmed. Out: NN-SONG-DEVICE.wav (48 kHz, 16-bit) and LISTENING.md.
"""
import argparse, array, json, os, struct, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
SONGS_SCRIPT = os.path.join(HERE, "listening", "songs.keys")
TABLE = os.path.join(ROOT, "data", "music", "instruments.json")
PIT_HZ, PCM_HZ = 1193182, 48000
# The listening order: the campaign's songs, then the cues.
ORDER = ("MECH", "MECHBAR", "VICTORY", "LAUNCH", "DEATH")
JUDGE = {
    "MECH": "the title: each part's instrument against the original's; strings, brass and timpani balance; drums",
    "MECHBAR": "the bar: the bass, the echo and pad parts; balance; drums",
    "VICTORY": "victory: horns and brass; the piccolo line; balance; drums",
    "LAUNCH": "the launch cue: timpani and strings; balance (no drums)",
    "DEATH": "the death cue: strings, violin and English horn; balance (no drums)",
}


def run(port, game, work, device):
    """One run of songs.keys on a device: its PCM (floats), channel count and song marks."""
    base = os.path.join(work, device)
    os.makedirs(base, exist_ok=True)
    ini = os.path.join(base, "sound.ini")
    with open(ini, "w") as f:
        f.write("combat_tick_ms=60\ncombat_speed=1\n")
        f.write("music=soundfont\n" if device == "soundfont" else f"sound_system={device}\n")
    pcm = os.path.join(base, "sound.pcm")
    # The feed's song: tokens need the event log on (they are its test aids).
    env = dict(os.environ, MECHWARRIOR_TIMING_CONFIG=ini, MECHWARRIOR_EVENT_LOG=os.path.join(base, "events.log"),
               MECHWARRIOR_AUDIO_PCM=pcm, MECHWARRIOR_DEBUG_VIRTUAL_CLOCK="1", SDL_AUDIO_DRIVER="dummy")
    rec = subprocess.run([sys.executable, os.path.join(HERE, "record.py"), "--exe", port, "--game", game,
                          "--script", SONGS_SCRIPT, "--out", os.path.join(base, "trace.jsonl"),
                          "--frames", os.path.join(base, "frames"), "--work", os.path.join(base, "run"),
                          "--timeout", "900", "--feed-calls"], env=env, capture_output=True, text=True)
    if rec.returncode:
        sys.exit(f"{device}: record.py failed\n{rec.stdout[-2000:]}{rec.stderr[-2000:]}")
    with open(pcm + ".songs", encoding="utf-8") as f:
        marks = [(int(c), h) for c, h in (line.split() for line in f if line.strip())]
    samples = array.array("f")  # little-endian floats, as every host here writes them
    with open(pcm, "rb") as f:
        data = f.read()
    samples.frombytes(data[:len(data) // 4 * 4])
    channels = 1 if device == "speaker" else 2
    return samples, channels, marks


def cut(samples, channels, marks, head, seconds):
    """The feed's own start of a song (its last start in the run) up to the next mark, trimmed."""
    starts = [i for i, (_, h) in enumerate(marks) if h == head]
    if not starts:
        return None
    i = starts[-1]
    first = marks[i][0] * PCM_HZ // PIT_HZ
    frames = len(samples) // channels
    last = marks[i + 1][0] * PCM_HZ // PIT_HZ if i + 1 < len(marks) else frames
    last = min(last, frames, first + seconds * PCM_HZ)
    piece = samples[first * channels:last * channels]
    # Trailing silence (below -60 dBFS: the AdLib core idles at about -76) trimmed, keeping half a second of tail.
    end = len(piece)
    while end > 0 and abs(piece[end - 1]) < 1e-3:
        end -= 1
    end = min(len(piece), end + PCM_HZ // 2 * channels)
    return piece[:end - end % channels]


def write_wav(path, piece, channels):
    """16-bit PCM WAV; the speaker's mono is written as both channels so every file plays alike."""
    if channels == 1:
        both = array.array("f", bytes(len(piece) * 8))
        both[0::2] = piece
        both[1::2] = piece
        piece = both
    peak = max((abs(v) for v in piece), default=0.0)
    out = array.array("h", (int(max(-1.0, min(1.0, v)) * 32767) for v in piece)).tobytes()
    with open(path, "wb") as f:
        f.write(b"RIFF" + struct.pack("<I", 36 + len(out)) + b"WAVEfmt " + struct.pack("<IHHIIHH", 16, 1, 2, PCM_HZ, PCM_HZ * 4, 4, 16)
                + b"data" + struct.pack("<I", len(out)) + out)
    return peak


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", required=True, help="the port build (with data/music beside it)")
    ap.add_argument("--game", required=True, help="MW1 game folder (only read)")
    ap.add_argument("--work", required=True, help="the runs' saves and dumps (outside the repo)")
    ap.add_argument("--out", required=True, help="the WAV files and LISTENING.md")
    ap.add_argument("--seconds", type=int, default=150, help="the longest a song is cut (default 150)")
    a = ap.parse_args()
    with open(TABLE, encoding="utf-8") as f:
        heads = {s["name"]: s["head"] for s in json.load(f)["songs"]}
    devices = ["soundfont", "speaker"]
    port, game, work, out = (os.path.abspath(p) for p in (a.port, a.game, a.work, a.out))
    os.makedirs(out, exist_ok=True)
    files = {}  # (song, device) -> (file, seconds, peak)
    for device in devices:
        samples, channels, marks = run(port, game, work, device)
        for n, song in enumerate(ORDER, 1):
            piece = cut(samples, channels, marks, heads[song], a.seconds)
            if not piece:
                sys.exit(f"{device}: no start of {song} in the run")
            name = f"{n:02d}-{song}-{device}.wav"
            peak = write_wav(os.path.join(out, name), piece, channels)
            files[song, device] = (name, len(piece) / channels / PCM_HZ, peak)
            print(f"{name}: {files[song, device][1]:.1f} s, peak {peak:.2f}")
    references = [d for d in devices if d != "soundfont"]
    lines = ["# MW1 Rebuild: listening sheet (S3, first instrument table)", "",
             "Each song as the rebuild plays it (`soundfont`), with the original's own notes played through "
             + " and ".join(references) + " beside it. Note timing isn't on this sheet: the event log checks it "
             "exactly. Judge the sound: does each instrument fit, is the balance right, do the drums fit.", ""]
    lines += ["| Song | Rebuild | Beside it | Judge | Notes |", "| --- | --- | --- | --- | --- |"]
    for song in ORDER:
        name, secs, _ = files[song, "soundfont"]
        beside = ", ".join(files[song, d][0] for d in references)
        lines.append(f"| {song} | {name} ({secs:.0f} s) | {beside} | {JUDGE[song]} | |")
    lines += ["", "Per song and part, the programs are in `data/music/instruments.json` (each part's MT-32 preset is "
              "beside its program). Notes on a part (\"MECH channel 4 too bright\") are enough for S4 to act on."]
    with open(os.path.join(out, "LISTENING.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print(f"{len(files)} files and LISTENING.md in {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
