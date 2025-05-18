#pragma once

#include <JuceHeader.h>
#include "../Utils/StyleManager.h"
#include "../Audio/AudioEngine.h"
#include "../Audio/MasterBusProcessor.h"
#include "SettingsComponent.h"

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    /** Return the SettingsComponent instance for UI updates. */
    auralis::SettingsComponent* getSettingsComponent() const { return settingsTab.get(); }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Image backgroundImage;
    
    // Audio processors
    std::unique_ptr<AudioEngine> audioEngine;
    std::unique_ptr<MasterBusProcessor> masterBusProcessor;
    
    juce::TabbedComponent tabbedComponent{juce::TabbedButtonBar::TabsAtTop};
    
    std::unique_ptr<juce::Component> routingTab;
    std::unique_ptr<juce::Component> channelsTab;
    std::unique_ptr<juce::Component> fxBusesTab;
    std::unique_ptr<juce::Component> masterTab;
    std::unique_ptr<auralis::SettingsComponent> settingsTab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
}; 