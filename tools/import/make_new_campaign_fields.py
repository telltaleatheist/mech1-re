#!/usr/bin/env python3
"""Writes src/Rules/new_campaign_fields.cpp from the save codec
(src/Original/Campaign/MW_MAIN_SAVE_FORMAT.cpp): each field of the save
block by the codec's name, with its offset, length and form, as
new_campaign.json names them. Refuses a codec whose fields don't cover the
block (CAMPAIGN_SAVE_BYTES) exactly.

    tools/import/make_new_campaign_fields.py
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
CODEC = os.path.join(ROOT, "src", "Original", "Campaign", "MW_MAIN_SAVE_FORMAT.cpp")
OUT = os.path.join(ROOT, "src", "Rules", "new_campaign_fields.cpp")
BLOCK_BYTES = 0x718
HEADERS = os.path.join(ROOT, "src", "include", "mechwarrior")


def owner_instances():
    """The campaign state's owner instances (plan slice B2): a field the codec
    reads as campaignEconomy.campaignFundsLow is named campaignFundsLow, as it
    was before its owner held it."""
    found = set()
    for name in os.listdir(HEADERS):
        if name.endswith(".h"):
            with open(os.path.join(HEADERS, name)) as f:
                found.update(re.findall(r"/\* Campaign state owned by [\w.]+ \(plan, slice B2\)\. \*/\nstruct (?:\w+)\n\{\n.*?\n\};\nextern \w+ (\w+);", f.read(), re.S))
    return found


def field_name(path, owners):
    first, _, rest = path.partition(".")
    return rest if rest and first in owners else path


def fields(codec):
    encode = codec[codec.index("void MwEncodeCampaignExchange"):codec.index("void MwDecodeCampaignExchange")]
    found = {}
    owners = owner_instances()
    for m in re.finditer(r"bytes\[0x([0-9A-F]{4}) ?(\+1|\+i\*2|\+i\*2\+1)?\]=\(?(?:uint8_t\))?\(?([\w.]+)", encode):
        entry = found.setdefault(int(m.group(1), 16), {"name": field_name(m.group(3), owners), "forms": set()})
        entry["forms"].add(m.group(2) or "")
    for m in re.finditer(r"memcpy\(bytes\+0x([0-9A-F]{4}) ?,([\w.]+),", encode):
        found[int(m.group(1), 16)] = {"name": field_name(m.group(2), owners), "forms": {"memcpy"}}
    offsets = sorted(found)
    out = []
    for i, offset in enumerate(offsets):
        length = (offsets[i + 1] if i + 1 < len(offsets) else BLOCK_BYTES) - offset
        forms = found[offset]["forms"]
        kind = ("Bytes" if "memcpy" in forms else "Words" if "+i*2" in forms
                else "Word" if "+1" in forms else "Byte")
        if (kind == "Byte" and length != 1) or (kind == "Word" and length != 2) or (kind == "Words" and length % 2):
            sys.exit(f"{found[offset]['name']} at {offset:04X}: {kind} over {length} bytes")
        out.append((found[offset]["name"], offset, length, kind))
    if not out or out[0][1] != 0:
        sys.exit("the codec's fields don't start the block")
    return out


def main():
    with open(CODEC) as f:
        rows = fields(f.read())
    with open(OUT, "w") as f:
        f.write("// Written by tools/import/make_new_campaign_fields.py from the save codec; don't edit.\n")
        f.write('#include "NewCampaignFields.h"\n\nnamespace mw1o::rules {\n\nnamespace {\n')
        f.write("constexpr NewCampaignField kFields[] = {\n")
        for name, offset, length, kind in rows:
            f.write(f'    {{"{name}", 0x{offset:04X}, {length}, NewCampaignField::Kind::{kind}}},\n')
        f.write("};\n}  // namespace\n\n")
        f.write("std::span<const NewCampaignField> new_campaign_fields() { return kFields; }\n\n}  // namespace mw1o::rules\n")
    print(f"{len(rows)} fields -> {os.path.relpath(OUT, ROOT)}")


if __name__ == "__main__":
    main()
