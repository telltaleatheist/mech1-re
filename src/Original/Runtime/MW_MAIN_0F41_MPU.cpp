#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_music.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>

enum { MPU_WRITE_BUSY=0x40,MPU_INPUT_EMPTY=0x80,MPU_RESET_COMMAND=0xFF,
    MPU_UART_COMMAND=0x3F,MPU_ACK=0xFE };

/* MW_MAIN0F41:0912. LOOPNE's last-read timeout remains; its CLI/STI are gone (H4c). */
uint16_t MwProbeMpuBackend(void)
{
    uint16_t remaining=0;uint8_t status;
    do {status=mwMusicHardware.readPort(MPU_STATUS_COMMAND_PORT);--remaining;}
    while (remaining && (status&MPU_WRITE_BUSY));
    if (!remaining) goto failed;
    mwMusicHardware.writePort(MPU_STATUS_COMMAND_PORT,MPU_RESET_COMMAND);
    remaining=0;
    do {status=mwMusicHardware.readPort(MPU_STATUS_COMMAND_PORT);--remaining;}
    while (remaining && (status&MPU_WRITE_BUSY));
    if (!remaining) goto failed;
    mwMusicHardware.writePort(MPU_STATUS_COMMAND_PORT,0);
    remaining=0;
    do {status=mwMusicHardware.readPort(MPU_STATUS_COMMAND_PORT);--remaining;}
    while (remaining && (status&MPU_INPUT_EMPTY));
    if (!remaining || mwMusicHardware.readPort(MPU_DATA_PORT)!=MPU_ACK) goto failed;
    return 1;
failed:
    return 0;
}
/* MW_MAIN0F41:0946. The saved IRQ2 owner is a typed owner, not a host pointer (H4c). */
void MwInitializeMpuUartBackend(void)
{
    if (!mwMusicHardware.readMpuIrqOwner || !mwMusicHardware.writeMpuIrqOwner || !mwMusicHardware.enableDeviceIrqs) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwMusicHardware.readMpuIrqOwner || !mwMusicHardware.writeMpuIrqOwner || !mwMusicHardware.enableDeviceIrqs");
    mwMusicHardware.writePort(MPU_STATUS_COMMAND_PORT,MPU_UART_COMMAND);
    campaignMusic.mpuQueueByteCount=0; campaignMusic.mpuQueueReadIndex=campaignMusic.mpuQueueByteCount; campaignMusic.mpuQueueWriteIndex=campaignMusic.mpuQueueReadIndex; /* Each read back, as the chained assignment did. */
    campaignMusic.mpuPreviousIrqOwner=mwMusicHardware.readMpuIrqOwner();
    mwMusicHardware.writeMpuIrqOwner(MwMpuIrqOwner::CampaignReceive);
    for (uint16_t reads=10;reads;--reads) mwMusicHardware.readPort(MPU_DATA_PORT);
    mwMusicHardware.enableDeviceIrqs(MwDeviceIrq::All); /* Mask 0; its EOI is gone (H4c). */
}
/* MW_MAIN0F41:098A. Waits for IRQ0 progress, never drains bytes synchronously. */
void MwDrainAndShutdownMpuBackend(void)
{
    while (campaignMusic.mpuQueueByteCount) mwMusicHardware.waitForInterrupt();
    mwMusicHardware.writeMpuIrqOwner(campaignMusic.mpuPreviousIrqOwner);
}
/* MW_MAIN0F41:09A4. The producer's CLI/STI are gone (H4c). */
uint16_t MwQueueMpuByte(uint16_t midiWord)
{
    while (campaignMusic.mpuQueueByteCount>=MIDI_MPU_QUEUE_CAPACITY) mwMusicHardware.waitForInterrupt();
    if (campaignMusic.mpuQueueWriteIndex>=MIDI_MPU_QUEUE_CAPACITY) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mpuQueueWriteIndex>=MIDI_MPU_QUEUE_CAPACITY"); /* Unrepresented corrupt-index alias. */
    campaignMusic.mpuTransmitQueue[campaignMusic.mpuQueueWriteIndex]=(uint8_t)midiWord;
    { const uint16_t next=(uint16_t)(campaignMusic.mpuQueueWriteIndex+1); campaignMusic.mpuQueueWriteIndex=next; if (next>=MIDI_MPU_QUEUE_CAPACITY) campaignMusic.mpuQueueWriteIndex=0; }
    campaignMusic.mpuQueueByteCount=(uint16_t)(campaignMusic.mpuQueueByteCount+1);
    return 1;
}
/* MW_MAIN0F41:09D0. Exactly one byte per ready fast callback. */
void MwTickMpuTransmitQueue(void)
{
    if (!campaignMusic.mpuQueueByteCount || (mwMusicHardware.readPort(MPU_STATUS_COMMAND_PORT)&MPU_WRITE_BUSY)) return;
    if (campaignMusic.mpuQueueReadIndex>=MIDI_MPU_QUEUE_CAPACITY) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mpuQueueReadIndex>=MIDI_MPU_QUEUE_CAPACITY");
    mwMusicHardware.writePort(MPU_DATA_PORT,campaignMusic.mpuTransmitQueue[campaignMusic.mpuQueueReadIndex]);
    { const uint16_t next=(uint16_t)(campaignMusic.mpuQueueReadIndex+1); campaignMusic.mpuQueueReadIndex=next; if (next>=MIDI_MPU_QUEUE_CAPACITY) campaignMusic.mpuQueueReadIndex=0; }
    campaignMusic.mpuQueueByteCount=(uint16_t)(campaignMusic.mpuQueueByteCount-1);
}
/* MW_MAIN0F41:0A01. CPU entry/IRET is owned by the host interrupt boundary. */
void MwMpuReceiveInterrupt(void)
{
    mwMusicHardware.readPort(MPU_DATA_PORT); /* Its EOI is gone (H4c). */
}
