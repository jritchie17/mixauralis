#include "TunerProcessor.h"

namespace auralis
{
    TunerProcessor::TunerProcessor()
    {
    }

    void TunerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        dryBuffer.setSize(2, samplesPerBlock);
        dryBuffer.clear();
    }

    void TunerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
    {
        const auto numChannels = buffer.getNumChannels();
        const auto numSamples = buffer.getNumSamples();
        const auto currentStrength = strength.load();

        // Store dry signal
        for (int channel = 0; channel < numChannels; ++channel)
            dryBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);

        // Process each channel
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            auto* dryData = dryBuffer.getReadPointer(channel);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                // Simple "fake autotune" - phase inversion simulates pitch change
                const float dry = dryData[sample];
                const float wet = -channelData[sample]; // Phase inverted
                channelData[sample] = dry * (1.0f - currentStrength) + wet * currentStrength;
            }
        }
    }
} 