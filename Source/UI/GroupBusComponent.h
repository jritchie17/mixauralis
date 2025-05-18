#pragma once

#include <JuceHeader.h>
#include "../Audio/GroupBusProcessor.h"
#include "../Utils/BlackwayLookAndFeel.h"

// Component for a single Group Bus row
class GroupBusRowComponent : public juce::Component,
                           public juce::Slider::Listener,
                           public juce::Button::Listener
{
public:
    GroupBusRowComponent(GroupBusProcessor* processor);
    ~GroupBusRowComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Listeners
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    
private:
    GroupBusProcessor* groupProcessor;
    
    juce::Label busNameLabel;
    juce::Slider outputGainSlider;  // Output fader
    
    // EQ controls
    juce::Slider eqLowGainSlider;
    juce::Label eqLowLabel;
    juce::Slider eqMidGainSlider;
    juce::Label eqMidLabel;
    juce::Slider eqHighGainSlider;
    juce::Label eqHighLabel;
    
    // Compressor control
    juce::ToggleButton compToggle;
    juce::Label compLabel;
    
    BlackwayLookAndFeel blackwayLookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GroupBusRowComponent)
};

// Main component that contains all group bus rows
class GroupBusComponent : public juce::Component
{
public:
    GroupBusComponent();
    ~GroupBusComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void connectToProcessors(std::vector<GroupBusProcessor*> processors);

private:
    std::vector<std::unique_ptr<GroupBusRowComponent>> busRows;
    std::vector<GroupBusProcessor*> groupProcessors;
    
    // Headers
    juce::Label busNameHeader;
    juce::Label gainHeader;
    juce::Label lowHeader;
    juce::Label midHeader;
    juce::Label highHeader;
    juce::Label compHeader;
    
    BlackwayLookAndFeel blackwayLookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GroupBusComponent)
}; 