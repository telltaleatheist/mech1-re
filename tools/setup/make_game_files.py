#!/usr/bin/env python3
"""Writes src/Setup/GameFilesTable.cpp: the game's files the program opens,
with the size and SHA-256 they have in the two copies the project knows.

    python3 -I tools/setup/make_game_files.py GAME_1990 GAME_1989 > src/Setup/GameFilesTable.cpp

Reads the two folders, writes names, sizes and hashes: never the game's bytes.
BTECH.EXE and MW_MAIN.EXE are not in the list (each has its own check). FILES
below is the list, with where each name is opened:

  import  read once, for the story; play never opens it
  start   opened at every start of the campaign
  play    opened in every campaign, or every battle
  some    opened only in some plays or battles, or under a setting (the terrain
          detail; the 'Mechs that fight; the ending)

51 of them were seen opened on 2026-11-24, in the 12 event scripts on the
1990-01-22 release; the rest are there by reading the named function.
HUD_CYAN.BMP is not: BtechLoadCockpitHudResources (BTECH_0810_COCKPIT.cpp) asks
for it only when the display isn't an EGA, and this program's always is
(QueryEgaInfo in SDLBackend_CombatPlatform.cpp; BtechDetectVideoAdapter stops on
any other answer).
"""
import hashlib, os, sys

CAMPAIGN = "the campaign can't start"
BATTLE = "a battle stops"
TITLE = "the title stops"


def mech(name):
    return f"a battle with a {name} stops"


# (names, when, where it is opened, what a play without it does; "" where only less is heard)
FILES = [
    (["MW_EGA.EXE"], "import", "import_story (src/Import/Story.cpp)", "the first start can't import the story"),
    (["6X6.FNT", "FOX88.FNT"], "start", "EgaLoadFonts (MW_EGA_0800.cpp)", CAMPAIGN),
    (["MW_GPICS.BIN"], "start", "MwLoadGeneralPictureBank (MW_MAIN_081B_RESOURCES.cpp)", CAMPAIGN),
    (["MW_CPICS.BIN"], "start", "MwLoadCockpitPictureBank (MW_MAIN_081B_RESOURCES.cpp)", CAMPAIGN),
    (["MW_2PICS.BIN"], "start", "MwLoadSecondaryPictureBank (MW_MAIN_081B_RESOURCES.cpp)", CAMPAIGN),
    (["MW_APICS.BIN"], "start", "MwLoadAlternatePictureBank (MW_MAIN_081B_RESOURCES.cpp)", TITLE),
    (["MW_TPICS.BIN"], "start", "MwLoadTPictureBank (MW_MAIN_081B_RESOURCES.cpp)", TITLE),
    (["MECH.MUS"], "start", "MwLoadMechMusic (MW_MAIN_081B_STARTUP.cpp)", "the title has no music"),
    (["MW_PICS.BIN"], "play", "MwLoadIndexedPicture (MW_MAIN_081B_RESOURCES.cpp)", "the campaign stops"),
    (["MW_1PICS.BIN"], "play", "MwLoadPrimaryPictureBank (MW_MAIN_081B_RESOURCES.cpp)", "the campaign stops"),
    (["MECHBAR.MUS", "DEATH.MUS", "LAUNCH.MUS"], "play", "MwLoadBarDeathAndLaunchMusic (MW_MAIN_081B_STARTUP.cpp)",
     "the campaign lacks music"),
    (["MW_FPICS.BIN"], "some", "MwLoadFPictureBank (MW_MAIN_081B_RESOURCES.cpp), the ending", "the ending stops"),
    (["VICTORY.MUS"], "some", "MwLoadVictoryMusic (MW_MAIN_081B_STARTUP.cpp), the ending", "the ending has no music"),
    (["ARCTIC.PAL", "DESERT.PAL", "TROPIC.PAL", "DMGPAL.PAL", "6X6B.FNT", "PROMPT.BMP", "BTECHSND.SND"], "play",
     "BtechInitializeCombatRuntime (BTECH_0810_STARTUP.cpp)", BATTLE),
    (["POINT.BMP", "SM_MECHS.BMP", "STRUTS.BMP", "COCKPIT.BMP", "HUD_NUMS.BMP"], "play",
     "BtechLoadCockpitHudResources (BTECH_0810_COCKPIT.cpp)", BATTLE),
    (["STATUS.SCR"], "play", "BtechShowCombatStageBriefing (BTECH_0810_BRIEFING.cpp)", BATTLE),
    (["MAP.SCR"], "some", "BtechRefreshLanceOrderScreen (BTECH_17FB_SCREEN.cpp), the order screen", "the order screen stops a battle"),
    (["SNARIO.DAT"], "play", "BtechLoadScenarioAndWorld (BTECH_0810_WORLD.cpp)", BATTLE),
    (["TERPCK.TBL", "TERPCK.GI", "OTHPCK.TBL"], "play", "BtechLoadCombatChassisResourceBanks (BTECH_0810_WORLD.cpp)", BATTLE),
    (["LIGHT.SCR", "MEDIUM.SCR", "HEAVY.SCR"], "some",
     "BtechRefreshCockpitViewportAndGauges (BTECH_0810_COCKPIT.cpp), by the 'Mech", "a battle in some 'Mechs stops"),
]
for code, name in [("LOC", "Locust"), ("JEN", "Jenner"), ("PHA", "Phoenix Hawk"), ("SHA", "Shadow Hawk"),
                   ("RIF", "Rifleman"), ("HAM", "Warhammer"), ("MAR", "Marauder"), ("BMA", "BattleMaster")]:
    FILES.append(([code + "PCK.TBL"], "some", "BtechLoadCombatChassisResourceBanks (BTECH_0810_WORLD.cpp), by the 'Mechs", mech(name)))
FILES.append((["LOC.BMP", "JEN.BMP", "PHO.BMP", "SHA.BMP", "RIF.BMP", "WAR.BMP", "MAR.BMP", "BAT.BMP"], "some",
              "BtechUpdateMechInspectionScreen (BTECH_0810_INSPECTION.cpp), by the 'Mech", "a look at some 'Mechs in a battle stops"))
for detail, letter in [("high", ""), ("medium", "B")]:
    FILES.append(([f"TILE{n}{letter}.{x}" for n in range(16) for x in ("WLD", "GRD")], "some",
                  f"BtechLoadTerrainWorldResources (BTECH_0810_WORLD.cpp), at {detail} terrain detail",
                  f"some battles stop at {detail} terrain detail"))
# Files known to be damaged: name, the copy they were found in.
DAMAGED = ["PHAPCK.TBL"]


def read(folder, name):
    for f in os.listdir(folder):
        if f.upper() == name:
            with open(os.path.join(folder, f), "rb") as h:
                return h.read()
    sys.exit(f"{folder} has no {name}")


def entry(data):
    return f'{len(data)}, "{hashlib.sha256(data).hexdigest()}"'


def main():
    good, old = sys.argv[1:]
    names = [n for group in FILES for n in group[0]]
    assert len(names) == len(set(names)) == 117, len(names)
    print("""// The game's files the program opens, as the two copies the project knows hold
// them (the 1989 release and the one of 1990-01-22; they differ in none of these
// but the known damaged one, and in BTECH.EXE and MW_MAIN.EXE, which have their
// own checks). Written by tools/setup/make_game_files.py, which says where each
// is opened; don't edit. Names, sizes and SHA-256 only.
#include "FileCheck.h"

namespace mw1o::setup {
namespace {

using When = GameFile::When;
constexpr GameFile kFiles[] = {""")
    for group, when, where, without in FILES:
        print(f"    // {where}: {when}")
        for name in group:
            data = read(good, name)
            if name not in DAMAGED and read(old, name) != data:
                sys.exit(f"{name} differs between the copies")
            print(f'    {{"{name}", {entry(data)}, When::k{when.capitalize()}, "{without}"}},')
    print("""};
constexpr KnownDamage kDamaged[] = {""")
    for name in DAMAGED:
        assert read(old, name) != read(good, name)
        print(f'    {{"{name}", {entry(read(old, name))}}},')
    print("""};

}  // namespace

std::span<const GameFile> game_files() { return kFiles; }
std::span<const KnownDamage> known_damage() { return kDamaged; }

}  // namespace mw1o::setup""")


if __name__ == "__main__":
    main()
