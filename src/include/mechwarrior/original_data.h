#ifndef MECHWARRIOR_ORIGINAL_DATA_H
#define MECHWARRIOR_ORIGINAL_DATA_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* MW_MAIN's start values on every EXEC, from story/, data/ and code; no exe is
 * read (docs/phase2/PLAN.md 8, slice 6b). In the importers, a zero (`name=0;`
 * or a memset) followed by a bare offset in a comment is a value the exe holds
 * as zero at that offset (of 1090, or of 28BE for the music): it is set, not
 * read (6a). A constant followed by an offset is set from native_start.h. */
bool MwImportCampaignInitialData(void);
bool MwImportCampaignMusicData(void);
/* The EGA overlay's start state, set without MW_EGA.EXE (slice 6e,
 * EgaLoadStartState). */
bool MwHostLoadEgaInitialData(void);
#endif
