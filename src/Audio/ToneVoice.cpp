#include "ToneVoice.h"
#include "AudioClock.h"
#include <cmath>

namespace mw1o::audio
{
void ToneVoice::reset()
{
    frequency_ = 0;
    phase_ = 0;
}

void ToneVoice::play(double frequency)
{
    if (!(frequency > 0))
    {
        stop();
        return;
    }
    if (!playing()) phase_ = 0;
    frequency_ = frequency;
}

void ToneVoice::stop()
{
    frequency_ = 0;
    phase_ = 0;
}

void ToneVoice::integrate(double duration, double &integral)
{
    if (!playing()) return;
    const double pi = 3.14159265358979323846;
    const double cycles = duration * frequency_ / PIT_CLOCK_HZ; /* The wave's cycles in `duration`. */
    const double secondsPerCycle = 1.0 / frequency_;
    /* The square, +1 for the first half of each cycle and -1 for the second,
     * as its odd harmonics: 4/(pi n) sin(2 pi n t). Integrated over the span
     * in PIT cycles. */
    for (unsigned harmonic = 1; harmonic <= 32 && harmonic * frequency_ < OUTPUT_SAMPLE_RATE / 2.0; harmonic += 2)
    {
        double angular = 2 * pi * harmonic; /* Radians per cycle of the wave. */
        double amplitude = 4 / (pi * harmonic);
        double start = angular * phase_, end = angular * (phase_ + cycles);
        /* d(PIT cycles) = PIT_CLOCK_HZ * secondsPerCycle / angular per radian. */
        integral += amplitude * (std::cos(start) - std::cos(end)) * PIT_CLOCK_HZ * secondsPerCycle / angular;
    }
    phase_ = std::fmod(phase_ + cycles, 1.0);
}
}
