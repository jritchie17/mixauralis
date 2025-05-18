#pragma once

#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"
#include "../Utils/BlackwayLookAndFeel.h"
#include "SoundcheckPanel.h"
#include "../Routing/RoutingManager.h"
#include "RoutingMatrixComponent.h"

// Forward declare factory class for friendship
class RoutingToolbarItemFactory;

/**
 * RoutingComponent - Handles input/output routing assignment
 * 
 * This component displays the routing matrix and includes a toolbar
 * with the Soundcheck button.
 */
class RoutingComponent : public juce::Component,
                        public juce::Button::Listener
{
public:
    static constexpr int numChannels = 32;
    
    RoutingComponent();
    ~RoutingComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;
    
    // Connect to the audio engine
    void connectToAudioEngine(AudioEngine* engine);
    
    // Handle toolbar button clicks (called by RoutingToolbarItemFactory)
    void handleToolbarButtonClicked(int toolbarItemId);
    
private:
    BlackwayLookAndFeel blackwayLookAndFeel;
    juce::Image backgroundImage;
    
    // Channel strips
    juce::OwnedArray<ChannelStripComponent> channelStrips;
    
    // Toolbar and buttons
    juce::Toolbar toolbar;
    std::unique_ptr<juce::ToolbarItemFactory> toolbarFactory;
    
    // Soundcheck panel
    std::unique_ptr<SoundcheckPanel> soundcheckPanel;
    
    // Audio engine reference
    AudioEngine* audioEngine = nullptr;
    
    // Callback for the Soundcheck button
    void soundcheckButtonClicked();
    
    // Friend declaration for the factory
    friend class RoutingToolbarItemFactory;
    
    void refreshChannelLabels();

    std::unique_ptr<RoutingMatrixComponent> routingMatrix;
    
    juce::TextButton soundcheckButton { "Soundcheck" };
    juce::TextButton autoMapButton { "Auto-Map Inputs" };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoutingComponent)
}; 