#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_sound.h"
#include <stdlib.h>

enum { MIDI_STATUS_FLAG=0x80,MIDI_META=0xFF,MIDI_END_OF_TRACK=0x2F,
    MIDI_SET_TEMPO=0x51,MIDI_IDLE=0xF0,MUSIC_TICK_MULTIPLIER=0x024E,
    MIDI_TEMPO_DIVIDEND=0x00039390 };

/*229C:0020. Authored one-byte RETF; preserve AX for the parser's AH input. */
uint16_t BtechMusicMessageNoOperation(uint16_t statusDisplacement,uint16_t channel,uint16_t packedData)
{
    (void)statusDisplacement;(void)channel;return packedData;
}

/*229C:0021. DIV BYTE leaves its remainder in AH before the whole-WORD SHR.
 * The presence flag stands for the native pair's selector, not a game flag. */
void BtechStartSoftwareMusicTrack(BtechSoundBytes musicHeader,uint16_t headerOffset)
{
    uint16_t division=BtechReadSoundWord(musicHeader,(uint16_t)(headerOffset+4));
    if (!combatSound.softwareMusicDivisionScale || division/combatSound.softwareMusicDivisionScale>UINT8_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !softwareMusicDivisionScale || division/softwareMusicDivisionScale>UINT8_MAX");
    uint16_t divided=((division%combatSound.softwareMusicDivisionScale)<<8)|(division/combatSound.softwareMusicDivisionScale);
    divided>>=1;combatSound.softwareMusicRateShift=(uint8_t)divided;
    combatSound.combatMidiStreamOffset=headerOffset+COMBAT_TONE_HEADER_BYTES;
    combatSound.midiStreamData=musicHeader;combatSound.midiStreamPresent=musicHeader.bytes!=NULL;
    BtechDecodeNextMidiEvent((uint8_t)(divided>>8));
    combatSound.softwareMusicDelayTicks=1;
}

/*229C:0051. Drain zero/quantized-zero deltas in this tick; no invented cap.
 * Callback AX residue is observable when parsing an idle next block. */
void BtechTickSoftwareMusic(void)
{
    if (--combatSound.softwareMusicDelayTicks) return;
    if (!combatSound.combatMidiStreamOffset && !combatSound.midiStreamPresent) return;
    for (;;)
    {
        uint16_t statusDisplacement=(combatSound.midiRunningStatus>>3)&0x0E;
        uint16_t channel=combatSound.midiChannel;
        uint16_t message=combatSound.midiMessageData[0]|((uint16_t)combatSound.midiMessageData[1]<<8);
        if (!combatSound.audioMusicCallback) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !audioMusicCallback");
        uint16_t callbackResidue=combatSound.audioMusicCallback(statusDisplacement,channel,message);
        BtechDecodeNextMidiEvent((uint8_t)(callbackResidue>>8));
        uint16_t delta=combatSound.queuedMidiDeltaTicks;
        if (!delta) continue;
        uint16_t delay;
        uint8_t shift=combatSound.softwareMusicRateShift;
        if (!shift)
        {
            delay=delta<<3;delay+=delta;delay<<=1;delay+=delta;
            delay<<=1;delay+=delta;delay<<=1;
            delay=(delay&0x00FF)|((uint16_t)(uint8_t)((delay>>8)+(uint8_t)delta)<<8);
            delay<<=1;
            if (!combatSound.mpuTempoInterval) /* Port emulation of a native CPU fault. */ MW_NATIVE_FAULT("Rejected condition: !mpuTempoInterval"); /* Parser's earlier writes survive the native DIV fault. */
            delay/=combatSound.mpuTempoInterval;
        }
        else
        {
            uint32_t product=(uint32_t)delta*MUSIC_TICK_MULTIPLIER;
            if (!combatSound.mpuTempoInterval || product/combatSound.mpuTempoInterval>UINT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mpuTempoInterval || product/mpuTempoInterval>UINT16_MAX");
            delay=(uint16_t)(product/combatSound.mpuTempoInterval);
            while (shift--) delay>>=1;
        }
        if (!delay) continue;
        combatSound.softwareMusicDelayTicks=delay;return;
    }
}

/*229C:00E2. Binary stream cursor wraps as a WORD. Known staging state is
 * ordinary typed data; malformed unchecked writes into other owners gate. */
uint8_t BtechDecodeNextMidiEvent(uint8_t inheritedDeltaHighByte)
{
nextTrack:
    if (!combatSound.combatMidiStreamOffset)
    {
        combatSound.midiRunningStatus=MIDI_IDLE;
        combatSound.queuedMidiDeltaTicks=((uint16_t)inheritedDeltaHighByte<<8)|MIDI_IDLE;
        return inheritedDeltaHighByte;
    }
    uint16_t cursor=combatSound.combatMidiStreamOffset;
    uint8_t deltaHighByte;
    for (;;)
    {
        uint8_t first=BtechReadSoundByte(combatSound.midiStreamData,cursor++);
        uint16_t delta=first;
        if (first&MIDI_STATUS_FLAG)
            delta=((uint16_t)(first&0x7F)<<7)|BtechReadSoundByte(combatSound.midiStreamData,cursor++);
        combatSound.queuedMidiDeltaTicks=delta;deltaHighByte=(uint8_t)(delta>>8);
        uint8_t status=BtechReadSoundByte(combatSound.midiStreamData,cursor);
        if (!(status&MIDI_STATUS_FLAG)) break; /* Running status retains this first data BYTE. */
        ++cursor;
        if (status!=MIDI_META)
        {
            combatSound.midiRunningStatus=status;combatSound.midiChannel=status&0x0F;
            combatSound.midiMessageDataLength=combatSound.midiMessageLengths[(status>>4)&7];
            deltaHighByte=0;break;
        }
        uint8_t meta=BtechReadSoundByte(combatSound.midiStreamData,cursor);
        if (meta==MIDI_END_OF_TRACK)
        {
            BtechSoundEvent next=BtechAdvanceMusicSequence();
            combatSound.combatMidiStreamOffset=next.offset;combatSound.midiStreamData=next.data;
            combatSound.midiStreamPresent=next.data.bytes!=NULL;
            ++combatSound.midiTrackIndex;
            inheritedDeltaHighByte=(uint8_t)(next.offset>>8);goto nextTrack;
        }
        if (meta==MIDI_SET_TEMPO)
        {
            uint16_t tempo=BtechReadSoundWord(combatSound.midiStreamData,(uint16_t)(cursor+2));
            combatSound.midiTempoHighBytes=(tempo<<8)|(tempo>>8);
            if (!combatSound.midiTempoHighBytes || MIDI_TEMPO_DIVIDEND/combatSound.midiTempoHighBytes>UINT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !midiTempoHighBytes || MIDI_TEMPO_DIVIDEND/midiTempoHighBytes>UINT16_MAX");
            combatSound.mpuTempoInterval=MIDI_TEMPO_DIVIDEND/combatSound.midiTempoHighBytes;
            cursor+=5; /* Third tempo BYTE intentionally ignored. */
        }
        else
        {
            uint8_t firstLength=BtechReadSoundByte(combatSound.midiStreamData,(uint16_t)(cursor+1));
            uint16_t payload=cursor+2,length=firstLength;
            if (firstLength&MIDI_STATUS_FLAG)
            {
                length=((uint16_t)(firstLength>>1)<<8)|BtechReadSoundByte(combatSound.midiStreamData,(uint16_t)(cursor+2));
                ++payload; /* Native AND clears Carry: no OR80 on this length path. */
            }
            cursor=payload+length;
        }
    }
    /* Native LOOP with length0 writes10000 bytes. Do not silently turn it
     * into an empty message or overwrite unrelated host objects. */
    if (!combatSound.midiMessageDataLength || combatSound.midiMessageDataLength>sizeof(combatSound.midiMessageData)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !midiMessageDataLength || midiMessageDataLength>sizeof(midiMessageData)");
    /* The count read once: a byte store may alias the global, so the compiler
     * could not bound the loop by the check above. */
    const uint16_t messageLength=combatSound.midiMessageDataLength;
    for (uint16_t byte=0;byte<messageLength;++byte)
        combatSound.midiMessageData[byte]=BtechReadSoundByte(combatSound.midiStreamData,cursor++);
    combatSound.combatMidiStreamOffset=cursor;return deltaHighByte;
}
