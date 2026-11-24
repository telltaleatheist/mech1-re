#include "mechwarrior/campaign.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/game_files.h"
#include "mechwarrior/original_data.h"

/* The music files' reads in MW_MAIN_081B_STARTUP.cpp, until they take a
 * GameFile as the rest do: they open, read and close one file at a time. */
static mw1o::files::GameFile musicFile;
bool MwDosOpenResourceReadWrite(const char *filename,uint16_t *handleOrError)
{musicFile=mw1o::files::open_game_file(filename);*handleOrError=0;return !musicFile;}
bool MwDosReadFile(uint8_t *buffer,uint16_t byteCount,uint16_t,uint16_t *bytesReadOrError)
{
    std::optional<uint16_t> bytesRead=musicFile.read(buffer,byteCount);
    *bytesReadOrError=bytesRead.value_or(0);return !bytesRead;
}
bool MwDosCloseFile(uint16_t) {return !musicFile.close();}

bool MwHostLoadEgaInitialData(void)
{
    /* Play never opens MW_EGA.EXE: the overlay's owners start as the exe
     * holds them, from code (slice 6e). */
    EgaLoadStartState();
    return EgaLoadStartStoryBytes();
}

