/* The kit's menu over the running game (rekit/sdl/game_menu.h): Shift+Alt+F1
 * (Cmd+Shift+F1 on the Mac) at any moment, Esc back to the game. The host's
 * half is SDLMenu.cpp (the window, the picture over each frame, the events,
 * the paused clock, the way out); what the menu says and does for MW1 is the
 * program's recipe (SDLSetup.cpp). */
#pragma once
#include <rekit/sdl/game_menu.h>
#include <functional>
#include <string>

/* The program's part, for the game folder in use: the words, the paths (the
 * player's folder as MECHWARRIOR_SAVE_DIRECTORY names it), the checks, the
 * settings and the updates. The host fills in window, draw, deliver, pause,
 * resume, quit and restart_for_setup. Not given (a test of the backend): no
 * menu at all. */
struct MwSdlMenuRecipe
{
    std::function<rekit::sdl::MenuHooks(const std::string &gameFolder)> hooks;
    std::function<rekit::sdl::Theme()> theme;
};
/* Before MwSdlInitialize, which starts the menu with it. */
void MwSdlSetMenuRecipe(MwSdlMenuRecipe recipe);
