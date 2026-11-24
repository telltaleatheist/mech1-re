#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_resources.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_effects.h"
#include "mechwarrior/combat_damage.h"
#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_projectiles.h"
#include "mechwarrior/combat_movement.h"
#include "mechwarrior/combat_navigation.h"
#include "mechwarrior/combat_tactics.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/rules.h"
#include <string.h>

CombatWorldState combatWorld;
CombatTacticsState combatTactics;
CombatPlatformState combatPlatform;
CombatResourceState combatResources;
CombatEffectState combatEffects;
BtechBodyDestructionEmitter bodyDestructionEmitters[COMBAT_DESTRUCTION_EMITTER_SLOTS];
CombatBallisticsState combatBallistics;
CombatMovementState combatMovement;
CombatNavigationState combatNavigation;
CombatProjectileState combatProjectiles;
static BtechRenderPrimitive ppcBeamPrimitive,laserBeamPrimitive;
static BtechRenderPolygon ppcBeamPolygons[2],laserBeamPolygons[2];
static uint8_t beamVerticalTriangleIndices[4],beamHorizontalTriangleIndices[4];
uint16_t *const effectResourceSequences[2]={combatEffects.defaultEffectResourceDeltas,combatEffects.machineGunEffectResourceDeltas};

/* Installation IO, not a runtime memory mirror. Strings are resolved exactly
 * once from authored near directories. Runtime consumers use real C strings. */
static uint32_t NullSegmentDword(uint16_t offset)
{return (uint32_t)MwReadNativeWord(combatWorld.nullNearSegment,offset)|((uint32_t)MwReadNativeWord(combatWorld.nullNearSegment,(uint16_t)(offset+2))<<16);}
BtechPosition3D BtechReadNullNearWorldObjectPosition(void)
{return BtechPosition3D{NullSegmentDword(0x0008),NullSegmentDword(0x000C),NullSegmentDword(0x0010)};}
bool BtechImportWorldInitialData(const uint8_t *data,size_t bytes,const uint8_t *segment22B6,size_t segmentBytes)
{
    if (!data || bytes<0x10000 || !segment22B6 || segmentBytes<0x6473) return false;
    if (MwReadNativeWord(data,0x4A6C) || MwReadNativeWord(data,0x4A6E)) return false;
    combatResources.borrowedArchiveWorkspace=NULL;
    memcpy(combatWorld.nullNearSegment,data,sizeof(combatWorld.nullNearSegment));
    memcpy(combatTactics.boundaryMaskByCode,data+0x0EB6,sizeof(combatTactics.boundaryMaskByCode));
    /* The rule tables come from data/rules (src/Rules), laid out as the exe's. */
    if (!MwRulesCopyWords("combat.opposing_side_first_unit_slots",combatTactics.opposingSideFirstUnitSlots,2,0)
        || !MwRulesCopyWords("combat.avoidance_heading_offsets",combatNavigation.avoidanceHeadingOffsets,AVOIDANCE_TURNS,0)) return false;
    for (uint16_t probe=0;probe<AVOIDANCE_PROBES;++probe)
    {
        combatNavigation.avoidanceProbeSignsX[probe]=(int16_t)MwReadNativeWord(data,0x243E +probe*2);
        combatNavigation.avoidanceProbeSignsY[probe]=(int16_t)MwReadNativeWord(data,0x2446+probe*2);
        uint16_t x=MwReadNativeWord(data,0x244E +probe*2),y=MwReadNativeWord(data,0x2456+probe*2);
        if ((x!=0x542A && x!=0x542C) || (y!=0x542A && y!=0x542C)) return false;
        combatNavigation.avoidanceProbeComponentsX[probe]=x==0x542A?&combatNavigation.avoidanceStepX:&combatNavigation.avoidanceStepY;
        combatNavigation.avoidanceProbeComponentsY[probe]=y==0x542A?&combatNavigation.avoidanceStepX:&combatNavigation.avoidanceStepY;
    }
    uint16_t clusterRows[COMBAT_WEAPON_KINDS];
    if (!MwRulesCopyWords("weapons.missile_cluster_rows",clusterRows,COMBAT_WEAPON_KINDS,0)) return false;
    for (uint16_t kind=0;kind<COMBAT_WEAPON_KINDS;++kind) combatProjectiles.missileClusterRowByWeapon[kind]=(int16_t)clusterRows[kind];
    for (uint16_t row=0;row<COMBAT_MISSILE_CLUSTER_ROWS;++row)
        if (!MwRulesCopyWords("combat.missile_cluster_rows",combatProjectiles.missileClusterRows[row],COMBAT_MISSILE_CLUSTER_ROLLS,row*0x18)) return false;
    for (uint16_t arc=0;arc<ATTACK_ARCS;++arc)
        if (!MwRulesCopyWords("combat.hit_locations",combatBallistics.hitLocationRows[arc],HIT_LOCATION_ROLLS,arc*0x16)) return false;
    if (!MwRulesCopyWords("combat.range_hit_modifiers",combatBallistics.rangeHitModifiers,WEAPON_RANGE_BANDS,0)) return false;
    for (uint16_t profile=0;profile<COMBAT_PROJECTILE_PROFILES;++profile)
    {
        uint16_t at=0x0E6E +profile*8;
        combatProjectiles.projectileProfiles[profile]=BtechProjectileProfile{MwReadNativeWord(data,at),MwReadNativeWord(data,at+2),MwReadNativeWord(data,at+4),MwReadNativeWord(data,at+6)};
    }
    if (!MwRulesCopyWords("weapons.projectile_profiles",combatProjectiles.projectileProfileIndicesByWeapon,COMBAT_WEAPON_KINDS,0)
        || !MwRulesCopyWords("combat.body_reference_point",combatProjectiles.bodyLocationReferenceIndices,COMBAT_BODY_LOCATIONS,0)) return false;
    for (uint16_t location=0;location<COMBAT_BODY_LOCATIONS;++location) combatBallistics.muzzleLateralOffsetsByBodyLocation[location]=(int16_t)MwReadNativeWord(data,0x2302+location*2);
    for (uint16_t kind=0;kind<COMBAT_WEAPON_KINDS;++kind) combatBallistics.beamHalfWidths[kind]=MwReadNativeWord(data,0x2802+kind*2);
    memcpy(beamVerticalTriangleIndices,data+0x27C2,4);memcpy(beamHorizontalTriangleIndices,data+0x27C6,4);
    if (beamVerticalTriangleIndices[3]!=POLYGON_VERTEX_LIST_END || beamHorizontalTriangleIndices[3]!=POLYGON_VERTEX_LIST_END) return false;
    for (uint16_t style=0;style<2;++style)
    {
        uint16_t at=style?0x27FE:0x27E2,primitiveAt=MwReadNativeWord(data,at+2);
        BtechRenderVariant *variant=style?&combatBallistics.laserBeamVariant:&combatBallistics.ppcBeamVariant;
        BtechRenderPrimitive *primitive=style?&laserBeamPrimitive:&ppcBeamPrimitive;
        BtechRenderPolygon *polygons=style?laserBeamPolygons:ppcBeamPolygons;
        if (primitiveAt>0xFFF8 || MwReadNativeWord(data,at)!=1 || data[primitiveAt+1]!=RENDER_PRIMITIVE_POLYGON || MwReadNativeWord(data,primitiveAt+2)!=2) return false;
        *variant=BtechRenderVariant{1,primitive};
        primitive->scaleLimit=data[primitiveAt];primitive->kind=data[primitiveAt+1];
        primitive->polygonPayload=BtechRenderPolygonPayload{2,polygons};
        uint16_t polygonAt=MwReadNativeWord(data,primitiveAt+4);
        for (uint16_t polygon=0;polygon<2;++polygon,polygonAt+=8)
        {
            if (polygonAt>0xFFF8 || MwReadNativeWord(data,polygonAt+6)!=(polygon?0x27C6:0x27C2)) return false;
            polygons[polygon]=BtechRenderPolygon{data[polygonAt],data[polygonAt+1],data[polygonAt+2],data[polygonAt+3],data[polygonAt+4],data[polygonAt+5],polygon?beamHorizontalTriangleIndices:beamVerticalTriangleIndices};
        }
    }
    for (uint16_t chassis=0;chassis<COMBAT_MECH_TEMPLATES;++chassis)
        for (uint16_t location=0;location<COMBAT_ARMOR_LOCATIONS;++location)
        {
            uint16_t at=0x0FC8+chassis*0x36+location*6;
            combatBallistics.fallenBodyReferenceBanks[chassis][location]=BtechBodyReferencePoint{(int16_t)MwReadNativeWord(data,at),(int16_t)MwReadNativeWord(data,at+2),(int16_t)MwReadNativeWord(data,at+4)};
        }
    if (!MwRulesCopyWords("assets.body_damage_effect_steps",combatEffects.bodyDamageEffectResourceDeltas,COMBAT_BODY_EFFECT_FRAMES,0)) return false;
    /* The exe's near directory of the two effect step lists (304E:0EAA, 0EAC)
     * is not needed: the lists are read by id. */
    if (!MwRulesCopyWords("assets.default_effect_steps",combatEffects.defaultEffectResourceDeltas,7,0)
        || !MwRulesCopyWords("assets.machine_gun_effect_steps",combatEffects.machineGunEffectResourceDeltas,3,0)) return false;
    for (uint16_t tile=0;tile<TERRAIN_TILE_COUNT;++tile)
    {
        combatWorld.terrainTileGridRowBases[tile]=MwReadNativeWord(data,0x07E4+tile*2);
        combatWorld.terrainTileGridColumnBases[tile]=MwReadNativeWord(data,0x07EC+tile*2);
    }
    for (uint16_t variant=0;variant<16;++variant)
    {
        uint16_t offset=MwReadNativeWord(data,0x07C4+variant*2);
        const uint8_t *end=static_cast<const uint8_t *>(memchr(data+offset,0,0x10000-offset));
        if (!end || (size_t)(end-data-offset)>=sizeof(combatWorld.terrainVariantFilenameSuffixes[variant])) return false;
        memcpy(combatWorld.terrainVariantFilenameSuffixes[variant],data+offset,(size_t)(end-data-offset)+1);
    }
    for (uint16_t chassis=0;chassis<8;++chassis)
    {
        uint16_t offset=MwReadNativeWord(data,0x06DC+chassis*2);
        const uint8_t *end=static_cast<const uint8_t *>(memchr(data+offset,0,0x10000-offset));
        if (!end || (size_t)(end-data-offset)>=sizeof(combatWorld.sceneChassisBankNames[chassis])) return false;
        memcpy(combatWorld.sceneChassisBankNames[chassis],data+offset,(size_t)(end-data-offset)+1);
    }
    memset(combatWorld.sceneChassisResourcePresent,0,sizeof(combatWorld.sceneChassisResourcePresent)); /*06EC*/
    memcpy(combatPlatform.characterClassStorage.bytes,data+0x4C69,sizeof(combatPlatform.characterClassStorage.bytes));
    for (uint16_t index=0;index<0x200;++index) combatGeometryTables.headingRatioAngles[index]=MwReadNativeWord(segment22B6,0x6073+index*2);
    for (uint16_t index=0;index<0x801;++index) combatGeometryTables.orientationCosines[index]=MwReadNativeWord(segment22B6,0x486D+index*2);
    for (uint16_t index=0;index<0x402;++index) combatGeometryTables.coefficientAngles[index]=MwReadNativeWord(segment22B6,0x586F+index*2);
    memcpy(combatGeometryTables.vertexNearPlaneState,segment22B6+0x312C,sizeof(combatGeometryTables.vertexNearPlaneState));
    memcpy(polygonShadeDeltas,segment22B6+0x476D,sizeof(polygonShadeDeltas));
    memcpy(polygonVertexIndices,segment22B6+0x4617,sizeof(polygonVertexIndices));
    uint32_t *wideCoordinates[]={wideScreenX,wideScreenY,widePolygonWorkX,widePolygonWorkY};
    for (uint16_t axis=0;axis<4;++axis)
        for (uint16_t vertex=0;vertex<WIDE_POLYGON_VERTICES;++vertex)
            wideCoordinates[axis][vertex]=MwReadNativeWord(segment22B6,0x462D+axis*0x50+vertex*4)
                |((uint32_t)MwReadNativeWord(segment22B6,0x462F+axis*0x50+vertex*4)<<16);
    BtechGeometryCoordinateCache *coordinateCaches[]={&geometryViewX,&geometryViewDepth,&geometryViewZ,&geometryProjectedX,&geometryProjectedY};
    for (uint16_t axis=0;axis<5;++axis)
        for (uint16_t word=0;word<GEOMETRY_CACHE_VERTICES*2;++word)
            coordinateCaches[axis]->words[word]=MwReadNativeWord(segment22B6,0x322B+axis*0x3FC+word*2);
    combatWorld.scenarioSelectors=BtechScenarioSelectors{}; /* 7D6A, past the file data. */
    return true;
}

/* EXEC/DFCE data projection only, not206D or a new game procedure. Clear
 * converted owners in the native4EDC..D5DF BSS interval; reload authored
 * scalars separately. Host bindings and campaign owners are untouched. */
void BtechInitializeFreshWorldOwners(const uint8_t *data)
{
    combatBallistics.bodyReferenceWorldPosition=BtechPosition3D{};
    combatMovement.jumpGroundHeight=0;
    combatBallistics.aiCandidateTargetSlot=combatBallistics.aiCandidateDistance=0;
    combatNavigation.avoidanceStepX=combatNavigation.avoidanceStepY=0;combatNavigation.avoidancePosition=NULL;combatNavigation.avoidanceOrientation=NULL;
    combatTactics.activeSideRule=NULL;
    memset(combatTactics.unitDistancesToSpecialTarget,0,sizeof(combatTactics.unitDistancesToSpecialTarget));
    memset(combatTactics.tacticalThreatUnitSlots,0,sizeof(combatTactics.tacticalThreatUnitSlots));
    memset(combatTactics.tacticalThreatHeadings,0,sizeof(combatTactics.tacticalThreatHeadings));
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot) combatMovement.unitMovementObstructionHistory[slot]=0;
    for (uint16_t slot=0;slot<COMBAT_PROJECTILE_SLOTS;++slot) combatProjectiles.projectiles[slot]=BtechProjectileState{};
    for (uint16_t row=0;row<COMBAT_MECH_SLOTS;++row)
        for (uint16_t column=0;column<COMBAT_MECH_SLOTS;++column) combatBallistics.unitDistances[row][column]=0;
    for (uint16_t slot=0;slot<COMBAT_BEAM_SLOTS;++slot)
    {
        combatBallistics.beamSlotOccupied[slot]=0;combatBallistics.beamResources[slot]=BtechShapeResource{};
        combatBallistics.beamRenderEntries[slot]=BtechRenderEntry{};combatBallistics.beamWorldObjects[slot]=BtechHeapWorldObject{};
        for (uint16_t vector=0;vector<COMBAT_BEAM_VECTORS;++vector)
            for (uint16_t axis=0;axis<3;++axis) combatBallistics.beamGeometry[slot][vector][axis]=0;
    }
    for (uint16_t slot=0;slot<COMBAT_BODY_EFFECT_SLOTS;++slot) combatEffects.bodyDamageEffects[slot]=BtechBodyDamageEffect{};
    for (uint16_t slot=0;slot<COMBAT_DESTRUCTION_EMITTER_SLOTS;++slot) bodyDestructionEmitters[slot]=BtechBodyDestructionEmitter{};
    for (uint16_t slot=0;slot<COMBAT_EFFECT_SLOTS;++slot) combatEffects.effectSequences[slot]=BtechEffectSequenceState{};
    for (uint16_t byte=0;byte<sizeof(combatResources.archiveStreamScratch);++byte) combatResources.archiveStreamScratch[byte]=0;
    combatWorld.scenarioSelectors=BtechScenarioSelectors{};
    for (uint16_t slot=0;slot<8;++slot)
        combatWorld.unitFormationPositions[slot]=BtechFormationPosition{};
    for (uint16_t slot=0;slot<TERRAIN_GRID_ROWS;++slot)
        for (uint16_t column=0;column<TERRAIN_GRID_COLUMNS;++column)
            combatWorld.terrainHeightGrid[slot][column]=0;
    for (uint16_t slot=0;slot<12;++slot)
        combatWorld.terrainWorldFilename[slot]=0;
    for (uint16_t slot=0;slot<12;++slot)
        combatWorld.terrainGridFilename[slot]=0;
    for (uint16_t slot=0;slot<COMBAT_POOLED_OBJECTS;++slot)
        combatWorld.pooledWorldObjects[slot]=BtechPooledWorldObject{};
    for (uint16_t slot=0;slot<COMBAT_WORLD_OBJECTS;++slot)
        combatWorld.worldObjects[slot]=NULL;
    for (uint16_t slot=0;slot<COMBAT_WORLD_OBJECTS;++slot)
        combatWorld.heapWorldObjectOwned[slot]=0;
    for (uint16_t slot=0;slot<COMBAT_WORLD_OBJECTS;++slot)
        combatWorld.cockpitWorldObjects[slot]=NULL;
    combatWorld.worldObjectCount=0;
    combatWorld.cockpitWorldObjectCount=0;
    combatWorld.distinctSceneMechChassisCount=0;
    for (uint16_t slot=0;slot<COMBAT_CAMERA_CONTEXTS;++slot)
        combatWorld.cameraContexts[slot]=BtechCameraContext{};
    combatWorld.cameraOrientationBasis=BtechOrientationBasis{};
    for (uint16_t slot=0;slot<3;++slot)
        combatWorld.cameraReferenceVector[slot]=0;
    combatWorld.playerUnitBinding=NULL;
    combatWorld.playerMechState=NULL;
    combatWorld.playerMechTemplate=NULL;
    combatWorld.playerPoseObject=NULL;
    combatWorld.orderMapCameraObject=NULL;
    combatWorld.specialTargetObject=NULL;
    combatWorld.objectiveVisibilityObject=NULL;
    combatWorld.orderMapCameraContext=NULL;
    combatWorld.playerPoseCameraContext=NULL;
    combatWorld.specialTargetDamageRemaining=0;
    for (uint16_t slot=0;slot<0x50;++slot)
        combatResources.resourceFilename[slot]=0;
    for (uint16_t slot=0;slot<RESOURCE_ARCHIVE_SLOTS;++slot)
        combatResources.archiveDescriptors[slot]=NULL;
    combatResources.selectedArchive=NULL;
    combatResources.archiveDestination=NULL;
    combatResources.archiveRemainingBytes=0;
    combatResources.archiveIoFlags=0;
    combatResources.lzwInitialLiteralPending=0;
    combatResources.lzwPhraseResumePending=0;
    combatResources.lzwCodeWidth=0;
    combatResources.lzwCodeWidthLimit=0;
    combatResources.lzwNextFreeCode=0;
    combatResources.lzwPacketBitOffset=0;
    combatResources.lzwPacketBitLimit=0;
    combatResources.lzwClearPending=0;
    combatResources.lzwPreviousCode=0;
    combatResources.lzwIncomingCode=0;
    combatResources.lzwPhraseFirstByte=0;
    for (uint16_t slot=0;slot<sizeof(combatWorld.sceneChassisResourcePresent)/sizeof(combatWorld.sceneChassisResourcePresent[0]);++slot) combatWorld.sceneChassisResourcePresent[slot]=0; /*06EC*/
    combatWorld.beforeObjectListCallbackOffset=0; /*2CE8*/
    combatWorld.afterObjectListCallbackOffset=0; /*2CEA*/
    combatPlatform.runtimeDelayActive=data[0x41E4];
    for (uint16_t slot=0;slot<sizeof(combatPlatform.joystickPairInstalled)/sizeof(combatPlatform.joystickPairInstalled[0]);++slot) combatPlatform.joystickPairInstalled[slot]=data[0x41CA+slot*1];
    for (uint16_t slot=0;slot<sizeof(combatPlatform.joystickAxisCentres)/sizeof(combatPlatform.joystickAxisCentres[0]);++slot) combatPlatform.joystickAxisCentres[slot]=data[0x41CC+slot*1];
    combatPlatform.runtimeDelayRemaining=0; /*41E5*/
    /* 41DB, the delay-loop count 1BF5:21FF calibrated, timed game-port reads
     * only and is gone (H3). */
    combatPlatform.joystickFirstAxisValue=0; /*41D9*/
    combatPlatform.joystickDirectionX=0; /*41DD*/
    combatPlatform.joystickDirectionY=0; /*41DF*/
    for (uint16_t slot=0;slot<sizeof(combatPlatform.joystickAxisReciprocals)/sizeof(combatPlatform.joystickAxisReciprocals[0]);++slot) combatPlatform.joystickAxisReciprocals[slot]=0; /*41D0*/
    combatPlatform.combatInputReplayMode=0; /*084C*/
    combatResources.lzwReverseReadOffset=0; /*4AAF*/
    for (uint16_t slot=0;slot<sizeof(combatResources.lzwPacketBytes)/sizeof(combatResources.lzwPacketBytes[0]);++slot) combatResources.lzwPacketBytes[slot]=data[0x4A9A+slot*1];
    combatPlatform.combatInputReplayFilename=NULL;
    combatWorld.chassisResourceBases=BtechChassisResourceBases{};
    combatResources.surfaceResourceBank=BtechSurfaceBank{};
    combatResources.initialShapeResourceBank=BtechResourceBank{};
    combatResources.shapeResourceBanks=&combatResources.initialShapeResourceBank;
    combatResources.shapeResourceBankCount=1;
}
