#include "SDLBackend.h"
#include "mechwarrior/audio_devices.h"
#include "AudioClock.h"
#include "Mixer.h"
#include <stdio.h>
#include <stdlib.h>

/* The output layer: the SDL3 device, the bounded PCM queue and its stream
 * callback. Synthesis (the speaker voice, the selected card, mixing on PIT
 * time) is mw1o::audio::Mixer in src/Audio, which hands this file blocks.
 * MIDI scheduling, allocation, instruments and rotation remain Original code;
 * the host never plays an SMF itself. */
static SDL_AudioStream *audioStream;
static SDL_Mutex *audioQueueMutex;
enum { AUDIO_QUEUE_FRAMES=0x4000,AUDIO_START_LEAD_FRAMES=0x0800 };
static float audioQueue[AUDIO_QUEUE_FRAMES][2];
static uint32_t outputChannels=1;
static uint64_t generatedFrames,playbackFrames,lateFrames,underrunFrames;
static uint64_t audioStartedNanoseconds,clockSkippedFrames;
static uint32_t playbackLeadFrames;
static uint32_t leadFrames;
static mw1o::audio::Mixer mixer;
static_assert(HOST_PIT_CLOCK_HZ==mw1o::audio::PIT_CLOCK_HZ && HOST_AUDIO_SAMPLE_RATE==mw1o::audio::OUTPUT_SAMPLE_RATE,
    "The host timeline and the mixer share one clock and one output rate");
/* Logging without ROMs (MECHWARRIOR_AUDIO_LOG, both ROM paths empty): the
 * MPU and its byte stream run as usual and the synthesizer is silent. */
static bool silentMt32;
/* MECHWARRIOR_AUDIO_PCM=FILE: every rendered frame as raw 32-bit floats at
 * 48 kHz (one channel for the speaker, two with a card), for comparing
 * synthesis offline (tools/trace/audio.py --pcm). */
static FILE *pcmDump;
void (*mwSdlAudioObserver)(const float *samples,uint32_t count);

/* SDL asks for PCM only. It never runs an original timer/game callback.
 * An underrun advances the physical playback clock, just like silence from
 * an unfed device. The producer must not replay those elapsed frames later. */
static void SDLCALL FeedAudio(void *unused,SDL_AudioStream *stream,int additionalBytes,int totalBytes)
{
    (void)unused;(void)totalBytes;
    /* Hardware sample demand and SDL's monotonic PIT clock are independent.
     * In particular dummy WaitDevice uses rounded millisecond delays. Advance
     * a lagging device cursor to its timestamp instead of accumulating minutes
     * of drift into a fatal queue overflow. Already elapsed PCM stays elapsed. */
    uint64_t elapsed = SDL_GetTicksNS() - audioStartedNanoseconds;
    uint64_t physicalFrames = elapsed / UINT64_C(1000000000) * HOST_AUDIO_SAMPLE_RATE
        + elapsed % UINT64_C(1000000000) * HOST_AUDIO_SAMPLE_RATE / UINT64_C(1000000000);
    SDL_LockMutex(audioQueueMutex);
    if (!leadFrames && physicalFrames > playbackLeadFrames
        && physicalFrames - playbackLeadFrames > playbackFrames)
    {
        uint64_t currentFrame = physicalFrames - playbackLeadFrames;
        clockSkippedFrames += currentFrame - playbackFrames;
        playbackFrames = currentFrame;
    }
    SDL_UnlockMutex(audioQueueMutex);
    float samples[512];
    while (additionalBytes>0)
    {
        uint32_t count=(uint32_t)additionalBytes/(sizeof(float)*outputChannels);
        if (count>256) count=256;
        if (!count) return;
        SDL_LockMutex(audioQueueMutex);
        for (uint32_t index=0;index<count;++index)
        {
            for (uint32_t channel=0;channel<outputChannels;++channel)
                samples[index*outputChannels+channel]=!leadFrames && playbackFrames<generatedFrames
                    ?audioQueue[playbackFrames%AUDIO_QUEUE_FRAMES][channel]:0;
            if (leadFrames) {--leadFrames;continue;}
            if (playbackFrames>=generatedFrames) ++underrunFrames;
            ++playbackFrames;
        }
        SDL_UnlockMutex(audioQueueMutex);
        if (!SDL_PutAudioStreamData(stream,samples,(int)(count*sizeof(float)*outputChannels))) MwSdlFail("Feed audio PCM");
        additionalBytes-=(int)(count*sizeof(float)*outputChannels);
    }
}

bool MwSdlInitializeAudio(void)
{
    if (audioStream || audioQueueMutex) return SDL_SetError("Audio is already initialized");
    if (mwSdlSoundSystem==MW_SOUND_ADLIB) MwAdlibReset(HOST_AUDIO_SAMPLE_RATE);
    silentMt32=mwSdlSoundSystem==MW_SOUND_MT32 && getenv("MECHWARRIOR_AUDIO_LOG")
        && !*mwSdlMt32ControlRom && !*mwSdlMt32PcmRom;
    if (mwSdlSoundSystem==MW_SOUND_MT32 && !silentMt32)
    {
        char error[1024];
        if (!MwMt32Open(mwSdlMt32ControlRom,mwSdlMt32PcmRom,HOST_AUDIO_SAMPLE_RATE,error,sizeof(error)))
            return SDL_SetError("%s",error);
    }
    mixer.reset(mwSdlSoundSystem==MW_SOUND_SPEAKER?mw1o::audio::Mixer::Card::None
        :mwSdlSoundSystem==MW_SOUND_MT32?mw1o::audio::Mixer::Card::Mt32:mw1o::audio::Mixer::Card::Adlib,silentMt32);
    outputChannels=mixer.channels();
    SDL_AudioSpec format={SDL_AUDIO_F32,(int)outputChannels,HOST_AUDIO_SAMPLE_RATE};
    audioQueueMutex=SDL_CreateMutex();
    if (!audioQueueMutex) goto failed;
    generatedFrames=playbackFrames=lateFrames=underrunFrames=clockSkippedFrames=0;leadFrames=AUDIO_START_LEAD_FRAMES;
    audioStream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&format,FeedAudio,NULL);
    if (!audioStream) goto failed;
    SDL_AudioSpec deviceFormat;int deviceFrames;
    if (!SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(audioStream),&deviceFormat,&deviceFrames)) goto failed;
    uint64_t deviceLead;deviceLead=(uint64_t)deviceFrames*HOST_AUDIO_SAMPLE_RATE*2/deviceFormat.freq;
    if (deviceLead>=AUDIO_QUEUE_FRAMES)
    {SDL_SetError("Audio device buffer exceeds bounded PCM capacity");goto failed;}
    if (deviceLead>leadFrames) leadFrames=(uint32_t)deviceLead;
    playbackLeadFrames = leadFrames;
    audioStartedNanoseconds = SDL_GetTicksNS();
    if (!SDL_ResumeAudioStreamDevice(audioStream)) goto failed;
    if (getenv("MECHWARRIOR_DIAGNOSTICS"))
    {
        printf("Audio device: %d Hz, %d channels, %d frames (%.2f ms)\n",
            deviceFormat.freq,deviceFormat.channels,deviceFrames,1000.0*deviceFrames/deviceFormat.freq);
        fflush(stdout);
    }
    {
        const char *pcmPath=getenv("MECHWARRIOR_AUDIO_PCM");
        if (pcmPath && !pcmDump && !(pcmDump=fopen(pcmPath,"wb"))) return SDL_SetError("Cannot open the PCM dump %s",pcmPath);
    }
    return true;
failed:
    {
        /* A failed device open owns the synth and mutex too. Preserve SDL's
         * cause while releasing partial host resources; original game state
         * and the hardware timeline are not reset here. */
        char error[1024];SDL_snprintf(error,sizeof(error),"%s",SDL_GetError());
        MwSdlShutdownAudio();
        return SDL_SetError("%s",error);
    }
}
/* The mixer's sink: one block of frames at a time, onto the bounded queue. */
static void QueueAudioBlock(const float *samples,uint32_t frames)
{
    if (mwSdlAudioObserver) mwSdlAudioObserver(samples,frames*outputChannels);
    if (pcmDump && fwrite(samples,sizeof(float),frames*outputChannels,pcmDump)!=frames*outputChannels)
        MwSdlFail("Write the PCM dump");
    if (MwSdlVirtualClock()) {generatedFrames+=frames;return;} /* Test clock outruns the device. */
    SDL_LockMutex(audioQueueMutex);
    for (uint32_t index=0;index<frames;++index,++generatedFrames)
    {
        if (generatedFrames<playbackFrames) {++lateFrames;continue;}
        if (generatedFrames-playbackFrames>=AUDIO_QUEUE_FRAMES)
        {
            SDL_UnlockMutex(audioQueueMutex);
            MwSdlFail("Audio producer advanced beyond bounded timeline");
        }
        for (uint32_t channel=0;channel<outputChannels;++channel)
            audioQueue[generatedFrames%AUDIO_QUEUE_FRAMES][channel]=samples[index*outputChannels+channel];
    }
    SDL_UnlockMutex(audioQueueMutex);
}
void MwSdlAdvanceAudio(double pitTime) {mixer.advance(pitTime,QueueAudioBlock);}
void MwSdlWriteSpeakerPort(uint16_t port,uint8_t value)
{
    if (!mixer.speaker().writePort(port,value)) MwSdlFail("Unsupported speaker hardware write");
}
uint8_t MwSdlReadSpeakerControl(void) {return mixer.speaker().control();}
uint32_t MwSdlQueuedAudioMilliseconds(void)
{
    int bytes=SDL_GetAudioStreamQueued(audioStream);
    if (bytes<0) MwSdlFail("Read audio queue duration");
    SDL_LockMutex(audioQueueMutex);
    uint64_t pending=leadFrames+(generatedFrames>playbackFrames?generatedFrames-playbackFrames:0);
    SDL_UnlockMutex(audioQueueMutex);
    return (uint32_t)((pending+(uint32_t)bytes/(sizeof(float)*outputChannels))*1000/HOST_AUDIO_SAMPLE_RATE);
}
uint64_t MwSdlLateAudioFrames(void)
{SDL_LockMutex(audioQueueMutex);uint64_t frames=lateFrames;SDL_UnlockMutex(audioQueueMutex);return frames;}
uint64_t MwSdlAudioUnderrunFrames(void)
{SDL_LockMutex(audioQueueMutex);uint64_t frames=underrunFrames;SDL_UnlockMutex(audioQueueMutex);return frames;}
uint64_t MwSdlAudioClockSkippedFrames(void)
{SDL_LockMutex(audioQueueMutex);uint64_t frames=clockSkippedFrames;SDL_UnlockMutex(audioQueueMutex);return frames;}
void MwSdlShutdownAudio(void)
{
    if (audioStream) SDL_DestroyAudioStream(audioStream);
    audioStream=NULL;
    SDL_DestroyMutex(audioQueueMutex);audioQueueMutex=NULL;
    MwMt32Close();
    if (pcmDump) fclose(pcmDump);
    pcmDump=NULL;
}
