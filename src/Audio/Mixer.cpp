#include "Mixer.h"
#include "AudioClock.h"
#include "mechwarrior/audio_devices.h"
#include <cmath>

namespace mw1o::audio
{
void Mixer::reset(Card card, bool silentMt32)
{
    card_ = card;
    silentMt32_ = silentMt32;
    speaker_.reset();
    time_ = sampleCycles_ = sampleIntegral_ = 0;
    blockFrames_ = 0;
}

void Mixer::advance(double pitTime, Sink sink)
{
    const double cyclesPerSample = (double)PIT_CLOCK_HZ / OUTPUT_SAMPLE_RATE;
    const uint32_t channelCount = channels();
    while (time_ < pitTime)
    {
        double duration = fmin(pitTime - time_, cyclesPerSample - sampleCycles_);
        speaker_.integrate(duration, sampleIntegral_);
        time_ += duration;
        sampleCycles_ += duration;
        if (sampleCycles_ < cyclesPerSample - 1e-8) continue;
        float speaker = (float)(0.15 * sampleIntegral_ / cyclesPerSample);
        float *frame = &block_[blockFrames_ * channelCount];
        if (card_ == Card::None) frame[0] = speaker;
        else
        {
            if (card_ == Card::Mt32)
            {
                if (silentMt32_) frame[0] = frame[1] = 0;
                else MwMt32Render(frame, 1);
            }
            else MwAdlibRender(frame, 1);
            frame[0] += speaker;
            frame[1] += speaker;
        }
        ++blockFrames_;
        sampleCycles_ = sampleIntegral_ = 0;
        if (blockFrames_ == BLOCK_FRAMES)
        {
            sink(block_, blockFrames_);
            blockFrames_ = 0;
        }
    }
}
}
