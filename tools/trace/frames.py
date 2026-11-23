#!/usr/bin/env python3
"""Turn a trace's indexed frames into PNGs for a person to look at.

The recorder copies each 320x200 indexed frame (.indices, one byte per pixel)
and its colours (.rgb: 17 RGB triples) into its --frames directory; for an
older frame with only its palette codes (.palette), palette.py gives the colours. This writes one PNG per frame and, with --sheet,
a contact sheet at half size. Frames are MW1 art: write them outside the repo.

    python3 frames.py <frames dir> <png dir> [--sheet sheet.png] [--cols 4]
"""
import argparse, glob, os, struct, sys, zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from palette import code_colour  # noqa: E402

W, H = 320, 200


def write_png(path, width, height, rgb_rows):
    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    raw = b"".join(b"\0" + row for row in rgb_rows)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


def load(indices_path):
    pixels = open(indices_path, "rb").read()
    if len(pixels) != W * H:
        sys.exit(f"{indices_path}: {len(pixels)} bytes, expected {W * H}")
    stem = indices_path[:-len(".indices")]
    if os.path.exists(stem + ".rgb"):
        rgb = open(stem + ".rgb", "rb").read()
        colours = [tuple(rgb[i * 3:i * 3 + 3]) for i in range(16)]
    else:
        colours = [code_colour(v) for v in open(stem + ".palette", "rb").read()[:16]]
    return [[colours[pixels[y * W + x] & 15] for x in range(W)] for y in range(H)]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("frames")
    ap.add_argument("png")
    ap.add_argument("--sheet")
    ap.add_argument("--cols", type=int, default=4)
    args = ap.parse_args()
    paths = sorted(glob.glob(os.path.join(args.frames, "*.indices")))
    if not paths:
        sys.exit(f"no .indices in {args.frames}")
    os.makedirs(args.png, exist_ok=True)
    images = []
    for path in paths:
        image = load(path)
        images.append(image)
        name = os.path.basename(path)[:-len(".indices")]
        write_png(os.path.join(args.png, name + ".png"), W, H, [bytes(c for px in row for c in px) for row in image])
    if args.sheet:
        cols = max(1, args.cols)
        rows = (len(images) + cols - 1) // cols
        sheet = [[(0, 0, 0)] * (W // 2 * cols) for _ in range(H // 2 * rows)]
        for n, image in enumerate(images):
            ox, oy = (n % cols) * (W // 2), (n // cols) * (H // 2)
            for y in range(H // 2):
                sheet[oy + y][ox:ox + W // 2] = image[y * 2][::2]
        write_png(args.sheet, W // 2 * cols, H // 2 * rows, [bytes(c for px in row for c in px) for row in sheet])
    print(f"{len(paths)} frames -> {args.png}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
