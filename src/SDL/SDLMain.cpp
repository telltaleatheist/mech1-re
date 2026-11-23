#define SDL_MAIN_HANDLED
#include "SDLBackend.h"
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
static FILE *diagnosticAudio;
static uint64_t nextDiagnosticFrame;
static void RecordDiagnosticAudio(const float *samples,uint32_t count)
{
    if (fwrite(samples,sizeof(float),count,diagnosticAudio)!=count)
        MwSdlFail("Write private diagnostic audio");
}
static void TraceDiagnosticFrame(void)
{
    uint64_t now=SDL_GetTicks();
    if (!mwSdlCombatActive || now<nextDiagnosticFrame || !playerUnitBinding
        || !playerUnitBinding->positionedLeftLeg) return;
    nextDiagnosticFrame=now+1000;
    const BtechPooledWorldObject *body=playerUnitBinding->positionedLeftLeg;
    fprintf(stderr,"Combat time=%llu tick=%u position=%ld,%ld step=%d pose=%u/%u gait=%u sound=%u timerLagMs=%.2f audioQueueMs=%u tickMs=%.1f speed=%u lateAudioFrames=%llu underrunFrames=%llu\n",
        (unsigned long long)now,combatTickCounter,
        (long)(int32_t)body->object.prefix.position.positionA,
        (long)(int32_t)body->object.prefix.position.positionB,
        body->forwardStepScale,playerPoseSequence,playerPoseFrame,
        playerPoseDistanceAccumulator,combatSoundEnabled,
        MwSdlTimerLagMilliseconds(),MwSdlQueuedAudioMilliseconds(),
        (double)MwSdlCombatTickNanoseconds()/1e6,(unsigned)mwSdlCombatSpeed,(unsigned long long)MwSdlLateAudioFrames(),
        (unsigned long long)MwSdlAudioUnderrunFrames());
    for (uint16_t slot=1;slot<COMBAT_MECH_SLOTS;++slot)
    {
        const BtechPooledWorldObject *other=unitBindings[slot].positionedLeftLeg;
        if (!unitBindings[slot].mech || !other) continue;
        fprintf(stderr,"AI slot=%u position=%ld,%ld step=%d\n",slot,
            (long)(int32_t)other->object.prefix.position.positionA,
            (long)(int32_t)other->object.prefix.position.positionB,
            other->forwardStepScale);
    }
    fflush(stderr);
}
#if defined(_MSC_VER)
#include <stdlib.h>
#include <signal.h>
#include <windows.h>
#include <dbghelp.h>

/* Host diagnostics only. Port failsafes print their reason before SIGABRT;
 * add a stack trace to the launcher's runtime.log. PDBs improve frame names,
 * but explicit reason/function/source diagnostics do not depend on symbols. */
static void ReportFatalAbort(int signalNumber)
{
    (void)signalNumber;
    HANDLE process=GetCurrentProcess();
    SymSetOptions(SYMOPT_LOAD_LINES|SYMOPT_UNDNAME|SYMOPT_DEFERRED_LOADS);
    BOOL symbols=SymInitialize(process,NULL,TRUE);
    void *frames[40];USHORT count=CaptureStackBackTrace(0,40,frames,NULL);
    fprintf(stderr,"MechWarrior fatal abort (combat active=%u).\n",mwSdlCombatActive?1:0);
    for (USHORT index=0;index<count;++index)
    {
        IMAGEHLP_LINE64 line={0};line.SizeOfStruct=sizeof(line);DWORD displacement;
        if (symbols && SymGetLineFromAddr64(process,(DWORD64)frames[index],&displacement,&line))
            fprintf(stderr,"%s:%lu\n",line.FileName,(unsigned long)line.LineNumber);
        else
            fprintf(stderr,"frame %u: %p\n",index,frames[index]);
    }
    fflush(stderr);_Exit(3);
}
#endif
/* An installed copy starts with no game folder (a double-click): the folder
 * is the one remembered in the player's MechWarrior.ini (game_folder), or the
 * player picks it once with a folder dialog. */
static void Tell(const char *text)
{
    fprintf(stderr,"%s\n",text);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,"MechWarrior",text,NULL);
}
static bool PathIs(const char *path,SDL_PathType type)
{SDL_PathInfo info;return SDL_GetPathInfo(path,&info) && info.type==type;}
static bool JoinWriteDirectory(const char *name,char *path,size_t size)
{
    char directory[4096];
    if (!MwSdlResolveWriteDirectory(directory,sizeof(directory))) return false;
    size_t length=strlen(directory);
    const char *separator=length && (directory[length-1]=='/' || directory[length-1]=='\\')?"":"/";
    return snprintf(path,size,"%s%s%s",directory,separator,name)<(int)size;
}
static bool StoryImported(void)
{char path[4200];return JoinWriteDirectory("story/import.json",path,sizeof(path)) && PathIs(path,SDL_PATHTYPE_FILE);}
static SDL_EnumerationResult SDLCALL FindMainProgram(void *found,const char *directory,const char *name)
{
    (void)directory;
    if (SDL_strcasecmp(name,"MW_MAIN.EXE")) return SDL_ENUM_CONTINUE;
    *(bool *)found=true;return SDL_ENUM_SUCCESS;
}
/* The first run imports the story from the game's programs; after it, play
 * needs only the folder's resource files. */
static bool HoldsTheGame(const char *folder)
{
    if (!PathIs(folder,SDL_PATHTYPE_DIRECTORY)) return false;
    if (StoryImported()) return true;
    bool found=false;SDL_EnumerateDirectory(folder,FindMainProgram,&found);return found;
}
struct FolderChoice {SDL_AtomicInt done;char path[4096];};
static void SDLCALL FolderChosen(void *userdata,const char *const *list,int filter)
{
    (void)filter;FolderChoice *choice=(FolderChoice *)userdata;
    choice->path[0]=0;
    if (list && list[0] && strlen(list[0])<sizeof(choice->path)) strcpy(choice->path,list[0]);
    SDL_SetAtomicInt(&choice->done,1);
}
static bool ChooseGameFolder(char *folder,size_t size)
{
    static FolderChoice choice;SDL_SetAtomicInt(&choice.done,0);
    SDL_ShowOpenFolderDialog(FolderChosen,&choice,NULL,mwSdlGameFolder[0]?mwSdlGameFolder:NULL,false);
    while (!SDL_GetAtomicInt(&choice.done)) SDL_WaitEventTimeout(NULL,50);
    if (!choice.path[0] || strlen(choice.path)>=size) return false;
    strcpy(folder,choice.path);return true;
}
/* Returns 0 started, 1 failed, 2 the player cancelled. */
static int StartInstalled(char *folder,size_t size,const char *captures)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {fprintf(stderr,"Cannot start SDL: %s\n",SDL_GetError());return 1;}
    bool remembered=mwSdlGameFolder[0] && strlen(mwSdlGameFolder)<size;
    if (remembered) strcpy(folder,mwSdlGameFolder);
    else Tell("Choose the folder that holds your copy of MechWarrior (the one with MW_MAIN.EXE).\n\n"
        "The game reads it the first time and remembers the folder.");
    for (;;)
    {
        if (!remembered && !ChooseGameFolder(folder,size)) return 2;
        remembered=false;
        if (!HoldsTheGame(folder))
        {Tell("That folder doesn't hold MechWarrior's MW_MAIN.EXE. Choose the folder with your copy of the game.");continue;}
        if (MwSdlInitialize(folder,captures)) break;
        if (StoryImported()) {Tell("MechWarrior could not start. mechwarrior.log in the save folder says why.");return 1;}
        Tell("MechWarrior couldn't read that copy of the game (a supported revision is needed; mechwarrior.log in the "
            "save folder says why). Choose another folder.");
    }
    if (strcmp(folder,mwSdlGameFolder)) MwSdlSaveUserText("game_folder",folder);
    return 0;
}
int main(int argc,char **argv)
{
#if defined(_MSC_VER)
    signal(SIGABRT,ReportFatalAbort);
#endif
    const char *pilotDirectory=NULL;
    /* --data DIR, anywhere: where our data/ folder is. */
    for (int index=1;index<argc;++index)
    {
        if (strcmp(argv[index],"--data")) continue;
        if (index+1>=argc || !MwSdlSetDataDirectory(argv[index+1]))
        {fprintf(stderr,"--data needs a folder.\n");return 2;}
        for (int rest=index;rest+2<=argc;++rest) argv[rest]=argv[rest+2];
        argc-=2;break;
    }
    if(argc>=3 && !strcmp(argv[argc-2],"--support-pilot"))
    {pilotDirectory=argv[argc-1];argc-=2;}
    else if(argc>=2 && !strcmp(argv[argc-1],"--no-support-pilot"))--argc;
    bool help=argc==2 && (!strcmp(argv[1],"--help") || !strcmp(argv[1],"-h"));
    bool installed=argc==1;
    if (installed)
    {
        /* No console when double-clicked: the log goes beside the saves. */
        char logPath[4200];
        if (JoinWriteDirectory("mechwarrior.log",logPath,sizeof(logPath)))
        {
            if (!freopen(logPath,"w",stdout) || !freopen(logPath,"a",stderr)) {}
            setvbuf(stderr,NULL,_IONBF,0);
        }
    }
    if (help || argc>3)
    {fprintf(help?stdout:stderr,"Usage: mechwarrior [<game folder> [existing capture directory]] [--support-pilot DIRECTORY | --no-support-pilot] [--data DIRECTORY]\n"
        "Without a game folder: the one remembered in the player's MechWarrior.ini, or chosen once in a folder dialog;\n"
        "the log and F12 captures then go beside the saves.\n"
        "EGA; sound device from MechWarrior.ini; arrows and Enter; Ctrl+Escape quits; F12 captures; F11 stops Support pilot;\n"
        "- and + change the combat speed.\n");return help?0:2;}
    SDL_SetMainReady();
    if(!MwSdlConfigureSupportPilot(pilotDirectory))
    {fprintf(stderr,"Cannot open Support pilot directory.\n");return 2;}
    const char *timingPath=getenv("MECHWARRIOR_TIMING_CONFIG");
    bool optionalTiming=timingPath==NULL;
    char defaultTimingPath[4096];
    if (!timingPath)
    {
        const char *base=SDL_GetBasePath();
        if (!base || snprintf(defaultTimingPath,sizeof(defaultTimingPath),"%sMechWarrior.ini",base)>=(int)sizeof(defaultTimingPath)) return 2;
        timingPath=defaultTimingPath;
    }
    if (!MwSdlLoadTimingConfiguration(timingPath,optionalTiming))
    {fprintf(stderr,"Invalid MechWarrior configuration: %s (combat tick, speed, sound system or ROM path).\n",timingPath);return 2;}
    if (!MwSdlLoadUserConfiguration())
    {fprintf(stderr,"Invalid player configuration: MechWarrior.ini in the save directory (combat tick, speed, sound system or ROM path).\n");return 2;}
    printf("Combat tick: %u ms at 1x, speed %ux (- and + in combat); PIT/audio keep hardware rate.\n",
        mwSdlCombatTickMs,(unsigned)mwSdlCombatSpeed);
    fflush(stdout); /* Keep startup information live when the launcher logs it. */
    char installedFolder[4096],captures[4200];
    if (installed)
    {
        /* F12 captures go beside the saves too. */
        if (!MwSdlResolveWriteDirectory(captures,sizeof(captures))) return 1;
        int started=StartInstalled(installedFolder,sizeof(installedFolder),captures);
        if (started) {SDL_Quit();return started==2?0:1;}
    }
    else if (!MwSdlInitialize(argv[1],argc==3?argv[2]:"."))
    {fprintf(stderr,"Cannot initialize the private installation/SDL: %s\n",SDL_GetError());return 1;}
    if (getenv("MECHWARRIOR_DIAGNOSTICS") && argc==3)
    {
        char audioPath[4096];
        if (snprintf(audioPath,sizeof(audioPath),"%s/speaker.f32",argv[2])>=(int)sizeof(audioPath)) return 2;
        diagnosticAudio=fopen(audioPath,"wbx");
        if (!diagnosticAudio) return 2;
        mwSdlAudioObserver=RecordDiagnosticAudio;mwSdlFrameObserver=TraceDiagnosticFrame;
    }
    int result=MwSdlRunCampaign();
    mwSdlAudioObserver=NULL;mwSdlFrameObserver=NULL;
    if (diagnosticAudio) {fclose(diagnosticAudio);diagnosticAudio=NULL;}
    MwSdlShutdown();return result;
}
