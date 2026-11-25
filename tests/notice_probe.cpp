// The program's notices (src/SDL/SDLNotice.h) as a process of its own, for tests/notice_test.py:
//   notice_probe attended|hidden failsafe|fail|refuse [log file]
//   notice_probe hidden lines <log file>: both streams into the log, in turn
#include <cstdio>
#include <cstring>
#include "SDLBackend.h"
#include "SDLNotice.h"
#include "mechwarrior/port_diagnostics.h"

int main(int argc,char **argv)
{
    if (argc<3) return 99;
    MwSdlPrepareNotices(!std::strcmp(argv[1],"attended") && !MwSdlScriptedRun());
    if (argc>3 && !std::strcmp(argv[2],"lines"))
    {
        MwSdlOpenLog(argv[3],true);
        for (int round=0;round<3;++round)
        {
            std::printf("out %d: a line of the output, longer than the other\n",round);std::fflush(stdout);
            std::fprintf(stderr,"err %d: a line of the errors\n",round);
        }
        return 0;
    }
    if (argc>3) MwSdlOpenLog(argv[3],false);
    if (!std::strcmp(argv[2],"failsafe")) MW_PORT_FAILSAFE("the failsafe's reason");
    if (!std::strcmp(argv[2],"fail")) MwSdlFail("The host's operation");
    MwSdlRefuseStart("the refusal's reason");
    return 2;
}
