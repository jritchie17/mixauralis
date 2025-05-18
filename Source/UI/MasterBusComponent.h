#pragma once

#include <JuceHeader.h>
#include "../Audio/MasterBusProcessor.h"
#include "LevelMeter.h"
#include "LoudnessMeterComponent.h"
#include "../Utils/StyleManager.h"

class MasterBusComponent : public juce::Component,
                           private juce::Timer
{
public:
    MasterBusComponent(MasterBusProcessor* processor);
    ~MasterBusComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    // Update the UI with current LUFS values
    void updateLufsDisplay();
    
    // Timer callback for periodic updates
    void timerCallback() override;
    
    // References
    MasterBusProcessor* masterProcessor;
    
    // UI Components
    juce::Image backgroundImage;
    LevelMeter levelMeter;
    
    // Output section
    juce::Label outputMeterLabel;
    juce::Label currentLufsLabel;
    juce::Label targetLufsLabel;
    
    // Effect toggles
    juce::ToggleButton compressorToggle;
    juce::ToggleButton limiterToggle;
    
    // Target selection
    juce::TextButton youtubeButton;
    juce::TextButton facebookButton;
    juce::TextButton customButton;
    juce::Slider customLufsSlider;
    juce::Label customLufsLabel;
    

    
    // Loudness meter
    std::unique_ptr<auralis::LoudnessMeterComponent> meter;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterBusComponent)
}; 