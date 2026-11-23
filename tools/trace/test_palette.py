#!/usr/bin/env python3
"""The trace tools' palette rule (palette.py) against the port's
(src/Video/Canvas.cpp, PaletteCodeColour, printed by tests/palette_rule.cpp):
the two are written apart and must agree on every code (G1b).

    test_palette.py PALETTE_RULE_EXE
"""
import os, subprocess, sys, unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from palette import code_colour, rgb_palette  # noqa: E402

EXE = None


class PaletteRule(unittest.TestCase):
    def test_every_code_agrees_with_the_port(self):
        out = subprocess.run([EXE], capture_output=True, text=True, check=True).stdout.split("\n")
        rows = [tuple(int(v) for v in line.split()) for line in out if line.strip()]
        self.assertEqual([r[0] for r in rows], list(range(256)))
        differ = [(code, rgb, code_colour(code)) for code, *rgb in rows if tuple(rgb) != code_colour(code)]
        self.assertEqual(differ, [], "the port and the trace tools disagree on these codes")

    def test_known_colours(self):
        self.assertEqual(code_colour(0x06), (0xAA, 0x55, 0x00))  # brown
        self.assertEqual(code_colour(0x3E), (0xFF, 0xFF, 0x55))  # bits 3 and 5 not shown: yellow
        self.assertEqual(code_colour(0x17), (0xFF, 0xFF, 0xFF))
        with self.assertRaises(ValueError):
            code_colour(256)

    def test_rgb_palette(self):
        self.assertEqual(rgb_palette(bytes(range(17)))[:6], bytes((0, 0, 0, 0, 0, 0xAA)))
        self.assertEqual(len(rgb_palette(bytes(17))), 51)
        with self.assertRaises(ValueError):
            rgb_palette(bytes(16))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    EXE = sys.argv.pop(1)
    unittest.main()
