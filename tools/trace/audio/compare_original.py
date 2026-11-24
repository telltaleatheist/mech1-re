#!/usr/bin/env python3
"""Compares a capture from the ORIGINAL under DOSBox-X
(tools/capture/run_audio_wsl.sh) with one of the port's sound-port goldens
(level 3 of docs/phase5/AUDIO_PLAN.md).

    compare_original.py CAPTURE.dro  traces/music.adlib.jsonl.gz
    compare_original.py midi.oss     traces/music.mt32.jsonl.gz
    compare_original.py CAPTURE.wav  traces/music.speaker.jsonl.gz

The adlib and mt32 goldens left with those paths in S8: take them from the
history before it (6aebf22) to repeat the AdLib and MIDI comparisons.

The original and the port don't spend time the same way on a script (the
original's empty polls take almost no emulated time), so the comparison is
per section: the golden's snaps split it, and each section with music is
found in the capture by its opening and compared from there, in order, until
either side ends.

- AdLib (.dro): the key-on sequence, channel-free: (F-number, block) for each
  melodic key-on and the rhythm bits for each drum hit. Channel numbers
  follow voice allocation and DRO drops writes that don't change a register,
  so register-level order isn't comparable across a different history; the
  notes are. Onset gaps are compared in milliseconds (DRO's resolution).
- Speaker (.wav, DX-CAPTURE /A): the notes. The driver leaves the speaker
  gated on and plays a note by writing PIT channel 2's divisor; a rest is
  divisor 20 (about 60 kHz, inaudible). The port's notes are the divisor in
  force after each instant's writes, each time it turns audible or changes;
  the original's are found in the WAV by the square wave's transitions: a
  note runs while their spacing holds steady, and starts at its first. Notes
  compare as semitones (the WAV can't give a divisor exactly), with onset
  gaps in milliseconds.
- MIDI (.oss): the byte stream. DOSBox-X sends its own reset first (Bn 78,
  79, 7B on all 16 channels) and doesn't pass on FF (system reset); both are
  set aside.

Prints the numbers only: no notes, no bytes of the music. Exit 0 when every
section that both sides reached matches, 1 otherwise.
"""
import argparse
import array
import gzip
import math
import json
import statistics
import struct
import sys
import wave

PIT_HZ = 1193182
WINDOW = 12  # notes or bytes that identify a section's opening
# MIDI between two sections: the driver's all-notes-off sweep (Bn 7B 00 on 16
# channels) and a few bytes on the screens between.
MIDI_SLACK = 64


def golden(path):
    """The golden's header device, its events and its snaps (name, index)."""
    with gzip.open(path, "rt") as f:
        lines = f.read().splitlines()
    return json.loads(lines[0]), [json.loads(l) for l in lines[1:]]


def notes(writes):
    """[(ms, kind, a, b, tag)] from [(ms, register, value, tag)]: melodic
    key-ons (rising key bit) as F-number and block, drum hits as rhythm bits."""
    regs, out = {}, []
    for ms, reg, value, tag in writes:
        old = regs.get(reg, 0)
        regs[reg] = value
        if 0xB0 <= reg <= 0xB8 and value & 0x20 and not old & 0x20:
            channel = reg - 0xB0
            out.append((ms, "n", ((value & 3) << 8) | regs.get(0xA0 + channel, 0), (value >> 2) & 7, tag))
        elif reg == 0xBD and value & 0x20:
            hits = value & 0x1F & ~(old & 0x1F if old & 0x20 else 0)
            if hits:
                out.append((ms, "r", hits, 0, tag))
    return out


def dro_writes(path):
    data = open(path, "rb").read()
    if data[:8] != b"DBRAWOPL" or struct.unpack_from("<HH", data, 8) != (2, 0):
        sys.exit(f"{path}: not a DROv2 file")
    pairs = struct.unpack_from("<I", data, 12)[0]
    short, long_, length = data[23], data[24], data[25]
    codemap, pos, ms, out = data[26:26 + length], 26 + length, 0, []
    for _ in range(pairs):
        if pos + 2 > len(data):
            break
        code, value = data[pos], data[pos + 1]
        pos += 2
        if code == short:
            ms += value + 1
        elif code == long_:
            ms += (value + 1) << 8
        else:
            out.append((ms, codemap[code & 0x7F] | (0x100 if code & 0x80 else 0), value, None))
    return out


def sections(events, snaps, items):
    """Splits items (each with its golden event index) at the snaps."""
    bounds = [0] + [i for _, i in snaps] + [len(events)]
    names = ["start"] + [n for n, _ in snaps]
    out = []
    for k in range(len(bounds) - 1):
        part = [x for i, x in items if bounds[k] <= i < bounds[k + 1]]
        out.append((f"{names[k]}..{names[k + 1] if k + 1 < len(names) else 'end'}", part))
    return out


def run_length(a, b):
    k = 0
    while k < len(a) and k < len(b) and a[k] == b[k]:
        k += 1
    return k


def compare_sections(theirs, parts, unit, slack, gaps=None):
    """Finds each golden section in the original by its opening and compares
    from there. A section matches when all of it does, or when the original
    stops partway because it moved on: it ends there, or the next section's
    opening follows within `slack` items (a different time spent on a screen,
    not different music)."""
    found, used = [], 0
    for name, part, key in parts:
        if len(key) < WINDOW:
            continue
        at = next((i for i in range(used, len(theirs) - WINDOW + 1) if theirs[i:i + WINDOW] == key[:WINDOW]), None)
        m = run_length(theirs[at:], key) if at is not None else 0
        found.append((name, part, key, at, m))
        if at is not None:
            used = at + m
    ok = True
    for k, (name, part, key, at, m) in enumerate(found):
        if at is None:
            print(f"  {name}: {len(key)} {unit}; its opening isn't in the original")
            ok = False
            continue
        nxt = next((f[3] for f in found[k + 1:] if f[3] is not None), None)
        note = ""
        if m < len(key):
            if at + m == len(theirs):
                note = " (the original's capture ends there)"
            elif nxt is not None and 0 <= nxt - (at + m) <= slack:
                note = " (the original moves on to the next section there)"
            else:
                ok = False
        timing = ""
        if gaps and m > 1:
            g = gaps(at, part, m)
            timing = f"; onset gaps differ by median {statistics.median(g)} ms, at most {max(map(abs, g))} ms"
        print(f"  {name}: {m} of {len(key)} {unit} match in order{note}{timing}")
    return ok


def compare_adlib(capture, events, snaps):
    writes, reg, items = [], None, []
    for i, e in enumerate(events):
        if e[1] == "w" and e[2] == 0x388:
            reg = e[3]
        elif e[1] == "w" and e[2] == 0x389 and reg is not None:
            writes.append((e[0] * 1000 // PIT_HZ, reg, e[3], i))
    ours = notes(writes)  # tagged with the golden event index
    theirs = notes(dro_writes(capture))
    print(f"original: {len(theirs)} key-ons; port: {len(ours)}")
    gaps = lambda at, part, m: [(theirs[at + k + 1][0] - theirs[at + k][0]) - (part[k + 1][0] - part[k][0])
                                for k in range(m - 1)]
    return compare_sections([n[1:4] for n in theirs], [(name, part, [n[1:4] for n in part])
                            for name, part in sections(events, snaps, [(n[4], n) for n in ours])],
                            "key-ons", 0, gaps)


SPEAKER_REST = 20  # the driver's rest divisor


def semitone(hz):
    return round(12 * math.log2(hz / 440))


MERGE_MS = 15  # the same note picked up again within this is one note


def speaker_segments(segments, tag=lambda seg: None):
    """[(ms, semitone, tag, periods)] from [(start ms, end ms, semitone,
    ...)], both sides alike: the same note again within MERGE_MS is one note
    (a dropped edge in the WAV, a short rest in the driver), and a note under
    1.5 periods is left out (the WAV can't show it)."""
    merged = []
    for seg in segments:
        if merged and merged[-1][2] == seg[2] and seg[0] - merged[-1][1] < MERGE_MS:
            merged[-1] = (merged[-1][0], seg[1]) + merged[-1][2:]
        else:
            merged.append(seg)
    out = []
    for seg in merged:
        periods = (seg[1] - seg[0]) * 440 * 2 ** (seg[2] / 12) / 1000
        if periods >= 1.5:
            out.append((seg[0], seg[2], tag(seg), periods))
    return out


def port_speaker_notes(events):
    """The port's notes: the channel 2 divisor in force after each instant's
    writes (43h starts a low/high pair at 42h); each audible one a segment
    until the next change."""
    lo, by_time = None, []
    for i, e in enumerate(events):
        if e[1] != "w":
            continue
        if e[2] == 0x43:
            lo = None
        elif e[2] == 0x42:
            if lo is None:
                lo = e[3]
                continue
            d, lo = lo | e[3] << 8, None
            if by_time and by_time[-1][0] == e[0]:
                by_time[-1] = (e[0], d, by_time[-1][2])
            else:
                by_time.append((e[0], d, i))
    segs = []
    for k, (t, d, i) in enumerate(by_time):
        if d in (SPEAKER_REST, 0) or k + 1 == len(by_time):
            continue
        end = by_time[k + 1][0]
        segs.append((t * 1000 / PIT_HZ, end * 1000 / PIT_HZ, semitone(PIT_HZ / d), i))
    return speaker_segments(segs, lambda seg: seg[3])


def wav_speaker_notes(path):
    """The original's notes from its WAV: the square wave's transitions (with
    hysteresis, around a slow mean); a segment runs while the half-periods
    stay within 5% of its first."""
    w = wave.open(path, "rb")
    rate, channels, width = w.getframerate(), w.getnchannels(), w.getsampwidth()
    if width != 2:
        sys.exit(f"{path}: 16-bit WAV expected")
    samples = array.array("h", w.readframes(w.getnframes()))
    if sys.byteorder == "big":
        samples.byteswap()
    mono = samples[::channels]
    peak = sorted(abs(x) for x in mono[::97])
    high = peak[int(len(peak) * 0.99)] if peak else 0
    if not high:
        return []
    h = high * 0.15
    edges, mean, state = [], 0.0, 0
    for k, x in enumerate(mono):
        mean += (x - mean) * 0.0005
        v = x - mean
        if state <= 0 and v > h:
            if state < 0:
                edges.append(k)
            state = 1
        elif state >= 0 and v < -h:
            if state > 0:
                edges.append(k)
            state = -1
    segs, k = [], 0
    while k + 2 < len(edges):
        half = edges[k + 1] - edges[k]
        j = k + 1
        while j + 1 < len(edges) and abs((edges[j + 1] - edges[j]) - half) <= max(1, half * 0.05):
            j += 1
        if j - k >= 3:
            span = (edges[j] - edges[k]) / (j - k)
            segs.append((edges[k] * 1000 / rate, (edges[j] + span) * 1000 / rate, semitone(rate / (2 * span))))
        k = j
    return speaker_segments(segs)


SHORT = 5  # periods: a note this short may be missing from either side


def compare_speaker(capture, events, snaps):
    """Each golden section is found in the WAV by its first long notes, then
    walked in step: equal notes match; a short note (under SHORT periods) on
    one side only is skipped and counted (the WAV can't always show a note
    that brief); any other difference ends the section's match."""
    ours = port_speaker_notes(events)
    theirs = wav_speaker_notes(capture)
    print(f"original: {len(theirs)} notes; port: {len(ours)}")
    longs = [k for k, n in enumerate(theirs) if n[3] >= SHORT]
    ok, used = True, 0
    for name, part in sections(events, snaps, [(n[2], n) for n in ours]):
        if len(part) < WINDOW:
            continue
        key = [n[1] for n in part if n[3] >= SHORT][:WINDOW]
        at = next((longs[i] for i in range(len(longs) - WINDOW + 1)
                   if longs[i] >= used and [theirs[k][1] for k in longs[i:i + WINDOW]] == key), None)
        if at is None:
            print(f"  {name}: {len(part)} notes; its opening isn't in the original")
            ok = False
            continue
        p = next(k for k, n in enumerate(part) if n[3] >= SHORT)
        t, matched, skipped, gaps, prev = at, 0, 0, [], None
        while p < len(part) and t < len(theirs):
            a, b = part[p], theirs[t]
            if a[1] == b[1]:
                if prev:
                    gaps.append((b[0] - prev[1]) - (a[0] - prev[0]))
                prev, matched, p, t = (a[0], b[0]), matched + 1, p + 1, t + 1
            elif a[3] < SHORT:
                skipped, p = skipped + 1, p + 1
            elif b[3] < SHORT:
                skipped, t = skipped + 1, t + 1
            else:
                break
        used = t
        note = ""
        if p < len(part):
            if t == len(theirs):
                note = " (the original's capture ends there)"
            else:
                ok = False
        timing = f"; onset gaps differ by median {statistics.median(gaps):.0f} ms, at most {max(map(abs, gaps)):.0f} ms" if gaps else ""
        print(f"  {name}: {matched} of {len(part)} notes match in order (short notes on one side only: {skipped})"
              f"{note}{timing}")
    return ok


def compare_midi(capture, events, snaps):
    data = open(capture, "rb").read()
    theirs = bytes(data[i + 1] for i in range(0, len(data) - 3, 4) if data[i] == 5)
    reset = bytes(x for c in range(16) for x in (0xB0 | c, 0x78, 0, 0xB0 | c, 0x79, 0, 0xB0 | c, 0x7B, 0))
    if theirs.startswith(reset):
        theirs = theirs[len(reset):]
    items = [(i, e[3]) for i, e in enumerate(events) if e[1] == "w" and e[2] == 0x330 and e[3] != 0xFF]
    print(f"original: {len(theirs)} MIDI bytes; port: {len(items)}")
    return compare_sections(theirs, [(name, part, bytes(part)) for name, part in sections(events, snaps, items)],
                            "bytes", MIDI_SLACK)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("capture", help="the original's .dro or midi.oss")
    ap.add_argument("golden", help="a traces/*.adlib or *.mt32 golden")
    a = ap.parse_args()
    header, events = golden(a.golden)
    snaps = [(e[2], i) for i, e in enumerate(events) if e[1] == "snap"]
    if a.capture.lower().endswith(".wav"):
        if header["device"] != "speaker":
            sys.exit("a .wav compares with a speaker golden")
        ok = compare_speaker(a.capture, events, snaps)
    elif a.capture.lower().endswith(".dro"):
        if header["device"] != "adlib":
            sys.exit("a .dro compares with an adlib golden")
        ok = compare_adlib(a.capture, events, snaps)
    else:
        if header["device"] != "mt32":
            sys.exit("a MIDI capture compares with an mt32 golden")
        ok = compare_midi(a.capture, events, snaps)
    print("match" if ok else "MISMATCH")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
