#pragma once

#include <JuceHeader.h>
#include "../Audio/FXBusProcessor.h"
#include "../Audio/GroupBusProcessor.h"
#include "../Utils/BlackwayLookAndFeel.h"
#include "GroupBusComponent.h"

// Component for a single FX bus row
class FXBusRowComponent : public juce::Component,
                         public juce::Slider::Listener,
                         public juce::Button::Listener
{
public:
    FXBusRowComponent(FXBusProcessor* processor);
    ~FXBusRowComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Listeners
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    
private:
    FXBusProcessor* fxProcessor;
    
    juce::Label busNameLabel;
    juce::Slider reverbWetSlider;
    juce::Label reverbLabel;
    juce::Slider delayWetSlider;
    juce::Label delayLabel;
    juce::ToggleButton bypassToggle;
    juce::Label bypassLabel;
    
    BlackwayLookAndFeel blackwayLookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXBusRowComponent)
};

// Main component for the FX Buses tab
class FXBusesComponent : public juce::Component
{
public:
    FXBusesComponent();
    ~FXBusesComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void connectToProcessors(std::vector<FXBusProcessor*> fxProcessors);
    void connectToGroupBusProcessors(std::vector<GroupBusProcessor*> groupProcessors);

private:
    std::vector<std::unique_ptr<FXBusRowComponent>> busRows;
    std::vector<FXBusProcessor*> fxProcessors;
    
    // Group Bus Component
    std::unique_ptr<GroupBusComponent> groupBusComponent;
    
    BlackwayLookAndFeel blackwayLookAndFeel;
    juce::Image backgroundImage;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXBusesComponent)
}; 