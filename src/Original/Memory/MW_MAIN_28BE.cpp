/* MW_MAIN.EXE runtime data owner28BE; authored values are decoded at EXE load. */
#include "mechwarrior/campaign_music.h"
#include "mechwarrior/original_data.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/story.h"
CampaignMusicState campaignMusic;

/* The driver segment's start values, decoded from its bytes: the story's
 * music.driver span (28BE:0000..0A11, story/native.json), not the exe. The
 * borrowed bytes are not retained by game code. */
static bool DecodeMusicDriver(const uint8_t *data,size_t byteCount)
{
    uint16_t hostEndian=1;
    if (!data || byteCount<0x0A12 || *(uint8_t *)&hostEndian!=1) return false;
    uint16_t backendOffset=MwReadNativeWord(data,0x03F8);
    if (backendOffset && backendOffset!=MIDI_SPEAKER_BACKEND_OFFSET && backendOffset!=MIDI_MPU_BACKEND_OFFSET
        && backendOffset!=MIDI_ADLIB_BACKEND_OFFSET) return false;
    for (size_t i=0;i<sizeof(campaignMusic.adlibBackend);++i) ((uint8_t *)&campaignMusic.adlibBackend)[i]=data[MIDI_ADLIB_BACKEND_OFFSET+i];
    for (size_t i=0;i<sizeof(campaignMusic.adlibInstruments);++i) ((uint8_t *)campaignMusic.adlibInstruments)[i]=data[MIDI_INSTRUMENTS_OFFSET+i];
    campaignMusic.adlibAddressDelayReads=MwReadNativeWord(data,0x00E0);
    campaignMusic.adlibDataDelayReads=MwReadNativeWord(data,0x00E2);
    campaignMusic.adlibRhythmRegister=data[0x00E4];
    for (size_t i=0;i<MIDI_ADLIB_NOTE_COUNT;++i) campaignMusic.adlibPackedNoteFrequencies[i]=MwReadNativeWord(data,(uint16_t)(0x00E6+i*2));
    for (size_t i=0;i<sizeof(campaignMusic.adlibVoiceRoutes);++i) ((uint8_t *)campaignMusic.adlibVoiceRoutes)[i]=data[0x01CC+i];
    for (size_t i=0;i<sizeof(campaignMusic.musicRuntimeHeader);++i) campaignMusic.musicRuntimeHeader[i]=data[i];
    for (size_t i=0;i<sizeof(campaignMusic.mpuBackend);++i) ((uint8_t *)&campaignMusic.mpuBackend)[i]=data[MIDI_MPU_BACKEND_OFFSET+i];
    for (size_t i=0;i<sizeof(campaignMusic.mpuTransmitQueue);++i) campaignMusic.mpuTransmitQueue[i]=data[0x0224+i];
    campaignMusic.mpuQueueWriteIndex=MwReadNativeWord(data,0x0324);
    campaignMusic.mpuQueueReadIndex=MwReadNativeWord(data,0x0326);
    campaignMusic.mpuQueueByteCount=MwReadNativeWord(data,0x0328);
    campaignMusic.mpuPreviousIrqOwner=MwMpuIrqOwner::System; /*0220: 0000:0000*/
    campaignMusic.midiTickDenominator=MwReadNativeWord(data,0x0042);
    campaignMusic.midiTickNumerator=MwReadNativeWord(data,0x0044);
    campaignMusic.midiTickRemainder=0; /*0046*/
    campaignMusic.midiBeatDivision=MwReadNativeWord(data,0x0048);
    campaignMusic.midiBarDivision=MwReadNativeWord(data,0x004A);
    campaignMusic.midiStreamOffset=0; /*004C*/
    campaignMusic.midiStreamSegment=0; /*004E*/
    campaignMusic.midiPlaybackFlags=0; /*0050*/
    campaignMusic.midiEventTickBalance=0; /*0052*/
    campaignMusic.midiBeatRemainder=0; /*0054*/
    campaignMusic.midiBarRemainder=0; /*0056*/
    campaignMusic.midiElapsedBars=0; /*0058*/
    campaignMusic.midiStateUpdateInProgress=data[0x005A];
    campaignMusic.midiTickInProgress=data[0x005B];
    campaignMusic.midiTrackRunningStatus=data[0x005C];
    for (uint16_t i=0;i<MIDI_TRACK_CHANNEL_HANDLERS;++i)
        campaignMusic.midiTrackChannelHandlers[i]=MwReadNativeWord(data,(uint16_t)(0x005D+i*2));
    for (uint16_t i=0;i<MIDI_TRACK_SYSTEM_HANDLERS;++i)
        campaignMusic.midiTrackSystemHandlers[i]=MwReadNativeWord(data,(uint16_t)(0x006B+i*2));
    for (uint16_t i=0;i<14;++i) campaignMusic.midiByteHandlers[i]=MwReadNativeWord(data,(uint16_t)(0x008C+i*2));
    campaignMusic.midiByteHandlerDisplacement=MwReadNativeWord(data,0x00A8);
    campaignMusic.midiDispatchChannel=0; /*00AA*/
    campaignMusic.midiFirstDataByte=0; /*00AC*/
    campaignMusic.midiControllerValue=0; /*00AE*/
    /* These typed native records have proved BYTE aliases and exact widths.
     * Import once at IO; ordinary procedures do not keep a backing DS image. */
    for (size_t i=0;i<sizeof(audioVoiceList);++i) ((uint8_t *)&audioVoiceList)[i]=data[0x03FA+i];
    for (size_t i=0;i<sizeof(audioVoices);++i) ((uint8_t *)audioVoices)[i]=data[0x0400+i];
    for (size_t i=0;i<sizeof(campaignMusic.audioChannels);++i) ((uint8_t *)campaignMusic.audioChannels)[i]=data[0x04C6+i];
    for (size_t i=0;i<sizeof(campaignMusic.audioDummyChannel);++i) ((uint8_t *)&campaignMusic.audioDummyChannel)[i]=data[0x0646+i];
    for (size_t i=0;i<sizeof(campaignMusic.midiSysex);++i) ((uint8_t *)&campaignMusic.midiSysex)[i]=data[0x07CE +i];
    campaignMusic.selectedAudioBackend=backendOffset==MIDI_MPU_BACKEND_OFFSET?&campaignMusic.mpuBackend
        :backendOffset==MIDI_ADLIB_BACKEND_OFFSET?&campaignMusic.adlibBackend:backendOffset?&speakerBackend:NULL;
    return true;
}
bool MwImportCampaignMusicData(void)
{
    uint8_t driver[0x0A12];
    return MwStoryCopyBytes("music.driver",driver,sizeof(driver)) && DecodeMusicDriver(driver,sizeof(driver));
}
