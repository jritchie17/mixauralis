#include "TunerProcessor.h"

namespace auralis
{
    TunerProcessor::TunerProcessor()
    {
    }

    void TunerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;
        dryBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);
        tunedBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);
        dryBuffer.clear();
        tunedBuffer.clear();
        interpolators.resize(getTotalNumInputChannels());
        for (auto& interp : interpolators)
            interp.reset();
    }

    void TunerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
    {
        dryBuffer.makeCopyOf (buffer);

        const float currentStrength = strength.load();
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* dryData   = dryBuffer.getReadPointer (channel);
            auto* tunedData = tunedBuffer.getWritePointer (channel);

            const float detectedPitch = detectPitch (dryData, numSamples);

            float ratio = 1.0f;
            if (detectedPitch > 0.0f)
            {
                const float midi = 69.0f + 12.0f * std::log2 (detectedPitch / 440.0f);
                const int nearest = static_cast<int> (std::round (midi));
                const float target = 440.0f * std::pow (2.0f, (nearest - 69) / 12.0f);
                ratio = target / detectedPitch;
            }

            if (ratio <= 0.0f || std::isnan (ratio) || ratio > 4.0f)
                ratio = 1.0f;

            interpolators[channel].reset();
            interpolators[channel].process (ratio, dryData, tunedData, numSamples);

            auto* out = buffer.getWritePointer (channel);
            for (int i = 0; i < numSamples; ++i)
                out[i] = dryData[i] * (1.0f - currentStrength) + tunedData[i] * currentStrength;
        }
    }

    float TunerProcessor::detectPitch (const float* data, int numSamples) const
    {
        int zeroCrossings = 0;
        for (int i = 1; i < numSamples; ++i)
        {
            const bool prevPos = data[i - 1] >= 0.0f;
            const bool currPos = data[i] >= 0.0f;
            if (prevPos != currPos)
                ++zeroCrossings;
        }

        if (zeroCrossings == 0)
            return 0.0f;

        const float frequency = static_cast<float> (currentSampleRate) * zeroCrossings
                                 / (2.0f * static_cast<float> (numSamples));
        return frequency;
    }
}
