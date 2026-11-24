#include "SDLBackend.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_sound.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Combat speed (docs/timing/PLAN.md, docs/CPU_TIMING_PROFILE.md): 60 ms per
 * tick at 1x, about 2.1 times the old cycle model's live-cockpit 127 ms. Keep
 * the shipped MechWarrior.ini consistent with it. */
enum { HOST_DEFAULT_COMBAT_TICK_MS=60,HOST_MINIMUM_TICK_NS=10000000,HOST_IDLE_WAIT_NS=100000,HOST_IDLE_MAX_NS=10000000,
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
/* The host's idle (H4d), defined with the timer below. A wait's deadline, while
 * one runs, bounds it. */
static void HostIdle(void);
static uint64_t idleDeadline;
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
    idleDeadline=deadline;
    while (MwSdlNowNanoseconds()<deadline) {MwSdlPollEvents();MwSdlPumpTimer();HostIdle();}
    idleDeadline=0;
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
/* The device IRQs passed to their owners (H4c), where the PIC mask was. */
static uint8_t enabledDeviceIrqs;
void MwSdlEnableDeviceIrqs(MwDeviceIrq irqs) {enabledDeviceIrqs|=(uint8_t)irqs;}
static bool DeviceIrqEnabled(MwDeviceIrq irq) {return enabledDeviceIrqs&(uint8_t)irq;}

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
    if (idleOwed) HostIdle();
    idleOwed=false;
}
/* The host's idle, when nothing is due (H4d). Under the real clock it sleeps
 * until the next due event: IRQ0's next tick, as the pump would deliver it. A running wait's deadline comes
 * first, and it sleeps at most HOST_IDLE_MAX_NS, so the window, the input and
 * the audio queue (2048 frames ahead at least, 43 ms) are still served. Under
 * the test clock it stays the 100 us step, so no log moves. */
static void HostIdle(void)
{
    if (virtualClock) {virtualNanoseconds+=HOST_IDLE_WAIT_NS;return;}
    uint64_t now=SDL_GetTicksNS(),wake=now+HOST_IDLE_MAX_NS;
    if (idleDeadline && idleDeadline<wake) wake=idleDeadline;
    double due=DeviceIrqEnabled(MwDeviceIrq::Timer)?nextIrq:DBL_MAX;
    double dueNanoseconds=(double)startNanoseconds+due*(double)HOST_NS_PER_SECOND/(double)HOST_PIT_CLOCK_HZ;
    if (dueNanoseconds<(double)wake) wake=(uint64_t)ceil(dueNanoseconds);
    if (wake>now) SDL_DelayNS(wake-now);
}
double MwSdlHardwareTime(void) {return hardwareTime;}
double MwSdlTimerLagMilliseconds(void)
{
    double elapsed=(double)(MwSdlNowNanoseconds()-startNanoseconds)/(double)HOST_NS_PER_MILLISECOND;
    return elapsed-hardwareTime*(double)HOST_MS_PER_SECOND/(double)HOST_PIT_CLOCK_HZ;
}
/* The sound log (MECHWARRIOR_AUDIO_LOG=FILE): each change of the tone and each
 * snap, one JSON array per line: [PIT cycle, "tone", Hz]. The device line comes
 * first. Exact under the test clock (docs/phase5). */
static FILE *audioLog;
static void AudioLogOpen(void)
{
    const char *path=getenv("MECHWARRIOR_AUDIO_LOG");
    if (audioLog || !path) return;
    audioLog=fopen(path,"w");
    if (!audioLog) MwSdlFail("Open the audio event log");
    static const char *const devices[]={"speaker","soundfont","off"};
    fprintf(audioLog,"{\"device\": \"%s\", \"clock\": \"%s\"}\n",devices[mwSdlMusic],
        virtualClock?"virtual":"wall");
}
void MwSdlAudioLogTone(double frequency)
{
    if (audioLog) fprintf(audioLog,"[%llu, \"tone\", %.3f]\n",(unsigned long long)hardwareTime,frequency);
}
void MwSdlAudioLogSnap(const char *name)
{
    if (audioLog) {fprintf(audioLog,"[%llu, \"snap\", \"%s\"]\n",(unsigned long long)hardwareTime,name);fflush(audioLog);}
}
/* The devices' ports are gone (S9): tones (S5, S6), the timer period (H4b), the
 * AdLib's and the MPU's. The hooks that still name them stop at H5. */
void MwSdlWriteDevicePort(uint16_t port,uint8_t value)
{
    (void)port;(void)value;
    MwSdlFail("Unsupported hardware write");
}
/* The timer's period (H4b), where the games programmed PIT channel 0: in the
 * PIT's clock units, so the interrupts fall where they did; 0 is the PIT's
 * 65536. The next interrupt is one period on, as when the count was loaded. */
void MwSdlSetTimerPeriod(uint32_t pitCycles)
{
    timerPeriod=pitCycles&&pitCycles<PIT_COUNTER_MODULUS?pitCycles:PIT_COUNTER_MODULUS;
    nextIrq=hardwareTime+timerPeriod;
}
uint8_t MwSdlReadDevicePort(uint16_t port)
{
    (void)port;
    MwSdlFail("Unsupported hardware read");
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
    enabledDeviceIrqs=(uint8_t)MwDeviceIrq::All;
    campaignInterruptsInstalled=inInterrupt=nestingAllowed=false;
    mwMusicHardware=MwMusicHardwareCalls{MwSdlAllowNestedTicks,MwSdlReadTimerHandler,MwSdlWriteTimerHandler,MwSdlWriteDevicePort,MwSdlReadDevicePort,
        MwSdlPumpTimer,MwSdlPlayTone,MwSdlSetTimerPeriod};
    /* Same physical PIT/speaker, different original sequencer. No backend
     * selection or invented combat callback is installed at this boundary. */
    btechHostWritePort=MwSdlWriteDevicePort;btechHostReadPort=MwSdlReadDevicePort;
    btechHostEnableDeviceIrqs=MwSdlEnableDeviceIrqs;
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
    if (!inInterrupt && DeviceIrqEnabled(MwDeviceIrq::Keyboard) && MwSdlCombatKeyboardPending())
        RunHandler(MwSdlDeliverCombatKeyEvent);
    double target=(double)(MwSdlNowNanoseconds()-startNanoseconds)*(double)HOST_PIT_CLOCK_HZ/(double)HOST_NS_PER_SECOND;
    for (;;)
    {
        double timerEvent=DeviceIrqEnabled(MwDeviceIrq::Timer)?nextIrq:DBL_MAX;
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
        if (pumpIdles) HostIdle(); /* Host idle only; no synthetic game ticks. */
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
