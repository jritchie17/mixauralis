#pragma once

#include <JuceHeader.h>
#include "ChannelStripComponent.h"
#include "../Audio/AudioEngine.h"
#include "../Utils/BlackwayLookAndFeel.h"

class ChannelsComponent : public juce::Component,
                          private juce::ScrollBar::Listener
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
    static constexpr int channelStripWidth = ChannelStripComponent::kStandardWidth;
    static constexpr int viewportMargin = ChannelStripComponent::kStandardPadding;
    
    BlackwayLookAndFeel blackwayLookAndFeel;
    juce::Image backgroundImage;
    
    std::vector<std::unique_ptr<ChannelStripComponent>> channelStrips;
    juce::Viewport viewport;
    juce::Component channelsContainer;
    juce::ScrollBar horizontalScrollbar{false};
    
    AudioEngine* audioEngine = nullptr;
    
    void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;
    void checkChannelLimits();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelsComponent)
}; 