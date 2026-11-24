#include "mechwarrior/combat_sound.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/combat_runtime.h"
#include <stdlib.h>
#include <string.h>

CombatSoundState combatSound;

/* Child-image IO only. Do not call this to reset an already-running child. */
bool BtechImportSoundInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x10000 || !BtechImportRuntimeInitialData(data,bytes)) return false;
    for (uint16_t bank=0;bank<COMBAT_AUDIO_BANK_ROOTS;++bank)
    {
        free(combatSound.soundBanks[bank].events);
        combatSound.soundBanks[bank]=BtechSoundBank{};
    }
    for (uint16_t stream=0;stream<COMBAT_TAGGED_STREAMS;++stream) combatSound.taggedStreams[stream]=BtechTaggedStream{};
    combatSound.taggedStreamBackup=BtechTaggedStream{};combatSound.soundAllocationOverride=BtechResourceAllocation{};
    if (MwReadNativeWord(data,0x4CD8)!=COMBAT_STREAM_TABLE+(COMBAT_STREAM_SLOTS-1)*COMBAT_STREAM_STRIDE) return false;
    for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
    {
        combatSound.runtimeStreams[slot].file=NULL;
        combatSound.runtimeStreams[slot].buffer=NULL;
        combatSound.runtimeStreams[slot].flags=data[COMBAT_STREAM_TABLE+slot*COMBAT_STREAM_STRIDE+6];
    }
    combatSound.selectedAudioBackendMask=0; /*28D4*/
    combatSound.savedMusicTimerDivisor=MwReadNativeWord(data,0x28D6);
    combatSound.savedMusicTimerOwner=MwTimerOwner::Previous; /*28D8: 0000:0000*/
    combatSound.musicTimerInstalled=data[0x28DC];combatSoundIrq.musicTimerCallback=BtechAudioTimerNoOp;
    combatSound.soundEffectsEnableMask=data[0x28DD];
    /* These are the actual authored pairs, not a replacement empty tick. */
    if (MwReadNativeWord(data,0x2C9B)!=0x0020 || MwReadNativeWord(data,0x2C9D)!=0x229C
        || MwReadNativeWord(data,0x2C9F)!=0x0051 || MwReadNativeWord(data,0x2CA1)!=0x229C) return false;
    combatSound.audioMusicCallback=BtechMusicMessageNoOperation;
    combatSound.audioTickCallback=BtechTickSoftwareMusic;
    combatSound.softwareMusicRateShift=0; /*2C80*/combatSound.midiTrackIndex=0; /*2C81*/
    combatSound.musicSequenceIndex=MwReadNativeWord(data,0x2C83);combatSound.softwareMusicDivisionScale=data[0x2C85];
    combatSound.mpuTempoInterval=MwReadNativeWord(data,0x2C86);
    combatSound.combatMidiStreamOffset=0; /*2C88*/
    if (combatSound.combatMidiStreamOffset || MwReadNativeWord(data,0x2C8A)) return false;
    combatSound.midiStreamData=BtechSoundBytes{};combatSound.midiStreamPresent=false;
    combatSound.queuedMidiDeltaTicks=0; /*2C8C*/combatSound.midiRunningStatus=0; /*2C8E*/
    combatSound.midiMessageData[0]=data[0x2C8F];combatSound.midiMessageData[1]=data[0x2C90];
    combatSound.softwareMusicDelayTicks=MwReadNativeWord(data,0x2C91);
    for (uint16_t index=0;index<8;++index) combatSound.midiMessageLengths[index]=data[0x2C93+index];
    combatSound.midiMessageDataLength=data[0x2CA3];combatSound.midiChannel=data[0x2CA4];
    combatSound.midiTempoHighBytes=0; /*2CA5*/
    combatSound.activeMusicBankIndex=MwReadNativeWord(data,0x2B4E);
    /* 7D8A lies past the file data; MUS:INF fills it at run time. */
    memset(combatSound.musicSequences,0,sizeof(combatSound.musicSequences));
    combatSound.activeSoundBankIndex=MwReadNativeWord(data,0x2B4C);
    combatSound.soundAllocationError=0; /*2B56*/
    return true;
}
