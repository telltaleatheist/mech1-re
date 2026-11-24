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
#include "SDLNotice.h"
#include "SDLSetup.h"
#include "mechwarrior/problem.h"
#include <string>
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
    if (!mwSdlCombatActive || now<nextDiagnosticFrame || !combatWorld.playerUnitBinding
        || !combatWorld.playerUnitBinding->positionedLeftLeg) return;
    nextDiagnosticFrame=now+1000;
    const BtechPooledWorldObject *body=combatWorld.playerUnitBinding->positionedLeftLeg;
    fprintf(stderr,"Combat time=%llu tick=%u position=%ld,%ld step=%d pose=%u/%u gait=%u sound=%u timerLagMs=%.2f audioQueueMs=%u tickMs=%.1f speed=%u lateAudioFrames=%llu underrunFrames=%llu\n",
        (unsigned long long)now,combatStartup.combatTickCounter,
        (long)(int32_t)body->object.prefix.position.positionA,
        (long)(int32_t)body->object.prefix.position.positionB,
        body->forwardStepScale,combatPose.playerPoseSequence,combatPose.playerPoseFrame,
        combatPose.playerPoseDistanceAccumulator,combatImport.combatSoundEnabled,
        MwSdlTimerLagMilliseconds(),MwSdlQueuedAudioMilliseconds(),
        (double)MwSdlCombatTickNanoseconds()/1e6,(unsigned)mwSdlCombatSpeed,(unsigned long long)MwSdlLateAudioFrames(),
        (unsigned long long)MwSdlAudioUnderrunFrames());
    for (uint16_t slot=1;slot<COMBAT_MECH_SLOTS;++slot)
    {
        const BtechPooledWorldObject *other=combatMechs.unitBindings[slot].positionedLeftLeg;
        if (!combatMechs.unitBindings[slot].mech || !other) continue;
        fprintf(stderr,"AI slot=%u position=%ld,%ld step=%d\n",slot,
            (long)(int32_t)other->object.prefix.position.positionA,
            (long)(int32_t)other->object.prefix.position.positionB,
            other->forwardStepScale);
    }
    fflush(stderr);
}
static bool JoinWriteDirectory(const char *name,char *path,size_t size)
{
    char directory[4096];
    if (!MwSdlResolveWriteDirectory(directory,sizeof(directory))) return false;
    size_t length=strlen(directory);
    const char *separator=length && (directory[length-1]=='/' || directory[length-1]=='\\')?"":"/";
    return snprintf(path,size,"%s%s%s",directory,separator,name)<(int)size;
}
/* Why the last step failed: the problem noted, else SDL's words. */
static std::string Because(void)
{
    const char *problem=MwLastProblem();
    if (!*problem) problem=SDL_GetError();
    return problem && *problem?problem:"the log says why.";
}
static int Refuse(int code,const std::string &why) {MwSdlRefuseStart(why);return code;}
int main(int argc,char **argv)
{
    MwSdlPrepareNotices(!MwSetupUnattended() && !MwSdlScriptedRun());
    /* A run with no player (a test's, a hidden one) is a background app on
     * macOS: SDL then never makes it a foreground app or activates it, so it
     * can't take the keyboard from whoever is at the machine. It comes before
     * every SDL_Init, the setup screen's too. */
    if (MwSetupUnattended()) SDL_SetHint(SDL_HINT_MAC_BACKGROUND_APP,"1");
    const char *pilotDirectory=NULL;
    bool badData=false;
    /* --data DIR, anywhere: where our data/ folder is. */
    for (int index=1;index<argc;++index)
    {
        if (strcmp(argv[index],"--data")) continue;
        if (index+1>=argc || !MwSdlSetDataDirectory(argv[index+1])) {badData=true;break;}
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
    if (help || setupOnly) MwSdlPrepareNotices(false); /* Their answer is what they print. */
    if (!help) MwSetupSettleUserFolder();
    /* Not for a start that is refused its arguments: too many, or an unknown option. */
    if (!help && argc<=3 && (argc==1 || argv[1][0]!='-')) MwSetupRecordProgram();
    if (!help && !setupOnly)
    {
        /* No console when double-clicked: the log goes beside the saves. */
        char logPath[4200];
        if (JoinWriteDirectory("mechwarrior.log",logPath,sizeof(logPath))) MwSdlOpenLog(logPath,argc==1);
    }
    if (badData) return Refuse(2,"--data needs a folder.");
    if (help || argc>3)
    {const char *name="mw1r";  /* the shipped program's name (an AppImage starts it as AppRun) */
    fprintf(help?stdout:stderr,"Usage: %s [<game folder> [existing capture directory]] [--support-pilot DIRECTORY | --no-support-pilot] [--data DIRECTORY]\n"
        "       %s --status | --setup\n"
        "Without a game folder: the one remembered in the player's MechWarrior.ini or the player's own copy, else the\n"
        "setup (pick your copy, its folder or the archive it came in: it is copied beside the saves); the log and F12\n"
        "captures then go beside the saves. --status prints what is found as JSON; --setup only sets the game up.\n"
        "EGA; music= in MechWarrior.ini; arrows and Enter; Ctrl+Escape quits; F12 captures; F11 stops Support pilot;\n"
        "- and + change the combat speed.\n",name,name);
    return help?0:Refuse(2,"It was started with more arguments than it knows.");}
    SDL_SetMainReady();
    if(!MwSdlConfigureSupportPilot(pilotDirectory))
        return Refuse(2,"Cannot open Support pilot directory.");
    const char *timingPath=getenv("MECHWARRIOR_TIMING_CONFIG");
    bool optionalTiming=timingPath==NULL;
    char defaultTimingPath[4096];
    if (!timingPath)
    {
        const char *base=SDL_GetBasePath();
        if (!base || snprintf(defaultTimingPath,sizeof(defaultTimingPath),"%sMechWarrior.ini",base)>=(int)sizeof(defaultTimingPath))
            return Refuse(2,"The program's own folder can't be read.");
        timingPath=defaultTimingPath;
    }
    if (!MwSdlLoadTimingConfiguration(timingPath,optionalTiming))
        return Refuse(2,std::string("Invalid MechWarrior configuration: ")+timingPath+" (combat tick, speed or music).");
    if (!MwSdlLoadUserConfiguration())
        return Refuse(2,"Invalid player configuration: MechWarrior.ini in the save directory (combat tick, speed or music).");
    printf("Combat tick: %u ms at 1x, speed %ux (- and + in combat); PIT/audio keep hardware rate.\n",
        mwSdlCombatTickMs,(unsigned)mwSdlCombatSpeed);
    fflush(stdout); /* Keep startup information live when the launcher logs it. */
    char installedFolder[4096],captures[4200];
    if (installed)
    {
        /* F12 captures go beside the saves too. */
        if (!MwSdlResolveWriteDirectory(captures,sizeof(captures))) return Refuse(1,"The folder for saves can't be made: "+Because());
        int ready=MwSetupResolveGame(setupOnly,installedFolder,sizeof(installedFolder));
        if (setupOnly || ready)
        {
            /* The setup has printed why; 1 is the player's own cancel. */
            const std::string why=Because();
            SDL_Quit();
            if (setupOnly || ready==1) return setupOnly?ready:0;
            return Refuse(1,"The setup failed: "+why);
        }
        if (!MwSdlInitialize(installedFolder,captures))
        {
            const std::string why="MechWarrior could not start from "+std::string(installedFolder)+": "+Because();
            SDL_Quit();return Refuse(1,why);
        }
    }
    else if (!MwSdlInitialize(argv[1],argc==3?argv[2]:"."))
        return Refuse(1,"Cannot initialize the private installation/SDL: "+Because());
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
