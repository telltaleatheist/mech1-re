#include "SDLBackend.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/combat_installation.h"
#include <string.h>

/* Physical BIOS transport and the interrupts' owners (H4c), not another game
 * state or replacement timer. Each owner is typed: no vector table is kept.
 * IRQ0's previous owner is the handler that ran before combat installed its
 * own (the campaign's), not invented BIOS bytes. */
static uint8_t biosData[0x100];
static MwTimerOwner timerOwner;
static MwKeyboardOwner keyboardOwner;
static MwDivideOwner divideOwner;
static MwMusicTimerHandler previousTimerHandler;

/* The original read its tone data from the interrupt table here (a missing
 * ITM leaves the tone pointer at 0000:0000), so these are the bytes it found:
 * vectors 0 to 5 as DOS left them, captured in DOSBox-X before EXEC, at the
 * briefing, the first scene and after firing. Only INT 0 changes, by whether
 * combat's divide handler is installed. Not designed tone data. */
enum { DOS_TONE_ZERO_WINDOW_START=0x1F4A,DOS_TONE_ZERO_WINDOW_BYTES=0x1D4,
    DOS_TONE_VECTOR_PREFIX=6,DOS_PRINT_SCREEN_INTERRUPT=5,
    HOST_REAL_MODE_ADDRESS_MASK=0xFFFFF,HOST_BIOS_MODEL_ADDRESS=0xFFFFE,HOST_BIOS_AT_MODEL=0xFC };
static const uint32_t toneFallbackDivideVector[2]={
    UINT32_C(0xF000CA60), /* DOS's own divide handler. */
    UINT32_C(0x1D3B1F4A)}; /* Combat's, 1BF5:1F4A with the image loaded at 0956. */
static_assert(UINT32_C(0x1D3B1F4A)==((UINT32_C(0x1BF5)+HOST_DOS_COMBAT_CODE_SELECTOR-BTECH_IMAGE_LOAD_SELECTOR)<<16|0x1F4A),
    "Combat's divide handler, relocated to where DOS loaded the image");
static const uint32_t toneFallbackIretVector=UINT32_C(0x0070000E),toneFallbackPrintScreenVector=UINT32_C(0xF000FF54);
static const uint8_t dosToneZeroWindow[DOS_TONE_ZERO_WINDOW_BYTES]={0};
static uint8_t ReadToneDosByte(uint16_t offset)
{
    if (offset<DOS_TONE_VECTOR_PREFIX*4)
    {
        uint16_t interrupt=offset/4;
        uint32_t vector=!interrupt?toneFallbackDivideVector[divideOwner==MwDivideOwner::Combat]
            :interrupt<DOS_PRINT_SCREEN_INTERRUPT?toneFallbackIretVector:toneFallbackPrintScreenVector;
        return (uint8_t)(vector>>((offset&3)*8));
    }
    if (offset>=DOS_TONE_ZERO_WINDOW_START
        && (uint16_t)(offset-DOS_TONE_ZERO_WINDOW_START)<sizeof(dosToneZeroWindow))
        return dosToneZeroWindow[offset-DOS_TONE_ZERO_WINDOW_START];
    char error[128];SDL_snprintf(error,sizeof(error),"Tone fallback read at 0000:%04X outside recovered physical DOS owners",offset);
    MwSdlFail(error);
}
static MwNativeByteView BorrowToneDosMemory(uint16_t selector)
{
    if (selector) MwSdlFail("Unrecovered physical DOS selector for tone fallback");
    return MwNativeByteView{NULL,0,0,ReadToneDosByte};
}
static uint8_t ReadBiosByte(uint16_t offset)
{
    if (offset>=sizeof(biosData)) MwSdlFail("Read outside represented BIOS data");
    return biosData[offset];
}
static void WriteBiosByte(uint16_t offset,uint8_t value)
{
    if (offset>=sizeof(biosData)) MwSdlFail("Write outside represented BIOS data");
    biosData[offset]=value;
}
static void WriteBiosWord(uint16_t offset,uint16_t value)
{WriteBiosByte(offset,(uint8_t)value);WriteBiosByte(offset+1,(uint8_t)(value>>8));}
static MwTimerOwner GetTimerOwner(void) {return timerOwner;}
static void SetTimerOwner(MwTimerOwner owner)
{
    timerOwner=owner;
    MwSdlWriteTimerHandler(owner==MwTimerOwner::CombatScheduler?BtechTimerSchedulerInterrupt
        :owner==MwTimerOwner::CombatMusic?BtechMusicTimerInterrupt:previousTimerHandler);
}
/* The scheduler's chain (H4c): IRQ0's previous owner, the campaign's timer
 * handler, runs its tick inside the scheduler's, as the chained jump did. */
static void RunPreviousTimerOwner(void)
{
    if (!previousTimerHandler) MwSdlFail("No previous IRQ0 owner to run");
    previousTimerHandler();
}
static MwKeyboardOwner GetKeyboardOwner(void) {return keyboardOwner;}
static void SetKeyboardOwner(MwKeyboardOwner owner) {keyboardOwner=owner;}
static MwDivideOwner GetDivideOwner(void) {return divideOwner;}
static void SetDivideOwner(MwDivideOwner owner) {divideOwner=owner;}
static uint8_t ReadRomByte(uint16_t selector,uint16_t offset)
{
    uint32_t address=(((uint32_t)selector<<4)+offset)&HOST_REAL_MODE_ADDRESS_MASK;
    return address==HOST_BIOS_MODEL_ADDRESS?HOST_BIOS_AT_MODEL:UINT8_MAX; /* Configured AT/EGA, not PCjr/Tandy. */
}
static uint16_t QueryEgaInfo(void) {return 0;}
static void ResetMouse(void) {mwCampaignInput.resetMouse();}
void MwSdlResetCombatHardware(void)
{
    MwSdlResetCombatTick();
    previousTimerHandler=MwSdlReadTimerHandler();
    timerOwner=MwTimerOwner::Previous;keyboardOwner=MwKeyboardOwner::System;
    divideOwner=MwDivideOwner::System;
    memset(biosData,0,sizeof(biosData));
    WriteBiosWord(COMBAT_BIOS_CRTC_PORT,COMBAT_COLOUR_CRTC_INDEX_PORT);
}
void MwSdlInitializeCombatHardware(void)
{
    btechHostReadBiosByte=ReadBiosByte;btechHostWriteBiosByte=WriteBiosByte;
    btechHostWriteBiosWord=WriteBiosWord;
    btechHostGetTimerOwner=GetTimerOwner;btechHostSetTimerOwner=SetTimerOwner;
    btechHostGetKeyboardOwner=GetKeyboardOwner;btechHostSetKeyboardOwner=SetKeyboardOwner;
    btechHostGetDivideOwner=GetDivideOwner;btechHostSetDivideOwner=SetDivideOwner;
    btechHostBorrowDosMemory=BorrowToneDosMemory;
    btechHostRunPreviousTimerOwner=RunPreviousTimerOwner;
    btechHostSetTimerPeriod=MwSdlSetTimerPeriod;
    btechHostRunDue=MwSdlRunDue;btechHostWaitIdle=MwSdlWaitIdle;
    btechHostReadRomByte=ReadRomByte;btechHostQueryEgaInfo=QueryEgaInfo;
    btechHostReadJoystick=MwSdlReadJoystick;
    btechHostPollInterrupts=MwSdlPump;btechHostPlayTone=MwSdlPlayTone;btechHostResetMouse=ResetMouse;
    btechHostPaceCombatTick=MwSdlPaceCombatTick;
    btechHostNoteCombatInputPoll=MwSdlDebugNoteCombatPoll;
}
