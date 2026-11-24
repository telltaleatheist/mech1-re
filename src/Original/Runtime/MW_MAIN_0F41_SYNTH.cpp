#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_music.h"
#include "mechwarrior/event_log.h"
#include "mechwarrior/music_out.h"
#include "mechwarrior/tone.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

MwMusicOutCalls mwMusicOut;

/* Writable native callback identities stay observable; synthesis lives below
 * the original byte/register outputs, not in a replacement host sequencer. */

/* MW_MAIN0F41:0B3C..0B45. */
uint16_t MwProbeMusicBackend(MwCampaignAudioBackend *backend)
{
    if (!backend) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !backend");
    if (backend->probeOffset==0x0912) return MwProbeRebuildMusic(); /* 'R' (S8). */
    if (backend->probeOffset!=0x0EE4) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: backend->probeOffset!=0x0EE4");
    return MwProbeSpeakerBackend();
}

/* 'R's fast timer callback (09D0, S8): it sent the queued bytes, and there is
 * no queue. It stays a callback so that the timer keeps the MPU path's rate. */
static void MwTickRebuildMusic(void) {}

/* MW_MAIN0F41:0B46..0B7F. Pending timer arguments precede initialization;
 * MPU's different BX residue is retained rather than repaired. */
void MwSelectAndInitializeMidiBackend(MwCampaignAudioBackend *backend)
{
    if (campaignMusic.selectedAudioBackend)
    {
        if (campaignMusic.selectedAudioBackend->shutdownOffset==0x098A) {} /* 'R': no queue to drain (S8). */
        else if (campaignMusic.selectedAudioBackend->shutdownOffset==0x0F01) MwShutdownSpeakerBackend();
        else if (campaignMusic.selectedAudioBackend->shutdownOffset==0x0736) {} /* Native RET. */
        else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Music backend has an unsupported initialization, vector, pitch-copy or timer entry.");
    }
    campaignMusic.selectedAudioBackend=backend;
    if (!backend) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !backend");
    uint16_t timerRate=backend->timerRateShift;
    uint16_t timerTarget=backend->timerOffset;
    /* 'R' (0946) is the rebuild's music (S8): nothing to set up. The MPU's
     * UART command, receive handler and IRQ2 save went with it, and so did the
     * install's byte copy from the saved IRQ2 owner's offset into the runtime
     * header (0B6C): nothing reads that header but its importer. Like the
     * original's MPU path, it leaves the voice count as it was. */
    if (backend->initializeOffset!=0x0946)
    {
        if (backend->initializeOffset==0x0EE8) MwInitializeSpeakerBackend();
        else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Music backend has an unsupported initialization, vector, pitch-copy or timer entry.");
        backend->selectedVoiceCount=backend->voiceCapacity;
    }
    MwResetMidiParserState(MIDI_RESET_PARSER_ENTRY);
    /* 'R' keeps the MPU path's timer rate, so the campaign's timer runs as it did. */
    if (timerTarget==0x09D0) MwTimerSetCallback(MUSIC_TIMER_FAST_SLOT,MwTickRebuildMusic,timerRate);
    else if (timerTarget==0x0F73) MwTimerSetCallback(MUSIC_TIMER_FAST_SLOT,MwTickSpeakerVoices,timerRate);
    else if (!timerTarget) MwTimerSetCallback(MUSIC_TIMER_FAST_SLOT,NULL,timerRate);
    else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Music backend has an unsupported initialization, vector, pitch-copy or timer entry.");
}

/* MW_MAIN0F41:0B80..0B9F. */
void MwShutdownMusicBackend(void)
{
    if (campaignMusic.selectedAudioBackend)
    {
        if (campaignMusic.selectedAudioBackend->shutdownOffset==0x098A) {} /* 'R': no queue to drain (S8). */
        else if (campaignMusic.selectedAudioBackend->shutdownOffset==0x0F01) MwShutdownSpeakerBackend();
        else if (campaignMusic.selectedAudioBackend->shutdownOffset==0x0736) {} /* Native RET. */
        else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Music backend has an unsupported shutdown entry.");
    }
    campaignMusic.selectedAudioBackend=NULL;
    MwTimerSetCallback(MUSIC_TIMER_FAST_SLOT,NULL,0);
}

/* MW_MAIN0F41:0BA0..0BAD. PC hook deliberately ignores the full input WORD. */
uint16_t MwSendMidiByteToSelectedBackend(uint16_t midiWord)
{
    /* 'R': the rebuild's music has every byte already (mwMusicOut, at dispatch),
     * so the byte is taken, as the MPU queue took it (S8). */
    if (campaignMusic.selectedAudioBackend && campaignMusic.selectedAudioBackend->rawByteOffset==0x09A4) return 1;
    if (campaignMusic.selectedAudioBackend && campaignMusic.selectedAudioBackend->rawByteOffset==0x0737) return 0;
    if (!campaignMusic.selectedAudioBackend || campaignMusic.selectedAudioBackend->rawByteOffset!=0x0F0A) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !selectedAudioBackend || selectedAudioBackend->rawByteOffset!=0x0F0A");
    return MwIgnoreSpeakerRawMidiByte();
}

/* MW_MAIN0F41:0BAE..0C36. Detach/reinsert before the note-off callback. */
uint16_t MwReleaseCampaignMidiVoice(uint16_t channelIndex,uint16_t note)
{
    uint16_t channelOffset=MIDI_CHANNELS_OFFSET+channelIndex*sizeof(MwMidiChannel);
    MwMidiChannel *channel=MwResolveMidiChannel(channelOffset);
    uint8_t eligibility=channel->backendEligibilityMask;
    if (!eligibility) return 0;
    if (eligibility&MIDI_DRUM_CHANNEL_BIT)
    {
        uint8_t drumIndex=(uint8_t)note-channel->transposeOrDrumBase;
        if (drumIndex>=MIDI_DRUM_CHANNEL_COUNT) return 0;
        channelIndex=(note&0xFF00)|(uint8_t)(drumIndex+MIDI_DRUM_CHANNEL_FIRST);
        channelOffset=MIDI_CHANNELS_OFFSET+channelIndex*sizeof(MwMidiChannel);
        note&=0xFF00;
    }
    uint16_t voiceOffset=MIDI_LIST_HEAD_OFFSET;
    for (;;)
    {
        voiceOffset=MwResolveMidiVoiceLinks(voiceOffset)->nextOffset;
        if (!MwResolveMidiVoiceLinks(voiceOffset)->nextOffset) return 0;
        MwMidiVoice *voice=MwResolveMidiVoice(voiceOffset);
        if (voice->channelOffset!=channelOffset || !voice->activePriority
            || voice->originalNote!=(uint8_t)note) continue;
        voice->activePriority=0;
        uint16_t next=voice->links.nextOffset,previous=voice->links.previousOffset;
        MwResolveMidiVoiceLinks(previous)->nextOffset=next;
        MwResolveMidiVoiceLinks(next)->previousOffset=previous;
        next=audioVoiceListHead;
        voice->links.nextOffset=next;
        voice->links.previousOffset=MIDI_LIST_HEAD_OFFSET;
        audioVoiceListHead=voiceOffset;
        MwResolveMidiVoiceLinks(next)->previousOffset=voiceOffset;
        if (!campaignMusic.selectedAudioBackend) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !selectedAudioBackend");
        if (campaignMusic.selectedAudioBackend->noteOffOffset==0x0F2D) MwStopSpeakerVoice(voice);
        else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Music backend has an unsupported note-off entry.");
        return 1;
    }
}

/* MW_MAIN0F41:0C37..0CEC. Stop at the first higher-priority voice; equal
 * priorities are inserted BEFORE existing equals. Reservation balances wrap. */
uint16_t MwAllocateCampaignMidiVoice(uint16_t channelIndex,uint16_t note,uint16_t velocity)
{
    uint16_t channelOffset=MIDI_CHANNELS_OFFSET+channelIndex*sizeof(MwMidiChannel);
    MwMidiChannel *channel=MwResolveMidiChannel(channelOffset);
    uint8_t eligibility=channel->backendEligibilityMask;
    if (!eligibility) return 0;
    if (eligibility&MIDI_DRUM_CHANNEL_BIT)
    {
        uint8_t drumIndex=(uint8_t)note-channel->transposeOrDrumBase;
        if (drumIndex>=MIDI_DRUM_CHANNEL_COUNT) return 0;
        channelIndex=(note&0xFF00)|(uint8_t)(drumIndex+MIDI_DRUM_CHANNEL_FIRST);
        channelOffset=MIDI_CHANNELS_OFFSET+channelIndex*sizeof(MwMidiChannel);
        note&=0xFF00;
        channel=MwResolveMidiChannel(channelOffset);
    }
    uint8_t priority=channel->voicePriority;
    eligibility=channel->backendEligibilityMask;
    uint16_t voiceOffset=MIDI_LIST_HEAD_OFFSET;
    MwMidiVoice *voice;
    for (;;)
    {
        voiceOffset=MwResolveMidiVoiceLinks(voiceOffset)->nextOffset;
        if (!MwResolveMidiVoiceLinks(voiceOffset)->nextOffset) return 0;
        voice=MwResolveMidiVoice(voiceOffset);
        if (voice->activePriority>priority) return 0;
        if (!(voice->backendMask&eligibility)) continue;
        MwMidiChannel *donor=MwResolveMidiChannel(voice->channelOffset);
        if (voice->channelOffset==channelOffset) break;
        if (!donor->reservationBalance) continue;
        --donor->reservationBalance;
        ++channel->reservationBalance;
        voice->channelOffset=channelOffset;
        break;
    }
    voice->activePriority=priority;
    uint16_t next=voice->links.nextOffset,previous=voice->links.previousOffset;
    MwResolveMidiVoiceLinks(previous)->nextOffset=next;
    MwResolveMidiVoiceLinks(next)->previousOffset=previous;
    while (MwResolveMidiVoiceLinks(next)->nextOffset
        && MwResolveMidiVoice(next)->activePriority<priority)
        next=MwResolveMidiVoiceLinks(next)->nextOffset;
    previous=MwResolveMidiVoiceLinks(next)->previousOffset;
    voice->links.nextOffset=next;
    voice->links.previousOffset=previous;
    MwResolveMidiVoiceLinks(previous)->nextOffset=voiceOffset;
    MwResolveMidiVoiceLinks(next)->previousOffset=voiceOffset;
    voice->originalNote=(uint8_t)note;
    voice->translatedNote=(uint8_t)(note+channel->transposeOrDrumBase);
    if (!campaignMusic.selectedAudioBackend) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !selectedAudioBackend");
    if (campaignMusic.selectedAudioBackend->noteOnOffset==0x0F0D) MwStartSpeakerVoice(voice);
    else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Music backend has an unsupported note-on entry.");
    return 1;
}

/* MW_MAIN0F41:0CEF..0D16. Follow live links after each callback. */
void MwSilenceCampaignMidiVoices(void)
{
    uint16_t voiceOffset=MIDI_LIST_HEAD_OFFSET;
    for (;;)
    {
        voiceOffset=MwResolveMidiVoiceLinks(voiceOffset)->nextOffset;
        if (!MwResolveMidiVoiceLinks(voiceOffset)->nextOffset) return;
        MwMidiVoice *voice=MwResolveMidiVoice(voiceOffset);
        if (!voice->activePriority) continue;
        voice->activePriority=0;
        if (!campaignMusic.selectedAudioBackend) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !selectedAudioBackend");
        if (campaignMusic.selectedAudioBackend->noteOffOffset==0x0F2D) MwStopSpeakerVoice(voice);
        else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Music backend has an unsupported note-off entry.");
    }
}

/* MW_MAIN0F41:0D17..0D3E. Full WORD program affects the instrument offset. */
void MwSetCampaignMidiProgram(uint16_t channelIndex,uint16_t instrumentId)
{
    MwMidiChannel *channel=MwResolveMidiChannel((uint16_t)(MIDI_CHANNELS_OFFSET+channelIndex*sizeof(MwMidiChannel)));
    channel->instrumentId=(uint8_t)instrumentId;
    channel->instrumentOffset=MIDI_INSTRUMENTS_OFFSET+instrumentId*MIDI_INSTRUMENT_BYTES;
}

/* MW_MAIN0F41:0574..0698. Interior JMP targets share the original FAR frame.
 * The native second-data handlers deliberately DO NOT reset the parser. */
void MwDispatchMidiByte(uint16_t midiWord)
{
    MwEventLogMidiByte((uint8_t)midiWord); /* Test instrument: the sequencer's output, before any backend. */
    if (mwMusicOut.midiByte) mwMusicOut.midiByte((uint8_t)midiWord); /* The rebuild's music (music_out.h). */
    if (MwSendMidiByteToSelectedBackend(midiWord)) return;
    uint8_t midiByte=midiWord;
    if (midiByte>=MIDI_STATUS_BYTE)
    {
        if (midiByte<MIDI_SYSTEM_STATUS)
        {
            campaignMusic.midiDispatchChannel=(midiWord&0xFF00)|(midiByte&0x0F);
            campaignMusic.midiByteHandlerDisplacement=(midiWord&0xFF70)>>2;
            return;
        }
        if (midiByte==0xF0)
        {
            MwDispatchMidiByteAndSysexState(midiWord);
            campaignMusic.midiByteHandlerDisplacement=0x0016;
            return;
        }
        if (midiByte==0xF7) MwDispatchMidiByteAndSysexState(midiWord);
        else if (midiByte==0xFF) MwResetMidiParserState(MIDI_RESET_PARSER_ENTRY);
        else if (midiByte>=MIDI_REALTIME_STATUS) return;
        campaignMusic.midiByteHandlerDisplacement=0x0012;
        return;
    }
    uint16_t displacement=campaignMusic.midiByteHandlerDisplacement;
    /* Unrepresented out-of-table code fetches are not a native bounds check. */
    if (displacement%2 || displacement/2>=14) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: displacement%2 || displacement/2>=14");
    switch (campaignMusic.midiByteHandlers[displacement/2])
    {
    case 0x05C4:
        campaignMusic.midiFirstDataByte=midiWord;
        campaignMusic.midiByteHandlerDisplacement=displacement+2;
        return;
    case 0x05CC:
        MwReleaseCampaignMidiVoice(campaignMusic.midiDispatchChannel,campaignMusic.midiFirstDataByte);return;
    case 0x05DE:
        if (!midiByte) MwReleaseCampaignMidiVoice(campaignMusic.midiDispatchChannel,campaignMusic.midiFirstDataByte);
        else MwAllocateCampaignMidiVoice(campaignMusic.midiDispatchChannel,campaignMusic.midiFirstDataByte,midiWord);
        return;
    case 0x0625:
        if (campaignMusic.midiFirstDataByte==1)
            campaignMusic.midiControllerValue=(uint16_t)(((uint16_t)midiByte<<8)|(midiWord>>8))>>1;
        else if (campaignMusic.midiFirstDataByte==33)
            campaignMusic.midiControllerValue^=(midiWord^campaignMusic.midiControllerValue)&0x7F;
        else if (campaignMusic.midiFirstDataByte>=MIDI_CONTROL_ALL_NOTES_OFF) MwSilenceCampaignMidiVoices();
        return; /* Native0CEE is one RETF, no synthesis effect. */
    case 0x0664: MwSetCampaignMidiProgram(campaignMusic.midiDispatchChannel,midiWord);return;
    case 0x0594:
        MwDispatchMidiByteAndSysexState(midiWord);
        campaignMusic.midiByteHandlerDisplacement=0x0016;return;
    case 0x05F5: /* Native0CED poly-pressure is one RETF. */
    case 0x0673: /* Native0D3F channel-pressure is one RETF. */
    case 0x0682: /* Native0D40 pitch-bend is one RETF, result unused. */
    case 0x0621: return;
    default: /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("MIDI byte dispatch selected an unsupported command handler.");
    }
}

/* MW_MAIN0F41:0D41..0DB1. BYTE writes retain the proved workspace aliases;
 * a malformed cursor outside that owned workspace is unsupported execution. */
void MwDispatchMidiByteAndSysexState(uint16_t midiWord)
{
    uint8_t midiByte=midiWord;
    if (midiByte==0xF0) {campaignMusic.midiSysex.parserOffset=0x0D58;return;}
    if (midiByte==0xF7) {campaignMusic.midiSysex.parserOffset=0x0DB0;return;}
    switch (campaignMusic.midiSysex.parserOffset)
    {
    case 0x0D58:
        if (!campaignMusic.selectedAudioBackend) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !selectedAudioBackend");
        campaignMusic.midiSysex.parserOffset=midiByte==campaignMusic.selectedAudioBackend->sysexManufacturer?0x0D66:0x0DB0;
        return;
    case 0x0D66:
        if (midiByte>=3) {campaignMusic.midiSysex.parserOffset=0x0DB0;return;}
        campaignMusic.midiSysex.handlerDisplacement=(uint16_t)(midiWord<<1);
        campaignMusic.midiSysex.parserOffset=0x0D74;return;
    case 0x0D74: campaignMusic.midiSysex.recordLength=midiByte;break;
    case 0x0D7E:
    {
        uint16_t writeOffset=campaignMusic.midiSysex.writeOffset;
        uint16_t relative=writeOffset-MIDI_SYSEX_BUFFER_OFFSET;
        if (relative>=sizeof(campaignMusic.midiSysex)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: relative>=sizeof(midiSysex)");
        ((uint8_t *)&campaignMusic.midiSysex)[relative]=midiByte;
        campaignMusic.midiSysex.writeOffset=writeOffset+1;
        if (--campaignMusic.midiSysex.bytesRemaining) return;
        uint16_t displacement=campaignMusic.midiSysex.handlerDisplacement;
        if (displacement%2 || displacement/2>=3) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: displacement%2 || displacement/2>=3");
        switch (campaignMusic.midiSysex.recordHandlers[displacement/2])
        {
        case 0x0DB2: MwApplySysexChannelConfiguration((const MwMidiChannelConfiguration *)campaignMusic.midiSysex.recordBuffer);break;
        case 0x0E57: MwApplySysexInstrument(campaignMusic.midiSysex.recordBuffer);break;
        case 0x0E73: MwApplySysexVoiceCount(campaignMusic.midiSysex.recordBuffer);break;
        default: /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("MIDI SysEx record selected an unsupported handler.");
        }
        break;
    }
    case 0x0DB0: return;
    default: /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("MIDI SysEx parser has an unsupported continuation.");
    }
    campaignMusic.midiSysex.bytesRemaining=campaignMusic.midiSysex.recordLength;
    campaignMusic.midiSysex.writeOffset=MIDI_SYSEX_BUFFER_OFFSET;
    campaignMusic.midiSysex.parserOffset=0x0D7E;
}

/* MW_MAIN0F41:0DB2..0E56. Six sequential BYTE reads, not a source snapshot.
 * E23 rereads OLD predecessor+6; head+6 is the first voice's NEXT LINK.
 * Consequently the final writer can be a voice prefix, not the requested
 * channel. Numeric links and the canonical channelAlias preserve this bug. */
void MwApplySysexChannelConfiguration(const MwMidiChannelConfiguration *configuration)
{
    const uint8_t *source=(const uint8_t *)configuration;
    uint16_t writerOffset=MIDI_CHANNELS_OFFSET+*source++*sizeof(MwMidiChannel);
    MwMidiChannel *writer=MwResolveMidiChannel(writerOffset);
    writer->backendEligibilityMask=*source++;
    writer->voicePriority=*source++;
    uint8_t delta=*source++-writer->reservedVoiceCount;
    if (delta&0x80)
    {
        writer->reservedVoiceCount+=delta;
        writer->reservationBalance-=delta;
    }
    else if (delta)
    {
        uint16_t budget=delta;
        if (writer->reservationBalance)
        {
            do
            {
                ++writer->reservedVoiceCount;
                --writer->reservationBalance;
                --budget;
            } while (budget && writer->reservationBalance);
        }
        if (budget)
        {
            uint8_t eligibility=writer->backendEligibilityMask;
            uint16_t scan=MIDI_LIST_HEAD_OFFSET,insertionPredecessor=scan;
            for (;;)
            {
                scan=MwResolveMidiVoiceLinks(scan)->nextOffset;
                uint16_t following=MwResolveMidiVoiceLinks(scan)->nextOffset;
                if (!--budget || !following) break;
                MwMidiVoice *voice=MwResolveMidiVoice(scan);
                if (!(voice->backendMask&eligibility)) continue;
                MwMidiChannel *donor=MwResolveMidiChannel(voice->channelOffset);
                if (!donor->reservationBalance) continue;
                --donor->reservationBalance;
                ++writer->reservedVoiceCount;
                voice->channelOffset=writerOffset;
                voice->activePriority=0;
                uint16_t next=voice->links.nextOffset,oldPrevious=voice->links.previousOffset;
                MwResolveMidiVoiceLinks(oldPrevious)->nextOffset=next;
                MwResolveMidiVoiceLinks(next)->previousOffset=oldPrevious;
                uint16_t insertionBase=insertionPredecessor;
                insertionPredecessor=oldPrevious;
                next=MwResolveMidiVoiceLinks(insertionBase)->nextOffset;
                voice->links.nextOffset=next;
                voice->links.previousOffset=insertionBase;
                MwResolveMidiVoiceLinks(insertionBase)->nextOffset=scan;
                MwResolveMidiVoiceLinks(next)->previousOffset=scan;
                scan=insertionPredecessor;
                insertionPredecessor=insertionBase;
                if (scan==MIDI_LIST_HEAD_OFFSET) writerOffset=audioVoices[0].links.nextOffset;
                else if (scan==MIDI_LIST_TAIL_OFFSET) writerOffset=audioVoices[0].links.previousOffset;
                else writerOffset=MwResolveMidiVoice(scan)->channelOffset;
                writer=MwResolveMidiChannel(writerOffset);
            }
        }
    }
    uint8_t instrumentId=*source++;
    writer->instrumentId=instrumentId;
    writer->instrumentOffset=MIDI_INSTRUMENTS_OFFSET+instrumentId*MIDI_INSTRUMENT_BYTES;
    uint8_t transpose=*source++;
    if (transpose&MIDI_DRUM_CHANNEL_BIT) transpose|=0x80;
    writer->transposeOrDrumBase=transpose;
}

/* MW_MAIN0F41:0E57..0E72. Decode the selected backend's instrument record;
 * speaker's actual decoder is a RET, not a synthesized instrument conversion. */
void MwApplySysexInstrument(const uint8_t *source)
{
    if (!campaignMusic.selectedAudioBackend || campaignMusic.selectedAudioBackend->decodeInstrumentOffset!=0x0F72) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !selectedAudioBackend || selectedAudioBackend->decodeInstrumentOffset!=0x0F72");
    MwIgnoreSpeakerInstrument();
}
/* MW_MAIN0F41:0E73..0E81. No capacity clamp before the shared rebuild. */
void MwApplySysexVoiceCount(const uint8_t *source)
{
    if (!campaignMusic.selectedAudioBackend) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !selectedAudioBackend");
    campaignMusic.selectedAudioBackend->selectedVoiceCount=*source;
    MwRebuildCampaignVoicePool();
}
/* MW_MAIN0F41:0E82 /0E9B shared0E9D..0EE3. entryOffset is host JMP
 * transport, not a new native argument. Tail.next is deliberately NOT reset. */
void MwResetMidiParserState(uint16_t entryOffset)
{
    if (entryOffset==MIDI_RESET_PARSER_ENTRY)
    {
        campaignMusic.midiSysex.parserOffset=0x0DB0;
        memset(campaignMusic.audioChannels,0,sizeof(campaignMusic.audioChannels));
    }
    else if (entryOffset!=MIDI_REBUILD_POOL_ENTRY) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: entryOffset!=MIDI_REBUILD_POOL_ENTRY");
    uint16_t previousHead=MIDI_LIST_TAIL_OFFSET;
    audioVoiceListHead=previousHead;
    audioVoiceListTailPrevious=MIDI_LIST_HEAD_OFFSET;
    if (!campaignMusic.selectedAudioBackend) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !selectedAudioBackend");
    uint16_t remaining=campaignMusic.selectedAudioBackend->selectedVoiceCount;
    audioVoiceCount=(uint8_t)remaining;
    uint16_t voiceOffset=MIDI_VOICES_OFFSET;
    while (remaining)
    {
        MwMidiVoice *voice=MwResolveMidiVoice(voiceOffset);
        voice->channelOffset=MIDI_DUMMY_CHANNEL_OFFSET;
        voice->activePriority=0;
        voice->links.nextOffset=previousHead;
        voice->links.previousOffset=MIDI_LIST_HEAD_OFFSET;
        audioVoiceListHead=voiceOffset;
        MwResolveMidiVoiceLinks(previousHead)->previousOffset=voiceOffset;
        previousHead=voiceOffset;
        voiceOffset+=sizeof(MwMidiVoice);
        --remaining;
    }
    if (!campaignMusic.selectedAudioBackend) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !selectedAudioBackend");
    campaignMusic.selectedAudioBackend->voicePoolOffset=MIDI_VOICES_OFFSET;
    /* 'R' (09CB) queued a MIDI reset byte (FF) for the MPU alone: the
     * sequencer's output never held it, and the rebuild's music takes the
     * sequencer's output (S8). */
    if (campaignMusic.selectedAudioBackend->initializeVoicesOffset==0x09CB) {}
    else if (campaignMusic.selectedAudioBackend->initializeVoicesOffset==0x0F4F) MwInitializeSpeakerVoiceRouting();
    else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Music backend has an unsupported voice-initialization entry.");
}

/* MW_MAIN0F41:0EE4..0EE7. Native unconditional success, no hardware probe. */
uint16_t MwProbeSpeakerBackend(void) {return 1;}
/* 'R' (0912, S8): the rebuild's music is always there; the MPU's handshake went. */
uint16_t MwProbeRebuildMusic(void) {return 1;}
/* S6: the speaker music sounds from the backend's start to its shutdown (the
 * original's port 61h gate), each period as a tone on the tone voice (where it
 * programmed PIT channel 2). The backend's silent period, an inaudible
 * 59.7 kHz on the speaker, is silence. */
static bool speakerMusicSounding;
static void PlaySpeakerPeriod(uint16_t period)
{
    if (!mwMusicHardware.playTone) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwMusicHardware.playTone");
    if (!speakerMusicSounding) return;
    mwMusicHardware.playTone(period==SPEAKER_SILENT_PERIOD?0:MwTonePeriodFrequency(period));
}
/* MW_MAIN0F41:0EE8..0F00. */
void MwInitializeSpeakerBackend(void)
{
    campaignMusic.midiSysex.currentPeriod=SPEAKER_SILENT_PERIOD;
    speakerMusicSounding=true;
    PlaySpeakerPeriod(SPEAKER_SILENT_PERIOD);
}
/* MW_MAIN0F41:0F01..0F09. */
void MwShutdownSpeakerBackend(void)
{
    if (!mwMusicHardware.playTone) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwMusicHardware.playTone");
    speakerMusicSounding=false;
    mwMusicHardware.playTone(0);
}
/* MW_MAIN0F41:0F0A..0F0C. Genuine XOR AX,AX /RET. */
uint16_t MwIgnoreSpeakerRawMidiByte(void) {return 0;}
/* MW_MAIN0F41:0F0D..0F2C. Higher BYTE notes read adjacent authored WORDs. */
uint16_t MwStartSpeakerVoice(MwMidiVoice *voice)
{
    uint16_t noteIndex=voice->translatedNote-SPEAKER_FIRST_NOTE;
    uint16_t period=(noteIndex&0x8000)?SPEAKER_SILENT_PERIOD:campaignMusic.midiSysex.tonePeriodsAndAdjacentWords[noteIndex];
    voice->pitPeriod=period;
    if (speakerVoiceCount<=1) return MwWriteSpeakerPeriod(period);
    return period;
}
/* MW_MAIN0F41:0F2D..0F3A. */
uint16_t MwStopSpeakerVoice(MwMidiVoice *voice)
{
    voice->pitPeriod=SPEAKER_SILENT_PERIOD;
    if (speakerVoiceCount<=1) return MwWriteSpeakerPeriod(SPEAKER_SILENT_PERIOD);
    return SPEAKER_SILENT_PERIOD;
}
/* MW_MAIN0F41:0F3B..0F4E. Native MOV AL,BH preserves AH on return. */
uint16_t MwWriteSpeakerPeriod(uint16_t period)
{
    campaignMusic.midiSysex.currentPeriod=period;
    PlaySpeakerPeriod(period);
    return (period&0xFF00)|(period>>8);
}
/* MW_MAIN0F41:0F4F..0F71. */
void MwInitializeSpeakerVoiceRouting(void)
{
    campaignMusic.midiSysex.currentVoiceOffset=speakerVoicePoolOffset;
    campaignMusic.midiSysex.voicesRemaining=speakerVoiceCount;
    uint16_t remaining=speakerVoiceCount,voiceOffset=speakerVoicePoolOffset;
    while (remaining)
    {
        MwMidiVoice *voice=MwResolveMidiVoice(voiceOffset);
        voice->backendMask=1;
        voice->pitPeriod=SPEAKER_SILENT_PERIOD;
        voiceOffset+=sizeof(MwMidiVoice);
        --remaining;
    }
}
/* MW_MAIN0F41:0F72. Genuine single NEAR RET, not an invented stub. */
void MwIgnoreSpeakerInstrument(void) {}
/* MW_MAIN0F41:0F73..0FBB. No CLI/STI on this IRQ callback. */
void MwTickSpeakerVoices(void)
{
    if (--campaignMusic.midiSysex.rotationCountdown) return;
    campaignMusic.midiSysex.rotationCountdown=SPEAKER_ROTATION_TICKS;
    if (speakerVoiceCount<=1) return;
    MwMidiVoice *voice=MwResolveMidiVoice(campaignMusic.midiSysex.currentVoiceOffset);
    uint16_t period=voice->pitPeriod;
    if (period!=campaignMusic.midiSysex.currentPeriod)
    {
        campaignMusic.midiSysex.currentPeriod=period;
        PlaySpeakerPeriod(period);
    }
    campaignMusic.midiSysex.currentVoiceOffset+=sizeof(MwMidiVoice);
    if (!--campaignMusic.midiSysex.voicesRemaining)
    {
        campaignMusic.midiSysex.voicesRemaining=speakerVoiceCount;
        campaignMusic.midiSysex.currentVoiceOffset=speakerVoicePoolOffset;
    }
}
