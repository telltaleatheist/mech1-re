#include "mechwarrior/combat_sound.h"
#include "mechwarrior/btech_start_image.h"

CombatToneState combatTones;

static_assert(sizeof(BtechToneChannelState)==0x30,"Native tone record");

bool BtechImportToneInitialData(void)
{
    const BtechStartRowData toneHeaderRow("tone.header");
    const BtechStartRowData toneChannelsRow("tone.channels");
    const BtechStartRowData toneChannelOffsetsRow("tone.channel_offsets");
    const BtechStartRowData toneSpeakerPeriodsRow("tone.speaker_periods");
    const BtechStartRowData toneTandyPeriodsRow("tone.tandy_periods");
    const BtechStartRowData toneNoisePeriodsRow("tone.noise_periods");
    const BtechStartRowData audioSoundTimerRow("audio.sound_timer");
    const BtechStartRowData audioTimerSoundEventRow("audio.timer_sound_event");
    const BtechStartRowData runtimeExitFlushRow("runtime.exit_flush");
    /* Shipped priority owner is2FEB, NOT304E. Other selectors require recovery
     * of their actual owning field, not a new segment address registry. */
    if (runtimeExitFlushRow.word(2)!=0x2FEB) return false;
    combatSound.samplePriorityOwner=&combatTones.sampleSoundPriority;
    combatTones.sampleSoundPriority=toneHeaderRow.word(0xAF);
    combatTones.soundOutputEnableMask=toneHeaderRow.byte();
    combatTones.musicNoteBase=toneHeaderRow.byte(1);combatTones.musicPitchTableOffset=toneHeaderRow.word(2);
    combatTones.musicChannelBias=toneHeaderRow.byte(4);combatTones.musicPriority=toneHeaderRow.word(5);
    for (uint16_t index=0;index<8;++index) combatTones.musicHandlerOffsets[index]=toneHeaderRow.word(7+index*2);
    for (uint16_t index=0;index<TONE_SPEAKER_PITCH_WORDS;++index) combatTones.speakerMusicPeriods[index]=toneSpeakerPeriodsRow.word(index*2);
    for (uint16_t index=0;index<TONE_TANDY_PITCH_WORDS;++index) combatTones.tandyMusicPeriods[index]=toneTandyPeriodsRow.word(index*2);
    for (uint16_t index=0;index<TONE_NOISE_PERIOD_WORDS;++index) combatTones.noiseMusicPeriods[index]=toneNoisePeriodsRow.word(index*2);
    combatTones.bytecodeReturnOffset=toneHeaderRow.word(0xF0);
    combatTones.currentChannelDisplacement=toneHeaderRow.word(0x3D);
    combatTones.selectedSpeakerPriority=toneHeaderRow.word(0x1B);
    if (toneHeaderRow.word(0xF4)) return false;
    combatTones.selectedSpeakerState=NULL;
    for (uint16_t entry=0;entry<4;++entry) combatTones.bytecodeBranchHandlers[entry]=toneHeaderRow.word(0xF6+entry*2);
    for (uint16_t channel=0;channel<COMBAT_TONE_CHANNELS;++channel)
    {
        combatTones.toneEventData[channel]=BtechSoundBytes{};combatTones.toneFallbackData[channel]=BtechToneFallback{};
        combatTones.fallbackAllocationOffsets[channel]=toneHeaderRow.word(0x3F+channel*2);
        combatTones.levelSequenceDirectoryOffsets[channel]=toneHeaderRow.word(0x7F+channel*2);
        combatTones.soundEventIndices[channel]=toneHeaderRow.word(0x1D+channel*2);
        combatTones.soundPriorities[channel]=toneHeaderRow.word(0x9F+channel*2);
        combatTones.channelFlags[channel]=BtechToneChannelFlags{toneHeaderRow.byte(0xDF+channel*2),toneHeaderRow.byte(0xE0+channel*2)};
        uint16_t offset=toneChannelOffsetsRow.word(channel*2);
        if (offset<0x00FE || (offset-0x00FE)%0x30 || (offset-0x00FE)/0x30>=COMBAT_TONE_CHANNELS) return false;
        combatTones.toneChannelStates[channel]=&combatTones.toneChannels[(offset-0x00FE)/0x30];
        /* All authored state WORDs are zero. Reject a different image rather
         * than interpreting an aligned host record as little-endian file data. */
        for (uint16_t byte=0;byte<0x30;++byte) if (toneChannelsRow.byte(channel*0x30+byte)) return false;
        combatTones.toneChannels[channel]=BtechToneChannelState{};
    }
    combatSound.soundTimerInstalled=audioSoundTimerRow.byte();
    combatSound.savedSoundTimerDivisor=audioSoundTimerRow.word(1);
    combatSound.timerSoundEventIndex=audioTimerSoundEventRow.word();
    return true;
}
