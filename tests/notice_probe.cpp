// The program's notices (src/SDL/SDLNotice.h) as a process of its own, for tests/notice_test.py:
//   notice_probe attended|hidden failsafe|fail|refuse [log file]
//   notice_probe hidden lines <log file>: both streams into the log, in turn
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "SDLBackend.h"
#include "SDLNotice.h"
#include "mechwarrior/port_diagnostics.h"

int main(int argc,char **argv)
{
    if (argc<3) return 99;
#if !defined(_MSC_VER)
    /* A failsafe's stop is an abort(); the probe ends with 3 instead, so the system writes no crash report for
     * it. On Windows the program's own handler (SDLNotice.cpp) does the same, and is what the test checks. */
    std::signal(SIGABRT,[](int){std::_Exit(3);});
#endif
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
