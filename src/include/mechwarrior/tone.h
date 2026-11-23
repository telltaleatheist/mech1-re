#ifndef MECHWARRIOR_TONE_H
#define MECHWARRIOR_TONE_H
#include <stdint.h>

/* The tone voice's notes (docs/phase5/NO_EMULATION_PLAN.md, S5 and S6): both
 * games' pitch tables count periods of this clock, so a period of N sounds at
 * MW_TONE_PERIOD_CLOCK_HZ / N Hz (0 meaning 65536). The host plays a
 * frequency, or silence for 0. */
#define MW_TONE_PERIOD_CLOCK_HZ 1193182.0
static inline double MwTonePeriodFrequency(uint16_t period)
{ return MW_TONE_PERIOD_CLOCK_HZ/(period?period:65536.0); }
#endif
