/* MW_MAIN.EXE runtime data owner28BE; authored values are decoded at EXE load. */
#include "mechwarrior/campaign_music.h"
#include "mechwarrior/original_data.h"
#include "mechwarrior/story_span.h"
CampaignMusicState campaignMusic;

/* The driver segment's start values, decoded from its bytes: the story's
 * music.driver span (28BE:0000..0A11, story/native.json), not the exe. */
bool MwImportCampaignMusicData(void)
{
    uint16_t hostEndian=1;
    if (*(uint8_t *)&hostEndian!=1) return false;
    const MwStorySpan data=MwStorySpan::native("music.driver",0x0A12);
    uint16_t backendOffset=data.word(0x03F8);
    /* The AdLib backend (00B0..0223: its descriptor, instruments, bus delays,
     * rhythm register, note frequencies, voice routes), the MPU's queue
     * (0224..0329) and the runtime header (0000..0041) went with their code
     * in S8; nothing else read them. The MPU's descriptor (01EE) stays: it is
     * 'R', the rebuild's music. */
    if (backendOffset && backendOffset!=MIDI_SPEAKER_BACKEND_OFFSET && backendOffset!=MIDI_MPU_BACKEND_OFFSET) return false;
    data.copy(&campaignMusic.mpuBackend,sizeof(campaignMusic.mpuBackend),MIDI_MPU_BACKEND_OFFSET);
    campaignMusic.midiTickDenominator=data.word(0x0042);
    campaignMusic.midiTickNumerator=data.word(0x0044);
    campaignMusic.midiTickRemainder=0; /*0046*/
    campaignMusic.midiBeatDivision=data.word(0x0048);
    campaignMusic.midiBarDivision=data.word(0x004A);
    campaignMusic.midiStreamOffset=0; /*004C*/
    campaignMusic.midiStream=NULL; /*004E*/
    campaignMusic.midiPlaybackFlags=0; /*0050*/
    campaignMusic.midiEventTickBalance=0; /*0052*/
    campaignMusic.midiBeatRemainder=0; /*0054*/
    campaignMusic.midiBarRemainder=0; /*0056*/
    campaignMusic.midiElapsedBars=0; /*0058*/
    campaignMusic.midiStateUpdateInProgress=data.byte(0x005A);
    campaignMusic.midiTickInProgress=data.byte(0x005B);
    campaignMusic.midiTrackRunningStatus=data.byte(0x005C);
    for (uint16_t i=0;i<MIDI_TRACK_CHANNEL_HANDLERS;++i)
        campaignMusic.midiTrackChannelHandlers[i]=data.word(0x005D+i*2);
    for (uint16_t i=0;i<MIDI_TRACK_SYSTEM_HANDLERS;++i)
        campaignMusic.midiTrackSystemHandlers[i]=data.word(0x006B+i*2);
    for (uint16_t i=0;i<14;++i) campaignMusic.midiByteHandlers[i]=data.word(0x008C+i*2);
    campaignMusic.midiByteHandlerDisplacement=data.word(0x00A8);
    campaignMusic.midiDispatchChannel=0; /*00AA*/
    campaignMusic.midiFirstDataByte=0; /*00AC*/
    campaignMusic.midiControllerValue=0; /*00AE*/
    /* These typed native records have proved BYTE aliases and exact widths.
     * Import once at IO; ordinary procedures do not keep a backing DS image. */
    data.copy(&audioVoiceList,sizeof(audioVoiceList),0x03FA);
    data.copy(audioVoices,sizeof(audioVoices),0x0400);
    data.copy(campaignMusic.audioChannels,sizeof(campaignMusic.audioChannels),0x04C6);
    data.copy(&campaignMusic.audioDummyChannel,sizeof(campaignMusic.audioDummyChannel),0x0646);
    data.copy(&campaignMusic.midiSysex,sizeof(campaignMusic.midiSysex),0x07CE);
    campaignMusic.selectedAudioBackend=backendOffset==MIDI_MPU_BACKEND_OFFSET?&campaignMusic.mpuBackend
        :backendOffset?&speakerBackend:NULL;
    return true;
}
