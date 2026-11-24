#!/usr/bin/env python3
"""schema.py's exchange fields are the save format's, and they cover it.

- The encoder (MwEncodeCampaignExchange, src/Original/Campaign/
  MW_MAIN_SAVE_FORMAT.cpp) is parsed statement by statement. Its four forms
  are a byte, a little-endian word (a low and a high store), a loop of words
  and a block copy; any other statement fails the test, so a new form can't
  quietly fall out of the schema.
- The schema's table is exactly the parsed list: names, offsets and kinds.
- The fields tile 0..0x718: no gap, no overlap, words on whole words, and the
  campaign_exchange.h offsets it asserts fall on the fields' starts.
- The record splits (salvage, unit damage) are the structs' own fields, in
  order, with the structs' sizes.
- Decoding a block and encoding the fields gives the block back, so every
  byte is in a field.

    python3 -I test_schema.py
"""
import os, random, re, sys, unittest

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import schema  # noqa: E402

ENCODER = os.path.join(ROOT, "src", "Original", "Campaign", "MW_MAIN_SAVE_FORMAT.cpp")
INCLUDE = os.path.join(ROOT, "src", "include", "mechwarrior")

HEX = r"0x[0-9A-Fa-f]+"
NAME = r"[A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)*"
BYTE = re.compile(rf"bytes\[({HEX})\]=({NAME});$")
LOW = re.compile(rf"bytes\[({HEX})\]=\(uint8_t\)({NAME});$")
HIGH = re.compile(rf"bytes\[({HEX}) ?\+1\]=\(uint8_t\)\(({NAME})>>8\);$")
LOOP = re.compile(rf"for \(size_t i=0;i<sizeof\(({NAME})\)/sizeof\(({NAME})\[0\]\);\+\+i\)$")
LOOP_LOW = re.compile(rf"bytes\[({HEX}) ?\+i\*2\]=\(uint8_t\)({NAME})\[i\];$")
LOOP_HIGH = re.compile(rf"bytes\[({HEX}) ?\+i\*2\+1\]=\(uint8_t\)\(({NAME})\[i\]>>8\);$")
COPY = re.compile(rf"memcpy\(bytes\+({HEX}),({NAME}),sizeof\(({NAME})\)\);$")


def leaf(name):
    """The field's name: a member path's last part (pilotIdentityStorage.roster.x is x)."""
    return name.rsplit(".", 1)[-1]


def encoder_body(path=ENCODER):
    text = open(path, encoding="utf-8").read()
    head = re.search(r"void MwEncodeCampaignExchange\([^)]*\)\s*\{", text)
    if not head:
        raise AssertionError("MwEncodeCampaignExchange not found")
    depth, at = 1, head.end()
    for i in range(at, len(text)):
        depth += {"{": 1, "}": -1}.get(text[i], 0)
        if depth == 0:
            return text[at:i]
    raise AssertionError("MwEncodeCampaignExchange has no end")


def parse_encoder(path=ENCODER):
    """[(name, offset, kind)] in the encoder's order; kind u8, u16, u16[] or bytes. Fails on any other statement."""
    lines = [l.strip() for l in encoder_body(path).splitlines()]
    lines = [l for l in lines if l]
    out, i = [], 0

    def fail(why):
        raise AssertionError(f"encoder line {i + 1}: {why}: {lines[i]!r}")

    while i < len(lines):
        line = lines[i]
        if m := BYTE.match(line):
            out.append((leaf(m[2]), int(m[1], 16), "u8"))
            i += 1
        elif m := LOW.match(line):
            h = HIGH.match(lines[i + 1]) if i + 1 < len(lines) else None
            if not h or int(h[1], 16) != int(m[1], 16) or h[2] != m[2]:
                fail("a low byte with no matching high byte")
            out.append((leaf(m[2]), int(m[1], 16), "u16"))
            i += 2
        elif m := LOOP.match(line):
            if m[1] != m[2] or lines[i + 1:i + 2] != ["{"] or lines[i + 4:i + 5] != ["}"]:
                fail("a loop that isn't one word store over its own array")
            low, high = LOOP_LOW.match(lines[i + 2]), LOOP_HIGH.match(lines[i + 3])
            if not low or not high or low[2] != m[1] or high[2] != m[1] or int(low[1], 16) != int(high[1], 16):
                i += 2
                fail("a loop body that isn't a word's low and high byte")
            out.append((leaf(m[1]), int(low[1], 16), "u16[]"))
            i += 5
        elif m := COPY.match(line):
            if m[2] != m[3]:
                fail("a copy whose size isn't its source's")
            out.append((leaf(m[2]), int(m[1], 16), "bytes"))
            i += 1
        else:
            fail("not one of the encoder's four forms")
    return out


def constants():
    """Every NAME = 0xHH or NAME = decimal in the campaign headers' enums."""
    values = {}
    for name in os.listdir(INCLUDE):
        if name.startswith("campaign") and name.endswith(".h"):
            for m in re.finditer(r"\b([A-Z][A-Z0-9_]+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)\b", open(os.path.join(INCLUDE, name)).read()):
                values.setdefault(m[1], int(m[2], 0))
    return values


def struct_fields(struct, header):
    """[(field, bytes)] of a typedef struct of uint8_t fields, from its header."""
    text = open(os.path.join(INCLUDE, header), encoding="utf-8").read()
    m = re.search(r"typedef struct " + struct + r"\s*\{(.*?)\}\s*" + struct + ";", text, re.S)
    if not m:
        raise AssertionError(f"{struct} not found in {header}")
    known, out = constants(), []
    for decl in m[1].split(";"):
        decl = decl.strip()
        if not decl:
            continue
        if not decl.startswith("uint8_t "):
            raise AssertionError(f"{struct}: not a uint8_t field: {decl!r}")
        for part in decl[len("uint8_t "):].split(","):
            f = re.fullmatch(r"\s*([A-Za-z_]\w*)(?:\[([A-Za-z0-9_]+)\])?\s*", part)
            if not f:
                raise AssertionError(f"{struct}: can't read {part!r}")
            count = 1 if f[2] is None else (int(f[2], 0) if f[2][0].isdigit() else known[f[2]])
            out.append((f[1], count))
    return out


class ExchangeSchema(unittest.TestCase):
    def test_parser_refuses_an_unknown_form(self):
        with self.assertRaises(AssertionError):
            import tempfile
            with tempfile.TemporaryDirectory() as d:
                path = os.path.join(d, "f.cpp")
                with open(path, "w") as f:
                    f.write("void MwEncodeCampaignExchange(uint8_t b[1])\n{\n    bytes[0x0000]=x;\n    bytes[0x0001]=y+1;\n}\n")
                parse_encoder(path)

    def test_table_is_the_encoders(self):
        parsed = parse_encoder()
        table = [(f.name, f.offset, f.kind) for f in schema.EXCHANGE_FIELDS]
        self.assertEqual(table, parsed)

    def test_fields_tile_the_block(self):
        at = 0
        for f in schema.EXCHANGE_FIELDS:
            self.assertEqual(f.offset, at, f"{f.name} starts at {f.offset:#06x}, the field before ends at {at:#06x}")
            self.assertGreater(f.size, 0, f.name)
            if f.kind in ("u16", "u16[]"):
                self.assertEqual(f.size % 2, 0, f"{f.name}: a word field of odd size")
            at += f.size
        self.assertEqual(at, schema.EXCHANGE_BYTES)

    def test_header_offsets_fall_on_fields(self):
        text = open(os.path.join(INCLUDE, "campaign_exchange.h"), encoding="utf-8").read()
        starts = {f.offset for f in schema.EXCHANGE_FIELDS}
        asserted = [int(m[1], 16) for m in re.finditer(r"offsetof\(MwCampaignTransferData,\w+\)==(0x[0-9A-Fa-f]+)", text)]
        self.assertTrue(asserted)
        for offset in asserted:
            self.assertIn(offset, starts)

    def test_records_are_the_structs(self):
        for name, (struct, header) in schema.RECORD_STRUCTS.items():
            self.assertEqual(list(schema.RECORDS[name]), struct_fields(struct, header), name)
        damage = sum(n for _, n in schema.RECORDS["ownedUnitDamage"])
        self.assertEqual(damage, constants()["CAMPAIGN_DAMAGE_BYTES"])
        body = 0
        for field, n in schema.RECORDS["ownedUnitDamage"]:
            if field == "bodyDamageTiers":
                self.assertEqual(body, constants()["CAMPAIGN_BODY_DAMAGE_OFFSET"])
            body += n
        for f in schema.EXCHANGE_FIELDS:
            if f.name in schema.RECORDS:
                self.assertEqual(f.size % sum(n for _, n in schema.RECORDS[f.name]), 0, f.name)

    def test_a_record_difference_names_its_field(self):
        golden = "00" * schema.record_size("ownedUnitDamage")
        ours = "00" * 6 + "02" + "00" * (schema.record_size("ownedUnitDamage") - 7)
        self.assertEqual(schema.difference("exchange.ownedUnitDamage.3", golden, ours),
                         "exchange.ownedUnitDamage.3 byte 6 (actuatorDamageTiers[1]): golden 0x00 ours 0x02")

    def test_round_trip(self):
        rng = random.Random(0x718)
        for _ in range(20):
            block = bytes(rng.randrange(256) for _ in range(schema.EXCHANGE_BYTES))
            self.assertEqual(schema.encode_exchange(schema.exchange_fields(block, None)), block)

    def test_hashed_fields_hold_no_bytes(self):
        block = bytes(range(256)) * 8
        fields = schema.exchange_fields(block[:schema.EXCHANGE_BYTES], b"k" * 32)
        for name in schema.HASHED:
            value = fields["exchange." + name]
            self.assertRegex(value, r"^[0-9a-f]{12}$", name)


if __name__ == "__main__":
    unittest.main()
