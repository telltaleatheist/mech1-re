#include "mechwarrior/combat_sound.h"

/*1ADF:0032. A zero request means all detected backends. Combat's sound is the
 * speaker's tones (2134) whatever the launcher's letter: the AdLib and MT-32
 * drivers (21CA, 2176) and their probes left with S8. */
uint16_t BtechSelectAndInstallAudioBackends(uint16_t requestedBackendMask)
{
    if (!requestedBackendMask) requestedBackendMask=UINT16_MAX;
    requestedBackendMask&=BtechProbeAudioBackendMask();
    if (requestedBackendMask&(COMBAT_SOUND_SPEAKER|COMBAT_SOUND_TANDY))
        audioMusicCallback=BtechDispatchToneMidiMessage;
    audioTickCallback=BtechTickSoftwareMusic;
    selectedAudioBackendMask=requestedBackendMask;
    return requestedBackendMask;
}

/*1ADF:00BE. Its AdLib and MPU probes (10EC, 1010) left with S8. */
uint16_t BtechProbeAudioBackendMask(void)
{
    return BtechProbePlatformAudioMask();
}
