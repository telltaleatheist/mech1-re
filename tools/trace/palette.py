"""The colour each palette code shows in MW1's 320x200 mode (BIOS mode 0Dh),
for the trace frames (docs/phase5/NO_EMULATION_PLAN.md, G1b).

Written here on its own, from the card's behaviour, and NOT from the port's
C++ (src/Video/Canvas.cpp, PaletteCodeColour): the two are checked against
each other over every code (test_palette.py, ctest palette_rule).

In the 200-line modes the EGA drives a CGA-type monitor: of each attribute
palette register's six bits, the monitor sees only red (bit 2), green (bit 1),
blue (bit 0) and intensity (bit 4), and shows the sixteen CGA colours, where
dim yellow is shown as brown.
"""

# The CGA colours by IRGB (intensity, red, green, blue), as 8-bit RGB.
CGA = (
    (0x00, 0x00, 0x00), (0x00, 0x00, 0xAA), (0x00, 0xAA, 0x00), (0x00, 0xAA, 0xAA),
    (0xAA, 0x00, 0x00), (0xAA, 0x00, 0xAA), (0xAA, 0x55, 0x00), (0xAA, 0xAA, 0xAA),
    (0x55, 0x55, 0x55), (0x55, 0x55, 0xFF), (0x55, 0xFF, 0x55), (0x55, 0xFF, 0xFF),
    (0xFF, 0x55, 0x55), (0xFF, 0x55, 0xFF), (0xFF, 0xFF, 0x55), (0xFF, 0xFF, 0xFF),
)


def code_colour(code):
    """(r, g, b) for one palette register's value (0..255)."""
    if not 0 <= code <= 255:
        raise ValueError(f"palette code {code} is not a byte")
    irgb = (8 if code & 0x10 else 0) | (code & 7)
    return CGA[irgb]


def rgb_palette(codes):
    """The frame's 17 palette codes (16 colours, then the border) as 51 bytes of RGB."""
    if len(codes) != 17:
        raise ValueError(f"{len(codes)} palette codes, expected 17")
    return bytes(v for code in codes for v in code_colour(code))
