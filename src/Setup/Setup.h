// MechWarrior 1's setup recipe for re-setup-kit (extern/re-setup-kit,
// DESIGN.md there): what an install of the 1989/1990 game is, where it may
// be, how it is copied into the player's folder, and the launcher contract's
// --status. The kit knows no game; everything MechWarrior-specific is here.
// Nothing in it reads or prints the game's text: files are judged by their
// size and CRC (src/Support/BtechImage.cpp).
#pragma once

#include <rekit/family_folder.h>
#include <rekit/sources.h>
#include <rekit/search.h>
#include <rekit/status.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace mw1o::setup {

inline constexpr const char* kId = "mech1";             // the launcher contract's game id
inline constexpr const char* kTitle = "MechWarrior 1";  // the setup screen's sentences
inline constexpr const char* kFolderName = "MechWarrior 1 Rebuild";  // a game folder made inside a picked one
inline constexpr const char* kFolderKey = "mw1r";  // its mark's game=<key> (the kit's game_folder.h), the launcher's name
// The player's folder (saves, story/, MechWarrior.ini, the log): SDL's per-user folder for these names, under
// the family's folder (BattleTech; the games' test installs used MicroProse, taken across by settle_user_folder).
inline constexpr const char* kOrganisation = rekit::kFamilyOrg;
inline constexpr const char* kApplication = "MechWarrior 1 Rebuild";
// Where the previews kept it; carried over once (carry_over).
inline constexpr const char* kOldOrganisation = "BattleTechReverse";
inline constexpr const char* kOldApplication = "MechWarrior";
inline constexpr const char* kCarriedMarker = "carried-over-from-preview.txt";
// In a folder our install made, and only there (rekit::swap_in_folder replaces only such a folder).
inline constexpr const char* kInstallMarker = "mechwarrior-install.txt";
// The game is about 1 MB: a pick holding far more (a Games folder, a drive's top) is refused before copying.
inline constexpr uint64_t kMaxGameBytes = uint64_t(512) << 20;

// The per-user folder for (organisation, application), ending in a separator, the way SDL_GetPrefPath names it
// (%APPDATA%, ~/Library/Application Support, $XDG_DATA_HOME or ~/.local/share), WITHOUT making it: --status
// writes nothing. "" when the platform's base cannot be found.
std::string pref_folder(const char* organisation, const char* application);
// The player's folder: MECHWARRIOR_SAVE_DIRECTORY, else pref_folder(kOrganisation, kApplication), or the same
// game's folder under the old name while that one is still in use (unmoved_folder). Not made here.
std::string user_folder();
// A folder with something in it besides the system's .DS_Store.
bool has_content(const std::string& dir);
// For the player's folder `folder` (BattleTech/<Game>): the same game's folder under the old name
// (MicroProse/<Game>, ending in a separator) when that one has something in it and `folder` has nothing (a move
// not made yet, or one that failed: it stays in use); "" otherwise.
std::string unmoved_folder(const std::string& folder);
// <user folder>/install: where the setup puts the game's files unless the player picks another folder.
std::string default_install(const std::string& user);

// The one-time carry-over from the previews' folder: every file of `from` copied into `to` (made when missing)
// where `to` has none of that name; nothing is ever moved, deleted or overwritten. A marker in `to` makes it once
// only. Skipped (done=false, no error) when `to` already has the marker or `from` does not exist.
struct Carried {
  bool done = false;
  int files = 0;
  std::string error;
};
Carried carry_over(const std::string& from, const std::string& to);

// The player's folder settled on each start, in this order:
//   1. `folder` (BattleTech/<Game>) has something in it: used as it is (kInUse).
//   2. Else the same game's folder under the old name (MicroProse/<Game>) has: renamed across once
//      (rekit::move_once, never a copy), then MicroProse removed only if that left it empty (kMoved). A move that
//      fails keeps the old folder in use for this start, tried again at the next (kMoveFailed, `folder` = the old
//      one, `error` why).
//   3. Else the previews' folder `preview` (BattleTechReverse/MechWarrior) copied in once (carry_over: kCarried),
//      or nothing to do (kNew).
struct Settled {
  enum Kind { kInUse, kMoved, kMoveFailed, kCarried, kNew } kind = kNew;
  std::string folder;  // the player's folder for this start
  std::string from;    // kMoved, kMoveFailed: the old folder; kCarried: the previews'
  int files = 0;       // kCarried
  std::string error;
};
Settled settle_user_folder(const std::string& folder, const std::string& preview);

// BTECH.EXE's revision by its bytes ("reference", "1990-01-22"; combat_installation.h), "" when not a known one.
std::string btech_revision(const std::vector<uint8_t>& bytes);

// What the kit's walk looks for: BTECH.EXE judged by its revision; a pick of BTECH.EXE or MW_MAIN.EXE stands
// for its folder.
rekit::SourceRules rules();
// What a pick holds when the walk found no copy in it, in one sentence for the setup screen (rekit::sdl::Game::
// recognise): a BTECH.EXE of a revision this program doesn't know, or another MechWarrior (2, 3, Pirate's Moon), by
// the files each is known by. "" when it is none of those (the kit's own sentence stands) or the pick can't be read.
// Given any path; never throws; the look is stopped after a while (kRecogniseSeconds).
std::string recognise(const std::string& path);
inline constexpr int kRecogniseSeconds = 20;
// Where a DOS game's copy is usually kept: no installer, so no registry; the usual folder names under the system
// drive, the home folder and any Wine prefix, and a folder dropped beside the program.
rekit::SearchHints hints();

// Whether the story was imported (story/import.json in the player's folder): after it, MW_MAIN.EXE is not needed.
bool story_imported(const std::string& user);

struct Check {
  rekit::Status::State state = rekit::Status::kMissing;
  std::string version;               // BTECH.EXE's revision
  std::vector<std::string> missing;  // kIncomplete: in plain words
  std::string why;                   // kMissing: why this folder is not the game
};
// Whether `dir` is a playable copy: a known BTECH.EXE, and MW_MAIN.EXE beside it until the story is imported.
Check check_install(const std::string& dir, bool imported);

// `dir` when it holds BTECH.EXE, else the first folder under it, `depth` levels down at most (shallowest first,
// names sorted); "" when none.
std::string game_in(const std::string& dir, int depth = 2);

// The install a start uses: `remembered` (MechWarrior.ini's game_folder) while it is still the game, then the
// player's own default_install, then the search (hints(), each place looked into two levels down). `searched`
// says the answer came from the search (offered, never taken unasked). "" when none.
std::string find_install(const std::string& remembered, const std::string& user, const std::string& program_dir,
                         bool* searched);
// A copy the search finds that is none of `not_these` (the one in use, the player's own), or "": choosing again.
std::string find_other_install(const std::vector<std::string>& not_these, const std::string& user, const std::string& program_dir);

// The answer to `--status`: nothing written, no window. `version` is the program's.
rekit::Status status(const std::string& remembered, const std::string& user, const std::string& program_dir,
                     const std::string& version);

// MechWarrior.ini's game_folder, read without SDL; "" when unset.
std::string remembered_folder(const std::string& user);

// Installing what the kit's walk found into `out_dir`: a folder on disk copied (rekit::copy_folder), an archive's
// game folder unpacked in one forward read; staged in `out_dir`.new and swapped in (rekit::swap_in_folder), so a
// failure or a cancel leaves nothing and `out_dir` is replaced only when our install made it. The player's
// original (folder or archive) is only read, never moved or deleted. One at a time into a folder: `out_dir`.lock is
// locked meanwhile, and a second install is refused. (A game that is playing isn't asked: it can see the swap.)
struct Installed {
  bool ok = false, cancelled = false;
  std::string dir;    // the folder now holding the game
  std::string error;  // why not, in plain words
  int files = 0;
  uint64_t bytes = 0;
};
using Progress = std::function<bool(double fraction, const std::string& file)>;  // false cancels
Installed install(const rekit::Found& found, const std::string& out_dir, const Progress& progress = nullptr);

}  // namespace mw1o::setup
