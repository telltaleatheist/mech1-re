#include "mechwarrior/game_files.h"

#include "mechwarrior/port_diagnostics.h"

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstring>
#include <system_error>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

void (*mwHostValidateFileDestination)(const std::uint8_t *buffer, std::uint16_t bytesRead);

namespace mw1o::files {
namespace {

namespace fs = std::filesystem;

fs::path g_game_directory;
fs::path g_save_directory;

std::string upper(std::string text) {
  for (char &c : text)
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  return text;
}

bool ends_with(const std::string &text, const char *suffix) {
  const std::size_t length = std::strlen(suffix);
  return text.size() >= length && text.compare(text.size() - length, length, suffix) == 0;
}

bool is_program(const std::string &name) {
  const std::string shouted = upper(name);
  return ends_with(shouted, ".EXE") || ends_with(shouted, ".COM");
}

bool has_separator(const std::string &name) { return name.find_first_of("/\\:") != std::string::npos; }

FileError last_error() {
  if (errno == ENOENT || errno == ENOTDIR) return FileError::kNotFound;
  if (errno == ENOSPC) return FileError::kNoRoom;
  return FileError::kDenied;
}

// The path's native narrow form, as every other path in the program is opened.
GameFile open_path(const fs::path &path, const char *mode) {
  std::error_code ec;
  if (fs::is_directory(path, ec)) return GameFile(FileError::kDenied);
  std::FILE *file = std::fopen(path.string().c_str(), mode);
  return file ? GameFile(file) : GameFile(last_error());
}

// An 8.3 save name's stem in upper case, or nothing: STEM.GAM, 1 to 8 letters, one dot, no DOS-reserved character.
std::optional<std::string> save_stem(const std::string &name) {
  const std::size_t dot = name.rfind('.');
  if (dot == std::string::npos || dot == 0 || dot > 8 || name.find('.') != dot || name.size() - dot != 4) return {};
  if (name.find_first_of(" /\\:*?\"<>|") != std::string::npos) return {};
  std::string shouted;
  for (const char c : name) shouted += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  if (shouted.compare(dot, 4, ".GAM") != 0) return {};
  return shouted.substr(0, dot);
}

bool listed(const fs::directory_entry &entry) {
  std::error_code ec;
  if (!entry.is_regular_file(ec) || ec) return false;
#if defined(_WIN32)
  const DWORD attributes = GetFileAttributesW(entry.path().c_str());
  if (attributes == INVALID_FILE_ATTRIBUTES) return false;
  if (attributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) return false;
#endif
  return true;
}

}  // namespace

std::optional<std::uint16_t> GameFile::read(std::uint8_t *buffer, std::uint16_t byte_count) {
  if (!file_) return {};
  std::clearerr(file_.get());
  std::vector<std::uint8_t> bytes(byte_count);
  const std::size_t count = std::fread(bytes.data(), 1, byte_count, file_.get());
  if (count) {
    if (mwHostValidateFileDestination) mwHostValidateFileDestination(buffer, static_cast<std::uint16_t>(count));
    std::memcpy(buffer, bytes.data(), count);
  }
  if (std::ferror(file_.get())) return {};
  return static_cast<std::uint16_t>(count);
}

bool GameFile::seek(std::uint32_t offset) {
  if (!file_) return false;
#if LONG_MAX < UINT32_MAX
  if (offset > LONG_MAX) return false;  // Host limit (a 32-bit long); the game's files fit.
#endif
  return std::fseek(file_.get(), static_cast<long>(offset), SEEK_SET) == 0;
}

bool GameFile::write(const std::uint8_t *buffer, std::uint16_t byte_count) {
  if (!file_) return false;
  if (std::fwrite(buffer, 1, byte_count, file_.get()) != byte_count || std::fflush(file_.get()) != 0) {
    error_ = last_error();
    return false;
  }
  return true;
}

bool GameFile::close() { return file_ && std::fclose(file_.release()) == 0; }

void set_game_directory(const std::string &directory) { g_game_directory = directory; }
void set_save_directory(const std::string &directory) { g_save_directory = directory; }

fs::path find_game_file(const fs::path &directory, const std::string &name, const char *caller) {
  if (is_program(name)) {
    std::fprintf(stderr, "MechWarrior: play opened the game's program %s (in %s); only the import reads the exes\n",
                 name.c_str(), caller);
    /* Port failsafe; not an original DOS exit. */
    MW_PORT_FAILSAFE("A game program opened at play (docs/phase2/PLAN.md 8.4, decision 5)");
  }
  const fs::path exact = directory / name;
#if !defined(_WIN32)  // Windows' file system already ignores case.
  std::error_code ec;
  if (fs::exists(exact, ec) || has_separator(name)) return exact;
  const std::string wanted = upper(name);
  fs::directory_iterator entry(directory.empty() ? fs::path(".") : directory, ec);
  for (; !ec && entry != fs::directory_iterator(); entry.increment(ec)) {
    const std::string found = entry->path().filename().string();
    if (upper(found) == wanted) return directory / found;
  }
#endif
  return exact;
}

GameFile open_game_file(const std::string &name) {
  const bool save = ends_with(name, ".GAM");
  if (save && (g_save_directory.empty() || has_separator(name))) return GameFile(FileError::kDenied);
  return open_path(find_game_file(save ? g_save_directory : g_game_directory, name, __func__), "rb");
}

GameFile create_save(const std::string &name) {
  if (g_save_directory.empty() || has_separator(name)) return GameFile(FileError::kDenied);
  return open_path(find_game_file(g_save_directory, name, __func__), "wb");
}

std::vector<std::string> list_saves() {
  std::vector<std::string> stems;
  if (g_save_directory.empty()) return stems;
  std::error_code ec;
  fs::directory_iterator entry(g_save_directory, ec);
  for (; !ec && entry != fs::directory_iterator(); entry.increment(ec)) {
    if (!listed(*entry)) continue;
    if (const auto stem = save_stem(entry->path().filename().string())) stems.push_back(*stem);
  }
  return stems;
}

}  // namespace mw1o::files
