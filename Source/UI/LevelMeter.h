#pragma once

#include <JuceHeader.h>
#include "../Utils/BlackwayLookAndFeel.h"

class LevelMeter : public juce::Component,
                   public juce::Timer
{
public:
    LevelMeter()
    {
        startTimerHz(30); // Update at 30fps
        
        // Set up the audio visualiser component
        visualiser.setBufferSize(128);
        visualiser.setSamplesPerBlock(32);
        visualiser.setColours(juce::Colours::black, juce::Colours::green);
        
        // Configure for vertical orientation
        visualiser.setRepaintRate(30);
        addAndMakeVisible(visualiser);
        
        // Initialize peak value
        peakLevel = 0.0f;
        holdTime = 0.0f;
        falloffRate = 0.9f; // Adjust for desired falloff speed
    }
    
    ~LevelMeter() override
    {
        stopTimer();
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Draw background
        g.setColour(juce::Colours::black);
        g.fillRect(bounds);
        
        // Draw border
        g.setColour(juce::Colours::darkgrey);
        g.drawRect(bounds, 1);
        
        // Draw peak hold indicator
        auto meterHeight = bounds.getHeight();
        auto peakY = bounds.getBottom() - (meterHeight * peakLevel);
        
        // Draw peak line
        g.setColour(juce::Colours::red);
        g.drawLine(bounds.getX(), peakY, bounds.getRight(), peakY, 2.0f);
    }
    
    void resized() override
    {
        // Make sure the visualiser occupies 100% of the component
        visualiser.setBounds(getLocalBounds().reduced(0, 0));
    }
    
    void setLevel(float newLevel)
    {
        // Update the audio visualiser with new audio data
        const int numSamples = 64;
        juce::AudioBuffer<float> buffer(1, numSamples);
        
        for (int i = 0; i < numSamples; ++i)
            buffer.setSample(0, i, newLevel * std::sin(i * 0.1f)); // Simple sine wave for visualization
            
        visualiser.pushBuffer(buffer);
        
        // Update peak level
        if (newLevel > peakLevel)
        {
            peakLevel = newLevel;
            holdTime = 1.0f; // Reset hold time
        }
        
        repaint();
    }
    
    void timerCallback() override
    {
        // In a real application, this would be connected to actual audio levels
        // For now, we'll just animate it for visual feedback
        float randomLevel = 0.1f + 0.9f * static_cast<float>(std::rand()) / RAND_MAX;
        setLevel(randomLevel);
        
        // Update peak hold and falloff
        if (holdTime > 0.0f)
        {
            holdTime -= 0.1f; // Decrement hold time
        }
        else
        {
            // Apply falloff to peak level
            peakLevel *= falloffRate;
        }
    }
    
private:
    juce::AudioVisualiserComponent visualiser{ 1 }; // 1 channel
    float peakLevel;
    float holdTime;
    float falloffRate;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
}; 