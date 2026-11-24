#include "mechwarrior/combat_pose.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/rules.h"

CombatPoseState combatPose;

/* Executable IO only: forty authored eight-byte records and their directory.
 * Runtime uses ordinary keyframe references, not a near-address registry. */
bool BtechImportPoseInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0xD1F0) return false;
    if (MwReadNativeWord(data,0x7750) || MwReadNativeWord(data,0xC35C)
        || MwReadNativeWord(data,0xC946) || MwReadNativeWord(data,0xD1EE)) return false;
    combatPose.activeUnitPositionedObject=NULL;combatPose.activeUnitTorsoObject=NULL;
    combatPose.activeMechState=NULL;combatPose.activeMechTemplate=NULL;
    for (uint16_t byte=0;byte<UNIT_ANIMATION_MAP_BYTES;++byte)
        combatPose.animationFrameMapStorage[byte]=data[0x245E +byte];
    for (uint16_t sequence=0;sequence<UNIT_ANIMATION_SEQUENCES;++sequence)
    {
        uint16_t offset=MwReadNativeWord(data,0x248E +sequence*2);
        if (offset<0x245E || offset>=0x248E) return false;
        combatPose.animationFrameMaps[sequence]=&combatPose.animationFrameMapStorage[offset-0x245E];
        combatPose.animationLogicalFrameCounts[sequence]=data[0x24A0+sequence];
        combatPose.animationPartFrameCounts[sequence]=data[0x24AA+sequence];
    }
    /* The rule tables come from data/rules (src/Rules), laid out as the exe's. */
    if (!MwRulesCopyWords("assets.animation_prefix_counts",combatPose.animationPrefixSequenceCounts,UNIT_ANIMATION_SEQUENCES,0)) return false;
    for (uint16_t chassis=0;chassis<COMBAT_MECH_TEMPLATES;++chassis)
        for (uint16_t sequence=0;sequence<UNIT_ANIMATION_SEQUENCES;++sequence)
            combatPose.chassisAnimationAvailability[chassis][sequence]=data[0x24C6+chassis*UNIT_ANIMATION_SEQUENCES+sequence];
    for (uint16_t frame=0;frame<PLAYER_POSE_KEYFRAMES;++frame)
    {
        uint16_t offset=0x250E +frame*PLAYER_POSE_KEYFRAME_BYTES;
        combatPose.playerPoseKeyframes[frame]=BtechPoseKeyframe{(int16_t)MwReadNativeWord(data,offset),
            (int16_t)MwReadNativeWord(data,offset+2),(int16_t)MwReadNativeWord(data,offset+4),MwReadNativeWord(data,offset+6)};
    }
    for (uint16_t sequence=0;sequence<PLAYER_POSE_SEQUENCES;++sequence)
    {
        uint16_t offset=MwReadNativeWord(data,0x264E +sequence*2);
        if (offset<0x250E || offset>=0x264E || (offset-0x250E)%PLAYER_POSE_KEYFRAME_BYTES) return false;
        combatPose.playerPoseSequences[sequence]=&combatPose.playerPoseKeyframes[(offset-0x250E)/PLAYER_POSE_KEYFRAME_BYTES];
    }
    if (!MwRulesCopyWords("assets.player_pose_frame_counts",combatPose.playerPoseFrameCounts,PLAYER_POSE_SEQUENCES,0)) return false;
    combatPose.playerPoseDistanceAccumulator=0; /*5432*/
    combatPose.playerPosePerturbationTicks=0; /*5434*/
    combatPose.playerPoseSequence=0; /*5436*/
    combatPose.playerPoseFrame=0; /*5438*/
    combatPose.unidentifiedField26=0; /*543A*/
    return true;
}
