#include "mechwarrior/combat_pose.h"
#include "mechwarrior/btech_start_image.h"
#include "mechwarrior/rules.h"

CombatPoseState combatPose;

/* Executable IO only: forty authored eight-byte records and their directory.
 * Runtime uses ordinary keyframe references, not a near-address registry. */
bool BtechImportPoseInitialData(void)
{
    const BtechStartRowData animationFrameMapsRow("animation.frame_maps");
    const BtechStartRowData animationFrameMapOffsetsRow("animation.frame_map_offsets");
    const BtechStartRowData animationLogicalFrameCountsRow("animation.logical_frame_counts");
    const BtechStartRowData animationPartFrameCountsRow("animation.part_frame_counts");
    const BtechStartRowData mechsAnimationsRow("mechs.animations");
    const BtechStartRowData animationPoseKeyframesRow("animation.pose_keyframes");
    const BtechStartRowData animationPoseOffsetsRow("animation.pose_offsets");
    combatPose.activeUnitPositionedObject=NULL;combatPose.activeUnitTorsoObject=NULL;
    combatPose.activeMechState=NULL;combatPose.activeMechTemplate=NULL;
    for (uint16_t byte=0;byte<UNIT_ANIMATION_MAP_BYTES;++byte)
        combatPose.animationFrameMapStorage[byte]=animationFrameMapsRow.byte(byte);
    for (uint16_t sequence=0;sequence<UNIT_ANIMATION_SEQUENCES;++sequence)
    {
        uint16_t offset=animationFrameMapOffsetsRow.word(sequence*2);
        if (offset<0x245E || offset>=0x248E) return false;
        combatPose.animationFrameMaps[sequence]=&combatPose.animationFrameMapStorage[offset-0x245E];
        combatPose.animationLogicalFrameCounts[sequence]=animationLogicalFrameCountsRow.byte(sequence);
        combatPose.animationPartFrameCounts[sequence]=animationPartFrameCountsRow.byte(sequence);
    }
    /* The rule tables come from data/rules (src/Rules), laid out as the exe's. */
    if (!MwRulesCopyWords("assets.animation_prefix_counts",combatPose.animationPrefixSequenceCounts,UNIT_ANIMATION_SEQUENCES,0)) return false;
    for (uint16_t chassis=0;chassis<COMBAT_MECH_TEMPLATES;++chassis)
        for (uint16_t sequence=0;sequence<UNIT_ANIMATION_SEQUENCES;++sequence)
            combatPose.chassisAnimationAvailability[chassis][sequence]=mechsAnimationsRow.byte(chassis*UNIT_ANIMATION_SEQUENCES+sequence);
    for (uint16_t frame=0;frame<PLAYER_POSE_KEYFRAMES;++frame)
    {
        uint16_t offset=frame*PLAYER_POSE_KEYFRAME_BYTES;
        combatPose.playerPoseKeyframes[frame]=BtechPoseKeyframe{(int16_t)animationPoseKeyframesRow.word(offset),
            (int16_t)animationPoseKeyframesRow.word(offset+2),(int16_t)animationPoseKeyframesRow.word(offset+4),animationPoseKeyframesRow.word(offset+6)};
    }
    for (uint16_t sequence=0;sequence<PLAYER_POSE_SEQUENCES;++sequence)
    {
        uint16_t offset=animationPoseOffsetsRow.word(sequence*2);
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
