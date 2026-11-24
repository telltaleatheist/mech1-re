#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_adlib.h"
#include "mechwarrior/combat_mpu.h"
#include <stdlib.h>

enum { MPU_WRITE_BUSY=0x40,MPU_INPUT_EMPTY=0x80,MPU_ACK=0xFE,
    MPU_RESET=0xFF,MPU_VERSION=0xAC,MPU_PROBE_READY_POLLS=0x1F40,
    MPU_PROBE_ACK_POLLS=0xFFFF,MPU_PROBE_IRQ_POLLS=0x4E20 };

/*1ADF:0032. A zero request means all detected backends. Failed device
 * initializers do not remove bits or restore the previously stored callbacks. */
uint16_t BtechSelectAndInstallAudioBackends(uint16_t requestedBackendMask)
{
    if (!requestedBackendMask) requestedBackendMask=UINT16_MAX;
    requestedBackendMask&=BtechProbeAudioBackendMask();
    if (requestedBackendMask&(COMBAT_SOUND_SPEAKER|COMBAT_SOUND_TANDY))
        audioMusicCallback=BtechDispatchToneMidiMessage;
    if ((requestedBackendMask&COMBAT_SOUND_ADLIB) && BtechInitializeAdlibMusicBackend())
        audioMusicCallback=BtechDispatchAdlibMidiMessage;
    audioTickCallback=BtechTickSoftwareMusic;
    if ((requestedBackendMask&COMBAT_SOUND_MPU) && BtechInitializeRolandMpuBackend())
    {audioMusicCallback=BtechAudioMusicNoOp;audioTickCallback=BtechAudioTimerNoOp;}
    selectedAudioBackendMask=requestedBackendMask;
    return requestedBackendMask;
}

/*1ADF:00BE. Probe all devices, even when the eventual request is speaker-only. */
uint16_t BtechProbeAudioBackendMask(void)
{
    uint16_t available=0;
    if (!BtechProbeAdlibTimerStatus()) available|=COMBAT_SOUND_ADLIB;
    if (BtechProbeMpuWithTemporaryInterrupt()) available|=COMBAT_SOUND_MPU;
    available|=BtechProbePlatformAudioMask();
    return available;
}

/*1ADF:109D. Non-ACK input consumes a second byte without dispatch. The
 * original's CLI around the command and STI after it are gone (H4c). */
uint16_t BtechSendMpuProbeCommandAndWaitAck(uint8_t command)
{
    if (!btechHostReadPort || !btechHostWritePort) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostReadPort || !btechHostWritePort");
    uint16_t statusPort=mpuProbeStatusPort;
    for (uint16_t ready=MPU_PROBE_READY_POLLS;ready;--ready)
    {
        if (btechHostReadPort(statusPort)&MPU_WRITE_BUSY) continue;
        btechHostWritePort(mpuProbeCommandPort,command);
        for (uint16_t acknowledgement=MPU_PROBE_ACK_POLLS;acknowledgement;--acknowledgement)
        {
            if (btechHostReadPort(mpuProbeStatusPort)&MPU_INPUT_EMPTY) continue;
            uint8_t response=btechHostReadPort(mpuProbeDataPort);
            if (response==MPU_ACK)
            {
                return ((uint16_t)command<<8)|response;
            }
            btechHostReadPort(mpuProbeDataPort);
        }
        return 0;
    }
    return 0;
}

/*1ADF:1010. Its STI, saved IF and restore are gone (H4c); the version
 * command's IRQ is delivered by the poll below. A missing input IRQ does not turn an
 * acknowledged version command into failure. Final PIC write masks IRQ2. */
uint16_t BtechProbeMpuWithTemporaryInterrupt(void)
{
    BtechSendMpuProbeCommandAndWaitAck(MPU_RESET);
    uint16_t response=BtechSendMpuProbeCommandAndWaitAck(MPU_RESET);
    if (!(uint8_t)response) return 0;
    btechHostReadPort(mpuProbeDataPort);
    if (!btechHostGetMpuIrqOwner || !btechHostSetMpuIrqOwner || !btechHostEnableDeviceIrqs || !btechHostDisableDeviceIrqs) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostGetMpuIrqOwner || !btechHostSetMpuIrqOwner || !btechHostEnableDeviceIrqs || !btechHostDisableDeviceIrqs");
    mpuProbePreviousOwner=btechHostGetMpuIrqOwner();
    btechHostSetMpuIrqOwner(MwMpuIrqOwner::CombatProbe);
    btechHostEnableDeviceIrqs(MwDeviceIrq::Mpu);
    mpuProbeReceivedByteCount=0;
    response=BtechSendMpuProbeCommandAndWaitAck(MPU_VERSION);
    if (response)
        for (uint16_t remaining=MPU_PROBE_IRQ_POLLS;remaining;--remaining)
        {
            /* Interrupt delivery only; the original loop reads this WORD live. */
            if (btechHostPollInterrupts) btechHostPollInterrupts();
            if (mpuProbeReceivedByteCount) break;
        }
    btechHostDisableDeviceIrqs(MwDeviceIrq::Mpu);
    btechHostSetMpuIrqOwner(mpuProbePreviousOwner);
    return (uint8_t)response;
}

/*1ADF:1082. Hardware adapter must deliver this version only with DS304E.
 * The original neither saves DS nor selects its own data segment. */
void BtechMpuProbeInterrupt(void)
{
    if (!btechHostReadPort) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostReadPort");
    if (!(btechHostReadPort(mpuProbeStatusPort)&MPU_INPUT_EMPTY))
    {
        btechHostReadPort(mpuProbeDataPort);
        ++mpuProbeReceivedByteCount;
    } /* Its EOI is gone (H4c). */
}
