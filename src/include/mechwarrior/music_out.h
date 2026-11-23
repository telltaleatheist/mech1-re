#ifndef MECHWARRIOR_MUSIC_OUT_H
#define MECHWARRIOR_MUSIC_OUT_H
#include <stdint.h>

/* The rebuild's music (docs/phase5/NO_EMULATION_PLAN.md, slice S3): the
 * sequencer's output, handed to the host as it happens, beside whatever the
 * selected backend does with it. The host sets these when it plays the music
 * itself (music=soundfont); unset, nothing is called. Never game state.
 *   songStarted: a song starts (MwStartMidiTrack), with its track's first 32
 *       bytes, which name it (the event log's "song" line hashes the same);
 *   midiByte: each byte the sequencer sends (MwDispatchMidiByte). */
typedef struct MwMusicOutCalls
{
    void (*songStarted)(const uint8_t head[32]);
    void (*midiByte)(uint8_t value);
} MwMusicOutCalls;
extern MwMusicOutCalls mwMusicOut;
#endif
