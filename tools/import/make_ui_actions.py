#!/usr/bin/env python3
"""Writes src/Ui/ui_actions.cpp: the menu actions data/ui names, from the
continuation enums in src/include/mechwarrior/campaign_continuations.h
(MwCampaignContinuation, MwCampaignMenuEntry). Each member CAMPAIGN_<KIND>_<NAME>
becomes "<kind>.<name>" in lower case: CAMPAIGN_CONTINUE_MECH_COMPLEX is
"continue.mech_complex". Refuses two members with one value.

    tools/import/make_ui_actions.py
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HEADER = os.path.join(ROOT, "src", "include", "mechwarrior", "campaign_continuations.h")
OUT = os.path.join(ROOT, "src", "Ui", "ui_actions.cpp")
KINDS = ("CONTINUE", "CALLBACK", "CALL")


def main():
    text = open(HEADER, encoding="utf-8").read()
    actions, seen = [], {}
    for m in re.finditer(r"\bCAMPAIGN_(CONTINUE|CALLBACK|CALL)_(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)", text):
        kind, name, value = m.group(1), m.group(2), int(m.group(3), 0)
        member = f"CAMPAIGN_{kind}_{name}"
        if value in seen:
            sys.exit(f"{member} and {seen[value]} share 0x{value:04X}")
        seen[value] = member
        actions.append((f"{kind.lower()}.{name.lower()}", member))
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write("// Written by tools/import/make_ui_actions.py from campaign_continuations.h; don't edit.\n"
                "#include \"UiActions.h\"\n\n"
                "#include \"mechwarrior/campaign_continuations.h\"\n\n"
                "namespace mw1o::ui {\n\n"
                "namespace {\n"
                "constexpr UiAction kActions[] = {\n")
        for name, member in actions:
            f.write(f'    {{"{name}", {member}}},\n')
        f.write("};\n"
                "}  // namespace\n\n"
                "std::span<const UiAction> ui_actions() { return kActions; }\n\n"
                "}  // namespace mw1o::ui\n")
    print(f"{len(actions)} actions -> {os.path.relpath(OUT, ROOT)}")


if __name__ == "__main__":
    main()
