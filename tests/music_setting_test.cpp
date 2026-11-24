// The music setting (src/SDL/SDLBackend_Config.cpp): music=soundfont, speaker
// or off; the old sound_system and ROM keys still load; the packaged ini is
// read first and the player's, read last, decides when it names the music.
#include "SDLBackend.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
int failures = 0;
const std::filesystem::path kDir = "music_setting_files";

void Check(bool ok, const std::string &what) {
  if (ok) return;
  ++failures;
  std::printf("FAIL %s\n", what.c_str());
}

// The host's own default, before any ini: read once, at the start.
const MwHostMusic kDefault = mwSdlMusic;

// Loads the packaged ini, then the player's if there is one; false if either is refused.
bool Load(const std::string &packaged, const char *player = nullptr) {
  mwSdlMusic = kDefault;
  const auto file = [](const char *name, const std::string &text) {
    std::ofstream(kDir / name) << text;
    return (kDir / name).string();
  };
  bool ok = MwSdlLoadTimingConfiguration(file("packaged.ini", packaged).c_str(), false);
  if (player) ok = MwSdlLoadTimingConfiguration(file("player.ini", player).c_str(), false) && ok;
  return ok;
}
bool Is(MwHostMusic want, const std::string &packaged, const char *player = nullptr) {
  return Load(packaged, player) && mwSdlMusic == want;
}
}  // namespace

int main(int, char **) {
  std::filesystem::create_directories(kDir);
  Check(kDefault == MW_MUSIC_SOUNDFONT, "the host's default: the soundfont");
  Check(Is(MW_MUSIC_SOUNDFONT, ""), "no key: the soundfont");
  Check(Is(MW_MUSIC_SPEAKER, "music=speaker\n"), "music=speaker");
  Check(Is(MW_MUSIC_SOUNDFONT, "music=soundfont\n"), "music=soundfont");
  Check(Is(MW_MUSIC_OFF, "music = off \n"), "music=off, with spaces");
  Check(!Load("music=loud\n") && mwSdlMusic == MW_MUSIC_SOUNDFONT, "an unknown value is refused and changes nothing");

  Check(Is(MW_MUSIC_SPEAKER, "sound_system=speaker\n"), "the old speaker, over the default");
  Check(Is(MW_MUSIC_SPEAKER, "music=soundfont\n", "sound_system=speaker\n"), "the old speaker in the player's ini, over the packaged soundfont");
  Check(Is(MW_MUSIC_SOUNDFONT, "sound_system=mt32\n"), "the old MT-32");
  Check(Is(MW_MUSIC_SOUNDFONT, "sound_system=adlib\n"), "the old AdLib");
  Check(!Load("sound_system=tandy\n"), "an unknown old value is refused");
  Check(Is(MW_MUSIC_OFF, "music=off\nsound_system=mt32\n"), "music= wins over the old key after it");
  Check(Is(MW_MUSIC_OFF, "sound_system=mt32\nmusic=off\n"), "music= wins over the old key before it");
  Check(Is(MW_MUSIC_SOUNDFONT, "mt32_control_rom=/x\nmt32_pcm_rom=\"C:\\y\"\n"), "the ROM keys are ignored");

  Check(Is(MW_MUSIC_SPEAKER, "music=soundfont\n", "music=speaker\n"), "the player's speaker over the packaged soundfont");
  Check(Is(MW_MUSIC_OFF, "music=soundfont\n", "music=off\n"), "the player's off over the packaged soundfont");
  Check(Is(MW_MUSIC_SOUNDFONT, "music=soundfont\n", "combat_speed=2\n"), "a player's ini without music keeps the packaged one");
  Check(!Load("music=speaker\n", "music=loud\n") && mwSdlMusic == MW_MUSIC_SPEAKER, "a refused player's ini changes nothing");

  std::filesystem::remove_all(kDir);
  std::printf("music_setting: %s\n", failures ? "FAILED" : "ok");
  return failures ? 1 : 0;
}
