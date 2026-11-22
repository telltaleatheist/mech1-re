#include "mechwarrior/story.h"
#include <stdio.h>
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/ui_layout.h"
#include <string.h>

uint16_t mechCockpitLayoutIndices[COMBAT_MECH_TEMPLATES];
BtechCockpitLayout cockpitLayouts[COMBAT_COCKPIT_LAYOUTS];
const char *cockpitScreenFilenames[COMBAT_COCKPIT_LAYOUTS];
const char *combatMissionTitles[COMBAT_BRIEFING_MISSIONS];
char mechTemplateNames[COMBAT_MECH_TEMPLATES][13],pilotDefinitionNames[COMBAT_PILOT_NAMES][11];
const char *mechTemplateNameDirectory[COMBAT_MECH_TEMPLATES],*pilotDefinitionNameDirectory[COMBAT_PILOT_NAMES];
const char *inspectionDamageBitmapFilenames[COMBAT_MECH_TEMPLATES];
static char inspectionBitmapNames[64];
char bodyLocationLabels[COMBAT_BODY_LOCATIONS][3],inspectionConditionLabels[6][11];
char inspectionLegendText[2][25],actuatorLabels[COMBAT_ACTUATORS][14];
char inspectionSubsystemLabels[5][14],heatSinksOfText[3],jumpJetsText[14];
char jumpJetsOfText[3],inspectionWeaponHeading[22],inspectionBlankWeaponRow[15];
const char errorInspectionShapeMemory[]="the inspection screen's damage pictures don't fit in memory";
uint16_t twoHitSystemConditionLabelIndices[3],engineConditionLabelIndices[4],actuatorConditionLabelIndices[5];
BtechHudGaugeAnchor inspectionDiagramOrigins[COMBAT_MECH_TEMPLATES];
BtechScreenRectangle inspectionDiagramRegions[COMBAT_MECH_TEMPLATES][COMBAT_ARMOR_LOCATIONS];
const BtechMechTemplate *inspectionMechTemplate;
const BtechMechState *inspectionMechState;
uint16_t cachedJumpChargeGaugeLevel,cachedHeatGaugeLevel,unidentifiedField25;
uint8_t cockpitFullRedrawPending;
BtechHudColourScheme hudColourSchemes[COMBAT_HUD_COLOUR_SCHEMES];
BtechSubsystemLampColours subsystemLampColourTables;
char subsystemLampLabels[4][2];
uint16_t rememberedSensorTargetSlot;
char zoomLabel[7],automaticWeaponsLabel[4],reactorShutdownText[25];
char simulationText[11],pausedText[7],retreatQuestionText[15],retreatChoiceText[14];
char briefingMissionLabel[19],briefingCrewLabel[13],briefingMechLabel[13];
uint16_t savedLineColour,savedFillColour,savedCopySourcePage,savedDrawPage;
uint16_t savedTextBackground,savedTextForeground,savedPolygonFilled,savedPolygonClip;
BtechDecimalFieldAndTargetCache decimalFieldAndTargetCache;
static_assert(offsetof(BtechDecimalFieldAndTargetCache,fields.targetSlot)==8,"53C4 overlaps decimal field tail");
BtechHudGaugeAnchor targetDiagramAnchors[COMBAT_MECH_TEMPLATES];
BtechScreenRectangle targetDiagramRegions[COMBAT_MECH_TEMPLATES][COMBAT_ARMOR_LOCATIONS];
uint16_t radarPreviousTick,radarMarkerCounts[COMBAT_RADAR_CACHE_PAGES];
BtechPooledWorldObject *radarPlayerObject;
uint16_t radarMarkerX[COMBAT_RADAR_MARKERS][COMBAT_RADAR_CACHE_PAGES];
uint16_t radarMarkerY[COMBAT_RADAR_MARKERS][COMBAT_RADAR_CACHE_PAGES];
uint16_t radarCentreX,radarCentreY;
uint16_t radarMarkerBackground[COMBAT_RADAR_MARKERS][COMBAT_RADAR_CACHE_PAGES];
uint16_t radarCachePage,radarMapOriginX,radarMapOriginY;
const BtechScreenRectangle *radarRectangle;
const char *radarRangeLabels[COMBAT_RADAR_RANGE_LABELS];
static char radarRangeText[28];
const char *weaponLabels[COMBAT_WEAPON_KINDS],*weaponRangeClassLabels[4],*energyAmmoPlaceholder;
static char weaponLabelText[60],weaponRangeAndAmmoText[12];
static char cockpitScreenNames[0x20],missionTitleText[0x39B];
static_assert(sizeof(BtechCockpitLayout)==0x6A,"Native cockpit layout stride");

/* Actual executable IO. Authored directories become references to their single
 * string owners; runtime game code never looks them up by DOS address. */
/* A layout piece (data/ui, src/Ui) copied into `view` at its native offset,
 * so the reads below keep the exe's addresses. */
static bool CopyLayoutPiece(uint8_t *view,size_t viewBytes,const char *id,uint16_t offset)
{
    const size_t size=MwUiPieceSize(id);
    return size && offset<=viewBytes && size<=viewBytes-offset && MwUiCopyNative(id,view+offset,size);
}
bool BtechImportCockpitInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x7755) return false;
    /* The layout's pieces, at their native offsets (304E:04D8..2426), and the
     * UI words (story/words.json, cockpit.word.* and briefing.word.*) at
     * theirs (304E:0052..241C), so the reads below keep the exe's addresses. */
    static uint8_t ui[0x2426],words[0x241C];
    memset(ui,0,sizeof(ui));memset(words,0,sizeof(words));
    if (!MwWordsCopyInto("cockpit.word.",words,sizeof(words)) || !MwWordsCopyInto("briefing.word.",words,sizeof(words))) return false;
    if (!CopyLayoutPiece(ui,sizeof(ui),"cockpit.inspection_diagram_origins",0x1878)
        || !CopyLayoutPiece(ui,sizeof(ui),"cockpit.inspection_diagram_regions",0x1898)
        || !CopyLayoutPiece(ui,sizeof(ui),"cockpit.target_diagram_anchors",0x1AD8)
        || !CopyLayoutPiece(ui,sizeof(ui),"cockpit.target_diagram_regions",0x1AF8)
        || !CopyLayoutPiece(ui,sizeof(ui),"cockpit.layout_by_template",0x1D38)
        || !CopyLayoutPiece(ui,sizeof(ui),"cockpit.layouts",0x1D48)
        || !CopyLayoutPiece(ui,sizeof(ui),"cockpit.hud_colour_schemes",0x1F3C)
        || !CopyLayoutPiece(ui,sizeof(ui),"cockpit.subsystem_lamp_colours",0x23CE)
        || !CopyLayoutPiece(ui,sizeof(ui),"cockpit.word_refs.legends",0x04F4)
        || !CopyLayoutPiece(ui,sizeof(ui),"cockpit.word_refs.weapon_labels",0x2286)
        || !CopyLayoutPiece(ui,sizeof(ui),"cockpit.word_refs.radar_ranges",0x2406)
        || !CopyLayoutPiece(ui,sizeof(ui),"cockpit.word_refs.range_classes",0x241C)
        || !CopyLayoutPiece(ui,sizeof(ui),"cockpit.word_refs.energy_ammo",0x2424)
        || !CopyLayoutPiece(ui,sizeof(ui),"cockpit.condition_labels.two_hit",0x04D8)
        || !CopyLayoutPiece(ui,sizeof(ui),"cockpit.condition_labels.engine",0x04DE)
        || !CopyLayoutPiece(ui,sizeof(ui),"cockpit.condition_labels.actuator",0x04E6)) return false;
    inspectionMechTemplate=NULL;inspectionMechState=NULL; /* Fresh native BSS only. */
    memcpy(inspectionBitmapNames,data+0x034E,sizeof(inspectionBitmapNames));
    for (uint16_t chassis=0;chassis<COMBAT_MECH_TEMPLATES;++chassis)
    {
        uint16_t offset=MwReadNativeWord(data,0x04A8+chassis*2);
        if (offset<0x034E || offset>=0x038E || !memchr(data+offset,0,0x038E -offset)) return false;
        inspectionDamageBitmapFilenames[chassis]=inspectionBitmapNames+offset-0x034E;
        inspectionDiagramOrigins[chassis]=BtechHudGaugeAnchor{(int16_t)MwReadNativeWord(ui,0x1878+chassis*4),(int16_t)MwReadNativeWord(ui,0x187A+chassis*4)};
        for (uint16_t armor=0;armor<COMBAT_ARMOR_LOCATIONS;++armor)
        {
            uint16_t at=0x1898+chassis*0x48+armor*8;
            inspectionDiagramRegions[chassis][armor]=BtechScreenRectangle{(int16_t)MwReadNativeWord(ui,at),(int16_t)MwReadNativeWord(ui,at+2),
                (int16_t)MwReadNativeWord(ui,at+4),(int16_t)MwReadNativeWord(ui,at+6)};
        }
    }
    memcpy(bodyLocationLabels,words+0x038E,sizeof(bodyLocationLabels));
    memcpy(inspectionConditionLabels,words+0x03A6,sizeof(inspectionConditionLabels));
    memcpy(actuatorLabels,words+0x046C,sizeof(actuatorLabels));
    memcpy(inspectionSubsystemLabels,words+0x0520,sizeof(inspectionSubsystemLabels));
    memcpy(heatSinksOfText,words+0x0566,sizeof(heatSinksOfText));
    memcpy(jumpJetsText,words+0x0569,sizeof(jumpJetsText));
    memcpy(jumpJetsOfText,words+0x0577,sizeof(jumpJetsOfText));
    memcpy(inspectionWeaponHeading,words+0x057A,sizeof(inspectionWeaponHeading));
    memcpy(inspectionBlankWeaponRow,words+0x0590,sizeof(inspectionBlankWeaponRow));
    for (uint16_t legend=0;legend<2;++legend)
    {
        uint16_t offset=MwReadNativeWord(ui,0x04F4+legend*2);
        if (offset>=sizeof(words)) return false;
        const uint8_t *end=static_cast<const uint8_t *>(memchr(words+offset,0,sizeof(words)-offset));
        if (!end || (size_t)(end-words-offset)>=sizeof(inspectionLegendText[legend])) return false;
        memcpy(inspectionLegendText[legend],words+offset,(size_t)(end-words-offset)+1);
    }
    for (uint16_t index=0;index<3;++index) twoHitSystemConditionLabelIndices[index]=MwReadNativeWord(ui,0x04D8+index*2);
    for (uint16_t index=0;index<4;++index) engineConditionLabelIndices[index]=MwReadNativeWord(ui,0x04DE +index*2);
    for (uint16_t index=0;index<5;++index) actuatorConditionLabelIndices[index]=MwReadNativeWord(ui,0x04E6+index*2);
    memcpy(simulationText,words+0x0076,sizeof(simulationText));
    memcpy(pausedText,words+0x0081,sizeof(pausedText));
    memcpy(retreatQuestionText,words+0x082F,sizeof(retreatQuestionText)); /* 14 characters and their NUL (083E starts the next) */
    memcpy(retreatChoiceText,words+0x083E,sizeof(retreatChoiceText));
    memcpy(briefingMissionLabel,words+0x0D96,sizeof(briefingMissionLabel));
    memcpy(briefingCrewLabel,words+0x0DA9,sizeof(briefingCrewLabel));
    memcpy(briefingMechLabel,words+0x0DB6,sizeof(briefingMechLabel));
    if (briefingMissionLabel[sizeof(briefingMissionLabel)-1] || briefingCrewLabel[sizeof(briefingCrewLabel)-1]
        || briefingMechLabel[sizeof(briefingMechLabel)-1]) return false;
    savedLineColour=0; /*4F7E*/savedFillColour=0; /*4F80*/
    savedCopySourcePage=0; /*4F82*/savedDrawPage=0; /*4F84*/
    savedTextBackground=0; /*4F86*/savedTextForeground=0; /*4F88*/
    savedPolygonFilled=0; /*4F8A*/savedPolygonClip=0; /*4F8C*/
    rememberedSensorTargetSlot=0; /*4EDC*/
    radarPreviousTick=MwReadNativeWord(data,0x240E);
    memcpy(weaponLabelText,words+0x2004,sizeof(weaponLabelText));
    memcpy(weaponRangeAndAmmoText,words+0x2410,sizeof(weaponRangeAndAmmoText));
    for (uint16_t weapon=0;weapon<COMBAT_WEAPON_KINDS;++weapon)
    {
        uint16_t offset=MwReadNativeWord(ui,0x2286+weapon*2);
        if (offset<0x2004 || offset>=0x2040 || !memchr(words+offset,0,0x2040-offset)) return false;
        weaponLabels[weapon]=weaponLabelText+offset-0x2004;
    }
    for (uint16_t range=0;range<4;++range)
    {
        uint16_t offset=MwReadNativeWord(ui,0x241C+range*2);
        if (offset<0x2410 || offset>=0x241C || !memchr(words+offset,0,0x241C-offset)) return false;
        weaponRangeClassLabels[range]=weaponRangeAndAmmoText+offset-0x2410;
    }
    uint16_t ammoOffset=MwReadNativeWord(ui,0x2424);
    if (ammoOffset<0x2410 || ammoOffset>=0x241C || !memchr(words+ammoOffset,0,0x241C-ammoOffset)) return false;
    energyAmmoPlaceholder=weaponRangeAndAmmoText+ammoOffset-0x2410;
    memcpy(radarRangeText,words+0x23EA,sizeof(radarRangeText));
    for (uint16_t range=0;range<COMBAT_RADAR_RANGE_LABELS;++range)
    {
        uint16_t offset=MwReadNativeWord(ui,0x2406+range*2);
        if (offset<0x23EA || offset>=0x2406 || !memchr(words+offset,0,0x2406-offset)) return false;
        radarRangeLabels[range]=radarRangeText+offset-0x23EA;
    }
    memcpy(zoomLabel,words+0x0052,sizeof(zoomLabel));
    memcpy(automaticWeaponsLabel,words+0x0059,sizeof(automaticWeaponsLabel));
    memcpy(reactorShutdownText,words+0x005D,sizeof(reactorShutdownText));
    if (zoomLabel[6] || automaticWeaponsLabel[3] || reactorShutdownText[24]) return false;
    memcpy(subsystemLampLabels,words+0x23BE,sizeof(subsystemLampLabels));
    for (uint16_t label=0;label<4;++label) if (subsystemLampLabels[label][1]) return false;
    for (uint16_t word=0;word<14;++word) subsystemLampColourTables.words[word]=MwReadNativeWord(ui,0x23CE +word*2);
    for (uint16_t scheme=0;scheme<COMBAT_HUD_COLOUR_SCHEMES;++scheme)
    {
        BtechHudColourScheme *colours=&hudColourSchemes[scheme];uint16_t base=0x1F3C+scheme*0x64;
#define HUD_COLOUR(field,offset) colours->field=MwReadNativeWord(ui,base+(offset))
        HUD_COLOUR(inspectionBackground,0x00);HUD_COLOUR(inspectionSubsystemText,0x02);
        HUD_COLOUR(inspectionEquipmentText,0x04);HUD_COLOUR(inspectionEquipmentHeading,0x06);
        HUD_COLOUR(inspectionFooter,0x08);HUD_COLOUR(inspectionMechName,0x0A);
        HUD_COLOUR(orderHighlight,0x0C);HUD_COLOUR(orderReferenceHighlight,0x0E);
        for (uint16_t i=0;i<5;++i) HUD_COLOUR(orderConditionColours[i],0x10+i*2);
        HUD_COLOUR(radarPlayer,0x1A);HUD_COLOUR(radarFriendly,0x1C);HUD_COLOUR(radarOpposing,0x1E);
        HUD_COLOUR(radarSpecialTarget,0x20);HUD_COLOUR(radarBackground,0x22);
        for (uint16_t i=0;i<4;++i) HUD_COLOUR(radarTerrainBands[i],0x24+i*2);
        HUD_COLOUR(weaponReady,0x2C);HUD_COLOUR(weaponUnavailable,0x2E);HUD_COLOUR(weaponReloading,0x30);
        HUD_COLOUR(ammoText,0x32);HUD_COLOUR(selectedMount,0x34);HUD_COLOUR(unselectedMount,0x36);
        HUD_COLOUR(forwardTicks,0x38);HUD_COLOUR(reverseTicks,0x3A);HUD_COLOUR(movementZeroReference,0x3C);
        HUD_COLOUR(selectedMovementTick,0x3E);HUD_COLOUR(jumpRising,0x40);HUD_COLOUR(jumpFalling,0x42);
        HUD_COLOUR(jumpCharged,0x44);HUD_COLOUR(jumpBelowThreshold,0x46);HUD_COLOUR(jumpReference,0x48);
        HUD_COLOUR(heatRising,0x4A);HUD_COLOUR(heatFalling,0x4C);
        for (uint16_t i=0;i<5;++i) HUD_COLOUR(rangeBuckets[i],0x4E + i*2);
        for (uint16_t i=0;i<2;++i) HUD_COLOUR(unidentifiedField17[i],0x58+i*2);
        HUD_COLOUR(undamaged,0x5C);HUD_COLOUR(aboveHalfCondition,0x5E);HUD_COLOUR(damaged,0x60);HUD_COLOUR(destroyed,0x62);
#undef HUD_COLOUR
    }
    memcpy(cockpitScreenNames,data+0x07F4,sizeof(cockpitScreenNames));
    memcpy(missionTitleText,data+0x09F0,sizeof(missionTitleText));
    /* The names come from the story file (combat.mech_template.N.name,
     * combat.pilot.N.name); the directories stay the exe's, and each name is
     * written at its directory offset in its own record, as the exe holds it. */
    memset(mechTemplateNames,0,sizeof(mechTemplateNames));
    memset(pilotDefinitionNames,0,sizeof(pilotDefinitionNames));
    for (uint16_t name=0;name<COMBAT_MECH_TEMPLATES;++name)
    {
        uint16_t offset=MwReadNativeWord(data,0x229C+name*2);
        if (offset<0x2045 || offset>=0x20AD) return false;
        char *at=(char *)mechTemplateNames+offset-0x2045,id[40];
        size_t room=sizeof(mechTemplateNames[0])<(size_t)(0x20AD-offset)?sizeof(mechTemplateNames[0]):(size_t)(0x20AD-offset);
        snprintf(id,sizeof(id),"combat.mech_template.%u.name",(unsigned)name+1);
        if (!MwStoryCopyText(id,at,room)) return false;
        mechTemplateNameDirectory[name]=at;
    }
    for (uint16_t name=0;name<COMBAT_PILOT_NAMES;++name)
    {
        uint16_t offset=MwReadNativeWord(data,0x22AC+name*2);
        if (offset<0x20AD || offset>=0x2286) return false;
        char *at=(char *)pilotDefinitionNames+offset-0x20AD,id[40];
        size_t room=sizeof(pilotDefinitionNames[0])<(size_t)(0x2286-offset)?sizeof(pilotDefinitionNames[0]):(size_t)(0x2286-offset);
        snprintf(id,sizeof(id),"combat.pilot.%u.name",(unsigned)name+1);
        if (!MwStoryCopyText(id,at,room)) return false;
        pilotDefinitionNameDirectory[name]=at;
    }
    for (uint16_t name=0;name<COMBAT_MECH_TEMPLATES;++name)
        if (mechTemplateNames[name][12]) return false;
    for (uint16_t name=0;name<COMBAT_PILOT_NAMES;++name)
        if (pilotDefinitionNames[name][10]) return false;
    for (uint16_t mission=0;mission<COMBAT_BRIEFING_MISSIONS;++mission)
    {
        uint16_t offset=MwReadNativeWord(data,0x0DD2+mission*2);
        if (offset<0x09F0 || offset>=0x0D8B || !memchr(data+offset,0,0x0D8B-offset)) return false;
        combatMissionTitles[mission]=missionTitleText+offset-0x09F0;
    }
    for (uint16_t chassis=0;chassis<COMBAT_MECH_TEMPLATES;++chassis)
    {
        mechCockpitLayoutIndices[chassis]=MwReadNativeWord(ui,0x1D38+chassis*2);
        targetDiagramAnchors[chassis]=BtechHudGaugeAnchor{(int16_t)MwReadNativeWord(ui,0x1AD8+chassis*4),(int16_t)MwReadNativeWord(ui,0x1ADA+chassis*4)};
        for (uint16_t armor=0;armor<COMBAT_ARMOR_LOCATIONS;++armor)
        {
            uint16_t offset=0x1AF8+chassis*0x48+armor*8;
            targetDiagramRegions[chassis][armor]=BtechScreenRectangle{(int16_t)MwReadNativeWord(ui,offset),(int16_t)MwReadNativeWord(ui,offset+2),
                (int16_t)MwReadNativeWord(ui,offset+4),(int16_t)MwReadNativeWord(ui,offset+6)};
        }
    }
    for (uint16_t index=0;index<COMBAT_COCKPIT_LAYOUTS;++index)
    {
        uint16_t offset=MwReadNativeWord(data,0x0814+index*2);
        if (offset<0x07F4 || offset>=0x0814 || !memchr(data+offset,0,0x0814-offset)) return false;
        cockpitScreenFilenames[index]=cockpitScreenNames+offset-0x07F4;
        offset=0x1D48+index*0x6A;
        BtechCockpitLayout *layout=&cockpitLayouts[index];
        layout->viewport=BtechScreenRectangle{(int16_t)MwReadNativeWord(ui,offset),(int16_t)MwReadNativeWord(ui,offset+2),(int16_t)MwReadNativeWord(ui,offset+4),(int16_t)MwReadNativeWord(ui,offset+6)};
        layout->radarPanel=BtechScreenRectangle{(int16_t)MwReadNativeWord(ui,offset+8),(int16_t)MwReadNativeWord(ui,offset+0x0A),(int16_t)MwReadNativeWord(ui,offset+0x0C),(int16_t)MwReadNativeWord(ui,offset+0x0E)};
        layout->targetPanel=BtechScreenRectangle{(int16_t)MwReadNativeWord(ui,offset+0x10),(int16_t)MwReadNativeWord(ui,offset+0x12),(int16_t)MwReadNativeWord(ui,offset+0x14),(int16_t)MwReadNativeWord(ui,offset+0x16)};
        layout->weaponListX=(int16_t)MwReadNativeWord(ui,offset+0x18);layout->weaponListY=(int16_t)MwReadNativeWord(ui,offset+0x1A);
        layout->heatGauge=BtechHudGaugeAnchor{(int16_t)MwReadNativeWord(ui,offset+0x1C),(int16_t)MwReadNativeWord(ui,offset+0x1E)};
        layout->movementGauge=BtechHudGaugeAnchor{(int16_t)MwReadNativeWord(ui,offset+0x20),(int16_t)MwReadNativeWord(ui,offset+0x22)};
        layout->jumpChargeGauge=BtechHudGaugeAnchor{(int16_t)MwReadNativeWord(ui,offset+0x24),(int16_t)MwReadNativeWord(ui,offset+0x26)};
        for (uint16_t anchor=0;anchor<3;++anchor)
        {
            layout->firingIndicatorBank0[anchor]=BtechHudGaugeAnchor{(int16_t)MwReadNativeWord(ui,offset+0x28+anchor*4),(int16_t)MwReadNativeWord(ui,offset+0x2A+anchor*4)};
            layout->firingIndicatorBank1[anchor]=BtechHudGaugeAnchor{(int16_t)MwReadNativeWord(ui,offset+0x34+anchor*4),(int16_t)MwReadNativeWord(ui,offset+0x36+anchor*4)};
        }
        memcpy(layout->unidentifiedField15,ui+offset+0x40,sizeof(layout->unidentifiedField15));
        layout->headingStripY=(int16_t)MwReadNativeWord(ui,offset+0x44);
        for (uint16_t placement=0;placement<COMBAT_COCKPIT_PLACEMENTS;++placement)
            layout->bitmapPlacements[placement]=BtechCockpitBitmapPlacement{MwReadNativeWord(ui,offset+0x46+placement*6),(int16_t)MwReadNativeWord(ui,offset+0x48+placement*6),(int16_t)MwReadNativeWord(ui,offset+0x4A+placement*6)};
    }
    cachedJumpChargeGaugeLevel=0; /*5338*/
    cachedHeatGaugeLevel=0; /*533A*/unidentifiedField25=0; /*533C*/
    cockpitFullRedrawPending=data[0x7754];
    return true;
}
