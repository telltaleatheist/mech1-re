#include "mechwarrior/story.h"
#include <stdio.h>
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/btech_start_image.h"
#include "mechwarrior/ui_layout.h"
#include <string.h>

CombatCockpitState combatCockpit;
static char inspectionBitmapNames[64];
const char errorInspectionShapeMemory[]="the inspection screen's damage pictures don't fit in memory";
static_assert(offsetof(BtechDecimalFieldAndTargetCache,fields.targetSlot)==8,"53C4 overlaps decimal field tail");
static char radarRangeText[28];
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
bool BtechImportCockpitInitialData(void)
{
    const BtechStartRowData fileInspectionBitmapsRow("file.inspection_bitmaps");
    const BtechStartRowData fileInspectionBitmapOffsetsRow("file.inspection_bitmap_offsets");
    const BtechStartRowData fileCockpitScreensRow("file.cockpit_screens");
    const BtechStartRowData fileCockpitScreenOffsetsRow("file.cockpit_screen_offsets");
    const BtechStartRowData namesMechTemplateDirectoryRow("names.mech_template_directory");
    const BtechStartRowData namesPilotDirectoryRow("names.pilot_directory");
    const BtechStartRowData cockpitRadarPreviousTickRow("cockpit.radar_previous_tick");
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
    combatCockpit.inspectionMechTemplate=NULL;combatCockpit.inspectionMechState=NULL; /* Fresh native BSS only. */
    fileInspectionBitmapsRow.copy(inspectionBitmapNames,sizeof(inspectionBitmapNames));
    for (uint16_t chassis=0;chassis<COMBAT_MECH_TEMPLATES;++chassis)
    {
        uint16_t offset=fileInspectionBitmapOffsetsRow.word(chassis*2);
        fileInspectionBitmapsRow.text(fileInspectionBitmapsRow.index(offset)); /* Its name ends in the row. */
        combatCockpit.inspectionDamageBitmapFilenames[chassis]=inspectionBitmapNames+fileInspectionBitmapsRow.index(offset);
        combatCockpit.inspectionDiagramOrigins[chassis]=BtechHudGaugeAnchor{(int16_t)MwReadNativeWord(ui,0x1878+chassis*4),(int16_t)MwReadNativeWord(ui,0x187A+chassis*4)};
        for (uint16_t armor=0;armor<COMBAT_ARMOR_LOCATIONS;++armor)
        {
            uint16_t at=0x1898+chassis*0x48+armor*8;
            combatCockpit.inspectionDiagramRegions[chassis][armor]=BtechScreenRectangle{(int16_t)MwReadNativeWord(ui,at),(int16_t)MwReadNativeWord(ui,at+2),
                (int16_t)MwReadNativeWord(ui,at+4),(int16_t)MwReadNativeWord(ui,at+6)};
        }
    }
    memcpy(combatCockpit.bodyLocationLabels,words+0x038E,sizeof(combatCockpit.bodyLocationLabels));
    memcpy(combatCockpit.inspectionConditionLabels,words+0x03A6,sizeof(combatCockpit.inspectionConditionLabels));
    memcpy(combatCockpit.actuatorLabels,words+0x046C,sizeof(combatCockpit.actuatorLabels));
    memcpy(combatCockpit.inspectionSubsystemLabels,words+0x0520,sizeof(combatCockpit.inspectionSubsystemLabels));
    memcpy(combatCockpit.heatSinksOfText,words+0x0566,sizeof(combatCockpit.heatSinksOfText));
    memcpy(combatCockpit.jumpJetsText,words+0x0569,sizeof(combatCockpit.jumpJetsText));
    memcpy(combatCockpit.jumpJetsOfText,words+0x0577,sizeof(combatCockpit.jumpJetsOfText));
    memcpy(combatCockpit.inspectionWeaponHeading,words+0x057A,sizeof(combatCockpit.inspectionWeaponHeading));
    memcpy(combatCockpit.inspectionBlankWeaponRow,words+0x0590,sizeof(combatCockpit.inspectionBlankWeaponRow));
    for (uint16_t legend=0;legend<2;++legend)
    {
        uint16_t offset=MwReadNativeWord(ui,0x04F4+legend*2);
        if (offset>=sizeof(words)) return false;
        const uint8_t *end=static_cast<const uint8_t *>(memchr(words+offset,0,sizeof(words)-offset));
        if (!end || (size_t)(end-words-offset)>=sizeof(combatCockpit.inspectionLegendText[legend])) return false;
        memcpy(combatCockpit.inspectionLegendText[legend],words+offset,(size_t)(end-words-offset)+1);
    }
    for (uint16_t index=0;index<3;++index) combatCockpit.twoHitSystemConditionLabelIndices[index]=MwReadNativeWord(ui,0x04D8+index*2);
    for (uint16_t index=0;index<4;++index) combatCockpit.engineConditionLabelIndices[index]=MwReadNativeWord(ui,0x04DE +index*2);
    for (uint16_t index=0;index<5;++index) combatCockpit.actuatorConditionLabelIndices[index]=MwReadNativeWord(ui,0x04E6+index*2);
    memcpy(combatCockpit.simulationText,words+0x0076,sizeof(combatCockpit.simulationText));
    memcpy(combatCockpit.pausedText,words+0x0081,sizeof(combatCockpit.pausedText));
    memcpy(combatCockpit.retreatQuestionText,words+0x082F,sizeof(combatCockpit.retreatQuestionText)); /* 14 characters and their NUL (083E starts the next) */
    memcpy(combatCockpit.retreatChoiceText,words+0x083E,sizeof(combatCockpit.retreatChoiceText));
    memcpy(combatCockpit.briefingMissionLabel,words+0x0D96,sizeof(combatCockpit.briefingMissionLabel));
    memcpy(combatCockpit.briefingCrewLabel,words+0x0DA9,sizeof(combatCockpit.briefingCrewLabel));
    memcpy(combatCockpit.briefingMechLabel,words+0x0DB6,sizeof(combatCockpit.briefingMechLabel));
    if (combatCockpit.briefingMissionLabel[sizeof(combatCockpit.briefingMissionLabel)-1] || combatCockpit.briefingCrewLabel[sizeof(combatCockpit.briefingCrewLabel)-1]
        || combatCockpit.briefingMechLabel[sizeof(combatCockpit.briefingMechLabel)-1]) return false;
    combatCockpit.savedLineColour=0; /*4F7E*/combatCockpit.savedFillColour=0; /*4F80*/
    combatCockpit.savedCopySourcePage=CombatPage::None; /*4F82*/combatCockpit.savedDrawPage=CombatPage::None; /*4F84*/
    combatCockpit.savedTextBackground=0; /*4F86*/combatCockpit.savedTextForeground=0; /*4F88*/
    combatCockpit.savedPolygonFilled=0; /*4F8A*/combatCockpit.savedPolygonClip=0; /*4F8C*/
    combatCockpit.rememberedSensorTargetSlot=0; /*4EDC*/
    combatCockpit.radarPreviousTick=cockpitRadarPreviousTickRow.word();
    memcpy(weaponLabelText,words+0x2004,sizeof(weaponLabelText));
    memcpy(weaponRangeAndAmmoText,words+0x2410,sizeof(weaponRangeAndAmmoText));
    for (uint16_t weapon=0;weapon<COMBAT_WEAPON_KINDS;++weapon)
    {
        uint16_t offset=MwReadNativeWord(ui,0x2286+weapon*2);
        if (offset<0x2004 || offset>=0x2040 || !memchr(words+offset,0,0x2040-offset)) return false;
        combatCockpit.weaponLabels[weapon]=weaponLabelText+offset-0x2004;
    }
    for (uint16_t range=0;range<4;++range)
    {
        uint16_t offset=MwReadNativeWord(ui,0x241C+range*2);
        if (offset<0x2410 || offset>=0x241C || !memchr(words+offset,0,0x241C-offset)) return false;
        combatCockpit.weaponRangeClassLabels[range]=weaponRangeAndAmmoText+offset-0x2410;
    }
    uint16_t ammoOffset=MwReadNativeWord(ui,0x2424);
    if (ammoOffset<0x2410 || ammoOffset>=0x241C || !memchr(words+ammoOffset,0,0x241C-ammoOffset)) return false;
    combatCockpit.energyAmmoPlaceholder=weaponRangeAndAmmoText+ammoOffset-0x2410;
    memcpy(radarRangeText,words+0x23EA,sizeof(radarRangeText));
    for (uint16_t range=0;range<COMBAT_RADAR_RANGE_LABELS;++range)
    {
        uint16_t offset=MwReadNativeWord(ui,0x2406+range*2);
        if (offset<0x23EA || offset>=0x2406 || !memchr(words+offset,0,0x2406-offset)) return false;
        combatCockpit.radarRangeLabels[range]=radarRangeText+offset-0x23EA;
    }
    memcpy(combatCockpit.zoomLabel,words+0x0052,sizeof(combatCockpit.zoomLabel));
    memcpy(combatCockpit.automaticWeaponsLabel,words+0x0059,sizeof(combatCockpit.automaticWeaponsLabel));
    memcpy(combatCockpit.reactorShutdownText,words+0x005D,sizeof(combatCockpit.reactorShutdownText));
    if (combatCockpit.zoomLabel[6] || combatCockpit.automaticWeaponsLabel[3] || combatCockpit.reactorShutdownText[24]) return false;
    memcpy(combatCockpit.subsystemLampLabels,words+0x23BE,sizeof(combatCockpit.subsystemLampLabels));
    for (uint16_t label=0;label<4;++label) if (combatCockpit.subsystemLampLabels[label][1]) return false;
    for (uint16_t word=0;word<14;++word) combatCockpit.subsystemLampColourTables.words[word]=MwReadNativeWord(ui,0x23CE +word*2);
    for (uint16_t scheme=0;scheme<COMBAT_HUD_COLOUR_SCHEMES;++scheme)
    {
        BtechHudColourScheme *colours=&combatCockpit.hudColourSchemes[scheme];uint16_t base=0x1F3C+scheme*0x64;
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
    fileCockpitScreensRow.copy(cockpitScreenNames,sizeof(cockpitScreenNames));
    /* The names come from the story file (combat.mech_template.N.name,
     * combat.pilot.N.name); the directories stay the exe's, and each name is
     * written at its directory offset in its own record, as the exe holds it. */
    memset(combatCockpit.mechTemplateNames,0,sizeof(combatCockpit.mechTemplateNames));
    memset(combatCockpit.pilotDefinitionNames,0,sizeof(combatCockpit.pilotDefinitionNames));
    for (uint16_t name=0;name<COMBAT_MECH_TEMPLATES;++name)
    {
        uint16_t offset=namesMechTemplateDirectoryRow.word(name*2);
        if (offset<0x2045 || offset>=0x20AD) return false;
        char *at=(char *)combatCockpit.mechTemplateNames+offset-0x2045,id[40];
        size_t room=sizeof(combatCockpit.mechTemplateNames[0])<(size_t)(0x20AD-offset)?sizeof(combatCockpit.mechTemplateNames[0]):(size_t)(0x20AD-offset);
        snprintf(id,sizeof(id),"combat.mech_template.%u.name",(unsigned)name+1);
        if (!MwStoryCopyText(id,at,room)) return false;
        combatCockpit.mechTemplateNameDirectory[name]=at;
    }
    for (uint16_t name=0;name<COMBAT_PILOT_NAMES;++name)
    {
        uint16_t offset=namesPilotDirectoryRow.word(name*2);
        if (offset<0x20AD || offset>=0x2286) return false;
        char *at=(char *)combatCockpit.pilotDefinitionNames+offset-0x20AD,id[40];
        size_t room=sizeof(combatCockpit.pilotDefinitionNames[0])<(size_t)(0x2286-offset)?sizeof(combatCockpit.pilotDefinitionNames[0]):(size_t)(0x2286-offset);
        snprintf(id,sizeof(id),"combat.pilot.%u.name",(unsigned)name+1);
        if (!MwStoryCopyText(id,at,room)) return false;
        combatCockpit.pilotDefinitionNameDirectory[name]=at;
    }
    for (uint16_t name=0;name<COMBAT_MECH_TEMPLATES;++name)
        if (combatCockpit.mechTemplateNames[name][12]) return false;
    for (uint16_t name=0;name<COMBAT_PILOT_NAMES;++name)
        if (combatCockpit.pilotDefinitionNames[name][10]) return false;
    /* The mission titles come from the story file (combat.mission.N.title),
     * end to end with their NULs as the exe holds them (304E:09F0, its
     * directory at 0DD2), each title pointing at its own. */
    {
        size_t used=0;
        for (uint16_t mission=0;mission<COMBAT_BRIEFING_MISSIONS;++mission)
        {
            char id[40];
            snprintf(id,sizeof(id),"combat.mission.%u.title",(unsigned)mission+1);
            if (used>=sizeof(missionTitleText) || !MwStoryCopyText(id,missionTitleText+used,sizeof(missionTitleText)-used)) return false;
            combatCockpit.combatMissionTitles[mission]=missionTitleText+used;
            used+=strlen(missionTitleText+used)+1;
        }
    }
    for (uint16_t chassis=0;chassis<COMBAT_MECH_TEMPLATES;++chassis)
    {
        combatCockpit.mechCockpitLayoutIndices[chassis]=MwReadNativeWord(ui,0x1D38+chassis*2);
        combatCockpit.targetDiagramAnchors[chassis]=BtechHudGaugeAnchor{(int16_t)MwReadNativeWord(ui,0x1AD8+chassis*4),(int16_t)MwReadNativeWord(ui,0x1ADA+chassis*4)};
        for (uint16_t armor=0;armor<COMBAT_ARMOR_LOCATIONS;++armor)
        {
            uint16_t offset=0x1AF8+chassis*0x48+armor*8;
            combatCockpit.targetDiagramRegions[chassis][armor]=BtechScreenRectangle{(int16_t)MwReadNativeWord(ui,offset),(int16_t)MwReadNativeWord(ui,offset+2),
                (int16_t)MwReadNativeWord(ui,offset+4),(int16_t)MwReadNativeWord(ui,offset+6)};
        }
    }
    for (uint16_t index=0;index<COMBAT_COCKPIT_LAYOUTS;++index)
    {
        uint16_t offset=fileCockpitScreenOffsetsRow.word(index*2);
        fileCockpitScreensRow.text(fileCockpitScreensRow.index(offset)); /* Its name ends in the row. */
        combatCockpit.cockpitScreenFilenames[index]=cockpitScreenNames+fileCockpitScreensRow.index(offset);
        offset=0x1D48+index*0x6A;
        BtechCockpitLayout *layout=&combatCockpit.cockpitLayouts[index];
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
    combatCockpit.cachedJumpChargeGaugeLevel=0; /*5338*/
    combatCockpit.cachedHeatGaugeLevel=0; /*533A*/combatCockpit.unidentifiedField25=0; /*533C*/
    combatCockpit.cockpitFullRedrawPending=0; /*7754*/
    return true;
}
