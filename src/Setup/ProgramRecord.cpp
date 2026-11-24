#include "ProgramRecord.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace fs = std::filesystem;

namespace mw1o::setup {
namespace {

// Paths here are UTF-8 whatever the system's code page, as the launcher reads them.
std::string utf8(const fs::path& p) {
  const std::u8string s = p.u8string();
  return std::string(s.begin(), s.end());
}
fs::path from_utf8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

}  // namespace

bool release_tag(const std::string& tag) {
  const std::string prefix = "mw1r-v";
  if (tag.rfind(prefix, 0) != 0) return false;
  int numbers = 0;
  size_t at = prefix.size();
  while (true) {
    const size_t from = at;
    while (at < tag.size() && tag[at] >= '0' && tag[at] <= '9') ++at;
    if (at == from) return false;
    ++numbers;
    if (at == tag.size()) return numbers == 3;
    if (tag[at++] != '.') return false;
  }
}

std::string self_path() try {
  std::error_code ec;
#if defined(_WIN32)
  wchar_t buf[4096];
  const DWORD n = GetModuleFileNameW(nullptr, buf, 4096);
  return n > 0 && n < 4096 ? utf8(fs::path(std::wstring(buf, n))) : std::string();
#elif defined(__APPLE__)
  char buf[4096];
  uint32_t n = sizeof buf;
  if (_NSGetExecutablePath(buf, &n) != 0) return {};
  const fs::path p = fs::weakly_canonical(fs::path(buf), ec);
  return ec ? std::string() : utf8(p);
#else
  const fs::path self = fs::read_symlink("/proc/self/exe", ec);
  return ec ? std::string() : utf8(self);
#endif
} catch (...) {
  return {};
}

std::string started_file(const std::string& self, const std::string& appimage, const std::string& appdir) {
  if (self.find("/AppTranslocation/") != std::string::npos) return {};
  std::error_code ec;
  if (!appimage.empty() && !appdir.empty() && self.rfind(appdir + "/", 0) == 0)
    return fs::is_regular_file(from_utf8(appimage), ec) ? appimage : std::string();
  return self;
}

bool launchers_own(const std::string& program) {
  std::error_code ec;
  fs::path at = fs::weakly_canonical(from_utf8(program), ec);
  // The package's own layout inside the game's folder: <x>.app/Contents/MacOS, squashfs-root/usr/bin.
  for (int up = 0; up < 8 && at.has_parent_path() && at != at.parent_path(); ++up) {
    at = at.parent_path();
    if (fs::is_regular_file(at / kLauncherMark, ec)) return true;
  }
  return false;
}

bool record_program(const ProgramStart& start) try {
  if (start.hidden || start.user.empty() || !release_tag(start.tag)) return false;
  const std::string program = started_file(start.self, start.appimage, start.appdir);
  if (program.empty() || launchers_own(program)) return false;
  for (const char c : program)
    if (static_cast<unsigned char>(c) < 0x20) return false;  // a line of its own in the record
  const std::string text = std::string("# Where this game is, for the BattleTech launcher (it links a copy installed without it).\n") +
                           "game=mw1r\nprogram=" + program + "\nversion=" + start.tag + "\n";
  const fs::path file = fs::path(start.user) / kProgramRecord;
  {
    std::ifstream in(file, std::ios::binary);
    if (in && std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()) == text) return false;
  }
  std::error_code ec;
  fs::create_directories(fs::path(start.user), ec);
  std::ofstream out(file, std::ios::binary | std::ios::trunc);
  out << text;
  out.close();
  return static_cast<bool>(out);
} catch (...) {
  return false;
}

}  // namespace mw1o::setup
