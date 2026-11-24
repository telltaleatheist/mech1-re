#include "mechwarrior/combat_sound.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/combat_runtime.h"
#include <stdlib.h>
#include <string.h>

BtechRuntimeStream runtimeStreams[COMBAT_STREAM_SLOTS];
BtechTaggedStream taggedStreams[COMBAT_TAGGED_STREAMS],taggedStreamBackup;
BtechSoundBank soundBanks[COMBAT_AUDIO_BANK_ROOTS];
uint16_t selectedAudioBackendMask,activeSoundBankIndex,soundAllocationError;
BtechResourceAllocation soundAllocationOverride;
uint16_t timerSoundEventIndex,savedSoundTimerDivisor;
uint8_t soundTimerInstalled;
uint8_t musicTimerInstalled;
uint16_t savedMusicTimerDivisor;
MwTimerOwner savedMusicTimerOwner;
uint8_t soundEffectsEnableMask;
void (*audioTickCallback)(void);
BtechMusicMessage audioMusicCallback;
uint8_t softwareMusicRateShift,softwareMusicDivisionScale;
uint16_t midiTrackIndex;
uint8_t midiRunningStatus,midiMessageData[2],midiMessageDataLength,midiChannel;
uint8_t midiMessageLengths[8];
uint16_t musicSequenceIndex,mpuTempoInterval,combatMidiStreamOffset;
uint16_t queuedMidiDeltaTicks,softwareMusicDelayTicks,midiTempoHighBytes;
MwNativeByteView midiStreamData;
bool midiStreamSelectorPresent;
uint16_t activeMusicBankIndex;
int16_t musicSequences[COMBAT_MUSIC_SEQUENCE_ROWS][COMBAT_MUSIC_SEQUENCE_WORDS];
uint16_t *samplePriorityOwner;

/* Child-image IO only. Do not call this to reset an already-running child. */
bool BtechImportSoundInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x10000 || !BtechImportRuntimeInitialData(data,bytes)) return false;
    for (uint16_t bank=0;bank<COMBAT_AUDIO_BANK_ROOTS;++bank)
    {
        free(soundBanks[bank].events);
        soundBanks[bank]=BtechSoundBank{};
    }
    for (uint16_t stream=0;stream<COMBAT_TAGGED_STREAMS;++stream) taggedStreams[stream]=BtechTaggedStream{};
    taggedStreamBackup=BtechTaggedStream{};soundAllocationOverride=BtechResourceAllocation{};
    if (MwReadNativeWord(data,0x4CD8)!=COMBAT_STREAM_TABLE+(COMBAT_STREAM_SLOTS-1)*COMBAT_STREAM_STRIDE) return false;
    for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
    {
        runtimeStreams[slot].file=NULL;
        runtimeStreams[slot].buffer=NULL;
        runtimeStreams[slot].flags=data[COMBAT_STREAM_TABLE+slot*COMBAT_STREAM_STRIDE+6];
    }
    selectedAudioBackendMask=0; /*28D4*/
    savedMusicTimerDivisor=MwReadNativeWord(data,0x28D6);
    savedMusicTimerOwner=MwTimerOwner::Previous; /*28D8: 0000:0000*/
    musicTimerInstalled=data[0x28DC];combatSoundIrq.musicTimerCallback=BtechAudioTimerNoOp;
    soundEffectsEnableMask=data[0x28DD];
    /* These are the actual authored pairs, not a replacement empty tick. */
    if (MwReadNativeWord(data,0x2C9B)!=0x0020 || MwReadNativeWord(data,0x2C9D)!=0x229C
        || MwReadNativeWord(data,0x2C9F)!=0x0051 || MwReadNativeWord(data,0x2CA1)!=0x229C) return false;
    audioMusicCallback=BtechMusicMessageNoOperation;
    audioTickCallback=BtechTickSoftwareMusic;
    softwareMusicRateShift=0; /*2C80*/midiTrackIndex=0; /*2C81*/
    musicSequenceIndex=MwReadNativeWord(data,0x2C83);softwareMusicDivisionScale=data[0x2C85];
    mpuTempoInterval=MwReadNativeWord(data,0x2C86);
    combatMidiStreamOffset=0; /*2C88*/
    if (combatMidiStreamOffset || MwReadNativeWord(data,0x2C8A)) return false;
    midiStreamData=MwNativeByteView{};midiStreamSelectorPresent=false;
    queuedMidiDeltaTicks=0; /*2C8C*/midiRunningStatus=0; /*2C8E*/
    midiMessageData[0]=data[0x2C8F];midiMessageData[1]=data[0x2C90];
    softwareMusicDelayTicks=MwReadNativeWord(data,0x2C91);
    for (uint16_t index=0;index<8;++index) midiMessageLengths[index]=data[0x2C93+index];
    midiMessageDataLength=data[0x2CA3];midiChannel=data[0x2CA4];
    midiTempoHighBytes=0; /*2CA5*/
    activeMusicBankIndex=MwReadNativeWord(data,0x2B4E);
    /* 7D8A lies past the file data; MUS:INF fills it at run time. */
    memset(musicSequences,0,sizeof(musicSequences));
    activeSoundBankIndex=MwReadNativeWord(data,0x2B4C);
    soundAllocationError=0; /*2B56*/
    return true;
}
