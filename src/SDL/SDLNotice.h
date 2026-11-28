/* What the player is told when the program can't start or has to stop. On
 * Windows a package's program has no console, so it is a message box as well as
 * the log; elsewhere it is stderr alone. */
#pragma once
#include <string>

/* Before anything can fail. `attended`: a player is at this run (not hidden, not
 * a scripted one); without one nothing is ever shown. On Windows (MSVC) an abort
 * then prints its stack and shows its reason. */
void MwSdlPrepareNotices(bool attended);
/* No player, or a run driven by a test: the debug control, the virtual clock, a scripted pick. */
bool MwSdlScriptedRun(void);
/* stdout and stderr to the log beside the saves: always for a start without a
 * game folder, else only where they would go nowhere (a Windows program with no
 * console and no redirection). */
void MwSdlOpenLog(const std::string &path,bool always);
/* The log MwSdlOpenLog opened; "" when the output goes where it went. */
const std::string &MwSdlLogPath(void);
/* Prints `why` to stderr and shows it, "MechWarrior couldn't start". */
void MwSdlRefuseStart(const std::string &why);
/* MECHWARRIOR_NOTICE_FILE, a test's stand-in for the box: the text is added to that file instead. */
