#ifndef MECHWARRIOR_EVENT_LOG_H
#define MECHWARRIOR_EVENT_LOG_H
#include <stdbool.h>
#include <stdint.h>

/* The game-event log (MECHWARRIOR_EVENT_LOG=FILE; docs/phase5/NO_EMULATION_PLAN.md,
 * slice T1): what the game asked for, not what it wrote to hardware, so it
 * stays exact while the hardware layer under it is rebuilt. One JSON array per
 * line, after a header line:
 *   ["note", tick, "on"|"off"|"poly"|"cc"|"prog"|"press"|"bend", channel, a, b]
 *       the sequencer's output, timed by its own tick count (MwTickMidiTrack
 *       calls); "bend" gives the 14-bit value as a, b = 0;
 *   ["note", tick, "sysex", length, "sha256 prefix"]   (no song bytes logged);
 *   ["song", tick, "sha256 prefix of the track's first 32 bytes"];
 *   ["tone", tick, period, on]  combat's speaker channel output on each change,
 *       timed by its own tick count (BtechTickSpeakerToneChannels calls);
 *       period is the PIT divisor the effect player chose (Hz = 1193182 / period);
 *   ["sound", event, priority, enabled]  a combat sound request;
 *   ["key", word]   a campaign key read;
 *   ["cpoll", n]    n combat input polls in a row (one a tick). Campaign polls
 *       are not logged: how often the host polls while the game waits depends
 *       on the time the port model charges (1 us a port read), not on the game;
 *   ["snap", name]  a trace snap.
 * Off unless the variable is set. Exact under the test clock
 * (MECHWARRIOR_DEBUG_VIRTUAL_CLOCK). */
bool MwEventLogActive(void);
void MwEventLogMusicTick(void);
void MwEventLogMidiByte(uint8_t value);
void MwEventLogSong(const uint8_t *stream,uint16_t offset);
void MwEventLogToneTick(uint16_t period,bool on);
void MwEventLogSound(int16_t event,uint16_t priority,bool enabled);
void MwEventLogKeyRead(uint16_t key);
void MwEventLogPoll(bool combat); /* combat polls only are logged */
void MwEventLogSnap(const char *name);
#endif
