#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/native_address.h"
#include "SDLBackend.h"
#include "mechwarrior/campaign_runtime.h"
#include "mechwarrior/campaign_ui.h"
#include "mechwarrior/original_data.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/game_files.h"
#include "mechwarrior/rules.h"
#include "mechwarrior/ui_layout.h"
#include "mechwarrior/story.h"
#include "mechwarrior/launcher.h"
#include "mechwarrior/campaign_startup.h"
#include "mechwarrior/combat_startup.h"
#include "Setup/Setup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <new>
#include <string>

static std::unique_ptr<MwCampaignBuffer> resourceAllocations[8];
static uint16_t allocationCount;
static char installationDirectory[4096];
static bool initialized;

static void ReleaseChildResources(void)
{
    /* DOS frees a child's allocations and files after EXEC returns, including
     * startup failures. Do not move this cleanup into native allocation errors. */
    /* What points at the buffers is cleared first, so that nothing names a freed
     * one and a use (a music tick among them) fails. */
    campaignCore.indexedPictureOutput=campaignCore.generalPictureBank=campaignCore.cockpitPictureBank=NULL;
    campaignCore.activePictureBank=campaignCore.secondaryPictureBank=campaignCore.compressedPictureBuffer=NULL;
    campaignCore.pictureOutput=campaignStartup.campaignMusicBuffer=campaignStartup.graphicsOverlay=NULL;
    campaignMusic.midiStream=NULL;
    for (uint16_t i=0;i<allocationCount;++i) resourceAllocations[i].reset();
    allocationCount=0;
}
static bool ExecuteChild(const char *filename,const MwCampaignLaunch *launch,uint8_t audioCode,uint8_t *exitCode)
{
    /* The children's programs are ours: no exe is opened, or probed, at
     * play (slice 6e). */
    ReleaseChildResources();
    if (!strcmp(filename,"BTECH.EXE")) return MwSdlExecuteCombatChild(launch,audioCode,exitCode);
    if (strcmp(filename,"MW_MAIN.EXE")) MwSdlFail("Unknown launcher child executable");
    /* Fresh DOS child image on EACH execution, then0002 decides new/resume.
     * Keep input/ticks, physical video and resident exchange alive across EXEC. */
    if (!MwImportCampaignInitialData() || !MwImportCampaignEventText()
        || !MwImportCampaignMusicData()) return true;
    *exitCode=(uint8_t)MwRunCampaignProgram(launch,audioCode);
    ReleaseChildResources();
    return false;
}

[[noreturn]] void MwSdlFail(const char *operation)
{
    fprintf(stderr,"MechWarrior: %s",operation);
    const char *error=SDL_GetError();
    if (error && *error) fprintf(stderr,". SDL: %s",error);
    fputc('\n',stderr);
    static char reason[1024];
    snprintf(reason,sizeof(reason),error && *error?"%s. SDL: %s":"%s",operation,error);
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE(reason);
}
static MwCampaignBuffer *AllocateParagraphs(uint16_t paragraphs)
{
    (void)paragraphs; /* Full resource windows retain native WORD-cursor wrap. */
    if (allocationCount>=8) return NULL;
    /* Zeroed. One read comes before a write: MwReadPictureNibble's one byte past a
     * picture's data, decoded at its end. It was whatever the allocation held. */
    std::unique_ptr<MwCampaignBuffer> buffer(new (std::nothrow) MwCampaignBuffer());
    if (!buffer) return NULL;
    resourceAllocations[allocationCount]=std::move(buffer);
    return resourceAllocations[allocationCount++].get();
}
static void ValidateFileDestination(const uint8_t *buffer,uint16_t bytesRead)
{
    uintptr_t destination=(uintptr_t)buffer;
    for (uint16_t i=0;i<allocationCount;++i)
    {
        uintptr_t allocation=(uintptr_t)resourceAllocations[i]->bytes;
        if (destination>=allocation && destination-allocation<NATIVE_SEGMENT_BYTE_COUNT)
        {
            if (bytesRead>NATIVE_SEGMENT_BYTE_COUNT-(destination-allocation))
                MwSdlFail("Unsupported resource read beyond its native offset window");
            return;
        }
    }
    /* Other destinations are typed IO buffers supplied by original wrappers. */
}
static bool LoadOverlay(const char *filename)
{
    if (strcmp(filename,"MW_EGA.EXE")) MwSdlFail("Only the native EGA overlay is converted");
    return !MwHostLoadEgaInitialData();
}
static bool MousePresent(void) {return true;} /* SDL's pointer. */
static void DosString(const char *text)
{while (*text && *text!='$') fputc(*text++,stderr);fflush(stderr);}
static void UnsupportedCampaignCall(uint16_t codeOffset)
{
    fprintf(stderr,"Unconverted campaign callback 081B:%04X.\n",codeOffset);
    /* This is a game conversion failure, not a failed SDL call. SDL's error
     * slot may still contain an unrelated device-probe diagnostic. */
    SDL_ClearError();MwSdlFail("Unconverted campaign callback");
}
static char dataDirectory[4096];
bool MwSdlSetDataDirectory(const char *directory)
{
    if (strlen(directory)>=sizeof(dataDirectory)) return false;
    strcpy(dataDirectory,directory);
    return true;
}
const char *MwSdlDataDirectory(void)
{
    if (!dataDirectory[0])
    {
        const char *base=SDL_GetBasePath();
        if (!base || snprintf(dataDirectory,sizeof(dataDirectory),"%sdata",base)>=(int)sizeof(dataDirectory)) dataDirectory[0]=0;
    }
    return dataDirectory;
}
/* The player's write directory (saves, story/, their MechWarrior.ini):
 * MECHWARRIOR_SAVE_DIRECTORY, or SDL's per-user folder under the family's
 * name (src/Setup/Setup.h names it the same way without making it, for
 * --status), or the same game's folder under the old name while a move
 * across has not happened (setup::unmoved_folder: it failed this start). */
bool MwSdlResolveWriteDirectory(char *directory,size_t size)
{
    const char *configuredSaveDirectory=getenv("MECHWARRIOR_SAVE_DIRECTORY");
    if (configuredSaveDirectory)
    {
        if (strlen(configuredSaveDirectory)>=size) return false;
        strcpy(directory,configuredSaveDirectory);
        return true;
    }
    const std::string unmoved=mw1o::setup::unmoved_folder(mw1o::setup::pref_folder(mw1o::setup::kOrganisation,mw1o::setup::kApplication));
    if (!unmoved.empty())
    {
        if (unmoved.size()>=size) return false;
        strcpy(directory,unmoved.c_str());
        return true;
    }
    char *prefPath=SDL_GetPrefPath(mw1o::setup::kOrganisation,mw1o::setup::kApplication);
    if (!prefPath || strlen(prefPath)>=size) {SDL_free(prefPath);return false;}
    strcpy(directory,prefPath);SDL_free(prefPath);
    return true;
}
/* A start refused after SDL_Init: the reason is kept, since SDL_Quit clears
 * SDL's error and the caller prints it. */
static bool RefuseStart(void)
{
    const std::string reason=SDL_GetError();
    MwSdlShutdown();
    return SDL_SetError("%s",reason.c_str());
}
bool MwSdlInitialize(const char *gameDirectory,const char *captureDirectory)
{
    if (initialized || strlen(gameDirectory)>=sizeof(installationDirectory)) return false;
    strcpy(installationDirectory,gameDirectory);
    /* The player's write dir holds the saves and the story file, which is
     * imported (once) before the campaign's initial data reads it. */
    char writeDirectory[4096];
    if (!MwSdlResolveWriteDirectory(writeDirectory,sizeof(writeDirectory))) return false;
    if (!MwSdlDataDirectory()[0]) return false;
    if (!MwHostPrepareRules(dataDirectory) || !MwHostPrepareUi(dataDirectory)) return false;
    mw1o::files::set_save_directory(writeDirectory);
    if (!MwHostPrepareStory(gameDirectory,writeDirectory)) return false;
    if (!MwImportCampaignInitialData() || !MwImportCampaignEventText()
        || !MwImportCampaignMusicData()) return false;
    mw1o::files::set_game_directory(gameDirectory);
    MwImportLauncherInitialData();
    if (!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS)) return false;
    initialized=true;
    if (!MwSdlInitializeVideo(captureDirectory) || !MwSdlInitializeAudio()) return RefuseStart();
    MwSdlInitializeTimer();MwSdlInitializeInput();
    MwSdlInitializeCombatHardware();
    if (!MwSdlInitializeCombatProcess(gameDirectory)) return RefuseStart();
    mwHostValidateFileDestination=ValidateFileDestination;
    mwCampaignPlatform=MwCampaignPlatformCalls{AllocateParagraphs,LoadOverlay,
        MwSdlInstallCampaignInterrupts,MwSdlRestoreCampaignInterrupts,
        MwSdlReadBiosTicks,MousePresent,DosString};
    campaignStartMusic=MwStartMidiTrackFar;campaignStopMusic=MwStopMidiTrackFar;
    campaignNearCallback=UnsupportedCampaignCall;
    mwLauncherSystem=MwLauncherSystemCalls{ExecuteChild};
    return true;
}
int MwSdlRunCampaign(void)
{
    if (!initialized) MwSdlFail("Host not initialized");
    /* MW.EXE's audio letters: S (speaker), and R for the rebuild's music, whose
     * notes go nowhere with music=off. */
    return MwLauncherMain((uint8_t)(mwSdlMusic==MW_MUSIC_SPEAKER?'S':'R'));
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
