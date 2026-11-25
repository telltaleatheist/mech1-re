#include "SDLNotice.h"
#include "SDLBackend.h"
#include "mechwarrior/problem.h"
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>
#include <system_error>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#if defined(_MSC_VER)
#include <signal.h>
#include <dbghelp.h>
#endif

static bool playerAttends;
static std::string logPath;

#if defined(_WIN32)
/* A service's or an ssh session's window station shows nothing, and a box there
 * would wait for ever. */
static bool VisibleDesktop(void)
{
    USEROBJECTFLAGS flags{};DWORD needed=0;
    return GetUserObjectInformationW(GetProcessWindowStation(),UOI_FLAGS,&flags,sizeof(flags),&needed)
        && (flags.dwFlags&WSF_VISIBLE);
}
static std::wstring Wide(const std::string &text)
{
    const int count=MultiByteToWideChar(CP_UTF8,0,text.data(),(int)text.size(),NULL,0);
    std::wstring wide((size_t)(count>0?count:0),L'\0');
    if (count>0) MultiByteToWideChar(CP_UTF8,0,text.data(),(int)text.size(),wide.data(),count);
    return wide;
}
#endif
static void Show(std::string text)
{
    if (!playerAttends) return;
    if (!logPath.empty()) text+="\n\nThe log is "+logPath+".";
    if (const char *file=getenv("MECHWARRIOR_NOTICE_FILE"))
    {
        if (FILE *out=fopen(file,"ab")) {fprintf(out,"%s\n",text.c_str());fclose(out);}
        return;
    }
#if defined(_WIN32)
    if (VisibleDesktop())
        MessageBoxW(NULL,Wide(text).c_str(),L"MechWarrior",MB_OK|MB_ICONERROR|MB_TASKMODAL|MB_SETFOREGROUND|MB_TOPMOST);
#endif
}

#if defined(_MSC_VER)
/* Host diagnostics only. Port failsafes print their reason before SIGABRT; add
 * a stack trace to the log. PDBs improve frame names, but explicit
 * reason/function/source diagnostics do not depend on symbols. */
static void ReportFatalAbort(int signalNumber)
{
    (void)signalNumber;
    HANDLE process=GetCurrentProcess();
    SymSetOptions(SYMOPT_LOAD_LINES|SYMOPT_UNDNAME|SYMOPT_DEFERRED_LOADS);
    BOOL symbols=SymInitialize(process,NULL,TRUE);
    void *frames[40];USHORT count=CaptureStackBackTrace(0,40,frames,NULL);
    fprintf(stderr,"MechWarrior fatal abort (combat active=%u).\n",mwSdlCombatActive?1:0);
    for (USHORT index=0;index<count;++index)
    {
        IMAGEHLP_LINE64 line={0};line.SizeOfStruct=sizeof(line);DWORD displacement;
        if (symbols && SymGetLineFromAddr64(process,(DWORD64)frames[index],&displacement,&line))
            fprintf(stderr,"%s:%lu\n",line.FileName,(unsigned long)line.LineNumber);
        else
            fprintf(stderr,"frame %u: %p\n",index,frames[index]);
    }
    fflush(stderr);
    std::string why=MwLastProblem();
    Show("MechWarrior had to stop: it met a problem it can't go on from."+(why.empty()?why:"\n\n"+why));
    _Exit(3);
}
#endif

bool MwSdlScriptedRun(void)
{
    return getenv("MECHWARRIOR_DEBUG_CONTROL_DIRECTORY") || getenv("MECHWARRIOR_DEBUG_VIRTUAL_CLOCK")
        || getenv("MECHWARRIOR_SETUP_PICK") || getenv("REKIT_SETUP_FRAMES");
}
void MwSdlPrepareNotices(bool attended)
{
    playerAttends=attended;
#if defined(_MSC_VER)
    signal(SIGABRT,ReportFatalAbort);
#endif
}
static bool OutputGoesNowhere(void)
{
#if defined(_WIN32)
    HANDLE handle=GetStdHandle(STD_ERROR_HANDLE);
    if (!handle || handle==INVALID_HANDLE_VALUE) return true;
    const DWORD type=GetFileType(handle);
    if (type==FILE_TYPE_DISK || type==FILE_TYPE_PIPE) return false;
    DWORD mode;
    return !GetConsoleMode(handle,&mode); /* A console is seen; NUL isn't. */
#else
    return false;
#endif
}
void MwSdlOpenLog(const std::string &path,bool always)
{
    if (!always && !OutputGoesNowhere()) return;
    /* A first start's save directory isn't there yet. */
    std::error_code ignored;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(),ignored);
    /* freopen closes a stream it can't open: see first that it can. This empties the log too. */
    FILE *log=fopen(path.c_str(),"w");
    if (!log) return;
    fclose(log);
    /* Both append: each has its own place in the file, and one that only writes would write over the other's lines. */
    if (!freopen(path.c_str(),"a",stdout) || !freopen(path.c_str(),"a",stderr)) {}
    setvbuf(stderr,NULL,_IONBF,0);
    logPath=path;
}
void MwSdlRefuseStart(const std::string &why)
{
    fprintf(stderr,"%s\n",why.c_str());fflush(stderr);
    Show("MechWarrior couldn't start.\n\n"+why);
}
