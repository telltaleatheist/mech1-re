/* OPTIONAL HOST PILOT sensor adapter. Reads canonical owners only.
 * Native radar/terrain visibility supplies contacts; the renderer supplies
 * screen coordinates and the shot-selection object. No new LOS/game calls. */
#include "mechwarrior/support_pilot.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_render.h"
#include <string.h>

static PilotPoint ReadPosition(const BtechWorldObjectPrefix *object)
{
    return PilotPoint{(int32_t)object->position.positionA,
        (int32_t)object->position.positionB,(int32_t)object->position.positionC};
}
void SupportPilotObserve(PilotObservation *view,bool combatActive)
{
    memset(view,0,sizeof(*view));view->tick=combatStartup.combatTickCounter;
    if(!combatActive || !combatWorld.playerUnitBinding || !combatWorld.playerMechState || !combatWorld.playerMechTemplate)return;
    const BtechPooledWorldObject *body=combatWorld.playerUnitBinding->positionedLeftLeg;
    if(!body || !combatWorld.playerUnitBinding->torso)return;
    view->cockpit=combatStartup.combatDispatchState==COMBAT_SCREEN_COCKPIT && combatStartup.combatTickCounter!=0;
    view->alive=combatMechs.unitControls[0].conditionScoreOrExitCode>COMBAT_CONDITION_SCORE_MINIMUM;
    view->missionComplete=combatStartup.sideCompletionFlags[0] || combatStartup.sideCompletionFlags[1];
    view->fallen=combatMechs.unitControls[0].movementMode==MOVEMENT_MODE_FALLEN_POSE;
    view->heatInhibited=combatMechs.heatInhibitFlags[0]!=0;view->heat=combatWorld.playerMechState->heatUnits;
    view->aws=combatStartup.mountGroupMode!=0;view->salvo=combatInput.fireCycleActive!=0;
    view->position=ReadPosition(&body->object.prefix);
    view->heading=body->object.prefix.orientationC;
    view->torsoHeading=combatWorld.playerUnitBinding->torso->object.prefix.orientationC;
    view->turnRate=body->orientationSteps[2];view->speed=body->forwardStepScale;
    view->requestedSpeed=(int16_t)combatStartup.requestedPlayerForwardStepScale;
    view->maximumSpeed=(int16_t)combatWorld.playerMechTemplate->maximumSpeedScale;
    view->lockedTarget=combatStartup.playerTargetSlot;view->selectedWeapon=combatStartup.selectedWeaponMount;
    view->reticleX=combatRender.screenCentreA;view->reticleY=(int16_t)combatStartup.aimReticleScreenY;
    for(unsigned slot=1;slot<COMBAT_MECH_SLOTS;++slot)
    {
        const BtechUnitBinding *binding=&combatMechs.unitBindings[slot];
        const BtechPooledWorldObject *drawn=combatStartup.unitLastDrawParts[slot];
        if(!binding->mech || !binding->positionedLeftLeg || !binding->torso)continue;
        PilotTarget *target=&view->targets[slot];
        target->visible=drawn && (drawn->object.prefix.flags&WORLD_OBJECT_TARGET_PROBE_VISIBLE)!=0
            && (drawn->object.prefix.renderFlags&OBJECT_PROJECTED_VISIBLE)!=0;
        target->known=target->visible || combatMechs.unitVisibility[0][slot]!=0;
        if(!target->known)continue;
        target->hostile=combatMechs.unitControls[slot].sideIndex!=combatMechs.unitControls[0].sideIndex;
        target->alive=combatMechs.unitControls[slot].conditionScoreOrExitCode>COMBAT_CONDITION_SCORE_MINIMUM;
        target->position=ReadPosition(&binding->positionedLeftLeg->object.prefix);
        if(target->visible)
        {
            /* CF5C stores the probe point only on OBJECT_LAST_MECH_DRAW_PART.
             * These are exactly the coordinates used by native target HUD6BCA;
             * the torso prefix's projected fields can legitimately remain zero. */
            target->screenX=drawn->object.projectedScreenA;
            target->screenY=drawn->object.projectedScreenB;
            const BtechPooledWorldObject *parts[]={binding->torso,binding->rightLeg,
                binding->positionedLeftLeg,binding->rightArm,binding->leftArm,binding->head};
            for(unsigned part=0;part<6;++part)
                if(parts[part] && combatStartup.selectedHitObject==&parts[part]->object.prefix)target->underReticle=true;
            for(unsigned i=0;i<COMBAT_ARMOR_LOCATIONS;++i)target->armor+=binding->mech->currentArmor[i];
            for(unsigned i=0;i<COMBAT_BODY_LOCATIONS;++i)target->internal+=binding->mech->currentInternalStructure[i];
        }
    }
    if(combatWorld.specialTargetObject && combatWorld.objectiveVisibilityObject &&
        (combatWorld.objectiveVisibilityObject->object.prefix.flags&WORLD_OBJECT_TARGET_PROBE_VISIBLE)
        && (combatWorld.objectiveVisibilityObject->object.prefix.renderFlags&OBJECT_PROJECTED_VISIBLE))
    {
        PilotTarget *target=&view->targets[COMBAT_MECH_SLOTS];
        target->known=target->visible=true;target->alive=combatWorld.specialTargetDamageRemaining!=0;
        target->hostile=combatImport.sideObjectiveRules[0]==OBJECTIVE_SPECIAL_TARGET;
        target->position=ReadPosition(&combatWorld.specialTargetObject->object.prefix);
        target->screenX=combatWorld.objectiveVisibilityObject->object.projectedScreenA;
        target->screenY=combatWorld.objectiveVisibilityObject->object.projectedScreenB;
        target->underReticle=combatStartup.selectedHitObject==&combatWorld.specialTargetObject->object.prefix;
        target->internal=combatWorld.specialTargetDamageRemaining;
    }
    for(unsigned mount=0;mount<combatWorld.playerMechTemplate->weaponMountCount && mount<COMBAT_WEAPON_MOUNTS;++mount)
    {
        int kind=combatWorld.playerMechTemplate->weaponMounts[mount].weaponKind;
        if(kind<0 || kind>=COMBAT_WEAPON_KINDS)continue;
        const BtechWeaponDefinition *weapon=&combatMechs.weaponDefinitions[kind];
        const BtechWeaponState *state=&combatWorld.playerMechState->weapons[mount];
        bool ammunition=weapon->ammoFamily==-1 || (weapon->ammoFamily>=0 && weapon->ammoFamily<COMBAT_AMMO_FAMILIES &&
            combatWorld.playerMechState->ammoSalvosRemaining[weapon->ammoFamily]!=0);
        view->weapons[mount]=PilotWeapon{state->operational && !state->reloadTicksRemaining && ammunition,
            weapon->heatUnits,(uint16_t)(weapon->longRangeBoundary*32*15)};
    }
}
