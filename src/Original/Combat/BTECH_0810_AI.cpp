#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_tactics.h"
#include "mechwarrior/combat_movement.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>
#include "mechwarrior/combat_scene.h"

/*0810:71ED. The actual non-player update chain, not a replacement AI tick.
 * Global control finishes one-past the eight records unless739C rebinds it. */
void BtechUpdateAiMechUnits(void)
{
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot) combatMechs.unitControls[slot].targetSelectionCount=0;
    BtechRefreshLastVisibleUnitPositions();
    BtechRecalculateUnitConditionScoresAndExitCodes();
    BtechUpdateUnitWeaponRangeThresholds();
    BtechUpdateSideCompletionFlags();
    combatImport.activeUnitControl=&combatMechs.unitControls[1];
    for (uint16_t slot=1;slot<COMBAT_MECH_SLOTS;++slot,++combatImport.activeUnitControl)
    {
        combatPose.activeMechState=combatMechs.unitBindings[slot].mech;
        if (!combatPose.activeMechState) continue;
        if (combatImport.activeUnitControl->sideIndex>=2) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeUnitControl->sideIndex>=2");
        combatTactics.activeSideRule=&combatImport.sideObjectiveRules[combatImport.activeUnitControl->sideIndex];
        combatPose.activeUnitPositionedObject=combatMechs.unitBindings[slot].positionedLeftLeg;
        if (combatPose.activeMechState->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeMechState->templateIndex>=COMBAT_MECH_TEMPLATES");
        combatPose.activeMechTemplate=&combatMechs.mechTemplates[combatPose.activeMechState->templateIndex];
        combatPose.activeUnitTorsoObject=combatMechs.unitBindings[slot].torso;
        if (combatImport.activeUnitControl->conditionScoreOrExitCode<COMBAT_MECH_DISABLED_VALUE) continue;
        if (!combatPose.activeUnitPositionedObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitPositionedObject");
        if (combatImport.activeUnitControl->movementMode==MOVEMENT_MODE_JUMP)
        {
            if (combatImport.activeUnitControl->conditionScoreOrExitCode!=COMBAT_MECH_DISABLED_VALUE
                && combatPose.activeMechState->currentInternalStructure[COMBAT_BODY_RIGHT_LEG]
                && combatPose.activeMechState->currentInternalStructure[COMBAT_BODY_LEFT_LEG]) goto tacticalUpdate;
            BtechSetActiveUnitGroundOrJumpMotion(combatPose.activeUnitPositionedObject->object.prefix.orientationC,MOTION_DIRECTION_STOP);
            BtechAdvanceActiveAiUnitMotion();
            goto animate;
        }
        if (!combatPose.activeMechState->currentInternalStructure[COMBAT_BODY_LEFT_LEG] || !combatPose.activeMechState->currentInternalStructure[COMBAT_BODY_RIGHT_LEG])
        {
            combatImport.activeUnitControl->animationRequestFlags=ANIMATION_REQUEST_FALL_POSE;
            combatImport.activeUnitControl->biasedHeadingSnapshot=combatPose.activeUnitPositionedObject->object.prefix.orientationC+AVOIDANCE_HALF_TURN;
        }
tacticalUpdate:
        if (combatImport.activeUnitControl->conditionScoreOrExitCode!=COMBAT_MECH_DISABLED_VALUE)
        {
            uint16_t mode=combatImport.activeUnitControl->movementMode;
            if (mode!=MOVEMENT_MODE_FALLEN_POSE && mode!=MOVEMENT_MODE_FALLING && mode!=MOVEMENT_MODE_RECOVERING)
            {
                BtechUpdateTacticalNavigation();
                if ((int16_t)combatPose.activeMechState->gyroHits<COMBAT_GYRO_DESTROYED_HITS
                    && combatPose.activeMechState->actuatorCondition[AI_LEFT_LEG_ACTUATOR] && combatPose.activeMechState->actuatorCondition[AI_RIGHT_LEG_ACTUATOR])
                {
                    BtechAdvanceActiveAiUnitMotion();
                    if ((int16_t)combatPose.activeMechState->sensorHits<COMBAT_SENSORS_DESTROYED_HITS) BtechChooseAiTargetAndFire();
                }
            }
            else if (mode==MOVEMENT_MODE_FALLEN_POSE)
            {
                if (combatPose.activeMechState->currentInternalStructure[COMBAT_BODY_LEFT_LEG] && combatPose.activeMechState->currentInternalStructure[COMBAT_BODY_RIGHT_LEG]
                    && combatPose.activeMechState->actuatorCondition[AI_LEFT_LEG_ACTUATOR] && combatPose.activeMechState->actuatorCondition[AI_RIGHT_LEG_ACTUATOR]
                    && !((uint8_t)combatStartup.combatTickCounter&AI_RECOVERY_TICK_MASK))
                {
                    if (combatImport.activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS");
                    int16_t roll=(int16_t)BtechRandomBelowBound(combatImport.pilotingRecoveryBounds[combatImport.activeUnitControl->unitSlot]);
                    combatImport.activeUnitControl->animationRequestFlags=roll<AI_RECOVERY_ROLL_LIMIT?ANIMATION_REQUEST_RECOVER:ANIMATION_REQUEST_FAILED_RECOVERY;
                }
            }
        }
animate:
        {
            BtechAdvanceUnitAnimationAndMovementMode();
            BtechSynchronizeMechRenderParts();
        }
    }
    BtechCoordinateSpecialObjectiveUnits();
}

/*0810:739C. Side ZERO's signed rule gates the whole routine. Worklist targets
 * are consumed even when reassignment fails. Formation waypoints use terrain height. */
void BtechCoordinateSpecialObjectiveUnits(void)
{
    if ((int16_t)combatImport.sideObjectiveRules[0]<=OBJECTIVE_OPPOSE_WITHDRAWAL) return;
    if (!combatWorld.specialTargetObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !specialTargetObject");
    BtechPosition3D *objective=&combatWorld.specialTargetObject->object.prefix.position;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        if (!combatMechs.unitBindings[slot].mech) {combatTactics.unitDistancesToSpecialTarget[slot]=0;continue;}
        if (!combatMechs.unitBindings[slot].positionedLeftLeg) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !unitBindings[slot].positionedLeftLeg");
        combatTactics.unitDistancesToSpecialTarget[slot]=(int32_t)BtechApproximatePlanarDistance(&combatMechs.unitBindings[slot].positionedLeftLeg->object.prefix.position,objective);
    }
    for (uint16_t side=0;side<2;++side)
    {
        if (combatImport.sideObjectiveRules[side]!=OBJECTIVE_DEFEND_SPECIAL_TARGET) continue;
        bool noTargetsRemaining=false;
        for (uint16_t pass=0;pass<COMBAT_LANCE_SLOTS && !noTargetsRemaining;++pass)
        {
            int32_t nearestTarget=OBJECTIVE_CANDIDATE_DISTANCE_LIMIT;
            uint16_t threat=0,firstTarget=combatTactics.opposingSideFirstUnitSlots[side];
            if (firstTarget>COMBAT_MECH_SLOTS-COMBAT_LANCE_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: firstTarget>COMBAT_MECH_SLOTS-COMBAT_LANCE_SLOTS");
            for (uint16_t slot=firstTarget;slot<firstTarget+COMBAT_LANCE_SLOTS;++slot)
                if (combatTactics.unitDistancesToSpecialTarget[slot] && combatMechs.unitControls[slot].conditionScoreOrExitCode>COMBAT_CONDITION_SCORE_MINIMUM && combatTactics.unitDistancesToSpecialTarget[slot]<nearestTarget)
                {nearestTarget=combatTactics.unitDistancesToSpecialTarget[slot];threat=slot;}
            if (nearestTarget==OBJECTIVE_CANDIDATE_DISTANCE_LIMIT) {noTargetsRemaining=true;continue;}
            int32_t nearestDefender=OBJECTIVE_CANDIDATE_DISTANCE_LIMIT;
            uint16_t defender=0,firstDefender=combatTactics.opposingSideFirstUnitSlots[1-side];
            bool alreadyCovered=false;
            if (firstDefender>COMBAT_MECH_SLOTS-COMBAT_LANCE_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: firstDefender>COMBAT_MECH_SLOTS-COMBAT_LANCE_SLOTS");
            int32_t incomingDistance=combatTactics.unitDistancesToSpecialTarget[threat];
            for (uint16_t slot=firstDefender;slot<firstDefender+COMBAT_LANCE_SLOTS;++slot)
            {
                int32_t objectiveDistance=combatTactics.unitDistancesToSpecialTarget[slot],targetDistance=(int32_t)combatBallistics.unitDistances[slot][threat];
                BtechUnitControlState *control=&combatMechs.unitControls[slot];
                if (!objectiveDistance || targetDistance<=0 || objectiveDistance>=OBJECTIVE_DEFENDER_RADIUS || control->conditionScoreOrExitCode<=COMBAT_CONDITION_SCORE_MINIMUM || targetDistance>=nearestDefender) continue;
                if (control->tacticalState!=TACTICAL_DEFENDING)
                {
                    if (control->targetUnitSlot==threat && objectiveDistance<incomingDistance) {alreadyCovered=true;break;}
                }
                else {nearestDefender=targetDistance;defender=slot;}
            }
            if (!alreadyCovered && nearestDefender!=OBJECTIVE_CANDIDATE_DISTANCE_LIMIT)
            {
                int32_t doubledDistance=(uint32_t)combatTactics.unitDistancesToSpecialTarget[defender]<<1;
                if (combatTactics.unitDistancesToSpecialTarget[threat]<doubledDistance)
                {
                    combatMechs.unitControls[defender].targetUnitSlot=threat;combatMechs.unitControls[defender].tacticalState=TACTICAL_CHARGING;
                    combatTactics.unitDistancesToSpecialTarget[defender]=0;
                    BtechSelectActiveUnitContext(defender);
                    if (!combatMechs.unitBindings[threat].positionedLeftLeg || !combatPose.activeUnitPositionedObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !unitBindings[threat].positionedLeftLeg || !activeUnitPositionedObject");
                    BtechPosition3D *target=&combatMechs.unitBindings[threat].positionedLeftLeg->object.prefix.position;
                    BtechPosition3D *root=&combatPose.activeUnitPositionedObject->object.prefix.position;
                    BtechApplyActiveUnitTacticalSteering(BtechCalculateVectorHeading(target->positionA-root->positionA,target->positionB-root->positionB));
                }
            }
            combatTactics.unitDistancesToSpecialTarget[threat]=0;
        }
    }
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        combatPose.activeMechState=combatMechs.unitBindings[slot].mech;
        if (!combatPose.activeMechState || combatMechs.unitControls[slot].tacticalState!=TACTICAL_DEFENDING) continue;
        BtechSelectActiveUnitContext(slot);
        BtechPosition3D destination={0};
        if (combatImport.activeUnitControl->commandCode==COMBAT_ORDER_ACT_ON_OWN)
        {
            destination.positionA=combatWorld.unitFormationPositions[slot].positionA;
            destination.positionB=combatWorld.unitFormationPositions[slot].positionB;
            destination.positionC=(int16_t)BtechSampleTerrainOrSurfaceHeight(&destination);
        }
        else
        {
            const int32_t *order=combatImport.activeUnitControl->orderDestinationPosition;
            destination=BtechPosition3D{(uint32_t)order[0],(uint32_t)order[1],(uint32_t)order[2]};
        }
        BtechNavigateActiveUnitTowardPosition(&destination);
    }
}
