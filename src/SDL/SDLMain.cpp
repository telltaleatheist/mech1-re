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
#include "SDLSetup.h"
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
static bool JoinWriteDirectory(const char *name,char *path,size_t size)
{
    char directory[4096];
    if (!MwSdlResolveWriteDirectory(directory,sizeof(directory))) return false;
    size_t length=strlen(directory);
    const char *separator=length && (directory[length-1]=='/' || directory[length-1]=='\\')?"":"/";
    return snprintf(path,size,"%s%s%s",directory,separator,name)<(int)size;
}
int main(int argc,char **argv)
{
#if defined(_MSC_VER)
    signal(SIGABRT,ReportFatalAbort);
#endif
    /* A run with no player (a test's, a hidden one) is a background app on
     * macOS: SDL then never makes it a foreground app or activates it, so it
     * can't take the keyboard from whoever is at the machine. It comes before
     * every SDL_Init, the setup screen's too. */
    if (MwSetupUnattended()) SDL_SetHint(SDL_HINT_MAC_BACKGROUND_APP,"1");
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
    /* The launcher contract (re-setup-kit): --status answers before anything
     * is read or written; --setup sets the game up and plays nothing. */
    if (argc==2 && !strcmp(argv[1],"--status")) return MwSetupPrintStatus();
    bool setupOnly=argc==2 && !strcmp(argv[1],"--setup");
    bool installed=argc==1 || setupOnly;
    if (!help) MwSetupSettleUserFolder();
    if (argc==1)
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
    {const char *name="mw1r";  /* the shipped program's name (an AppImage starts it as AppRun) */
    fprintf(help?stdout:stderr,"Usage: %s [<game folder> [existing capture directory]] [--support-pilot DIRECTORY | --no-support-pilot] [--data DIRECTORY]\n"
        "       %s --status | --setup\n"
        "Without a game folder: the one remembered in the player's MechWarrior.ini or the player's own copy, else the\n"
        "setup (pick your copy, its folder or the archive it came in: it is copied beside the saves); the log and F12\n"
        "captures then go beside the saves. --status prints what is found as JSON; --setup only sets the game up.\n"
        "EGA; sound device from MechWarrior.ini; arrows and Enter; Ctrl+Escape quits; F12 captures; F11 stops Support pilot;\n"
        "- and + change the combat speed.\n",name,name);return help?0:2;}
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
        int ready=MwSetupResolveGame(setupOnly,installedFolder,sizeof(installedFolder));
        if (setupOnly || ready) {SDL_Quit();return setupOnly?ready:ready==1?0:1;}
        if (!MwSdlInitialize(installedFolder,captures))
        {fprintf(stderr,"MechWarrior could not start from %s: %s\n",installedFolder,SDL_GetError());SDL_Quit();return 1;}
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
