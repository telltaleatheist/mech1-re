// The story file: the original's creative content, imported once from the
// player's own exes into the player's data folder (docs/phase2/PLAN.md):
//
//   <write dir>/story/text.json     {"texts": [{"id", "address", "text"}, ...]}
//   <write dir>/story/words.json    the UI words (PLAN.md section 7), {"words":
//                                   [{"id", "address", "text"}, ...]}: a
//                                   string without its NUL, a block whole
//   <write dir>/story/native.json   native spans (PLAN.md 8.3), {"native":
//                                   [{"id", "address", "bytes"}, ...]}, the
//                                   bytes in hex
//   <write dir>/story/pilots.json, planets.json, missions.json
//                                   typed records (Records.h)
//   <write dir>/story/import.json   what was imported: the import version, the
//                                   exes' SHA-256, the revision, the table's hash
//
// import.json is written last, so an interrupted import counts as missing.
// From then on the game reads the story file; the exe is needed only to
// import. Text keeps the original's control codes and 8-bit characters as
// \u00NN.
#pragma once

#include <filesystem>
#include <map>
#include <span>
#include <stdexcept>
#include <string>

#include "ImportTable.h"

namespace mw1o::import {

struct ImportError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

std::filesystem::path story_dir(const std::filesystem::path& write_dir);

enum class StoryState { Current, Missing, Outdated };
StoryState story_state(const std::filesystem::path& write_dir,
                       std::span<const ImportPiece> table = import_table());

// Reads every piece of `table` from the exes in `game_dir` and writes the
// story file into `write_dir`. Throws ImportError (a missing exe, or the
// pieces that don't match, by id) or std::filesystem errors.
void import_story(const std::filesystem::path& game_dir, const std::filesystem::path& write_dir,
                  std::span<const ImportPiece> table = import_table());

// Imports if the story file is missing or outdated; true if it did. Throws
// ImportError saying what to do if an import is needed and fails.
bool ensure_story(const std::filesystem::path& game_dir, const std::filesystem::path& write_dir,
                  std::span<const ImportPiece> table = import_table());

using StoryText = std::map<std::string, std::string, std::less<>>;

// story/text.json, id -> text. Throws ImportError (naming the file) if it
// can't be read, isn't the story file's shape, or names an id twice.
StoryText load_story_text(const std::filesystem::path& write_dir);

using Words = std::map<std::string, std::string, std::less<>>;

// story/words.json, id -> the word's bytes as the exe holds them (a string
// with its NUL). Every word piece of `table` must be there once, at its
// table length, and nothing else. Throws ImportError naming the file and the
// word.
Words load_words(const std::filesystem::path& write_dir, std::span<const ImportPiece> table = import_table());
using Native = std::map<std::string, std::string, std::less<>>;
// story/native.json, id -> the span's bytes. Every native piece of `table`
// must be there once, at its table length, and nothing else. Throws
// ImportError naming the file and the piece.
Native load_native(const std::filesystem::path& write_dir, std::span<const ImportPiece> table = import_table());

}  // namespace mw1o::import
