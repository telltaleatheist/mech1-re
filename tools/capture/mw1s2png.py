#!/usr/bin/env python3
"""Converts an MWSNAP capture (MW1Snnnn.BIN, mode 0Dh) to a 320x200 PNG.

    mw1s2png.py CAPTURE.BIN OUT.PNG
"""
import struct
import sys
import zlib


def png(path, w, h, rows):
    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data))
    raw = b"".join(b"\0" + bytes(r) for r in rows)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def main():
    d = open(sys.argv[1], "rb").read()
    if d[:4] != b"MW1S" or d[5] != 0x0D:
        sys.exit(f"{sys.argv[1]}: not an MWSNAP mode 0Dh capture")
    attr = d[8:24]
    dac = d[24:24 + 768]
    planes = [d[792 + p * 8000: 792 + (p + 1) * 8000] for p in range(4)]
    rgb = [tuple(min(255, dac[a * 3 + i] * 255 // 63) for i in range(3)) for a in (x & 0x3F for x in attr)]
    rows = []
    for y in range(200):
        row = []
        for x in range(320):
            o, bit = y * 40 + x // 8, 7 - x % 8
            c = sum(((planes[p][o] >> bit) & 1) << p for p in range(4))
            row += rgb[c]
        rows.append(row)
    png(sys.argv[2], 320, 200, rows)


if __name__ == "__main__":
    main()
