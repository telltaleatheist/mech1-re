/* The program's side of the setup (src/Setup/Setup.h is the recipe): the
 * launcher contract's --status and --setup, the first start's search, and
 * the one-time carry-over of the previews' saves. */
#pragma once
#include <stddef.h>
#include <rekit/sdl/setup_screen.h>
#include "SDLMenu.h"
#include <string>
#include <vector>

/* --status: one JSON line on stdout, exit 0; no window, nothing written. */
int MwSetupPrintStatus(void);
/* `--setup --describe | --search | --look <pick> | --install <pick> [--folder <dir>]`: the setup on the launcher's own
 * screens (the kit's rekit/sdl/setup_serve.h). JSON lines on stdout and nothing else there, the last one `done`; no
 * window, no picker. A "cancel" line on stdin stops a look or an install. MwSetupServedAsk: whether the word after
 * --setup is one of these. MwSetupServe answers; `words` are the ask and what follows it. What it installs is
 * remembered as after the setup screen. Returns the launcher contract's exit; wrong arguments get one line saying so
 * and exit 2. */
bool MwSetupServedAsk(const char *arg);
int MwSetupServe(int count,char **words);
/* No player at this run: a hidden or scripted one (MW1O_HIDDEN, an offscreen
 * or dummy video driver). No picker, message box or window that takes focus
 * opens then, and on macOS the program runs as a background app. */
bool MwSetupUnattended(void);
/* The player's folder settled for this start (setup::settle_user_folder: in use,
 * a test version's MicroProse one moved across, or the previews' copied once). Logged. */
void MwSetupSettleUserFolder(void);
/* Where this program is, written into the player's folder for the BattleTech launcher (src/Setup/ProgramRecord.h:
 * a released build's start with a player at it). After MwSetupSettleUserFolder; only at a start that plays. */
void MwSetupRecordProgram(void);
/* The game folder a start plays from, into `folder`: the remembered one, the
 * player's own install, else the setup (the player picks a copy, a folder or
 * an archive, which is copied into the player's folder). `setupOnly`: the
 * setup whatever is found. Returns the launcher contract's exit: 0 ready,
 * 1 the player cancelled, 2 failed (with --setup the reason goes to stdout as
 * {"error": ...}). */
enum MwSetupMode
{
    MW_SETUP_PLAY,      /* a start: the setup only when nothing is found */
    MW_SETUP_ONLY,      /* --setup: the setup, nothing played (the launcher's) */
    MW_SETUP_THEN_PLAY  /* --setup --then-play: the setup, then the game (the menu's CHANGE) */
};
int MwSetupResolveGame(MwSetupMode mode,char *folder,size_t size);
/* MW1's recipe for the kit's setup screen, for a fresh setup of the player's folder `user` (tests/setup_download_test.cpp). */
rekit::sdl::Game MwSetupKitGame(const std::string &user);
/* What an update backs up first, as the player's folder `user` holds it now: the saved games (*.GAM), the story and
 * the settings file `settings` ("" none). The menu's UPDATES asks it at backup time (UpdateSource::keep_now). */
std::vector<std::string> MwSetupBackupList(const std::string &user,const std::string &settings);
/* MW1's part of the kit's menu over the game (SDLMenu.h): its words, the player's folder, the log, the file check,
 * RESET SETTINGS and UPDATES. */
MwSdlMenuRecipe MwSetupMenuRecipe(void);
