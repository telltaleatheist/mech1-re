#include "SDLBackend.h"
#include "mechwarrior/audio_devices.h"
#include "mechwarrior/music_out.h"
#include "AudioClock.h"
#include "Mixer.h"
#include "Music.h"
#include "../Support/Sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <string>

/* The output layer: the SDL3 device, the bounded PCM queue and its stream
 * callback. Synthesis (the tone voice, the selected card, mixing on PIT
 * time) is mw1o::audio::Mixer in src/Audio, which hands this file blocks.
 * MIDI scheduling, allocation, instruments and rotation remain Original code;
 * the host never plays an SMF itself. */
static SDL_AudioStream *audioStream;
static SDL_Mutex *audioQueueMutex;
enum { AUDIO_QUEUE_FRAMES=0x4000,AUDIO_START_LEAD_FRAMES=0x0800,HOST_EFFECT_TICK_PORT_READ_NS=1000 };
static float audioQueue[AUDIO_QUEUE_FRAMES][2];
static uint32_t outputChannels=1;
static uint64_t generatedFrames,playbackFrames,lateFrames,underrunFrames;
static uint64_t audioStartedNanoseconds,clockSkippedFrames;
static uint32_t playbackLeadFrames;
static uint32_t leadFrames;
static mw1o::audio::Mixer mixer;
/* music=soundfont: the sequencer's output (music_out.h), played from
 * data/music's soundfont and instrument table. */
static mw1o::audio::Music music;
/* With MECHWARRIOR_AUDIO_PCM, on any device: "<PIT cycle> <head>" per song
 * start, in <dump>.songs, so tools/trace/listening.py can cut the songs out
 * (frame = cycle * 48000 / 1193182, as audio.py counts). A test aid. */
static FILE *songMarks;
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

/* Each event sounds at the PIT time it happens: render up to now first. */
static void MusicSongStarted(const uint8_t head[32])
{
    const std::string prefix=mw1o::support::sha256_hex(std::span<const uint8_t>(head,32)).substr(0,16);
    if (songMarks) {fprintf(songMarks,"%.0f %s\n",MwSdlHardwareTime(),prefix.c_str());fflush(songMarks);}
    if (!music.loaded()) return; /* Song marks only. */
    MwSdlAdvanceAudio(MwSdlHardwareTime());
    music.startSong(prefix);
}
static void MusicByte(uint8_t value)
{
    MwSdlAdvanceAudio(MwSdlHardwareTime());
    music.midiByte(value);
}
static bool LoadMusic(void)
{
    const std::string folder=std::string(MwSdlDataDirectory())+"/music/";
    size_t bankSize=0,tableSize=0;
    void *bank=SDL_LoadFile((folder+"FluidR3_GM-subset.sf2").c_str(),&bankSize);
    if (!bank) return SDL_SetError("music=soundfont: cannot read %sFluidR3_GM-subset.sf2: %s",folder.c_str(),SDL_GetError());
    char *table=(char *)SDL_LoadFile((folder+"instruments.json").c_str(),&tableSize);
    if (!table) {SDL_free(bank);return SDL_SetError("music=soundfont: cannot read %sinstruments.json: %s",folder.c_str(),SDL_GetError());}
    std::string error;
    bool loaded=false;
    try {loaded=music.load(bank,bankSize,mw1o::audio::ParseInstrumentTable(std::string_view(table,tableSize)),HOST_AUDIO_SAMPLE_RATE,error);}
    catch (const std::exception &failure) {error=failure.what();}
    SDL_free(bank);SDL_free(table);
    if (!loaded) return SDL_SetError("music=soundfont: %s",error.c_str());
    mwMusicOut=MwMusicOutCalls{MusicSongStarted,MusicByte};
    return true;
}

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
    if (mwSdlMusicSoundfont) {if (!LoadMusic()) return false;} /* No MT-32 synthesizer: the MPU's output goes nowhere. */
    else if (mwSdlSoundSystem==MW_SOUND_MT32 && !silentMt32)
    {
        char error[1024];
        if (!MwMt32Open(mwSdlMt32ControlRom,mwSdlMt32PcmRom,HOST_AUDIO_SAMPLE_RATE,error,sizeof(error)))
            return SDL_SetError("%s",error);
    }
    mixer.reset(mwSdlMusicSoundfont?mw1o::audio::Mixer::Card::Soundfont
        :mwSdlSoundSystem==MW_SOUND_SPEAKER?mw1o::audio::Mixer::Card::None
        :mwSdlSoundSystem==MW_SOUND_MT32?mw1o::audio::Mixer::Card::Mt32:mw1o::audio::Mixer::Card::Adlib,silentMt32,&music);
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
        if (pcmPath && !songMarks)
        {
            if (!(songMarks=fopen((std::string(pcmPath)+".songs").c_str(),"w"))) return SDL_SetError("Cannot open the song marks beside %s",pcmPath);
            mwMusicOut.songStarted=MusicSongStarted;
        }
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
/* One tone on the tone voice, from the hardware time now, as a port write
 * landed before: the campaign's speaker music (S6) and combat's effects (S5). */
void MwSdlPlayTone(double frequency)
{
    MwSdlAdvanceAudio(MwSdlHardwareTime());
    if (frequency<0) frequency=0;
    if (frequency!=mixer.tone().frequency()) MwSdlAudioLogTone(frequency); /* Each change, for the sound log. */
    if (frequency>0) mixer.tone().play(frequency);
    else mixer.tone().stop();
}
/* Combat's effect player, each tick. The test clock still charges the 61h
 * read each tick made before S5 (H4 removes the per-read charges), so the
 * logs' timeline stays the same. */
void MwSdlPlayEffectTone(double frequency)
{
    if (MwSdlVirtualClock()) MwSdlSpendNanoseconds(HOST_EFFECT_TICK_PORT_READ_NS);
    MwSdlPlayTone(frequency);
}
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
    mwMusicOut=MwMusicOutCalls{};
    if (pcmDump) fclose(pcmDump);
    pcmDump=NULL;
    if (songMarks) fclose(songMarks);
    songMarks=NULL;
}
