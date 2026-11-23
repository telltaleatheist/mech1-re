#include "SDLBackend.h"
#include "mechwarrior/audio_devices.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_mpu.h"
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Combat speed (docs/timing/PLAN.md, docs/CPU_TIMING_PROFILE.md): 60 ms per
 * tick at 1x, about 2.1 times the old cycle model's live-cockpit 127 ms. Keep
 * the shipped MechWarrior.ini consistent with it. */
enum { HOST_DEFAULT_COMBAT_TICK_MS=60,HOST_MINIMUM_TICK_NS=10000000,HOST_IDLE_WAIT_NS=100000,
    HOST_NS_PER_MILLISECOND=1000000,HOST_NS_PER_SECOND=1000000000,
    HOST_MS_PER_SECOND=1000,HOST_SECONDS_PER_MINUTE=60,HOST_SECONDS_PER_HOUR=3600 };
static const uint64_t HOST_SPEED_NOTICE_NS=UINT64_C(1500000000);
uint32_t mwSdlCombatTickMs=HOST_DEFAULT_COMBAT_TICK_MS;
uint8_t mwSdlCombatSpeed=MW_COMBAT_SPEED_MIN;
static uint64_t speedNoticeUntil;
static bool virtualClock;
static uint64_t virtualNanoseconds;
bool MwSdlVirtualClock(void) {return virtualClock;}
uint64_t MwSdlNowNanoseconds(void) {return virtualClock?virtualNanoseconds:SDL_GetTicksNS();}
/* A host wait: real sleep, or under the test clock the same time spent at once. */
void MwSdlSpendNanoseconds(uint64_t nanoseconds)
{if (virtualClock) virtualNanoseconds+=nanoseconds;else SDL_DelayNS(nanoseconds);}
static bool combatTickActive;
static uint64_t combatTickStarted;

void MwSdlResetCombatTick(void) {combatTickActive=false;}
uint64_t MwSdlCombatTickNanoseconds(void)
{
    uint64_t tick=(uint64_t)mwSdlCombatTickMs*HOST_NS_PER_MILLISECOND/mwSdlCombatSpeed;
    return tick<HOST_MINIMUM_TICK_NS?HOST_MINIMUM_TICK_NS:tick;
}
/* The player's speed keys (main-row - and =/+, combat only). The step is
 * remembered in the player's MechWarrior.ini and shown for a moment. */
void MwSdlStepCombatSpeed(int direction)
{
    int speed=(int)mwSdlCombatSpeed+direction;
    if (speed<MW_COMBAT_SPEED_MIN) speed=MW_COMBAT_SPEED_MIN;
    if (speed>MW_COMBAT_SPEED_MAX) speed=MW_COMBAT_SPEED_MAX;
    if (speed!=mwSdlCombatSpeed) {mwSdlCombatSpeed=(uint8_t)speed;MwSdlSaveUserSetting("combat_speed",(unsigned)speed);}
    fprintf(stderr,"Combat speed %ux: %.1f ms per tick\n",(unsigned)mwSdlCombatSpeed,
        (double)MwSdlCombatTickNanoseconds()/(double)HOST_NS_PER_MILLISECOND);
    speedNoticeUntil=SDL_GetTicksNS()+HOST_SPEED_NOTICE_NS; /* Real time, also under the test clock. */
    MwSdlPresentVideo(true);
}
bool MwSdlCombatSpeedNotice(char *text,size_t size)
{
    if (!speedNoticeUntil || SDL_GetTicksNS()>=speedNoticeUntil) return false;
    SDL_snprintf(text,size,"SPEED %ux",(unsigned)mwSdlCombatSpeed);
    return true;
}
static void WaitDeadline(uint64_t deadline)
{
    while (MwSdlNowNanoseconds()<deadline)
    {MwSdlPollEvents();MwSdlPumpTimer();MwSdlSpendNanoseconds(HOST_IDLE_WAIT_NS);}
}
/* Each combat tick (0810:0019's loop) lasts MwSdlCombatTickNanoseconds from
 * its start, however much it drew and whatever the host. */
void MwSdlPaceCombatTick(void)
{
    if (combatTickActive) WaitDeadline(combatTickStarted+MwSdlCombatTickNanoseconds());
    /* No accumulated catch-up debt after loading, a native wait or a slow
     * host. Explicit original delays already consumed their wall time. */
    combatTickStarted=MwSdlNowNanoseconds();combatTickActive=true;
    MwSdlUpdateSupportPilot(); /* Optional host input at the existing loop boundary. */
}

static MwMusicTimerHandler irq0;
/* The original's interrupt flag is gone (H4c): outside a handler the host
 * delivers only at its own points (the pumps, the key reads' due work), and a
 * handler runs with nothing nested unless it allows nested ticks, as the
 * campaign timer's coarse callback does (the original's STI there). */
static bool campaignInterruptsInstalled,inInterrupt,nestingAllowed;
static uint64_t startNanoseconds;
static double hardwareTime,nextIrq;
static uint32_t timerPeriod,biosTicks;
static uint8_t picMask;

static void BiosTimerInterrupt(void)
{
    ++biosTicks;
    if (biosTicks>=BIOS_TICKS_PER_DAY) biosTicks=0;
    if (campaignInterruptsInstalled) MwCampaignTimerTick();
}
void MwSdlAllowNestedTicks(void)
{
    if (!inInterrupt) MwSdlFail("Nested ticks allowed outside a timer handler");
    nestingAllowed=true;
}
/* A handler as the original's IRQ ran it: nothing nested on entry, and the
 * interrupted state back at its IRET. */
template<class Handler> static void RunHandler(Handler handler)
{
    bool wasInInterrupt=inInterrupt,wasNesting=nestingAllowed;
    inInterrupt=true;nestingAllowed=false;
    handler();
    inInterrupt=wasInInterrupt;nestingAllowed=wasNesting;
}
MwMusicTimerHandler MwSdlReadTimerHandler(void) {return irq0;}
void MwSdlWriteTimerHandler(MwMusicTimerHandler handler) {irq0=handler;}
uint32_t MwSdlBiosTicksValue(void) {return biosTicks;}
/* Combat's key reads run the host's due work first (H4c), where the CLI's
 * pump used to: outside an interrupt, with interrupts on. Only the due work:
 * a read takes no time; combat's waits take it in MwSdlWaitIdle. */
static bool pumpIdles=true,idleOwed;
void MwSdlRunDue(void)
{
    idleOwed=false;
    if (inInterrupt) return;
    pumpIdles=false;MwSdlPump();pumpIdles=true;
}
/* The end of a pass of one of combat's waits for a key (H4c): the host's idle
 * step, if the pass's key read found nothing due, as the campaign's waits idle. */
void MwSdlWaitIdle(void)
{
    if (idleOwed) MwSdlSpendNanoseconds(HOST_IDLE_WAIT_NS);
    idleOwed=false;
}
double MwSdlHardwareTime(void) {return hardwareTime;}
double MwSdlTimerLagMilliseconds(void)
{
    double elapsed=(double)(MwSdlNowNanoseconds()-startNanoseconds)/(double)HOST_NS_PER_MILLISECOND;
    return elapsed-hardwareTime*(double)HOST_MS_PER_SECOND/(double)HOST_PIT_CLOCK_HZ;
}
/* Sound-port event log (MECHWARRIOR_AUDIO_LOG=FILE): every access to the
 * AdLib and MPU ports, one JSON array per line:
 * [PIT cycle, "w" or "r", port, value]. Reads log the value returned. The
 * device lines come first. Exact under the test clock (docs/phase5). */
static FILE *audioLog;
static void AudioLogOpen(void)
{
    const char *path=getenv("MECHWARRIOR_AUDIO_LOG");
    if (audioLog || !path) return;
    audioLog=fopen(path,"w");
    if (!audioLog) MwSdlFail("Open the audio event log");
    static const char *const devices[]={"speaker","mt32","adlib"};
    fprintf(audioLog,"{\"device\": \"%s\", \"clock\": \"%s\"}\n",devices[mwSdlSoundSystem],
        virtualClock?"virtual":"wall");
}
static bool AudioPort(uint16_t port)
{
    return port==ADLIB_INDEX_PORT || port==ADLIB_DATA_PORT || port==MPU_DATA_PORT || port==MPU_STATUS_COMMAND_PORT
        || port==0 || port==1;
}
static void AudioLogEvent(char kind,uint16_t port,uint8_t value)
{
    if (audioLog && AudioPort(port))
        fprintf(audioLog,"[%llu, \"%c\", %u, %u]\n",(unsigned long long)hardwareTime,kind,port,value);
}
void MwSdlAudioLogTone(double frequency)
{
    if (audioLog) fprintf(audioLog,"[%llu, \"tone\", %.3f]\n",(unsigned long long)hardwareTime,frequency);
}
void MwSdlAudioLogSnap(const char *name)
{
    if (audioLog) {fprintf(audioLog,"[%llu, \"snap\", \"%s\"]\n",(unsigned long long)hardwareTime,name);fflush(audioLog);}
}
void MwSdlWriteDevicePort(uint16_t port,uint8_t value)
{
    AudioLogEvent('w',port,value);
    /* Device writes occur at the delivered hardware time, including inside
     * an IRQ. No game state is ever touched by an SDL audio thread. */
    MwSdlAdvanceAudio(hardwareTime);
    if (port==PIC_COMMAND_PORT && value==PIC_END_OF_INTERRUPT) return;
    if (port==PIC_MASK_PORT) {picMask=value;return;}
    if (port==MPU_DATA_PORT || port==MPU_STATUS_COMMAND_PORT)
    {
        if (mwSdlSoundSystem==MW_SOUND_MT32) MwMpuWritePort(port,value);
        return;
    }
    if (port==ADLIB_INDEX_PORT || port==ADLIB_DATA_PORT)
    {
        if (mwSdlSoundSystem==MW_SOUND_ADLIB)
            MwAdlibWritePort(port==ADLIB_DATA_PORT,value,hardwareTime*1000000.0/(double)HOST_PIT_CLOCK_HZ);
        return;
    }
    /* Native rhythm note-off can retain DX=0 from the VLQ reader. Those writes
     * reach unused DMA channel 0 ports, not the OPL card; never redirect them. */
    if (port==0 || port==1) return;
    MwSdlFail("Unsupported hardware write"); /* The PIT and port 61h are gone: tones (S5, S6), the timer period (H4b). */
}
/* The timer's period (H4b), where the games programmed PIT channel 0: in the
 * PIT's clock units, so the interrupts fall where they did; 0 is the PIT's
 * 65536. The next interrupt is one period on, as when the count was loaded. */
void MwSdlSetTimerPeriod(uint32_t pitCycles)
{
    timerPeriod=pitCycles&&pitCycles<PIT_COUNTER_MODULUS?pitCycles:PIT_COUNTER_MODULUS;
    nextIrq=hardwareTime+timerPeriod;
}
static uint8_t ReadDevicePort(uint16_t port);
uint8_t MwSdlReadDevicePort(uint16_t port)
{
    uint8_t value=ReadDevicePort(port);
    AudioLogEvent('r',port,value);
    return value;
}
static uint8_t ReadDevicePort(uint16_t port)
{
    if (port==PIC_MASK_PORT) return picMask;
    if (port==MPU_DATA_PORT || port==MPU_STATUS_COMMAND_PORT)
        return mwSdlSoundSystem==MW_SOUND_MT32?MwMpuReadPort(port):UINT8_MAX;
    if (port==ADLIB_INDEX_PORT)
        return mwSdlSoundSystem==MW_SOUND_ADLIB
            ?MwAdlibReadStatus(hardwareTime*1000000.0/(double)HOST_PIT_CLOCK_HZ):UINT8_MAX;
    if (port!=0 && port!=1) MwSdlFail("Unsupported hardware read");
    return 0; /* Only ignored calibrated-delay reads here. */
}
void MwSdlInitializeTimer(void)
{
    MwSdlResetCombatTick();
    time_t now=time(NULL);struct tm localTime;
#if defined(_MSC_VER)
    localtime_s(&localTime,&now);
#else
    localTime=*localtime(&now);
#endif
    uint32_t seconds=localTime.tm_hour*HOST_SECONDS_PER_HOUR+localTime.tm_min*HOST_SECONDS_PER_MINUTE+localTime.tm_sec;
    biosTicks=(uint32_t)((double)seconds*(double)HOST_PIT_CLOCK_HZ/(double)PIT_COUNTER_MODULUS);
    virtualClock=getenv("MECHWARRIOR_DEBUG_VIRTUAL_CLOCK")!=NULL;virtualNanoseconds=0;
    AudioLogOpen();
    startNanoseconds=MwSdlNowNanoseconds();hardwareTime=0;timerPeriod=PIT_COUNTER_MODULUS;
    nextIrq=timerPeriod;irq0=BiosTimerInterrupt;
    picMask=0;
    campaignInterruptsInstalled=inInterrupt=nestingAllowed=false;
    MwMpuReset();
    mwMusicHardware=MwMusicHardwareCalls{MwSdlAllowNestedTicks,MwSdlReadTimerHandler,MwSdlWriteTimerHandler,MwSdlWriteDevicePort,MwSdlReadDevicePort,
        MwSdlPumpTimer,btechHostGetMpuIrqOwner,btechHostSetMpuIrqOwner,MwSdlPlayTone,MwSdlSetTimerPeriod};
    /* Same physical PIT/speaker, different original sequencer. No backend
     * selection or invented combat callback is installed at this boundary. */
    btechHostWritePort=MwSdlWriteDevicePort;btechHostReadPort=MwSdlReadDevicePort;
}
void MwSdlPumpTimer(void)
{
#if !defined(NDEBUG)
    /* H4c removed the interrupt flag on a probe of every trace and event
     * script: no pump ran inside a handler that hadn't allowed nested ticks,
     * nor with the flag off outside one. A pump that does is a change the
     * removal didn't cover; it must be looked at, not returned from. */
    if (inInterrupt && !nestingAllowed) MwSdlFail("Pump inside a timer handler that didn't allow nested ticks");
#endif
    if (inInterrupt && !nestingAllowed) return;
    if (!inInterrupt && mwSdlCombatActive && mwSdlSoundSystem==MW_SOUND_MT32
        && !(picMask&BTECH_MPU_IRQ_MASK) && !(MwMpuReadPort(MPU_STATUS_COMMAND_PORT)&BTECH_MPU_INPUT_EMPTY))
        RunHandler(MwSdlDeliverCombatMpuInterrupt);
    if (!inInterrupt && !(picMask&PIC_KEYBOARD_IRQ_MASK) && MwSdlCombatKeyboardPending())
        RunHandler(MwSdlDeliverCombatKeyEvent);
    double target=(double)(MwSdlNowNanoseconds()-startNanoseconds)*(double)HOST_PIT_CLOCK_HZ/(double)HOST_NS_PER_SECOND;
    for (;;)
    {
        double mpuEvent=mwSdlCombatActive && mwSdlSoundSystem==MW_SOUND_MT32?MwMpuNextEventTime():DBL_MAX;
        double timerEvent=(picMask&PIC_TIMER_IRQ_MASK)?DBL_MAX:nextIrq;
        if (mpuEvent<=target && mpuEvent<timerEvent)
        {
            hardwareTime=mpuEvent;MwSdlAdvanceAudio(hardwareTime);MwMpuAdvance(hardwareTime);
            if (!(picMask&BTECH_MPU_IRQ_MASK) && !(MwMpuReadPort(MPU_STATUS_COMMAND_PORT)&BTECH_MPU_INPUT_EMPTY))
                RunHandler(MwSdlDeliverCombatMpuInterrupt);
            continue;
        }
        if (timerEvent>target) break;
        /* BTECH's installed scheduler must service every elapsed PIT edge,
         * independently of presentation/poll frequency. Keep each original
         * callback and its timestamp; do not batch its game-state writes.
         * Campaign1610 waits for exact tick equality, so retain one BIOS tick
         * per caller observation while catching up intervening fast edges. */
        bool combatScheduler=irq0==BtechTimerSchedulerInterrupt;
        bool campaignScheduler=irq0==MwCampaignTimerInterrupt;
        MwMusicTimerHandler dispatchedHandler=irq0;
        uint32_t biosTicksBefore=biosTicks;
        hardwareTime=nextIrq;nextIrq+=timerPeriod;
        MwSdlAdvanceAudio(hardwareTime);
        if (!irq0) MwSdlFail("Unmapped IRQ0 handler");
        RunHandler(irq0);
        /* MPU slot8 runs much faster than the speaker. Catch up those hardware
         * edges, but expose at most one BIOS tick to campaign1610's equality
         * wait. Stopping after every fast edge would slow MIDI with poll rate. */
        if (irq0!=dispatchedHandler || (!combatScheduler && (!campaignScheduler || biosTicks!=biosTicksBefore))) return;
    }
    if (nextIrq>target)
    {
        hardwareTime=target;MwSdlAdvanceAudio(hardwareTime);
        if (pumpIdles) MwSdlSpendNanoseconds(HOST_IDLE_WAIT_NS); /* Host idle only; no synthetic game ticks. */
        else idleOwed=true; /* A key read's: a wait takes it at the pass's end (MwSdlWaitIdle). */
    }
}
void MwSdlInstallCampaignInterrupts(void) {campaignInterruptsInstalled=true;}
void MwSdlRestoreCampaignInterrupts(void) {campaignInterruptsInstalled=false;}
uint32_t MwSdlReadBiosTicks(void)
{
    MwSdlPump();
    uint32_t pinned;
    return MwSdlDebugPinnedBiosTicks(&pinned)?pinned:biosTicks;
}
