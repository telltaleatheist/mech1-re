// The rebuild's music (src/Audio/Music; docs/phase5/NO_EMULATION_PLAN.md, S3):
// the instrument table's strict reading, and Music on the real subset bank.
//
//     music_test SUBSET.sf2 data/music/instruments.json
#include "Music.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

void Check(bool ok, const char *what) {
  if (!ok) {
    std::printf("FAIL %s\n", what);
    ++failures;
  }
}

std::string Read(const char *path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) throw std::runtime_error(std::string("cannot read ") + path);
  return std::string(std::istreambuf_iterator<char>(file), {});
}

bool Refused(const std::string &json) {
  try {
    mw1o::audio::ParseInstrumentTable(json);
  } catch (const std::exception &) {
    return true;
  }
  return false;
}

void Send(mw1o::audio::Music &music, std::initializer_list<int> bytes) {
  for (int b : bytes) music.midiByte((uint8_t)b);
}

float Peak(mw1o::audio::Music &music, uint32_t frames) {
  std::vector<float> out(frames * 2);
  music.render(out.data(), frames);
  float peak = 0;
  for (float v : out) peak = std::fmax(peak, std::fabs(v));
  return peak;
}

constexpr char kSong[] = R"({"songs": [{"name": "T", "head": "0123456789abcdef", "parts": [)";

}  // namespace

int main(int argc, char **argv) {
  if (argc != 3) {
    std::fprintf(stderr, "usage: %s SUBSET.sf2 instruments.json\n", argv[0]);
    return 2;
  }
  const std::string bank = Read(argv[1]), text = Read(argv[2]);

  // The table: the real one reads; what's wrong is refused.
  mw1o::audio::InstrumentTable table;
  try {
    table = mw1o::audio::ParseInstrumentTable(text);
  } catch (const std::exception &e) {
    std::printf("FAIL the table: %s\n", e.what());
    return 1;
  }
  Check(table.songs.size() == 5, "the table names the five songs");
  const std::string ok = std::string(kSong) + R"({"channel": 2, "program": 48}]}]})";
  Check(!Refused(ok), "a minimal table reads");
  Check(Refused(std::string(kSong) + R"({"channel": 2, "program": 48, "colour": 1}]}]})"), "unknown key refused");
  Check(Refused(std::string(kSong) + R"({"channel": 2, "program": 48}, {"channel": 2, "program": 40}]}]})"),
        "a channel listed twice refused");
  Check(Refused(std::string(kSong) + R"({"channel": 2, "program": 128}]}]})"), "program 128 refused");
  Check(Refused(std::string(kSong) + R"({"channel": 17, "program": 1}]}]})"), "channel 17 refused");
  Check(Refused(std::string(kSong) + R"({"channel": 2}]}]})"), "a melodic part without a program refused");
  Check(Refused(R"({"songs": [{"name": "T", "head": "0123", "parts": [{"channel": 2, "program": 1}]}]})"),
        "a short head refused");
  Check(Refused(std::string(R"({"drum_keys": [[36, 200]], )") + (kSong + 1) + R"({"channel": 2, "program": 48}]}]})"),
        "a drum key out of range refused");

  // Music on the real bank and table.
  mw1o::audio::Music music;
  std::string error;
  if (!music.load(bank.data(), bank.size(), table, 48000, error)) {
    std::printf("FAIL the bank and table: %s\n", error.c_str());
    return 1;
  }
  const mw1o::audio::InstrumentTable::Song &song = table.songs[0];
  int melodic = -1, drums = -1;
  for (int c = 0; c < 16; ++c) {
    if (song.parts[c].used && !song.parts[c].drums && melodic < 0) melodic = c;
    if (song.parts[c].used && song.parts[c].drums) drums = c;
  }
  Check(music.startSong(song.head) == song.name, "a song in the table starts by its head");
  Check(music.startSong("ffffffffffffffff").empty(), "an unknown song is not in the table");
  Check(Peak(music, 4800) == 0, "silent before any note");

  // A note, then its note-off by running status (note-on, velocity 0).
  Send(music, {0x90 | melodic, 60, 100});
  Check(music.activeVoices() > 0, "a note-on starts a voice");
  Check(Peak(music, 4800) > 0.001f, "the note sounds");
  Send(music, {60, 0});
  Peak(music, 48000 * 4);
  Check(music.activeVoices() == 0, "running-status note-off ends it");

  // SysEx is skipped, even where its bytes look like a note.
  Send(music, {0xF0, 0x41, 0x10, 0x16, 0x12, 0x90 & 0x7F, 0x3C, 0x7F, 0xF7});
  Check(music.activeVoices() == 0, "SysEx makes no sound");
  // A real-time byte inside a message changes nothing.
  Send(music, {0x90 | melodic, 0xF8, 64, 90});
  Check(music.activeVoices() > 0, "a real-time byte inside a message is skipped");

  // All notes off (the game's stop sweep).
  Send(music, {0x90 | melodic, 67, 90, 0xB0 | melodic, 123, 0});
  Peak(music, 48000 * 4);
  Check(music.activeVoices() == 0, "all-notes-off ends every note");

  // The rhythm part plays the kit.
  if (drums >= 0) {
    Send(music, {0x90 | drums, 38, 110});
    Check(music.activeVoices() > 0 && Peak(music, 4800) > 0.001f, "the kit sounds on the drum channel");
  }

  std::printf("music: %s\n", failures ? "FAILED" : "ok");
  return failures ? 1 : 0;
}
