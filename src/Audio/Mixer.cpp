#include "Mixer.h"
#include "AudioClock.h"
#include "Music.h"
#include <algorithm>
#include <cmath>

namespace mw1o::audio
{
void Mixer::reset(Music *music)
{
    music_ = music;
    musicFrames_.assign(2 * MUSIC_RUN_FRAMES, 0.0f);
    musicRead_ = musicCount_ = 0;
    tone_.reset();
    time_ = sampleCycles_ = sampleIntegral_ = 0;
    blockFrames_ = 0;
}

void Mixer::advance(double pitTime, Sink sink)
{
    const double cyclesPerSample = (double)PIT_CLOCK_HZ / OUTPUT_SAMPLE_RATE;
    const uint32_t channelCount = channels();
    // The frames this call completes: the music renders no further ahead than these.
    uint64_t musicDue = music_ ? (uint64_t)((sampleCycles_ + (pitTime - time_)) / cyclesPerSample) : 0;
    while (time_ < pitTime)
    {
        double duration = fmin(pitTime - time_, cyclesPerSample - sampleCycles_);
        tone_.integrate(duration, sampleIntegral_);
        time_ += duration;
        sampleCycles_ += duration;
        if (sampleCycles_ < cyclesPerSample - 1e-8) continue;
        float tone = (float)(0.15 * sampleIntegral_ / cyclesPerSample);
        float *frame = &block_[blockFrames_ * channelCount];
        if (!music_) frame[0] = tone;
        else
        {
            if (musicRead_ == musicCount_)
            {
                const uint32_t run = (uint32_t)std::clamp<uint64_t>(musicDue, 1, MUSIC_RUN_FRAMES);
                music_->render(musicFrames_.data(), run);
                musicRead_ = 0, musicCount_ = run;
                musicDue -= std::min<uint64_t>(musicDue, run);
            }
            frame[0] = musicFrames_[2 * musicRead_];
            frame[1] = musicFrames_[2 * musicRead_ + 1];
            ++musicRead_;
            frame[0] += tone;
            frame[1] += tone;
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
