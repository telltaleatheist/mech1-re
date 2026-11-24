#!/usr/bin/env python3
"""The snapshot schema (plan slice B1): every compared fact of a trace
record, by name.

A record's "fields" is an ordered object of name -> value:
- the status, as the port's status.json (record.py) or the original's memory
  (state.py) give it: campaign.*, menu.N.*, and in a battle combat.*,
  unit.S.* and objective.*;
- on a campaign snap, the campaign exchange block (0x718 bytes, the save
  format's layout, which is the original's at MW_MAIN DS:0448) as its own
  fields, exchange.<name>, named as MwEncodeCampaignExchange names them
  (src/Original/Campaign/MW_MAIN_SAVE_FORMAT.cpp). test_schema.py checks this
  table against that encoder and that the fields cover every byte.

Values: numbers; lists of numbers for word arrays; a hex string for a byte
array, and one per record for an array of records (exchange.<name>.N), whose
difference names the record's field. A field that can hold text (HASHED) is a 12-hex keyed hash, as the
menu labels are, never its bytes: no MW1 text goes into a trace.
"""
import hashlib, hmac, struct
from collections import OrderedDict, namedtuple

EXCHANGE_BYTES = 0x0718
Field = namedtuple("Field", "name offset kind size")  # kind: u8, u16, u16[] or bytes

EXCHANGE_FIELDS = [
    Field("unidentifiedField18", 0x0000, "u8", 1),
    Field("missionCategoryOrMemoryTier", 0x0001, "u8", 1),
    Field("deploymentLandscapeIndex", 0x0002, "u8", 1),
    Field("combatEnemyHeavyCount", 0x0003, "u8", 1),
    Field("combatEnemyMediumCount", 0x0004, "u8", 1),
    Field("combatEnemyLightCount", 0x0005, "u8", 1),
    Field("combatPlanetSelector", 0x0006, "u8", 1),
    Field("combatScenarioCode", 0x0007, "u8", 1),
    Field("deploymentTerrainDetailLevel", 0x0008, "u8", 1),
    Field("rosterStatus", 0x0009, "bytes", 4),
    Field("salvageEntries", 0x000D, "bytes", 16),
    Field("returnedDisabledEnemyCount", 0x001D, "u16", 2),
    Field("combatTransitionResult", 0x001F, "u8", 1),
    Field("acceptedContractActive", 0x0020, "u8", 1),
    Field("currentPlanetIndex", 0x0021, "u16", 2),
    Field("currentPlanetLandscapeIndex", 0x0023, "u16", 2),
    Field("currentPlanetHouseIndex", 0x0025, "u16", 2),
    Field("currentPlanetMarketTier", 0x0027, "u16", 2),
    Field("currentPlanetFacilityDisplayFlag", 0x0029, "u16", 2),
    Field("currentPlanetMapPositionX", 0x002B, "u16", 2),
    Field("currentPlanetMapPositionY", 0x002D, "u16", 2),
    Field("planetMarketPremiumPercent", 0x002F, "u16", 2),
    Field("calendarSubmonthUnits", 0x0031, "u16", 2),
    Field("calendarMonthIndex", 0x0033, "u16", 2),
    Field("calendarYear", 0x0035, "u16", 2),
    Field("marketRefreshUnits", 0x0037, "u16", 2),
    Field("elapsedBillingMonths", 0x0039, "u16", 2),
    Field("commanderNameBytes", 0x003B, "bytes", 10),
    Field("unidentifiedField19", 0x0045, "bytes", 2),
    Field("commanderAgeYears", 0x0047, "u16", 2),
    Field("campaignFundsLow", 0x0049, "u16", 2),
    Field("campaignFundsHigh", 0x004B, "u16", 2),
    Field("houseNetStandings", 0x004D, "u16[]", 10),
    Field("houseFavorPoints", 0x0057, "u16[]", 10),
    Field("houseGrievancePoints", 0x0061, "u16[]", 10),
    Field("playerCombatCredit", 0x006B, "u16", 2),
    Field("pilotRosterCount", 0x006D, "u16", 2),
    Field("rosterPilotIds", 0x006F, "u16[]", 8),
    Field("unavailablePilots1To42", 0x0077, "bytes", 42),
    Field("pilotDepartureFlagTail", 0x00A1, "u8", 1),
    Field("rosterPayTiers", 0x00A2, "u16[]", 8),
    Field("rosterGunneryTiers", 0x00AA, "u16[]", 8),
    Field("rosterPilotingTiers", 0x00B2, "u16[]", 8),
    Field("rosterUnknownWords", 0x00BA, "u16[]", 8),
    Field("rosterMechAssignments", 0x00C2, "u16[]", 8),
    Field("rosterPortraitIds", 0x00CA, "u16[]", 8),
    Field("dropShipMissedUpkeepPayments", 0x00D2, "u16", 2),
    Field("unpaidPilotWageMarker", 0x00D4, "u16", 2),
    Field("availableRecruitCount", 0x00D6, "u16", 2),
    Field("availableRecruitIds", 0x00D8, "u16[]", 16),
    Field("ownedUnitCount", 0x00E8, "u16", 2),
    Field("ownedUnitKinds", 0x00EA, "u16[]", 24),
    Field("ownedUnitDamage", 0x0102, "bytes", 348),
    Field("ownedAmmoQuantitiesByGroup", 0x025E, "u16[]", 72),
    Field("marketAmmoQuantitiesByGroup", 0x02A6, "u16[]", 72),
    Field("reserveAmmoQuantitiesByFamily", 0x02EE, "u16[]", 12),
    Field("ownedAmmoKindsByGroup", 0x02FA, "u16[]", 72),
    Field("marketAmmoKindsByGroup", 0x0342, "u16[]", 72),
    Field("marketUnitCount", 0x038A, "u16", 2),
    Field("marketUnitKinds", 0x038C, "u16[]", 24),
    Field("marketUnitDamage", 0x03A4, "bytes", 348),
    Field("missionOfferCount", 0x0500, "u16", 2),
    Field("missionDescriptionOffsets", 0x0502, "u16[]", 12),
    Field("missionPlanetRecordIndices", 0x050E, "u16[]", 12),
    Field("missionTargetHouseIndices", 0x051A, "u16[]", 12),
    Field("missionTypeCodes", 0x0526, "u16[]", 12),
    Field("missionEnemyHeavyCounts", 0x0532, "u16[]", 12),
    Field("missionEnemyMediumCounts", 0x053E, "u16[]", 12),
    Field("missionEnemyLightCounts", 0x054A, "u16[]", 12),
    Field("missionPaymentThousands", 0x0556, "u16[]", 12),
    Field("missionSalvagePercentages", 0x0562, "u16[]", 12),
    Field("missionAdvancePercentages", 0x056E, "u16[]", 12),
    Field("missionPaymentCeilingsThousands", 0x057A, "u16[]", 12),
    Field("missionSalvagePercentageCeilings", 0x0586, "u16[]", 12),
    Field("missionAdvancePercentageCeilings", 0x0592, "u16[]", 12),
    Field("contractNegotiationAttemptLimit", 0x059E, "u16", 2),
    Field("requestedContractPaymentThousands", 0x05A0, "u16", 2),
    Field("requestedContractSalvagePercent", 0x05A2, "u16", 2),
    Field("requestedContractAdvancePercent", 0x05A4, "u16", 2),
    Field("requestContractPaymentEnabled", 0x05A6, "u16", 2),
    Field("requestContractSalvageEnabled", 0x05A8, "u16", 2),
    Field("requestContractAdvanceEnabled", 0x05AA, "u16", 2),
    Field("unidentifiedField20", 0x05AC, "bytes", 6),
    Field("acceptedContractPaymentThousands", 0x05B2, "u16", 2),
    Field("acceptedContractSalvagePercent", 0x05B4, "u16", 2),
    Field("acceptedContractAdvancePercent", 0x05B6, "u16", 2),
    Field("remainingContractPaymentThousands", 0x05B8, "u16", 2),
    Field("houseMissionTypeLimits", 0x05BA, "bytes", 40),
    Field("audioOptionLabelIndex", 0x05E2, "u8", 1),
    Field("missionScenarioCodes", 0x05E3, "u16[]", 12),
    Field("pilotGunneryTiers", 0x05EF, "bytes", 43),
    Field("pilotPilotingTiers", 0x061A, "bytes", 43),
    Field("pilotExperienceCounts", 0x0645, "bytes", 43),
    Field("eventDirectoryIndex", 0x0670, "u16", 2),
    Field("eventSeenFlags", 0x0672, "bytes", 140),
    Field("campaignEventPlanetSelectors", 0x06FE, "u16[]", 12),
    Field("majorCampaignMissionRollThreshold", 0x070A, "u8", 1),
    Field("acceptedContractTargetHouseIndex", 0x070B, "u16", 2),
    Field("campaignTerrainDetailLevel", 0x070D, "u8", 1),
    Field("dealerUnitPurchaseLimit", 0x070E, "u16", 2),
    Field("unitsSoldSinceMarketRefresh", 0x0710, "u16", 2),
    Field("playerSkillProgress", 0x0712, "u16", 2),
    Field("darkWingBaseVictoryRecorded", 0x0714, "u8", 1),
    Field("launcherMemoryTier", 0x0715, "u16", 2),
    Field("savedVideoMode", 0x0717, "u8", 1),
]

# Byte arrays made of records: one hex field per record, and a difference names
# the record's field (test_schema.py reads the structs): (struct, header).
RECORD_STRUCTS = {
    "salvageEntries": ("CampaignSalvageEntry", "campaign_exchange.h"),
    "ownedUnitDamage": ("CampaignUnitDamage", "campaign_mechanics.h"),
    "marketUnitDamage": ("CampaignUnitDamage", "campaign_mechanics.h"),
}
SALVAGE = (("unitKind", 1), ("leadingZeroByte", 1), ("conditionPercent", 1), ("trailingZeroByte", 1))
DAMAGE = (("engineHits", 1), ("gyroDamageTier", 1), ("sensorDamageTier", 1), ("lifeSupportDamageTier", 1),
          ("destroyedHeatSinks", 1), ("actuatorDamageTiers", 4), ("destroyedJumpJetGroups", 1),
          ("weaponDamageTiers", 10), ("bodyDamageTiers", 9))
RECORDS = {"salvageEntries": SALVAGE, "ownedUnitDamage": DAMAGE, "marketUnitDamage": DAMAGE}

# Fields that can hold text: kept only as a keyed hash. The commander's name is
# text. B1 scanned every golden's blocks for printable runs of 4 or more, and
# this was the only field flagged; one that holds text later joins it here.
HASHED = ("commanderNameBytes",)


def keyed(key, data):
    """A 12-hex HMAC-SHA-256 under the label key (record.label_key): a short text can't be found from it."""
    return hmac.new(key, data, hashlib.sha256).hexdigest()[:12]


def exchange_fields(block, key):
    """The block's fields, in the block's order. With key None the HASHED fields stay as hex (tests only)."""
    if len(block) != EXCHANGE_BYTES:
        raise ValueError(f"an exchange block is {EXCHANGE_BYTES:#x} bytes, not {len(block):#x}")
    out = OrderedDict()
    for f in EXCHANGE_FIELDS:
        raw = block[f.offset:f.offset + f.size]
        name = "exchange." + f.name
        if f.name in HASHED and key is not None:
            out[name] = keyed(key, raw)
        elif f.kind == "u8":
            out[name] = raw[0]
        elif f.kind == "u16":
            out[name] = struct.unpack("<H", raw)[0]
        elif f.kind == "u16[]":
            out[name] = list(struct.unpack(f"<{f.size // 2}H", raw))
        elif f.name in RECORDS:
            size = record_size(f.name)
            for i in range(f.size // size):
                out[f"{name}.{i}"] = raw[i * size:(i + 1) * size].hex()
        else:
            out[name] = raw.hex()
    return out


def encode_exchange(fields):
    """The block from its fields (exchange_fields with key None): test_schema.py's round trip."""
    block = bytearray(EXCHANGE_BYTES)
    for f in EXCHANGE_FIELDS:
        name = "exchange." + f.name
        if f.kind == "u8":
            raw = bytes([fields[name]])
        elif f.kind == "u16":
            raw = struct.pack("<H", fields[name])
        elif f.kind == "u16[]":
            raw = struct.pack(f"<{f.size // 2}H", *fields[name])
        elif f.name in RECORDS and f.name not in HASHED:
            raw = b"".join(bytes.fromhex(fields[f"{name}.{i}"]) for i in range(f.size // record_size(f.name)))
        else:
            raw = bytes.fromhex(fields[name])
        block[f.offset:f.offset + f.size] = raw
    return bytes(block)


def record_size(name):
    return sum(n for _, n in RECORDS[name])


def record_part(name, index):
    """The record field holding byte `index` of a record: "actuatorDamageTiers[1]", "engineHits"."""
    at = 0
    for part, n in RECORDS[name]:
        if index < at + n:
            return part if n == 1 else f"{part}[{index - at}]"
        at += n
    return "past the record"


def label_hash(row, key):
    """A menu row's label as a keyed hash, "" with no label."""
    label = row.get("label", "")
    return keyed(key, label.encode("latin-1")) if label else ""


def status_fields(status, key):
    """The status's fields: the campaign, then the menu or the battle. `key` hashes the labels."""
    c = status.get("campaign", {})
    out = OrderedDict()
    out["campaign.random"] = list(c.get("random", [0, 0]))
    for name, source in (("planet", "planet"), ("contract", "contractActive"), ("category", "missionCategory"),
                         ("scenario", "scenario"), ("destination", "destination"), ("house", "targetHouse"),
                         ("payment", "paymentThousands"), ("result", "result"), ("funds", "funds"),
                         ("months", "months"), ("row", "selectedRow")):
        out["campaign." + name] = c.get(source)
    out["combat"] = 1 if status.get("combat") else 0
    if status.get("combat"):
        out.update(combat_fields(status))
        return out
    for row in c.get("menu", []):
        at = f"menu.{row['index']}."
        out[at + "enabled"] = row["enabled"]
        out[at + "callback"] = row["callback"]
        # A text row's picture word is where its label lives, not a picture: the label hash stands for it.
        out[at + "picture"] = "-" if row.get("label") else row["picture"]
        out[at + "links"] = list(row["links"])
        out[at + "label"] = label_hash(row, key) or "-"
    return out


def combat_fields(status):
    """The battle: the tick and screen, then each unit on the field, then the objective."""
    out = OrderedDict()
    out["combat.tick"], out["combat.screen"], out["combat.target"] = status.get("tick"), status.get("screen"), status.get("target")
    for u in status.get("units", []):
        at = f"unit.{u['slot']}."
        for name, source in (("side", "side"), ("chassis", "chassis"), ("position", "position"),
                             ("orientation", "orientation"), ("step", "step"), ("mode", "movementMode"),
                             ("jump", "jumpCharge"), ("torso", "torsoOrientation"), ("heat", "heat"),
                             ("engine", "engineHits"), ("gyro", "gyroHits"), ("cockpit", "cockpitDisabled"),
                             ("armor", "armor"), ("internal", "internal"), ("ammo", "ammo")):
            v = u[source]
            out[at + name] = list(v) if isinstance(v, list) else v
        out[at + "weapons"] = [[w["operational"], w["reload"]] for w in u["weapons"]]
    obj = status.get("objective")
    if obj:
        out["objective.position"] = list(obj["position"])
        out["objective.damage"] = obj["damageRemaining"]
    return out


def difference(name, golden, ours):
    """One line saying how a field differs: the first differing element or byte, for an array or a hex field."""
    if isinstance(golden, list) and isinstance(ours, list):
        for i, (a, b) in enumerate(zip(golden, ours)):
            if a != b:
                return f"{name}[{i}]: golden {a} ours {b}"
        return f"{name}: golden {len(golden)} elements, ours {len(ours)}"
    if is_hex_field(name) and isinstance(golden, str) and isinstance(ours, str):
        a, b = bytes.fromhex(golden), bytes.fromhex(ours)
        parts = name.split(".")
        for i, (x, y) in enumerate(zip(a, b)):
            if x != y:
                where = f" ({record_part(parts[1], i)})" if parts[1] in RECORDS else ""
                return f"{name} byte {i}{where}: golden {x:#04x} ours {y:#04x}"
        return f"{name}: golden {len(a)} bytes, ours {len(b)}"
    return f"{name}: golden {golden} ours {ours}"


def is_hex_field(name):
    """True for a byte array kept as hex (not a hashed field)."""
    if not name.startswith("exchange."):
        return False
    parts = name.split(".")
    base = parts[1]
    if base in HASHED:
        return False
    if base in RECORDS:
        return len(parts) == 3
    return any(f.name == base and f.kind == "bytes" for f in EXCHANGE_FIELDS)
