#!/usr/bin/env python3
"""The phase 2 inventory: every item src/Original/Memory holds, sorted into
the groups of docs/phase2/PLAN.md (extracted, ours, dropped, code).

    python3 tools/phase2/inventory.py src/Original/Memory > docs/phase2/INVENTORY.md

The sort is by name, with OVR for the exceptions; names and offsets only.
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
            rows.append((f,n,offs[0] if offs else '-',len(offs),''))
    return rows
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
rows2=[]
for f,name,off,n,full in rows:
    c=OVR.get(name) or cls(name,off,n)
    if c=='?' : c='code'
    if c=='code:state': c='code'
    if f.startswith('MW_MAIN_1090') and off!='-' and 0x448<=int(off,16)<0xB60 and c not in ('extracted','dropped'): c='ours:start'
    rows2.append((c,f,name,off))
# items the file-scope scan misses
rows2+= [('extracted','MW_MAIN_1AA5.cpp','eventTextStorage / eventTextRecords','1AA5:0000'),
         ('extracted','MW_MAIN_1090_EVENTS.cpp','the event script (records, conditions, actions)','MwImportCampaignEventData'),
         ('ours:rules','BTECH_304E.cpp','mechTemplates / weaponDefinitions (BtechImportMechInitialData)','BtechImportMechInitialData')]
G=[('extracted','Extracted: the original\'s creative content (the story file, first run)'),('ours:ui','Ours: UI words and system messages (data/text/interface.json)'),
   ('ours:layout','Ours: screen layout, panels and menus (data/ui/*.json)'),('ours:rules','Ours: rules and constants (data/rules/*.json)'),
   ('ours:start','Ours: the new campaign\'s starting state (data/rules/new_campaign.json)'),('dropped','Dropped'),('code','Code: runtime state and renderer tables (no file)')]
cnt=collections.Counter(r[0] for r in rows2)
print('# Phase 2 inventory\n\nGenerated from `src/Original/Memory` (every file-scope item, with the first native offset its importer reads; `-` means the item is runtime state with no import). The groups follow `docs/phase2/PLAN.md`. Names and offsets only: no MW1 text.\n')
print('| Group | Items |\n|---|---|')
for k,t in G: print(f'| {t} | {cnt[k]} |')
for k,t in G:
    print(f'\n## {t}\n')
    if k=='code':
        byf=collections.Counter(r[1] for r in rows2 if r[0]=='code')
        print('Not listed one by one: '+', '.join(f'{f} {n}' for f,n in sorted(byf.items()))+'.'); continue
    print('| File | Item | Native offset |\n|---|---|---|')
    for c,f,name,off in sorted(r for r in rows2 if r[0]==k): print(f'| {f} | `{name}` | {off} |')
