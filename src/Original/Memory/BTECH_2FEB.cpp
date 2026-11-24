#include "mechwarrior/combat_sound.h"
#include "mechwarrior/native_address.h"

CombatToneState combatTones;

static_assert(sizeof(BtechToneChannelState)==0x30,"Native tone record");

bool BtechImportToneInitialData(const uint8_t *data,size_t bytes,const uint8_t *toneData,size_t toneBytes)
{
    if (!data || bytes<0x4DFE || !toneData || toneBytes<0x622) return false;
    /* Shipped priority owner is2FEB, NOT304E. Other selectors require recovery
     * of their actual owning field, not a new segment address registry. */
    if (MwReadNativeWord(data,0x4DFC)!=0x2FEB) return false;
    combatSound.samplePriorityOwner=&combatTones.sampleSoundPriority;
    combatTones.sampleSoundPriority=MwReadNativeWord(toneData,0x00AF);
    combatTones.soundOutputEnableMask=toneData[0];
    combatTones.musicNoteBase=toneData[1];combatTones.musicPitchTableOffset=MwReadNativeWord(toneData,2);
    combatTones.musicChannelBias=toneData[4];combatTones.musicPriority=MwReadNativeWord(toneData,5);
    for (uint16_t index=0;index<8;++index) combatTones.musicHandlerOffsets[index]=MwReadNativeWord(toneData,7+index*2);
    for (uint16_t index=0;index<TONE_SPEAKER_PITCH_WORDS;++index) combatTones.speakerMusicPeriods[index]=MwReadNativeWord(toneData,TONE_SPEAKER_PITCH_TABLE+index*2);
    for (uint16_t index=0;index<TONE_TANDY_PITCH_WORDS;++index) combatTones.tandyMusicPeriods[index]=MwReadNativeWord(toneData,TONE_TANDY_PITCH_TABLE+index*2);
    for (uint16_t index=0;index<TONE_NOISE_PERIOD_WORDS;++index) combatTones.noiseMusicPeriods[index]=MwReadNativeWord(toneData,0x61C+index*2);
    combatTones.bytecodeReturnOffset=MwReadNativeWord(toneData,0x00F0);
    combatTones.currentChannelDisplacement=MwReadNativeWord(toneData,0x003D);
    combatTones.selectedSpeakerPriority=MwReadNativeWord(toneData,0x001B);
    if (MwReadNativeWord(toneData,0x00F4)) return false;
    combatTones.selectedSpeakerState=NULL;
    for (uint16_t entry=0;entry<4;++entry) combatTones.bytecodeBranchHandlers[entry]=MwReadNativeWord(toneData,0x00F6+entry*2);
    for (uint16_t channel=0;channel<COMBAT_TONE_CHANNELS;++channel)
    {
        combatTones.toneEventData[channel]=MwNativeByteView{};combatTones.toneFallbackData[channel]=MwNativeByteView{};
        combatTones.fallbackAllocationOffsets[channel]=MwReadNativeWord(toneData,0x003F+channel*2);
        combatTones.levelSequenceDirectoryOffsets[channel]=MwReadNativeWord(toneData,0x007F+channel*2);
        combatTones.soundEventIndices[channel]=MwReadNativeWord(toneData,0x001D+channel*2);
        combatTones.soundPriorities[channel]=MwReadNativeWord(toneData,0x009F+channel*2);
        combatTones.channelFlags[channel]=BtechToneChannelFlags{toneData[0x00DF+channel*2],toneData[0x00E0+channel*2]};
        uint16_t offset=MwReadNativeWord(toneData,0x039E +channel*2);
        if (offset<0x00FE || (offset-0x00FE)%0x30 || (offset-0x00FE)/0x30>=COMBAT_TONE_CHANNELS) return false;
        combatTones.toneChannelStates[channel]=&combatTones.toneChannels[(offset-0x00FE)/0x30];
        /* All authored state WORDs are zero. Reject a different image rather
         * than interpreting an aligned host record as little-endian file data. */
        for (uint16_t byte=0;byte<0x30;++byte) if (toneData[0x00FE +channel*0x30+byte]) return false;
        combatTones.toneChannels[channel]=BtechToneChannelState{};
    }
    combatSound.soundTimerInstalled=data[0x28D0];
    combatSound.savedSoundTimerDivisor=MwReadNativeWord(data,0x28D1);
    combatSound.timerSoundEventIndex=MwReadNativeWord(data,0x28F2);
    return true;
}
