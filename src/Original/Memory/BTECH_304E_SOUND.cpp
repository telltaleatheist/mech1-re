#include "mechwarrior/combat_sound.h"
#include "mechwarrior/btech_start_image.h"
#include "mechwarrior/combat_runtime.h"
#include <stdlib.h>
#include <string.h>

CombatSoundState combatSound;

/* Child-image IO only. Do not call this to reset an already-running child. */
bool BtechImportSoundInitialData(void)
{
    const BtechStartRowData audioMusicTimerDivisorRow("audio.music_timer_divisor");
    const BtechStartRowData audioMusicTimerFlagsRow("audio.music_timer_flags");
    const BtechStartRowData audioBankIndicesRow("audio.bank_indices");
    const BtechStartRowData musicSoftwareStateRow("music.software_state");
    const BtechStartRowData runtimeStreamsRow("runtime.streams");
    const BtechStartRowData runtimeStreamAndCharacterTablesRow("runtime.stream_and_character_tables");
    if (!BtechImportRuntimeInitialData()) return false;
    for (uint16_t bank=0;bank<COMBAT_AUDIO_BANK_ROOTS;++bank)
    {
        free(combatSound.soundBanks[bank].events);
        combatSound.soundBanks[bank]=BtechSoundBank{};
    }
    for (uint16_t stream=0;stream<COMBAT_TAGGED_STREAMS;++stream) combatSound.taggedStreams[stream]=BtechTaggedStream{};
    combatSound.taggedStreamBackup=BtechTaggedStream{};combatSound.soundAllocationOverride=BtechResourceAllocation{};
    if (runtimeStreamAndCharacterTablesRow.word(0x78)!=COMBAT_STREAM_TABLE+(COMBAT_STREAM_SLOTS-1)*COMBAT_STREAM_STRIDE) return false;
    for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
    {
        combatSound.runtimeStreams[slot].file=NULL;
        combatSound.runtimeStreams[slot].buffer=NULL;
        combatSound.runtimeStreams[slot].flags=runtimeStreamsRow.byte(slot*COMBAT_STREAM_STRIDE);
    }
    combatSound.selectedAudioBackendMask=0; /*28D4*/
    combatSound.savedMusicTimerDivisor=audioMusicTimerDivisorRow.word();
    combatSound.savedMusicTimerOwner=MwTimerOwner::Previous; /*28D8: 0000:0000*/
    combatSound.musicTimerInstalled=audioMusicTimerFlagsRow.byte();combatSoundIrq.musicTimerCallback=BtechAudioTimerNoOp;
    combatSound.soundEffectsEnableMask=audioMusicTimerFlagsRow.byte(1);
    /* These are the actual authored pairs, not a replacement empty tick. */
    if (musicSoftwareStateRow.word(0x18)!=0x0020 || musicSoftwareStateRow.word(0x1A)!=0x229C
        || musicSoftwareStateRow.word(0x1C)!=0x0051 || musicSoftwareStateRow.word(0x1E)!=0x229C) return false;
    combatSound.audioMusicCallback=BtechMusicMessageNoOperation;
    combatSound.audioTickCallback=BtechTickSoftwareMusic;
    combatSound.softwareMusicRateShift=0; /*2C80*/combatSound.midiTrackIndex=0; /*2C81*/
    combatSound.musicSequenceIndex=musicSoftwareStateRow.word();combatSound.softwareMusicDivisionScale=musicSoftwareStateRow.byte(2);
    combatSound.mpuTempoInterval=musicSoftwareStateRow.word(3);
    combatSound.combatMidiStreamOffset=0; /*2C88*/
    if (combatSound.combatMidiStreamOffset || musicSoftwareStateRow.word(7)) return false;
    combatSound.midiStreamData=BtechSoundBytes{};combatSound.midiStreamPresent=false;
    combatSound.queuedMidiDeltaTicks=0; /*2C8C*/combatSound.midiRunningStatus=0; /*2C8E*/
    combatSound.midiMessageData[0]=musicSoftwareStateRow.byte(0xC);combatSound.midiMessageData[1]=musicSoftwareStateRow.byte(0xD);
    combatSound.softwareMusicDelayTicks=musicSoftwareStateRow.word(0xE);
    for (uint16_t index=0;index<8;++index) combatSound.midiMessageLengths[index]=musicSoftwareStateRow.byte(0x10+index);
    combatSound.midiMessageDataLength=musicSoftwareStateRow.byte(0x20);combatSound.midiChannel=musicSoftwareStateRow.byte(0x21);
    combatSound.midiTempoHighBytes=0; /*2CA5*/
    combatSound.activeMusicBankIndex=audioBankIndicesRow.word(2);
    /* 7D8A lies past the file data; MUS:INF fills it at run time. */
    memset(combatSound.musicSequences,0,sizeof(combatSound.musicSequences));
    combatSound.activeSoundBankIndex=audioBankIndicesRow.word();
    combatSound.soundAllocationError=0; /*2B56*/
    return true;
}
