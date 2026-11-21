#include "mechwarrior/port_diagnostics.h"
#include "SDLBackend.h"
#include "mechwarrior/campaign_runtime.h"
#include "mechwarrior/campaign_ui.h"
#include "mechwarrior/original_data.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/dos_files.h"
#include "mechwarrior/rules.h"
#include "mechwarrior/story.h"
#include "mechwarrior/launcher.h"
#include "mechwarrior/campaign_startup.h"
#include "mechwarrior/combat_startup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t *resourceAllocations[8];
static uint16_t allocationCount;
static char installationDirectory[4096];
static bool initialized;

static void ReleaseChildResources(void)
{
    /* DOS frees a child's allocations and files after EXEC returns, including
     * startup failures. Do not move this cleanup into native allocation errors. */
    MwHostCloseProcessFiles();
    for (uint16_t i=0;i<allocationCount;++i)
    {MwBindNativeSegment((uint16_t)(0x4000+i*0x1000),NULL);free(resourceAllocations[i]);resourceAllocations[i]=NULL;}
    allocationCount=0;
}
static bool ExecuteChild(const char *filename,const MwCampaignLaunch *launch,uint8_t audioCode,uint8_t *exitCode)
{
    char path[4096];
    if (!MwHostJoinGamePath(path,sizeof(path),installationDirectory,filename)) return true;
    FILE *file=fopen(path,"rb");
    if (!file) return true;
    if (fclose(file)!=0) return true;
    ReleaseChildResources();
    if (!strcmp(filename,"BTECH.EXE")) return MwSdlExecuteCombatChild(path,launch,audioCode,exitCode);
    if (strcmp(filename,"MW_MAIN.EXE")) MwSdlFail("Unknown launcher child executable");
    /* Fresh DOS child image on EACH execution, then0002 decides new/resume.
     * Keep input/ticks, physical video and resident exchange alive across EXEC. */
    if (!MwHostLoadCampaignInitialData(path) || !MwImportCampaignEventText()
        || !MwHostLoadCampaignMusicData(path)) return true;
    *exitCode=(uint8_t)MwRunCampaignProgram(launch,audioCode);
    ReleaseChildResources();
    return false;
}
static void ClearLauncherText(void) {MwSdlSetVideoMode(3);}

[[noreturn]] void MwSdlFail(const char *operation)
{
    fprintf(stderr,"MechWarrior: %s",operation);
    const char *error=SDL_GetError();
    if (error && *error) fprintf(stderr,". SDL: %s",error);
    fputc('\n',stderr);/* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("SDL backend operation failed; see the preceding operation and SDL diagnostic.");
}
static bool AllocateParagraphs(uint16_t paragraphs,uint16_t *selectorOrError)
{
    (void)paragraphs; /* Full resource windows retain native WORD-cursor wrap. */
    if (allocationCount>=8) { *selectorOrError=8;return true; }
    uint8_t *buffer=static_cast<uint8_t *>(malloc(NATIVE_SEGMENT_BYTE_COUNT));
    if (!buffer) { *selectorOrError=8;return true; }
    *selectorOrError=(uint16_t)(0x4000+allocationCount*0x1000);
    resourceAllocations[allocationCount++]=buffer;
    MwBindNativeSegment(*selectorOrError,buffer);
    return false;
}
static void ValidateFileDestination(const uint8_t *buffer,uint16_t bytesRead)
{
    uintptr_t destination=(uintptr_t)buffer;
    for (uint16_t i=0;i<allocationCount;++i)
    {
        uintptr_t allocation=(uintptr_t)resourceAllocations[i];
        if (destination>=allocation && destination-allocation<NATIVE_SEGMENT_BYTE_COUNT)
        {
            if (bytesRead>NATIVE_SEGMENT_BYTE_COUNT-(destination-allocation))
                MwSdlFail("Unsupported resource read beyond its native offset window");
            return;
        }
    }
    /* Other destinations are typed IO buffers supplied by original wrappers. */
}
static bool LoadOverlay(const char *filename,uint16_t selector)
{
    (void)selector;
    if (strcmp(filename,"MW_EGA.EXE")) MwSdlFail("Only the native EGA overlay is converted");
    char path[4096];
    if (!MwHostJoinGamePath(path,sizeof(path),installationDirectory,filename)) return true;
    return !MwHostLoadEgaInitialData(path);
}
static uint16_t ResetMouse(void) {return UINT16_MAX;} /* SDL supplies the INT33-equivalent boundary. */
static void DosString(const char *text)
{while (*text && *text!='$') fputc(*text++,stderr);fflush(stderr);}
static void UnsupportedCampaignCall(uint16_t codeOffset)
{
    fprintf(stderr,"Unconverted campaign callback 081B:%04X.\n",codeOffset);
    /* This is a game conversion failure, not a failed SDL call. SDL's error
     * slot may still contain an unrelated device-probe diagnostic. */
    SDL_ClearError();MwSdlFail("Unconverted campaign callback");
}
static bool LoadResidentLauncher(const char *path)
{
    uint8_t header[0x1C],data[0x09EE];FILE *file=fopen(path,"rb");if (!file) return false;
    bool loaded=false;
    if (fread(header,1,sizeof(header),file)!=sizeof(header) || header[0]!='M' || header[1]!='Z') goto finished;
    uint16_t paragraphs;paragraphs=(uint16_t)(header[8]|((uint16_t)header[9]<<8));
    /* MW.EXE load0800, resident data0806, real transfer offset0004. */
    if (fseek(file,(long)paragraphs*0x10+0x60,SEEK_SET)!=0) goto finished;
    if (fread(data,1,sizeof(data),file)!=sizeof(data) || ferror(file)) goto finished;
    loaded=MwImportLauncherInitialData(data,sizeof(data));
finished:
    if (fclose(file)!=0) loaded=false;
    return loaded;
}
static char dataDirectory[4096];
bool MwSdlSetDataDirectory(const char *directory)
{
    if (strlen(directory)>=sizeof(dataDirectory)) return false;
    strcpy(dataDirectory,directory);
    return true;
}
bool MwSdlInitialize(const char *gameDirectory,const char *captureDirectory)
{
    if (initialized || strlen(gameDirectory)>=sizeof(installationDirectory)) return false;
    strcpy(installationDirectory,gameDirectory);
    /* The player's write dir holds the saves and the story file, which is
     * imported (once) before the campaign's initial data reads it. */
    char writeDirectory[4096];
    const char *configuredSaveDirectory=getenv("MECHWARRIOR_SAVE_DIRECTORY");
    if (configuredSaveDirectory)
    {
        if (strlen(configuredSaveDirectory)>=sizeof(writeDirectory)) return false;
        strcpy(writeDirectory,configuredSaveDirectory);
    }
    else
    {
        char *prefPath=SDL_GetPrefPath("BattleTechReverse","MechWarrior");
        if (!prefPath || strlen(prefPath)>=sizeof(writeDirectory)) {SDL_free(prefPath);return false;}
        strcpy(writeDirectory,prefPath);SDL_free(prefPath);
    }
    if (!dataDirectory[0])
    {
        const char *base=SDL_GetBasePath();
        if (!base || snprintf(dataDirectory,sizeof(dataDirectory),"%sdata",base)>=(int)sizeof(dataDirectory)) return false;
    }
    if (!MwHostPrepareRules(dataDirectory)) return false;
    if (!MwHostSetCampaignSaveDirectory(writeDirectory) || !MwHostPrepareStory(gameDirectory,writeDirectory)) return false;
    char path[4096];
    if (!MwHostJoinGamePath(path,sizeof(path),gameDirectory,"MW_MAIN.EXE")
        || !MwHostLoadCampaignInitialData(path) || !MwImportCampaignEventText()
        || !MwHostLoadCampaignMusicData(path) || !MwHostSetOriginalGameDirectory(gameDirectory)) return false;
    if (!MwHostJoinGamePath(path,sizeof(path),gameDirectory,"MW.EXE") || !LoadResidentLauncher(path)) return false;
    if (!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS)) return false;
    initialized=true;
    if (!MwSdlInitializeVideo(captureDirectory) || !MwSdlInitializeAudio()) {MwSdlShutdown();return false;}
    MwSdlInitializeTimer();MwSdlInitializeInput();
    MwSdlInitializeCombatHardware();
    mwMusicHardware.readInterruptVector=btechHostGetInterruptVector;
    mwMusicHardware.writeInterruptVector=btechHostSetInterruptVector;
    if (!MwSdlInitializeCombatProcess(gameDirectory)) {MwSdlShutdown();return false;}
    mwHostValidateFileDestination=ValidateFileDestination;
    mwCampaignPlatform=MwCampaignPlatformCalls{AllocateParagraphs,LoadOverlay,
        MwSdlInstallCampaignInterrupts,MwSdlRestoreCampaignInterrupts,MwSdlGetVideoMode,MwSdlSetVideoMode,
        MwSdlReadBiosTicks,ResetMouse,DosString};
    campaignStartMusic=MwStartMidiTrackFar;campaignStopMusic=MwStopMidiTrackFar;
    campaignNearCallback=UnsupportedCampaignCall;
    mwLauncherSystem=MwLauncherSystemCalls{ClearLauncherText,ExecuteChild};
    return true;
}
int MwSdlRunCampaign(void)
{
    if (!initialized) MwSdlFail("Host not initialized");
    /* MW.EXE's audio letters: R (MT-32), A (AdLib), S (speaker). */
    return MwLauncherMain((uint8_t)(mwSdlSoundSystem==MW_SOUND_MT32?'R':mwSdlSoundSystem==MW_SOUND_ADLIB?'A':'S'));
}
void MwSdlPump(void)
{MwSdlPollEvents();MwSdlPumpTimer();MwSdlPresentVideo(false);}
void MwSdlShutdown(void)
{
    if (!initialized) return;
    MwSdlShutdownDebugControl();
    mwHostValidateFileDestination=NULL;
    ReleaseChildResources();
    MwSdlShutdownAudio();MwSdlShutdownVideo();
    SDL_Quit();initialized=false;
}
