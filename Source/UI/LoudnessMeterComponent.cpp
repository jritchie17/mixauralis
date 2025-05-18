#include "LoudnessMeterComponent.h"

namespace auralis
{
    LoudnessMeterComponent::LoudnessMeterComponent()
    {
        startTimerHz(10);
        lufsBuffer.setSize(2, 19200); // 400 ms @ 48 kHz
    }

    void LoudnessMeterComponent::pushSamples(const juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        // Copy input to lufsBuffer (wrapping writePos)
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* src = buffer.getReadPointer(ch);
            float* dst = lufsBuffer.getWritePointer(ch, lufsWritePos);
            
            for (int i = 0; i < numSamples; ++i)
            {
                dst[i] = src[i];
            }
        }

        // Find true-peak (abs sample max)
        float maxSample = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* src = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                maxSample = juce::jmax(maxSample, std::abs(src[i]));
            }
        }
        currentTruePk = juce::Decibels::gainToDecibels(maxSample);

        // Update write position
        lufsWritePos = (lufsWritePos + numSamples) % lufsBuffer.getNumSamples();
    }

    void LoudnessMeterComponent::timerCallback()
    {
        // Calculate short-term LUFS
        float sumSquared = 0.0f;
        int count = 0;

        for (int ch = 0; ch < lufsBuffer.getNumChannels(); ++ch)
        {
            const float* data = lufsBuffer.getReadPointer(ch);
            for (int i = 0; i < lufsBuffer.getNumSamples(); ++i)
            {
                sumSquared += data[i] * data[i];
                count++;
            }
        }

        if (count > 0)
        {
            float rms = std::sqrt(sumSquared / count);
            currentLufs = 10.0f * std::log10(rms * rms) - 0.691f;
        }

        repaint();
    }

    void LoudnessMeterComponent::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds();
        
        // Draw background
        g.fillAll(juce::Colours::black);
        
        // Draw meter bar
        float lufsHeight = juce::jmap(currentLufs, -40.0f, 0.0f, 0.0f, 1.0f);
        lufsHeight = juce::jlimit(0.0f, 1.0f, lufsHeight);
        
        auto meterBounds = bounds.removeFromLeft(bounds.getWidth() * 0.7f);
        g.setColour(juce::Colours::green);
        g.fillRect(meterBounds.removeFromBottom(meterBounds.getHeight() * lufsHeight));
        
        // Draw text
        bounds.removeFromLeft(5);
        g.setColour(juce::Colours::white);
        g.setFont(12.0f);
        g.drawText(juce::String::formatted("LUFS: %.1f   TP: %.1f", currentLufs, currentTruePk),
                  bounds, juce::Justification::centredLeft);
    }
} 