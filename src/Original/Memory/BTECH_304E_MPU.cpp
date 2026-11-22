#include "mechwarrior/combat_mpu.h"
uint16_t combatMpuDataPort,combatMpuStatusPort,combatMpuCommandPort,combatMpuReceivedBytes;
uint8_t combatMpuInstalled,combatMpuPicUnmask,combatMpuVector;
uint32_t combatMpuPreviousVector;
uint16_t combatMpuInstrumentOffset,combatMpuInstrumentSelector;
bool BtechImportMpuInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x10000) return false;
    combatMpuReceivedBytes=0; /*2B76*/
    combatMpuDataPort=MwReadNativeWord(data,0x2B78);combatMpuStatusPort=MwReadNativeWord(data,0x2B7A);
    combatMpuCommandPort=MwReadNativeWord(data,0x2B7C);combatMpuInstalled=0; /*2B7E*/
    combatMpuPicUnmask=data[0x2B7F];combatMpuVector=data[0x2B80];
    combatMpuPreviousVector=0; /*2B81*/
    combatMpuInstrumentOffset=0; /*6F74*/
    combatMpuInstrumentSelector=0; /*6F76*/
    return true;
}
