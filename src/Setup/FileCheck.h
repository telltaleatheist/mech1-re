// The game's files against the ones this program knows: what a copy lacks, what
// in it is known to be damaged, and what differs. A report for the player, who
// can always go on; it stops nothing. Names, sizes and SHA-256 only.
// BTECH.EXE and MW_MAIN.EXE have their own checks (Setup.h) and aren't here;
// nor are the saves, or any file the program doesn't open.
#pragma once

#include <rekit/sources.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace mw1o::setup {

// A file the program opens, as the copies the project knows hold it
// (GameFilesTable.cpp, written by tools/setup/make_game_files.py, which says
// where each is opened).
struct GameFile {
  enum class When {
    kImport,  // read once, for the story
    kStart,   // at every start of the campaign
    kPlay,    // in every campaign or every battle
    kSome,    // only in some plays, or under a setting
  };
  const char* name;  // upper case
  uint32_t size;
  const char* sha256;
  When when;
  const char* without;  // what a play without it does
};
std::span<const GameFile> game_files();
// A file known to be damaged, by its bytes.
struct KnownDamage {
  const char* name;
  uint32_t size;
  const char* sha256;
};
std::span<const KnownDamage> known_damage();

// What is at a listed file's name: its size, and its SHA-256, which the reader
// may leave empty when no listed file of that name has that size.
struct SeenFile {
  uint64_t size = 0;
  std::string sha256;
};
// The file of that name (any case), nullopt when there is none (a folder of the name is none).
using FileReader = std::function<std::optional<SeenFile>(const std::string& name)>;

struct FilesReport {
  std::vector<std::string> missing, damaged, different;  // names, in the list's order
  std::vector<std::string> effects;  // what a play without the missing and damaged ones does, each once
  std::string unchecked;             // why nothing was checked; "" when it was
  bool all_known() const { return unchecked.empty() && missing.empty() && damaged.empty() && different.empty(); }
};
// `imported`: the story is imported, so the files only the import reads are left out.
FilesReport check_files(const FileReader& reader, bool imported, std::span<const GameFile> files = game_files(),
                        std::span<const KnownDamage> damage = known_damage());

// The files of a folder; of what the kit's walk found (a folder, or a folder in an archive: one forward read of the
// archive, only the listed files' bytes kept, and not at all past kMaxCheckedArchive). Never throws.
FilesReport check_folder(const std::string& dir, bool imported, std::span<const GameFile> files = game_files(),
                         std::span<const KnownDamage> damage = known_damage());
FilesReport check_found(const rekit::Found& found, bool imported);
inline constexpr uint64_t kMaxCheckedArchive = uint64_t(64) << 20;

// For the player: "" when every file is one this program knows. At most kNamed names a list, "and N more".
std::string files_note(const FilesReport& report);
inline constexpr size_t kNamed = 3;
// For the launcher's one line (--status); "" when there is nothing to say, or it wasn't checked.
std::string files_status_note(const FilesReport& report);
// For the log: one line, every name.
std::string files_log(const FilesReport& report);

}  // namespace mw1o::setup
