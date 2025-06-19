#pragma once

#include <JuceHeader.h>
#include "ChannelStripComponent.h"
#include "../Audio/AudioEngine.h"
#include "../Utils/StyleManager.h"

class ChannelsComponent : public juce::Component
{
public:
    ChannelsComponent();
    ~ChannelsComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Connect to the audio engine to get channel processors
    void connectToAudioEngine(AudioEngine* engine);
    
    // Refresh all channel strips from their processors
    void refreshAllChannelStrips();

private:
    static constexpr int numChannels = 32;

    juce::Image backgroundImage;

    std::vector<std::unique_ptr<ChannelStripComponent>> channelStrips;

    AudioEngine* audioEngine = nullptr;

    void checkChannelLimits();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelsComponent)
}; 