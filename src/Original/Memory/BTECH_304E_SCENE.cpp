#include "mechwarrior/combat_scene.h"
CombatSceneState combatScene;
/* Fresh EXEC/BSS projection, not a new game reset procedure. */
void BtechInitializeFreshSceneOwners(void)
{
    combatScene.objectSortOrSelectionScratch=BtechObjectSortScratch{};
    for (uint16_t slot=0;slot<COMBAT_WORLD_OBJECTS;++slot)
    {combatScene.distanceSortObjects[slot]=NULL;combatScene.distanceSortDistances[slot]=0;}
    for (uint16_t bucket=0;bucket<WORLD_DISTANCE_BUCKETS;++bucket)
    {combatScene.distanceBucketEnds[bucket]=0;combatScene.distanceBucketCounts[bucket]=0;}
    combatScene.distanceBucketShift=combatScene.surfacePriorityPrefixCount=0;
    combatScene.nearbyFilterCameraPosition=BtechPosition3D{};
}
