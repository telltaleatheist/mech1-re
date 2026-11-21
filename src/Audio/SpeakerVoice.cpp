#include "SpeakerVoice.h"
#include "AudioClock.h"
#include "mechwarrior/campaign_music.h"
#include "mechwarrior/hardware_ports.h"
#include <cmath>

namespace mw1o::audio
{
void SpeakerVoice::reset()
{
    control_ = 0;
    divisor_ = 0x10000;
    highByteNext_ = counting_ = false;
    phase_ = 0;
}

bool SpeakerVoice::writePort(uint16_t port, uint8_t value)
{
    if (port == PIT_CONTROL_PORT && value == PIT_CHANNEL2_SQUARE_WAVE)
    {
        highByteNext_ = false;
        counting_ = false;
        phase_ = 0;
        return true;
    }
    if (port == PIT_CHANNEL2_PORT)
    {
        if (!highByteNext_)
        {
            periodLow_ = value;
            highByteNext_ = true;
            return true;
        }
        uint16_t period = periodLow_ | ((uint16_t)value << 8);
        divisor_ = period ? period : 0x10000;
        highByteNext_ = false;
        counting_ = true;
        phase_ = 0;
        return true;
    }
    if (port == SPEAKER_CONTROL_PORT)
    {
        if (!(value & 1)) phase_ = 0; /* Gate low holds the mode 3 counter. */
        control_ = value;
        return true;
    }
    return false;
}

void SpeakerVoice::integrate(double duration, double &integral)
{
    const double pi = 3.14159265358979323846;
    if (counting_ && (control_ & SPEAKER_GATE_BITS) == SPEAKER_GATE_BITS)
    {
        double duty = (double)((divisor_ + 1) / 2) / divisor_;
        double frequency = (double)PIT_CLOCK_HZ / divisor_;
        for (uint32_t harmonic = 1; harmonic <= 32 && harmonic * frequency < OUTPUT_SAMPLE_RATE / 2.0; ++harmonic)
        {
            double angular = 2 * pi * harmonic / divisor_;
            double start = angular * phase_ - pi * harmonic * duty;
            double amplitude = 4 * sin(pi * harmonic * duty) / (pi * harmonic);
            integral += amplitude * (sin(start + angular * duration) - sin(start)) / angular;
        }
    }
    if (counting_ && (control_ & 1)) phase_ = fmod(phase_ + duration, divisor_);
}
}
