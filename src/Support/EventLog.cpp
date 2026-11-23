// The game-event log (mechwarrior/event_log.h): host test instrument, not game code.
#include "mechwarrior/event_log.h"

#include "Sha256.h"

#include <cstdio>
#include <cstdlib>
#include <span>
#include <string>
#include <vector>

namespace {

FILE *eventLog;
bool opened;
uint64_t musicTicks, toneTicks;
// The MIDI message being assembled from the sequencer's bytes.
uint8_t runningStatus;
uint8_t data[2];
int dataCount;
bool inSysex;
std::vector<uint8_t> sysex;
// The combat speaker's last output, logged on change only.
uint16_t lastPeriod = 0xFFFF;
bool lastOn;
// Combat polls in a row, written as one line before the next game-stream line (sound, key, snap). Notes,
// songs and tones don't end a run: the timer drives them, and its interleaving with combat polls depends
// on the sound device.
uint64_t pendingPolls;
bool pendingCombat;

FILE *Log() {
  if (!opened) {
    opened = true;
    if (const char *path = std::getenv("MECHWARRIOR_EVENT_LOG")) {
      eventLog = std::fopen(path, "w");
      if (eventLog) std::fprintf(eventLog, "{\"event_log\": 1}\n");
    }
  }
  return eventLog;
}

void FlushPolls() {
  if (!pendingPolls) return;
  std::fprintf(eventLog, "[\"%s\", %llu]\n", pendingCombat ? "cpoll" : "poll", (unsigned long long)pendingPolls);
  pendingPolls = 0;
}

std::string Prefix(std::span<const uint8_t> bytes) { return mw1o::support::sha256_hex(bytes).substr(0, 16); }

int DataBytes(uint8_t status) {
  switch (status & 0xF0) {
    case 0xC0: case 0xD0: return 1;
    default: return 2;
  }
}

void Message(uint8_t status) {
  static const char *const kinds[8] = {"off", "on", "poly", "cc", "prog", "press", "bend", ""};
  const unsigned channel = status & 0x0F, kind = (status >> 4) & 7;
  if (kind == 6)
    std::fprintf(eventLog, "[\"note\", %llu, \"bend\", %u, %u, 0]\n", (unsigned long long)musicTicks, channel,
                 (unsigned)(data[0] | (data[1] << 7)));
  else
    std::fprintf(eventLog, "[\"note\", %llu, \"%s\", %u, %u, %u]\n", (unsigned long long)musicTicks, kinds[kind],
                 channel, data[0], DataBytes(status) == 2 ? data[1] : 0u);
}

}  // namespace

bool MwEventLogActive(void) { return Log() != nullptr; }

void MwEventLogMusicTick(void) {
  if (Log()) ++musicTicks;
}

void MwEventLogMidiByte(uint8_t value) {
  if (!Log()) return;
  if (inSysex) {
    if (value == 0xF7 || (value >= 0x80 && value < 0xF8)) {
      if (value == 0xF7) sysex.push_back(value);
      std::fprintf(eventLog, "[\"note\", %llu, \"sysex\", %zu, \"%s\"]\n", (unsigned long long)musicTicks,
                   sysex.size(), Prefix(sysex).c_str());
      inSysex = false;
      sysex.clear();
      if (value == 0xF7) return;
    } else {
      sysex.push_back(value);
      return;
    }
  }
  if (value >= 0xF8) return;  // real-time bytes: none in the songs
  if (value == 0xF0) {
    inSysex = true;
    sysex.assign(1, value);
    runningStatus = 0;
    return;
  }
  if (value >= 0xF0) {  // other system common bytes: none in the songs
    runningStatus = 0;
    return;
  }
  if (value >= 0x80) {
    runningStatus = value;
    dataCount = 0;
    return;
  }
  if (!runningStatus) return;
  data[dataCount++] = value;
  if (dataCount == DataBytes(runningStatus)) {
    Message(runningStatus);
    dataCount = 0;
  }
}

void MwEventLogSong(const uint8_t *stream, uint16_t offset) {
  if (!Log()) return;
  uint8_t head[32];
  for (unsigned i = 0; i < sizeof(head); ++i) head[i] = stream[(uint16_t)(offset + i)];
  std::fprintf(eventLog, "[\"song\", %llu, \"%s\"]\n", (unsigned long long)musicTicks, Prefix(head).c_str());
}

void MwEventLogToneTick(uint16_t period, bool on) {
  if (!Log()) return;
  ++toneTicks;
  if (period == lastPeriod && on == lastOn) return;
  lastPeriod = period, lastOn = on;
  std::fprintf(eventLog, "[\"tone\", %llu, %u, %d]\n", (unsigned long long)toneTicks, period, on ? 1 : 0);
}

void MwEventLogSound(int16_t event, uint16_t priority, bool enabled) {
  if (!Log()) return;
  FlushPolls();
  std::fprintf(eventLog, "[\"sound\", %d, %u, %d]\n", event, priority, enabled ? 1 : 0);
}

void MwEventLogKeyRead(uint16_t key) {
  if (!Log()) return;
  FlushPolls();
  std::fprintf(eventLog, "[\"key\", %u]\n", key);
}

void MwEventLogPoll(bool combat) {
  if (!Log() || !combat) return;  // campaign polls count the host's waiting, not the game (event_log.h)
  pendingCombat = true;
  ++pendingPolls;
}

void MwEventLogSnap(const char *name) {
  if (!Log()) return;
  FlushPolls();
  std::fprintf(eventLog, "[\"snap\", \"%s\"]\n", name);
  std::fflush(eventLog);
}

void MwEventLogCampaignStart(uint16_t ticks, uint16_t word0, uint16_t word1, uint16_t planet) {
  const char *path = std::getenv("MECHWARRIOR_START_LOG");
  if (!path) return;
  FILE *log = std::fopen(path, "a");
  if (!log) return;
  std::fprintf(log, "[\"start\", %u, %u, %u, %u]\n", ticks, word0, word1, planet);
  std::fclose(log);
}
