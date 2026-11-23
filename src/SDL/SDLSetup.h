/* The program's side of the setup (src/Setup/Setup.h is the recipe): the
 * launcher contract's --status and --setup, the first start's search, and
 * the one-time carry-over of the previews' saves. */
#pragma once
#include <stddef.h>

/* --status: one JSON line on stdout, exit 0; no window, nothing written. */
int MwSetupPrintStatus(void);
/* The player's folder settled for this start (setup::settle_user_folder: in use,
 * a test version's MicroProse one moved across, or the previews' copied once). Logged. */
void MwSetupSettleUserFolder(void);
/* The game folder a start plays from, into `folder`: the remembered one, the
 * player's own install, else the setup (the player picks a copy, a folder or
 * an archive, which is copied into the player's folder). `setupOnly`: the
 * setup whatever is found (--setup). Returns the launcher contract's exit:
 * 0 ready, 1 the player cancelled, 2 failed (with --setup the reason goes to
 * stdout as {"error": ...}). */
int MwSetupResolveGame(bool setupOnly,char *folder,size_t size);
