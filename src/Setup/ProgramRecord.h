// Where this program is, for the BattleTech launcher (mech3-re, docs/launcher.md, "Copies installed without the
// launcher"): a start of a released build writes rebuild-program.txt into the player's folder, and the launcher
// links the copy it names.
#pragma once
#include <string>

namespace mw1o::setup {

inline constexpr const char* kProgramRecord = "rebuild-program.txt";
inline constexpr const char* kLauncherMark = "rebuild-game.txt";  // at the top of a game's folder the launcher installed

// A released build's tag: "mw1r-v", then three numbers ("mw1r-v1.0.0"). Any other label is not one.
bool release_tag(const std::string& tag);

// This build's tag, kept in the program's file as "MW3-PROGRAM-VERSION:<tag>", where the launcher reads it without
// running the program; "" (and no such string in the file) when the build is not a release (MECHWARRIOR_RELEASE_TAG).
const char* build_tag();

// The running program's own file, "" when it can't be told.
std::string self_path();

// The file the player starts, of a program running from `self`: the AppImage (`appimage`, $APPIMAGE) when `self` is
// inside its mount (`appdir`, $APPDIR), else `self`. "" when there is none worth naming: an AppImage that is gone, or
// a copy under macOS App Translocation, which is gone when it ends.
std::string started_file(const std::string& self, const std::string& appimage, const std::string& appdir);

// `program` is in a folder the launcher installed (its mark in a folder above).
bool launchers_own(const std::string& program);

struct ProgramStart {
  bool hidden = false;  // no player at this run
  std::string tag;      // build_tag()
  std::string self, appimage, appdir;
  std::string user;  // the player's folder, settled
};
// Writes the record when this start is one to record: a release's, not hidden, not the launcher's own copy. True when
// the file was written; a record that already says the same is left alone. Never throws.
bool record_program(const ProgramStart& start);

}  // namespace mw1o::setup
