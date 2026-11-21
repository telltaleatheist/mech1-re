#!/usr/bin/env python3
"""The phase 2 inventory: every item src/Original/Memory holds, sorted into
the groups of docs/phase2/PLAN.md (extracted, ours, dropped, code).

    python3 tools/phase2/inventory.py src/Original/Memory > docs/phase2/INVENTORY.md

The sort is by name, with OVR for the exceptions; names and offsets only.
An item read through a door is sorted by where it comes from instead: a
story piece (src/Import/pieces.txt) is extracted, a rule piece
(src/Rules/rule_pieces.txt) is a rule, and a save codec field decoded from
new_campaign.json (src/Rules/new_campaign_fields.cpp) is the new campaign.
The offset shown is then the exe's, from those tables.
"""
import collections, glob, os, re, sys

def scan(memory_dir):
    """Every file-scope item in src/Original/Memory, with the first native
    offset its importer reads ('-' when none)."""
    TYPES=r'(?:static\s+)?(?:const\s+)?(?:uint8_t|uint16_t|int16_t|uint32_t|int32_t|char|bool|Mw\w+|Btech\w+|MW\w+|const char\s*\*(?:const)?|[A-Z]\w+)\s*\*?\s*(?:const\s+)?'
    decl=re.compile(r'^'+TYPES+r'(.+);\s*(?:/\*.*)?$')
    rows=[]
    for path in sorted(glob.glob(os.path.join(memory_dir,'*.cpp'))):
        f=os.path.basename(path);lines=open(path).read().split('\n');depth=0;names=[]
        for l in lines:
            if depth==0 and not l.startswith((' ','#','/','}','{','\t','static_assert','typedef','enum','extern','namespace','using')) and '(' not in l.split('=')[0]:
                m=decl.match(l)
                if m:
                    body=re.sub(r'=\s*\{[^}]*\}','',m.group(1));body=re.sub(r'=[^,;]*','',body)
                    for part in body.split(','):
                        n=re.match(r'\s*\**\s*(\w+)',part)
                        if n: names.append(n.group(1))
            depth+=l.count('{')-l.count('}')
        for n in names:
            offs=[]
            for l in lines:
                if re.search(r'\b'+n+r'\b',l): offs+=re.findall(r'(?:data\s*[+,]\s*\(?(?:uint16_t\)\()?)(0x[0-9A-Fa-f]{3,4})',l)
            rows.append((f,n,offs[0] if offs else '-',len(offs),door_source(n,lines)))
    return rows

ROOT=os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
def pieces(path):
    """id -> 'EXE SEG:OFF' from a pieces table."""
    out={}
    for l in open(path):
        w=l.split()
        if len(w)>=3 and not l.startswith('#'): out[w[0]]=w[1].replace('.EXE','')+' '+w[2]
    return out
STORY=pieces(os.path.join(ROOT,'src','Import','pieces.txt'))
RULE=pieces(os.path.join(ROOT,'src','Rules','rule_pieces.txt'))
FIELDS={m.group(1):int(m.group(2),16) for m in re.finditer(r'\{"([\w.]+)", 0x([0-9A-F]{4})',
        open(os.path.join(ROOT,'src','Rules','new_campaign_fields.cpp')).read())}
# Macros that name part of an item (combat_import.h and friends).
ALIAS={m.group(1):m.group(2) for p in glob.glob(os.path.join(ROOT,'src','include','mechwarrior','*.h'))
       for m in re.finditer(r'#define\s+(\w+)\s+(\w+)\.',open(p).read())}
# Items a door fills through a local copy (signed words, double words, rows).
VIA={'mechFallbackTemplateSteps':'rule combat.fallback_template_steps','mechTemplateBudgetCostsBytes':'rule combat.template_budget_costs',
     'missileClusterRowByWeapon':'rule weapons.missile_cluster_rows','weaponDefinitions':'rule weapons.definitions',
     'mechTemplates':'rule mechs.templates','mechTemplateNames':'story combat.mech_template.1.name',
     'pilotDefinitionNames':'story combat.pilot.1.name','pilotDefinitionNameDirectory':'story combat.pilot.1.name',
     'pilotDefinitions':'builder MwStoryBuildPilots','unitAmmoProfileEntries':'rule units.ammo_profile_entries',
     'unitAmmoProfiles':'rule units.ammo_profiles','weaponRepairPricesByKind':'rule units.weapon_repair_price_lists',
     **{f'eventPlanetChoicePool{i}':'builder MwStoryCopyPlanetPools' for i in range(6)}}
DOOR=re.compile(r'(MwStoryCopyText|MwStoryCopy\w+|ImportRule(?:Words|Bytes)|MwRulesCopy(?:Words|Bytes|Native))\(\s*"([^"]+)"\s*,\s*([^;]*)')
BUILDER=re.compile(r'\b(MwStoryBuild\w+|MwStoryCopyAdlibInstruments|MwStoryCopyPlanetPools)\(')
def door_source(name,lines):
    """'story <id>', 'rule <id>' or '' for the door that fills `name`."""
    if name in VIA: return VIA[name]
    names={name}|{a for a,root in ALIAS.items() if root==name}
    for l in lines:
        for m in DOOR.finditer(l):
            if any(re.search(r'\b'+n+r'\b',m.group(3)) for n in names):
                return ('story ' if m.group(1).startswith('MwStory') else 'rule ')+m.group(2)
    for i,l in enumerate(lines):
        m=re.search(r'MwStoryCopyText\(\s*id\s*,\s*([^;]*)',l)
        if m and any(re.search(r'\b'+n+r'\b',m.group(1)) for n in names):
            for back in lines[max(0,i-3):i+1][::-1]:
                fmt=re.search(r'snprintf\(id,[^"]*"([^"]+)"',back)
                if fmt:
                    pattern=re.compile('^'+re.escape(fmt.group(1)).replace('%u',r'\d+').replace('%s',r'\w+')+'$')
                    first=next((k for k in STORY if pattern.match(k)),fmt.group(1))
                    return 'story '+first
    for i,l in enumerate(lines):
        m=BUILDER.search(l)
        if not m: continue
        call,depth,j='',0,i
        while j<len(lines):
            call+=lines[j];depth=call.count('(')-call.count(')');j+=1
            if depth<=0: break
        if any(re.search(r'\b'+n+r'\b',call) for n in names): return 'builder '+m.group(1)
    if name in FIELDS or any(k.startswith(name+'.') for k in FIELDS): return 'field '+name
    return ''
def where(src):
    """The exe offset of a door source."""
    kind,id_=src.split(' ',1)
    if kind=='story': return STORY.get(id_,'-')
    if kind=='builder': return '-'
    if kind=='rule': return RULE.get(id_,'-')
    off=FIELDS.get(id_) if id_ in FIELDS else min(v for k,v in FIELDS.items() if k.startswith(id_+'.'))
    return f'MW_MAIN 1090:{0x448+off:04X}'
X=re.compile(r'^(intro\w+Text|contractNarrative|missionDescription\w*|afterAction(Kurita|Liao|General)(Failure|Success)|settlement\w+Text|darkWing\w*Text|deploymentUneventfulText|missionTitleText|pilotDefinition\w*|pilotIdentityStorage|planetRecord\w+|alphabeticPlanetRecordOffsets|eventPlanetChoicePool\d|campaignEventPlanetSelectors|(combat|campaign)DefaultCommanderNameBytes|mechTemplateNames|unitDisplayNames|shopUnitNames|houseContractNames|musicSequences|combatAdlibDefaultInstruments|enemyPilot\w*|newsArticle\w*|houseContractRefusalText|contractOfficePlanetGroupStorage|houseMissionPlanetTableOffsets)$')
D=re.compile(r'^(copyProtection\w*|launcher(CampaignDiskPrompt|CombatDiskPrompt|MemoryError|GraphicsPrompt|DiskSuffix|ReleaseError|MemoryTier|Identity|CampaignFilename|CombatFilename|ExitCode)|\w*DiskPrompt\w*|changeDiskPrompt|insertDiskOnePrompt|resourceDiskPromptNumber|combatDiskPrompt|tandyGraphicsSelected|startupErrorText|herculesDisplayRowOffsets|combatExecutableFilename|saveDiskPrompt)$')
UI=re.compile(r'(Label|Labels|Text|TextBytes|Heading|Caption|Title|Footer|Prompt|Names|Suffix|Done|Cancel|BuyLabel|SellLabel|AtStart|AtEnd|BoundaryText|ProfileName|BodyCondition|Condition)$')
LAYOUT=re.compile(r'(Panel|Menu|Records|Rows|Origins|Anchors|XOffsets|YOrigins|TextX|TextY|ColumnOrigins|YearXOffsets|Colours|ColourTables|Inset\w*|LeadingSpacesByKind|RowOffset|SchemeIndex|LayoutIndices|MonthNameWidths)$')
RULES=re.compile(r'(Price|Prices|Percent|Percentages|Thresholds?|Limit|Limits|Tiers?|TierBy\w+|Multipliers|Weights?\w*|Budget\w*|Bias|Rows|Modifiers|Rates|Tonnages|Kph|Metres|Counts|Maxima|Starts|Ceilings\w*|Divisors|Rules|Profiles|Bounds|Numbers|Penalties|ByDamageTier|ByKind|ByCondition|ByHits|ByOperational|ByScenario|Amounts|Ranges?|ByWeapon|ByBodyLocation|Locations|Templates|Steps|Policies|ByCategory|Quantities|Kinds|Entries|Base\w*|Points|Standings|Codes|Indices)$')
def cls(name,off,n):
    if X.match(name): return 'extracted'
    if D.match(name): return 'dropped'
    if off=='-': return 'code:state'
    if re.match(r'^(ega|polygon|graphics|clip|span|ellipse|selection|horizon|render|projection|dotProduct|signedDivision|screenCentre|nearClip|objectProjection|combatProjection|egaFont|palette|drawColour|video|backPage|frontPage|copySource|drawPage|saved\w+|runtime\w+|stream\w+|lzw\w+|scheduler\w+|pit\w+|keyboard\w+|joystick\w+|mouse\w+|midi\w+|mpu\w+|combatMpu\w+|adlib\w+|combatAdlib\w+|softwareMusic\w+|music\w+|activeSound\w+|activeMusic\w+|soundAllocation\w+|selectedAudio\w+|picture\w+|indexedPicture\w+|\w+Selector|\w+Handle|numeric\w+|textInput\w+|textPanel\w+|notification\w+|panel\w+|campaignTimer\w+|random\w+|terrainTileGrid\w+|boundaryMaskByCode|beam\w+|muzzle\w+|travelSquareRootValues|characterClassStorage|cached\w+|previous\w+|selected\w+|current\w+|calendar(TextX|TextY|Background|Foreground)|unidentified\w+|\w+Scratch|\w+Pending|\w+Count|\w+Index|\w+Row|\w+Slot|\w+ByteOffset|\w+Mode|\w+Flag|\w+Flags|\w+Tick|\w+Counter|\w+Accumulator|\w+Remaining|\w+Active|\w+Installed|\w+Latched|\w+Phase|\w+Selection|\w+X|\w+Y|\w+Low|\w+High)$',name) and not UI.search(name): return 'code:state'
    if UI.search(name): return 'ours:ui'
    if LAYOUT.search(name): return 'ours:layout'
    if RULES.search(name): return 'ours:rules'
    return '?'
OVR={'inspectionBitmapNames':'code','cockpitScreenNames':'code','hitLocationRows':'ours:rules','missileClusterRows':'ours:rules',
 'actuatorIntegrityTierByCondition':'ours:rules','graphicsRowOffsets':'code','palettePrimarySlots':'code','paletteSecondarySlots':'code',
 'graphicsColourPatterns':'code','egaFontBuffer':'code','polygonSpanStorage':'code','scenarioSelectors':'ours:rules',
 'opposingSideFirstUnitSlots':'ours:rules','avoidanceHeadingOffsets':'ours:rules','bodyDamageEffectResourceDeltas':'ours:rules',
 'defaultEffectResourceDeltas':'ours:rules','machineGunEffectResourceDeltas':'ours:rules','orderNavigationPanelDeltas':'ours:layout',
 'orderNavigationRowDeltas':'ours:layout','orderMapPositionMask':'ours:layout','hiredTransportFrames':'ours:layout',
 'dropShipTravelFrames':'ours:layout','mechComplexAnimationFrames':'ours:layout','repairJumpJetCountTemplateBytes':'ours:ui',
 'combatTemplateByCampaignUnitKind':'ours:rules','campaignUnitKindByCombatTemplate':'ours:rules','travelDestinationName':'code',
 'hubFirstNavigationProbe':'code','hubSecondNavigationProbe':'code','hubThirdNavigationProbe':'code','saveFindPattern':'code',
 'newsArticleCursor':'code','newsArticleListStart':'code','missionDescriptionMaximumIndicesByCategory':'extracted',
 'combatAdlibDefaultInstruments':'extracted','musicSequences':'extracted','generateStartingJenner':'ours:rules'}
rows=scan(sys.argv[1])
# Slice 3's reclassifications (docs/phase2/PLAN.md 5.1, 5.6): past BTECH's file
# data (zero in the exe, written in play), and the label-index tables, which
# move with their labels in slice 5.
OVR.update({'mountedWeaponCountsByKind':'code','operationalWeaponCountsByKind':'code','selectionObjectListIndices':'code',
            'scenarioSelectors':'code','actuatorConditionLabelIndices':'ours:ui','engineConditionLabelIndices':'ours:ui',
            'twoHitSystemConditionLabelIndices':'ours:ui','orderDisplayIndices':'ours:ui'})
GROUP_OF_SOURCE={'story':'extracted','builder':'extracted','rule':'ours:rules','field':'ours:start'}
rows2=[]
for f,name,off,n,src in rows:
    if src:
        c=GROUP_OF_SOURCE[src.split(' ',1)[0]]
        rows2.append((c,f,name,where(src),src.replace('field ','new_campaign.json ').replace('builder ','story, ')))
        continue
    c=OVR.get(name) or cls(name,off,n)
    if c=='?' : c='code'
    if c=='code:state': c='code'
    rows2.append((c,f,name,off,'-'))
# items the file-scope scan misses
rows2+= [('extracted','MW_MAIN_1AA5.cpp','eventTextStorage / eventTextRecords','-','story, MwStoryBuildEventText'),
         ('extracted','MW_MAIN_1090_EVENTS.cpp','the event script (records, conditions, actions)','-','story, MwStoryBuildCampaignEvents')]
G=[('extracted','Extracted: the original\'s creative content (the story file, first run)'),('ours:ui','Ours: UI words and system messages (data/text/interface.json)'),
   ('ours:layout','Ours: screen layout, panels and menus (data/ui/*.json)'),('ours:rules','Ours: rules and constants (data/rules/*.json)'),
   ('ours:start','Ours: the new campaign\'s starting state (data/rules/new_campaign.json)'),('dropped','Dropped'),('code','Code: runtime state and renderer tables (no file)')]
cnt=collections.Counter(r[0] for r in rows2)
print('# Phase 2 inventory\n\nGenerated from `src/Original/Memory` by `tools/phase2/inventory.py` (every file-scope item). An item read through a door shows where it now comes from: a story piece, a rule piece, or a save codec field of `new_campaign.json`, with the exe offset those tables give. Any other item shows the first native offset its importer reads, or `-` for runtime state with no import. The groups follow `docs/phase2/PLAN.md`. Names and offsets only: no MW1 text.\n')
print('| Group | Items |\n|---|---|')
for k,t in G: print(f'| {t} | {cnt[k]} |')
for k,t in G:
    print(f'\n## {t}\n')
    if k=='code':
        byf=collections.Counter(r[1] for r in rows2 if r[0]=='code')
        print('Not listed one by one: '+', '.join(f'{f} {n}' for f,n in sorted(byf.items()))+'.'); continue
    print('| File | Item | Offset | From |\n|---|---|---|---|')
    for c,f,name,off,src in sorted(r for r in rows2 if r[0]==k): print(f'| {f} | `{name}` | {off} | {src} |')
print("""
## Reclassified or removed in slice 3

- `houseMissionPlanetTableOffsets` (MW_MAIN 1090:8E90) was listed as story content; it is the pointer table of the Houses' mission-planet policies, a rule (`houses.mission_planet_policy_offsets`).
- `repairAttemptFailuresByKind` (MW_MAIN 1090:3A77, 220 bytes) was listed as a rule; it is zero in the exe and written in play, so it is code, zeroed where the exe reloaded it.
- `unusedRepairTimeByDamageTier` (MW_MAIN 1090:3A6F, 8 bytes) was listed as a rule; it was imported and never read, and is gone.
- `combatMissionSideZeroRules` (BTECH 304E:092C, 74 bytes; an alias into `missionRules`) and `mechFallbackTemplateSteps` (BTECH 304E:0992, 16 bytes) were missing; both are rules.
- `mountedWeaponCountsByKind`, `operationalWeaponCountsByKind` (both BTECH 304E:4F32), `selectionObjectListIndices` (6F5A) and `scenarioSelectors` (7D6A) lie past BTECH's file data (304E:4EE0), so the exe holds zeros and play writes them: code.
- `actuatorConditionLabelIndices`, `engineConditionLabelIndices`, `twoHitSystemConditionLabelIndices` and `orderDisplayIndices` pick labels and move with them in slice 5: UI words.
- Eight start values are always written before they're read, and are code, zeroed where the exe reloaded them (all eight are zero in the exe): `settlementPayoutPercent` (1090:123F), `pilotCardGunneryTier`, `pilotCardPilotingTier`, `pilotCardQuotedWageTier` (2440, 2442, 2446), `missionForceBudget` (4B79), `missionPaymentBaseThousands` (4B7B), `travelDestinationTimeWeight` (54DA, written and never read) and `sideObjectiveRules` (BTECH 304E:0ED0). The other four, `ammunitionTransactionPrices`, `generateStartingJenner`, `generatedAmmoKinds` and `generatedAmmoQuantities`, are read first on some path and stay `"start"` entries in market.json.
- Two of the save codec's 105 fields, `pilotRosterCount` and `rosterPilotIds`, are owned in `src/Original/Campaign`, so this list (of `src/Original/Memory`) doesn't show them; new_campaign.json holds them.""")
