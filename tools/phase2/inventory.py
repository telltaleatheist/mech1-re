#!/usr/bin/env python3
"""The phase 2 inventory: every item src/Original/Memory holds, sorted into
the groups of docs/phase2/PLAN.md (extracted, ours, dropped, code).

    python3 tools/phase2/inventory.py src/Original/Memory > docs/phase2/INVENTORY.md

The sort is by name, with OVR for the exceptions; names and offsets only.
An item read through a door is sorted by where it comes from instead: a
story piece (src/Import/pieces.txt) is extracted, a rule piece
(src/Rules/rule_pieces.txt) is a rule, and a save codec field decoded from
new_campaign.json (src/Rules/new_campaign_fields.cpp) is the new campaign,
a layout piece (src/Ui/ui_pieces.txt) is layout, and a UI word
(src/Import/word_pieces.txt, as src/Import/import_table.cpp lists them with
the captions' texts) is a word: one a words door fills (MwWordsCopyText,
MwWordsCopyBlock), one read from an importer's view of the words (`words`,
`mainWords`) at an offset a word covers, directly, through a pointer table
or past a range check, a pointer into such an item, or a caption whose text
is a word. The offset shown is then the exe's, from those tables.
"""
import collections, glob, os, re, sys

def owners():
    """The owner structs (plan slices B2 and B3: campaign and combat state): instance -> (type, member lines)."""
    out={}
    for p in glob.glob(os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),'src','include','mechwarrior','*.h')):
        text=open(p).read()
        for m in re.finditer(r'/\* (?:Campaign|Combat) state owned by [\w.]+ \(plan, slice B\d[a-e]?\)\. \*/\nstruct (\w+)\n\{\n(.*?)\n\};\nextern \1 (\w+);',text,re.S):
            members=[l.strip() for l in re.sub(r'/\*.*?\*/','',m.group(2),flags=re.S).split('\n') if l.strip()]
            out[m.group(3)]=(m.group(1),members)
    return out
OWNERS=owners()
def top_level(text):
    """`text` split at the commas outside brackets."""
    out,depth,cur=[],0,''
    for c in text:
        if c in '([{': depth+=1
        elif c in ')]}': depth-=1
        if c==',' and depth==0: out.append(cur);cur=''
        else: cur+=c
    return out+[cur]
def unfold(text):
    """A memory file as it read before B2 grouped its globals: an owner's
    instance is its members again, one declaration each (an initialiser the
    instance gives a member stays with it), and `instance.member` is the
    member. The items, their offsets and their doors read as before."""
    for inst,(typ,members) in OWNERS.items():
        m=re.search(r'^'+typ+r' '+inst+r'(?:=\{\n(.*?)\n\})?;$',text,re.S|re.M)
        if m:
            inits={}
            for item in top_level(re.sub(r'/\*.*?\*/','',m.group(1) or '',flags=re.S)):
                d=re.match(r'\s*\.(\w+)\s*=\s*(.*)$',item,re.S)
                if d: inits[d.group(1)]=d.group(2).strip()  # its own lines, as the definition had them
            decls=[]
            for line in members:
                t,rest=re.match(r'((?:(?:const|volatile|unsigned|signed|struct)\s+)*[\w:]+(?:\s+(?:const|volatile))*)\s*(.*)$',line.rstrip(';')).groups()
                for part in [x.strip() for x in top_level(rest)]:
                    n=re.match(r'\(?\s*\**\s*(\w+)',part).group(1)  # also a function pointer, (*name)(...)
                    decls.append(f'{t} {part}'+(f' = {inits[n]}' if n in inits else '')+';')
            text=text[:m.start()]+'\n'.join(decls)+text[m.end():]
        text=re.sub(r'\b'+inst+r'\.','',text)
    return text

def scan(memory_dir):
    """Every file-scope item in src/Original/Memory, with the first native
    offset its importer reads ('-' when none)."""
    TYPES=r'(?:static\s+)?(?:const\s+)?(?:uint8_t|uint16_t|int16_t|uint32_t|int32_t|char|bool|Mw\w+|Btech\w+|MW\w+|const char\s*\*(?:const)?|[A-Z]\w+)\s*\*?\s*(?:const\s+)?'
    decl=re.compile(r'^'+TYPES+r'(.+);\s*(?:/\*.*)?$')
    rows=[]
    for path in sorted(glob.glob(os.path.join(memory_dir,'*.cpp'))):
        f=os.path.basename(path);lines=unfold(open(path).read()).split('\n');depth=0;names=[]
        for l in lines:
            l=l.replace('(int)','')  # a cast in an array bound
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
                if re.search(r'\b'+n+r'\b',l): offs+=re.findall(r'(?:\bdata\s*(?:[+,]|\[)\s*\(?(?:uint16_t\)\()?)(0x[0-9A-Fa-f]{3,4})',l)
            rows.append((f,n,offs[0] if offs else '-',len(offs),word_source(n,lines,f) or door_source(n,lines)))
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
UIP=pieces(os.path.join(ROOT,'src','Ui','ui_pieces.txt'))
# The words, captions' texts included: id -> (exe, segment, offset, length).
WORD={m.group(1):(m.group(2).replace('.EXE',''),int(m.group(3),16),int(m.group(4),16),int(m.group(5)))
      for m in re.finditer(r'Kind::Word(?:Bytes)?, "([\w.]+)", "(\w+\.EXE)", 0x([0-9A-F]+), 0x([0-9A-F]+), (\d+)',
                           open(os.path.join(ROOT,'src','Import','import_table.cpp')).read())}
ENUMS={m.group(1):int(m.group(2),16) for p in glob.glob(os.path.join(ROOT,'src','include','mechwarrior','*.h'))
       for m in re.finditer(r'\b([A-Z][A-Z0-9_]+)\s*=\s*0x([0-9A-Fa-f]+)',open(p).read())}
UIJSON={}
for p in glob.glob(os.path.join(ROOT,'data','ui','*.json')):
    import json
    UIJSON[os.path.basename(p)[:-5]]=json.load(open(p)).get('tables',{})
UISIZE={l.split()[0]:int(l.split()[3]) for l in open(os.path.join(ROOT,'src','Ui','ui_pieces.txt')) if len(l.split())>=4 and not l.startswith('#')}
FIELDS={m.group(1):int(m.group(2),16) for m in re.finditer(r'\{"([\w.]+)", 0x([0-9A-F]{4})',
        open(os.path.join(ROOT,'src','Rules','new_campaign_fields.cpp')).read())}
# Macros that name part of an item (combat_import.h and friends).
ALIAS={m.group(1):m.group(2) for p in glob.glob(os.path.join(ROOT,'src','include','mechwarrior','*.h'))
       for m in re.finditer(r'#define\s+(\w+)\s+(?:(?:'+'|'.join(OWNERS)+r')\.)?(\w+)\.',open(p).read())}
# Items a door fills through a local copy (signed words, double words, rows).
VIA={'randomStateWord0':'rule new_campaign.start.random_seed','randomStateWord1':'rule new_campaign.start.random_seed',
     'mechFallbackTemplateSteps':'rule combat.fallback_template_steps','mechTemplateBudgetCostsBytes':'rule combat.template_budget_costs',
     'missileClusterRowByWeapon':'rule weapons.missile_cluster_rows','weaponDefinitions':'rule weapons.definitions',
     'mechTemplates':'rule mechs.templates','mechTemplateNames':'story combat.mech_template.1.name',
     'pilotDefinitionNames':'story combat.pilot.1.name','pilotDefinitionNameDirectory':'story combat.pilot.1.name',
     'pilotDefinitions':'builder MwStoryBuildPilots','unitAmmoProfileEntries':'rule units.ammo_profile_entries',
     'unitAmmoProfiles':'rule units.ammo_profiles','weaponRepairPricesByKind':'rule units.weapon_repair_price_lists',
     **{f'eventPlanetChoicePool{i}':'builder MwStoryCopyPlanetPools' for i in range(6)},
     # Layout pieces read at computed offsets or through a local table.
     'inspectionDiagramRegions':'ui cockpit.inspection_diagram_regions','targetDiagramRegions':'ui cockpit.target_diagram_regions',
     'cockpitLayouts':'ui cockpit.layouts','hudColourSchemes':'ui cockpit.hud_colour_schemes',
     'landscapeCameraColourRows':'ui cockpit.landscape_camera_rows','mechBodyDiagramLayouts':'ui complex.table.body_diagrams',
     'pilotAssignmentCaptionColours':'ui crew.table.pilot_assignment_captions',
     'reserveAmmoCompatibilityPanels':'ui dealer.table.compatibility_panels',
     'eventTwoChoiceMenu':'ui events.menu.two_choices','eventThreeChoiceMenu':'ui events.menu.three_choices'}
DOOR=re.compile(r'(MwStoryCopyText|MwStoryCopy\w+|ImportRule(?:Words|Bytes)|MwRulesCopy(?:Words|Bytes|Native))\(\s*"([^"]+)"\s*,\s*([^;]*)')
# The layout door: an item it fills is its first argument after the id.
UIDOOR=re.compile(r'(?:MwUiCopy(?:Words|Bytes|Native)|MwImportUi(?:Panel|ListPanel|Menu))\(\s*"([^"]+)"\s*,\s*([^,]*)')
# A caption owner: {id or ids[i], &item...}; ids[] names its first id.
CAPTION=re.compile(r'\{\s*(?:"([\w.]+)"|(\w+)\[\w+\])\s*,\s*&(\w+)')
# BTECH's importers copy pieces into a local view at their exe offsets.
VIEWCOPY=re.compile(r'(?:CopyLayoutPiece\(\s*\w+\s*,\s*sizeof\(\w+\)\s*,\s*"([^"]+)"\s*,\s*0x([0-9A-Fa-f]+)|MwUiCopyNative\(\s*"([^"]+)"\s*,\s*ui\s*\+\s*0x([0-9A-Fa-f]+))')
VIEWREAD=re.compile(r'\bui\s*(?:,|\+|\[)\s*\(?0x([0-9A-Fa-f]+)')
MENUINIT=re.compile(r'MwUiMenu\s+(\w+)\s*=\s*\{\s*(\w+)')
def ui_source(name,lines):
    """'ui <id>', 'ui-caption <id>' or '' for the layout door that fills `name`."""
    text='\n'.join(lines)
    names={name}|{m.group(1) for m in MENUINIT.finditer(text) if m.group(2)==name}
    for l in lines:
        for m in UIDOOR.finditer(l):
            if any(re.search(r'\b'+n+r'\b',m.group(2)) for n in names): return 'ui '+m.group(1)
        for m in CAPTION.finditer(l):
            if m.group(3)!=name: continue
            if m.group(1): return 'ui-caption '+m.group(1)
            first=re.search(r'\b'+m.group(2)+r'\[[^\]]*\]\s*=\s*\{\s*"([\w.]+)"',text)
            if first: return 'ui-caption '+first.group(1)
    views=[(int(m.group(2) or m.group(4),16),m.group(1) or m.group(3)) for m in VIEWCOPY.finditer(text)]
    for l in lines:
        if not re.search(r'\b'+name+r'\b',l): continue
        for m in VIEWREAD.finditer(l):
            off=int(m.group(1),16)
            for start,id_ in views:
                if start<=off<start+UISIZE.get(id_,0): return 'ui '+id_
    return ''
WORDDOOR=re.compile(r'MwWordsCopy(?:Text|Block)\(\s*"([\w.]+)"\s*,\s*(?:\([^)]*\)\s*)?([^,;]*)')
VIEWOFF=re.compile(r'\b(?:mainWords|words)\s*(?:\+|,)\s*(0x[0-9A-Fa-f]+|\w+(?:\[\w+\])?)')
def covering(exe,off):
    """The word whose bytes hold offset `off` of `exe`'s data segment."""
    return next((k for k,(e,_,o,n) in sorted(WORD.items(),key=lambda kv:kv[1][2]) if e==exe and o<=off<o+n),None)
def table_word(text,var):
    """The first word of the pointer table a layout door copies into `var`."""
    m=re.search(r'MwUiCopyWords\(\s*"(\w+)\.table\.(\w+)"\s*,\s*'+var+r'\b',text)
    entries=UIJSON.get(m.group(1),{}).get(m.group(2)) if m else None
    return f'{m.group(1)}.word.{entries[0]}' if entries and isinstance(entries,list) and isinstance(entries[0],str) else None
def word_source(name,lines,f):
    """'word <id>' or '' for the words door or words view that fills `name`."""
    if name in ('words','mainWords'): return ''  # the views themselves
    exe='BTECH' if f.startswith('BTECH') else 'MW_MAIN'
    text='\n'.join(lines);hit=lambda l:re.search(r'\b'+name+r'\b',l)
    for i,l in enumerate(lines):
        if not hit(l): continue
        for m in WORDDOOR.finditer(l):
            if re.search(r'\b'+name+r'\b',m.group(2)) and m.group(1) in WORD: return 'word '+m.group(1)
        for m in VIEWOFF.finditer(l):
            at=m.group(1);var=at.split('[')[0];word=None
            if at.startswith('0x'): word=covering(exe,int(at,16))
            elif var in ENUMS: word=covering(exe,ENUMS[var])
            else:
                word=table_word(text,var)
                assigned=re.search(r'\b'+var+r'\s*=\s*(\w+)\[',text)
                if not word and assigned: word=table_word(text,assigned.group(1))
                for back in lines[max(0,i-3):i+1][::-1]:
                    bound=re.search(r'\b'+var+r'\s*<\s*(0x[0-9A-Fa-f]+|[A-Z_][A-Z0-9_]+)',back)
                    if not word and bound: word=covering(exe,int(bound.group(1),16) if bound.group(1).startswith('0x') else ENUMS.get(bound.group(1),-1))
            if word: return 'word '+word
        # A pointer into an item the words fill: name[...]=item+...
        m=re.search(r'\b'+name+r'\b(?:\[[^\]]*\])*\s*=\s*(?:\([^)]*\))?\s*(\w+)\s*\+',l)
        if m and m.group(1)!=name:
            via=word_source(m.group(1),lines,f)
            if via: return via
    return ''
BUILDER=re.compile(r'\b(MwStoryBuild\w+|MwStoryCopyPlanetPools)\(')
def door_source(name,lines):
    """'story <id>', 'rule <id>' or '' for the door that fills `name`."""
    if name in VIA: return VIA[name]
    ui=ui_source(name,lines)
    if ui: return ui
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
    if kind in ('ui','ui-caption'): return UIP.get(id_,'-')
    if kind=='word':
        e,seg,off,_=WORD[id_]; return f'{e} {seg:04X}:{off:04X}'
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
 'orderNavigationRowDeltas':'ours:layout','orderMapPositionMask':'code','hiredTransportFrames':'ours:layout',
 'dropShipTravelFrames':'ours:layout','mechComplexAnimationFrames':'ours:layout','repairJumpJetCountTemplateBytes':'ours:ui',
 'combatTemplateByCampaignUnitKind':'ours:rules','campaignUnitKindByCombatTemplate':'ours:rules','travelDestinationName':'code',
 'hubFirstNavigationProbe':'code','hubSecondNavigationProbe':'code','hubThirdNavigationProbe':'code','saveFindPattern':'code',
 'newsArticleCursor':'code','newsArticleListStart':'code','missionDescriptionMaximumIndicesByCategory':'extracted',
 'musicSequences':'code','generateStartingJenner':'ours:rules',
 'positionedText':'code','newsAtStart':'code','newsAtEnd':'code'}
rows=scan(sys.argv[1])
# Slice 3's reclassifications (docs/phase2/PLAN.md 5.1, 5.6): past BTECH's file
# data (zero in the exe, written in play), and the label-index tables, which
# move with their labels in slice 5.
OVR.update({'mountedWeaponCountsByKind':'code','operationalWeaponCountsByKind':'code','selectionObjectListIndices':'code',
            'scenarioSelectors':'code','actuatorConditionLabelIndices':'ours:ui','engineConditionLabelIndices':'ours:ui',
            'twoHitSystemConditionLabelIndices':'ours:ui','orderDisplayIndices':'ours:ui'})
# Items still read straight from the exe, with the slice that will move them
# (docs/phase2/PLAN.md 3, 2g).
OWNER={}
# Importers that decode a native span of the story (story/native.json) rather
# than the exe: an item they read is extracted, at the span's exe offset.
SPANS={'MW_MAIN_28BE.cpp':('music.driver','MW_MAIN 28BE')}
# BTECH's start image (src/Original/Memory/BtechStartImage.cpp, slices 6c and
# 6d): the rows that lay out what BTECH's importers read. An item a BTECH
# importer reads at an offset of 304E is sorted by the row that holds that
# offset: a story span is extracted, a rule or layout piece ours, the rest code.
START_TEXT=open(os.path.join(ROOT,'src','Original','Memory','BtechStartImage.cpp')).read()
START_ROWS=[(m.group(1),int(m.group(2),16),int(m.group(3),16),int(m.group(4),16),m.group(5)) for m in
            re.finditer(r'\{"([\w.]+)",0x([0-9A-F]{4}),0x([0-9A-F]{4}),0x([0-9A-F]+),BTECH_START_(\w+),',START_TEXT)]
def start_row(f,off):
    """The start image row of a BTECH 304E item at `off`, or None."""
    if not f.startswith('BTECH_304E') or not off.startswith('0x'): return None
    o=int(off,16)
    for id_,seg,at,size,source in START_ROWS:
        if seg==0x304E and at<=o<at+size: return (id_,source)
    return None
GROUP_OF_SOURCE={'story':'extracted','builder':'extracted','rule':'ours:rules','field':'ours:start','ui':'ours:layout','ui-caption':'ours:ui','word':'ours:ui'}
rows2=[]
for f,name,off,n,src in rows:
    if src.startswith('ui-caption ') and src.split(' ',1)[1]+'.text' in WORD: src='word '+src.split(' ',1)[1]+'.text'
    if src:
        c=GROUP_OF_SOURCE[src.split(' ',1)[0]]
        rows2.append((c,f,name,where(src),src.replace('field ','new_campaign.json ').replace('builder ','story, ').replace('ui-caption ','ui, head ').replace('word ','words, ')))
        continue
    row=None if src else start_row(f,off)
    if not src and f.startswith('BTECH_304E') and off.startswith('0x') and int(off,16)>=0x4EDC:
        # Past the file data (304E:4EDC..D5DF): BSS, zero at every EXEC, not the exe's.
        rows2.append(('code',f,name,f'BSS 304E:{off[2:]}','-'))
        continue
    if row:
        id_,source=row
        if source=='STORY': rows2.append(('extracted',f,name,f'BTECH 304E:{off[2:]}',f'story, {id_} (native span)'))
        elif source=='RULES': rows2.append(('ours:rules',f,name,f'BTECH 304E:{off[2:]}',f'rule {id_} (start image)'))
        elif source=='UI': rows2.append(('ours:layout',f,name,f'BTECH 304E:{off[2:]}',f'ui {id_} (start image)'))
        else: rows2.append(('code',f,name,f'BTECH 304E:{off[2:]}','start image, '+id_))
        continue
    if f in SPANS and off!='-':
        span,base=SPANS[f]
        rows2.append(('extracted',f,name,f'{base}:{off[2:]}',f'story, {span} (native span)'))
        continue
    c=OVR.get(name) or cls(name,off,n)
    if c=='?' : c='code'
    if c=='code:state': c='code'
    rows2.append((c,f,name,off,OWNER.get(name,'-')))
# items the file-scope scan misses
rows2+= [('extracted','MW_MAIN_1AA5.cpp','eventTextStorage / eventTextRecords','-','story, MwStoryBuildEventText'),
         ('extracted','MW_MAIN_1090_EVENTS.cpp','the event script (records, conditions, actions)','-','story, MwStoryBuildCampaignEvents')]
G=[('extracted','Extracted: the original\'s creative content (the story file, first run)'),('ours:ui','Ours: UI words and system messages (story/words.json, imported)'),
   ('ours:layout','Ours: screen layout, panels and menus (data/ui/*.json)'),('ours:rules','Ours: rules and constants (data/rules/*.json)'),
   ('ours:start','Ours: the new campaign\'s starting state (data/rules/new_campaign.json)'),('dropped','Dropped'),('code','Code: runtime state and renderer tables (no file)')]
cnt=collections.Counter(r[0] for r in rows2)
print('# Phase 2 inventory\n\nGenerated from `src/Original/Memory` by `tools/phase2/inventory.py` (every file-scope item). An item read through a door shows where it now comes from: a story piece, a rule piece, a layout piece, a UI word, or a save codec field of `new_campaign.json`, with the exe offset those tables give. Any other item shows the first native offset its importer reads, or `-` for runtime state with no import; its group names the slice that moves it, and the story item not yet moved names its slice. An item an importer decodes from a native span of the story (`story/native.json`) shows the span and the exe offset it holds. A BTECH item read from the BTECH start image (`BtechStartImage.cpp`) shows its row: code, a story span, or a rule or layout piece. The groups follow `docs/phase2/PLAN.md`. Names and offsets only: no MW1 text.\n')
print('| Group | Items |\n|---|---|')
for k,t in G: print(f'| {t} | {cnt[k]} |')
for k,t in G:
    print(f'\n## {t}\n')
    if k=='code':
        byf=collections.Counter(r[1] for r in rows2 if r[0]=='code')
        print('Not listed one by one: '+', '.join(f'{f} {n}' for f,n in sorted(byf.items()))+'.')
        # Slice 6 (docs/phase2/PLAN.md 8) moves the rest off the exe: zero, data or code.
        onexe=collections.Counter(r[1] for r in rows2 if r[0]=='code' and r[3].startswith('0x'))
        print('\nStill read from the exe (slice 6 moves them): '+(', '.join(f'{f} {n}' for f,n in sorted(onexe.items())) or 'none')+'.'); continue
    print('| File | Item | Offset | From |\n|---|---|---|---|')
    for c,f,name,off,src in sorted(r for r in rows2 if r[0]==k): print(f'| {f} | `{name}` | {off} | {src} |')
print("""
## Reclassified or removed in slice 3

- `houseMissionPlanetTableOffsets` (MW_MAIN 1090:8E90) was listed as story content; it is the pointer table of the Houses' mission-planet policies, a rule (`houses.mission_planet_policy_offsets`).
- `repairAttemptFailuresByKind` (MW_MAIN 1090:3A77, 220 bytes) was listed as a rule; it is zero in the exe and written in play, so it is code, zeroed where the exe reloaded it.
- `unusedRepairTimeByDamageTier` (MW_MAIN 1090:3A6F, 8 bytes) was listed as a rule; it was imported and never read, and is gone.
- `combatMissionSideZeroRules` (BTECH 304E:092C, 74 bytes; an alias into `missionRules`) and `mechFallbackTemplateSteps` (BTECH 304E:0992, 16 bytes) were missing; both are rules.
- `mountedWeaponCountsByKind`, `operationalWeaponCountsByKind` (both BTECH 304E:4F32), `selectionObjectListIndices` (6F5A) and `scenarioSelectors` (7D6A) lie past BTECH's file data (304E:4EE0), so the exe holds zeros and play writes them: code.
- `musicSequences` (BTECH 304E:7D8A) was listed as story content; it lies past the file data too, and MUS:INF fills it at run time: code.
- `actuatorConditionLabelIndices`, `engineConditionLabelIndices`, `twoHitSystemConditionLabelIndices` and `orderDisplayIndices` pick labels and move with them in slice 5: UI words.
- Eight start values are always written before they're read, and are code, zeroed where the exe reloaded them (all eight are zero in the exe): `settlementPayoutPercent` (1090:123F), `pilotCardGunneryTier`, `pilotCardPilotingTier`, `pilotCardQuotedWageTier` (2440, 2442, 2446), `missionForceBudget` (4B79), `missionPaymentBaseThousands` (4B7B), `travelDestinationTimeWeight` (54DA, written and never read) and `sideObjectiveRules` (BTECH 304E:0ED0). The other four, `ammunitionTransactionPrices`, `generateStartingJenner`, `generatedAmmoKinds` and `generatedAmmoQuantities`, are read first on some path and stay `"start"` entries in market.json.
- Two of the save codec's 105 fields, `pilotRosterCount` and `rosterPilotIds`, are owned in `src/Original/Campaign`, so this list (of `src/Original/Memory`) doesn't show them; new_campaign.json holds them.

## Reclassified in slice 4

- Every item the layout door fills (`src/Ui/ui_pieces.txt`) is layout, with the piece it comes from. A caption whose head is a layout piece stays with the UI words, since its text is still read from the exe until slice 5 moves it; its row names the head's piece.
- Nine start values are always written before they're read, and are code, zeroed where the exe reloaded them (all nine are zero in the exe): `calendarTextX`, `calendarTextY`, `calendarBackground`, `calendarForeground` (1090:117E..1183, set as the calendar is drawn), `pilotCardTextX` (243C) and `crewScreenRightEdge` (2438, set as the roster card is drawn), `contractRequestTextColour` (4B34, set by the term editor before its only reader), `afterActionRosterRowOffset` (1229, set at each after-action) and `reserveAmmoMechColours` (3F70, set on entering the shop). `lanceOrderPanelTextX` (BTECH 304E:008C) is read first after an EXEC, and is orders.json's `"start"`.
- The font metrics (`fontAdvanceWidths`, `combatFontHeights`, `fontFirstCharacters`, `fontCharacterCounts`, BTECH 304E:2E84..2ED3) are written when a font loads into its slot, and are zero in the exe: code, zeroed where the exe reloaded them.
- `orderMapPositionMask` (BTECH 304E:4EF0) lies past BTECH's file data and is written before it's read: code, zeroed where the exe reloaded it.
- `positionedText` (MW_MAIN 1090:55EC) is the dealer component menu's past-end probe: code, like the other probes.

## Reclassified in slice 5

- Every item the words fill (`src/Import/word_pieces.txt`, through `MwWordsCopyText`, `MwWordsCopyBlock` or an importer's view of the words) is a UI word, with the word it comes from; a caption's row names its text's word (`<caption>.text`). None is read from the exe.
- `positionedText` (1090:55EC) and `travelDestinationName` (5C4E) are now filled from the eighth planet row's and the destination header's caption texts: UI words.
- The pointers into word blocks (`calendarMonthNames`, `weaponLabels`, `weaponRangeClassLabels`, `radarRangeLabels`, `energyAmmoPlaceholder`) show the block they point into.
- The four index tables (`actuatorConditionLabelIndices`, `engineConditionLabelIndices`, `twoHitSystemConditionLabelIndices`, `orderDisplayIndices`) and `notificationTextOffsets` are layout pieces: layout.
- `newsAtStart` and `newsAtEnd` (1090:257F, 2580) are the news screen's flags, zero in the exe and reset as the screen opens: code (their names had sorted them with the words).""")
