#ifndef MW1O_AUDIO_CLOCK_H
#define MW1O_AUDIO_CLOCK_H

namespace mw1o::audio
{
/* The PIT's input clock and the output rate: every synth works in PIT
 * cycles and the mixer turns them into frames at this rate. */
inline constexpr unsigned PIT_CLOCK_HZ = 1193182, OUTPUT_SAMPLE_RATE = 48000;
}
#endif
