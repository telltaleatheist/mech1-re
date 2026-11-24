#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_tactics.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>
#include "mechwarrior/combat_scene.h"

/* Select the BattleMech, chassis, body objects and orders evaluated by the AI.
 *0810:7766. Ordered writes bind the original five active owners. */
void BtechSelectActiveUnitContext(uint16_t unitSlot)
{
    if (unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: unitSlot>=COMBAT_MECH_SLOTS");
    BtechUnitBinding *binding=&unitBindings[unitSlot];
    combatPose.activeMechState=binding->mech;
    if (!combatPose.activeMechState || combatPose.activeMechState->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeMechState || activeMechState->templateIndex>=COMBAT_MECH_TEMPLATES");
    combatPose.activeMechTemplate=&mechTemplates[combatPose.activeMechState->templateIndex];
    combatPose.activeUnitPositionedObject=binding->positionedLeftLeg;
    combatPose.activeUnitTorsoObject=binding->torso;
    activeUnitControl=&unitControls[unitSlot];
}

/*0810:7A80. Reload is deliberately ignored. WORD sums wrap before SAR;
 * equality at half maximum internal structure requests disengagement. */
uint16_t BtechShouldDisengageForWeaponsOrStructure(void)
{
    if (!combatPose.activeMechTemplate || !combatPose.activeMechState) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeMechTemplate || !activeMechState");
    int16_t mountIndex=0,weaponMountCount=(int16_t)combatPose.activeMechTemplate->weaponMountCount;
    for (;mountIndex<weaponMountCount;++mountIndex)
    {
        if (mountIndex>=COMBAT_WEAPON_MOUNTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mountIndex>=COMBAT_WEAPON_MOUNTS");
        int16_t weaponKind=combatPose.activeMechTemplate->weaponMounts[mountIndex].weaponKind;
        if (weaponKind<0 || weaponKind>=COMBAT_WEAPON_KINDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: weaponKind<0 || weaponKind>=COMBAT_WEAPON_KINDS");
        int16_t ammoFamily=weaponDefinitions[weaponKind].ammoFamily;
        if ((uint16_t)ammoFamily!=COMBAT_AMMO_NONE)
        {
            if ((uint16_t)ammoFamily>=COMBAT_AMMO_FAMILIES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)ammoFamily>=COMBAT_AMMO_FAMILIES");
            if (!combatPose.activeMechState->ammoSalvosRemaining[ammoFamily]) continue;
        }
        if (combatPose.activeMechState->weapons[mountIndex].operational) break;
    }
    if (mountIndex==weaponMountCount) return 1;
    uint16_t maximumStructure=0,currentStructure=0;
    for (uint16_t location=0;location<COMBAT_BODY_LOCATIONS;++location)
    {
        maximumStructure+=combatPose.activeMechTemplate->maximumInternalStructure[location];
        currentStructure+=combatPose.activeMechState->currentInternalStructure[location];
    }
    int16_t halfMaximumStructure=(maximumStructure>>1)|(maximumStructure&0x8000);
    return halfMaximumStructure>=(int16_t)currentStructure;
}

/*0810:7B5A. Special-target geometry precedes ordinary visibility checks.
 * Signed thresholds are sign-extended WORDs; equality is not nearby. */
uint8_t BtechHasNearbyVisibleThreat(void)
{
    if (!activeUnitControl || activeUnitControl->sideIndex>=2) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitControl || activeUnitControl->sideIndex>=2");
    BtechUnitControlState *control=activeUnitControl;
    if (sideObjectiveRules[control->sideIndex]==OBJECTIVE_SPECIAL_TARGET && specialTargetDamageRemaining)
    {
        if (!combatPose.activeUnitPositionedObject || !specialTargetObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitPositionedObject || !specialTargetObject");
        int32_t distance=(int32_t)BtechApproximatePlanarDistance(&combatPose.activeUnitPositionedObject->object.prefix.position,&specialTargetObject->object.prefix.position);
        if (control->readyWeaponRangeMinusMargin>distance
            && (control->commandCode==COMBAT_ORDER_ACT_ON_OWN || control->commandCode==TACTICAL_ORDER_ATTACK || control->commandCode==TACTICAL_ORDER_MOVE_ATTACK)) return 1;
    }
    if (control->unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: control->unitSlot>=COMBAT_MECH_SLOTS");
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
        if (unitBindings[slot].mech && unitControls[slot].sideIndex!=control->sideIndex
            && unitVisibility[control->unitSlot][slot] && unitControls[slot].conditionScoreOrExitCode>COMBAT_CONDITION_SCORE_MINIMUM
            && (int32_t)unitDistances[control->unitSlot][slot]<(int16_t)control->twiceReadyWeaponRange)
        {
            return 1;
        }
    return 0;
}

/*0810:7C48. The selected target contributes LAST-VISIBLE, not live position.
 * Snapshot transport is safe:94B0 writes tracked+20, never last-visible+2E. */
void BtechRefreshTargetAndNavigate(void)
{
    BtechScoreAndSelectTacticalTarget();
    if (activeUnitControl->targetUnitSlot>=0)
    {
        uint16_t targetUnitSlot=activeUnitControl->targetUnitSlot;
        if (targetUnitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: targetUnitSlot>=COMBAT_MECH_SLOTS"); /* Native special/adjacent-control alias. */
        const int32_t *lastSeen=unitControls[targetUnitSlot].lastVisiblePosition;
        BtechPosition3D destination={(uint32_t)lastSeen[0],(uint32_t)lastSeen[1],(uint32_t)lastSeen[2]};
        BtechNavigateActiveUnitTowardPosition(&destination);
        activeUnitControl->navigationReferenceSlot=activeUnitControl->targetUnitSlot;
    }
    else
    {
        if (!combatPose.activeUnitPositionedObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitPositionedObject");
        BtechSteerActiveUnitAroundObstacles(combatPose.activeUnitPositionedObject->object.prefix.orientationC,1);
    }
}

/*0810:7D3A. Only operational, reloaded mounts count; ammunition does NOT.
 * Native also computes an unused minimum. Absent slots retain stale ranges. */
void BtechUpdateUnitWeaponRangeThresholds(void)
{
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        BtechMechState *mech=unitBindings[slot].mech;
        if (!mech) continue;
        if (mech->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->templateIndex>=COMBAT_MECH_TEMPLATES");
        BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
        int16_t longestReadyWeaponRange=0;
        for (int16_t mount=0;mount<(int16_t)chassis->weaponMountCount;++mount)
        {
            if (mount>=COMBAT_WEAPON_MOUNTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mount>=COMBAT_WEAPON_MOUNTS");
            if (!mech->weapons[mount].operational || mech->weapons[mount].reloadTicksRemaining) continue;
            int16_t weaponKind=chassis->weaponMounts[mount].weaponKind;
            if (weaponKind<0 || weaponKind>=COMBAT_WEAPON_KINDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: weaponKind<0 || weaponKind>=COMBAT_WEAPON_KINDS");
            int16_t longRangeBoundary=weaponDefinitions[weaponKind].longRangeBoundary;
            if (longRangeBoundary>longestReadyWeaponRange) longestReadyWeaponRange=longRangeBoundary;
        }
        BtechUnitControlState *control=&unitControls[slot];
        control->twiceReadyWeaponRange=(uint16_t)(longestReadyWeaponRange*TACTICAL_RANGE_SCALE*2);
        control->readyWeaponRangeMinusMargin=(int16_t)(longestReadyWeaponRange*TACTICAL_RANGE_SCALE-TACTICAL_RANGE_MARGIN);
        uint16_t readyWeaponRange=longestReadyWeaponRange*TACTICAL_RANGE_SCALE;
        control->quarterReadyWeaponRange=(int16_t)((readyWeaponRange>>TACTICAL_QUARTER_RANGE_SHIFT)|((readyWeaponRange&0x8000)?0xC000:0));
    }
}
