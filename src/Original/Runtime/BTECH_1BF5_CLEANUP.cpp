#include "mechwarrior/video_mode.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/input_hardware.h"
#include "mechwarrior/combat_platform.h"
#include <stdlib.h>

/*1BF5:00D5. Fatal DE52 calls this AFTER text restore; normal1337 does the
 * reverse. Keep the original caller ordering when those roots are linked. */
void BtechShutdownPlatformHandlers(void)
{
    BtechRestoreKeyboardAndTickVectors();
    BtechShutdownMouseDriver();
    BtechRestoreTimerScheduler();
    BtechDeactivateDivideErrorHook();
}

/*1BF5:00EA. FF means no saved adapter, not a request to force mode03. */
void BtechRestoreTextVideoMode(void)
{
    if (videoAdapter!=VIDEO_ADAPTER_UNSAVED) BtechSetBiosTextModeAndAdapterBits(videoAdapter);
}

/*1BF5:0287. Native BIOS equipment BYTE has two ordered read/modify/writes. */
void BtechSetBiosTextModeAndAdapterBits(uint16_t displayBits)
{
    if (!btechHostReadBiosByte || !btechHostWriteBiosByte) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostReadBiosByte || !btechHostWriteBiosByte");
    btechHostWriteBiosByte(BIOS_VIDEO_EQUIPMENT,btechHostReadBiosByte(BIOS_VIDEO_EQUIPMENT)&BIOS_EQUIPMENT_CLEAR_DISPLAY_BITS);
    btechHostWriteBiosByte(BIOS_VIDEO_EQUIPMENT,btechHostReadBiosByte(BIOS_VIDEO_EQUIPMENT)|(uint8_t)(displayBits<<BIOS_EQUIPMENT_DISPLAY_SHIFT));
    MwSetVideoMode(BIOS_TEXT_MODE_80_COLUMN);
}

/*1BF5:2C8D. Flag clears before discarding queued keys and restoring IVT. */
void BtechRestoreKeyboardAndTickVectors(void)
{
    if (!keyboardHandlersInstalled) return;
    keyboardHandlersInstalled=0;
    if (!btechHostSetKeyboardOwner) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostSetKeyboardOwner");
    combatKeyQueueHead=combatKeyQueueTail;
    btechHostSetKeyboardOwner(savedKeyboardOwner);
    /* INT 1Ch's restore is gone with its never-installed hook (H4c). */
}

/*1BF5:2428. INT33 reset is a hardware boundary, not a game-input callback. */
void BtechShutdownMouseDriver(void)
{
    if (!mouseDriverInstalled) return;
    mouseDriverInstalled=0;
    if (!btechHostResetMouse) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostResetMouse");
    btechHostResetMouse();
}

/*1BF5:25AB. The literal mask 28h on the active path passes the host's three
 * devices (its CLI/STI and the mask port are gone since H4c). */
void BtechRestoreTimerScheduler(void)
{
    if (!runtimeDelayActive) return;
    if (!btechHostEnableDeviceIrqs || !btechHostSetTimerOwner || !btechHostSetTimerPeriod) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostEnableDeviceIrqs || !btechHostSetTimerOwner || !btechHostSetTimerPeriod");
    btechHostSetTimerPeriod(PIT_COUNTER_MODULUS); /* The BIOS's period (H4b; was 43h, 40h 0, 40h 0). */
    btechHostEnableDeviceIrqs(MwDeviceIrq::All);
    btechHostSetTimerOwner(savedTimerOwner);
    runtimeDelayActive=0;
}

/*1BF5:1F84. The shipped26 A1 instructions READ INT00; they do not restore it.
 * Keep that native defect, not an idealized unhook: combat's divide handler
 * stays the owner. The discarded read went with the vectors (H4c). */
void BtechDeactivateDivideErrorHook(void)
{
    if (!divideErrorHookMarkedActive) return;
    divideErrorHookMarkedActive=0;
}
