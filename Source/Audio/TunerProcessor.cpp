#include "TunerProcessor.h"

namespace auralis
{
    TunerProcessor::TunerProcessor()
    {
    }

    void TunerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        dryBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);
        dryBuffer.clear();
    }

    void TunerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
    {
        // Store dry signal
        dryBuffer.makeCopyOf(buffer);

        // Get current strength value
        const float currentStrength = strength.load();

        // Process each channel
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            auto* dryData = dryBuffer.getReadPointer(channel);

            // Process each sample
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                // Simple phase inversion as placeholder for pitch correction
                const float invertedSample = -dryData[sample];
                
                // Blend between dry and "tuned" signal
                channelData[sample] = dryData[sample] * (1.0f - currentStrength) + 
                                    invertedSample * currentStrength;
            }
        }
    }
} 