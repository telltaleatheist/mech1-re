#ifndef MECHWARRIOR_ORIGINAL_DATA_H
#define MECHWARRIOR_ORIGINAL_DATA_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Installation-file decoding, not native game procedures or a live DS image.
 * Input storage is borrowed only for the duration of each call. In the
 * importers, a zero (`name=0;` or a memset) followed by a bare offset in a
 * comment is a value the exe holds as zero at that offset (of 1090, or of
 * 28BE for the music): it is set, not read (docs/phase2/PLAN.md 8, 6a). */
bool MwImportCampaignInitialData(const uint8_t *data, size_t byteCount);
bool MwHostLoadCampaignInitialData(const char *executablePath);
bool MwHostLoadEgaInitialData(const char *executablePath);
bool MwImportCampaignMusicData(const uint8_t *data,size_t byteCount);
bool MwHostLoadCampaignMusicData(const char *executablePath);
#endif
