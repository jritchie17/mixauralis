#include "ChannelsComponent.h"
#include "../MainApp.h"
#include "../../Subscription/SubscriptionManager.h"
#include "../Utils/StyleManager.h"

ChannelsComponent::ChannelsComponent()
{
    // Use global look and feel for consistency
    auto& lf = StyleManager::getInstance().getLookAndFeel();
    setLookAndFeel(&lf);
    horizontalScrollbar.setLookAndFeel(&lf);
    
    // Load background image
    auto assetsDir = juce::File::getCurrentWorkingDirectory().getChildFile("Assets");
    backgroundImage = juce::ImageCache::getFromFile(assetsDir.getChildFile("background_03.png"));
    
    // Create the channel strips
    for (int i = 0; i < numChannels; ++i)
    {
        auto channelStrip = std::make_unique<ChannelStripComponent>();
        channelStrip->setChannelName("Channel " + juce::String(i + 1));
        channelStrip->setChannelType(i < 8 ? ChannelStripComponent::ChannelType::SingingVocal : 
                                    (i < 16 ? ChannelStripComponent::ChannelType::Instrument : 
                                             ChannelStripComponent::ChannelType::Other));
        
        channelsContainer.addAndMakeVisible(channelStrip.get());
        channelStrips.push_back(std::move(channelStrip));
    }
    
    // Set up the viewport for horizontal scrolling
    viewport.setViewedComponent(&channelsContainer, false);
    viewport.setScrollBarsShown(false, false);
    addAndMakeVisible(viewport);
    
    // Set up the scrollbar
    horizontalScrollbar.setRangeLimits(0.0, 1.0);
    horizontalScrollbar.setAutoHide(false);
    horizontalScrollbar.addListener(this);
    addAndMakeVisible(horizontalScrollbar);
    
    // Try to connect to the audio engine
    auto* app = dynamic_cast<MainApp*>(juce::JUCEApplication::getInstance());
    if (app != nullptr)
    {
        connectToAudioEngine(&app->getAudioEngine());
    }

    // Initial check of channel limits
    checkChannelLimits();
}

ChannelsComponent::~ChannelsComponent()
{
    horizontalScrollbar.removeListener(this);
    horizontalScrollbar.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void ChannelsComponent::paint(juce::Graphics& g)
{
    if (backgroundImage.isValid())
    {
        // Draw the background image
        g.drawImage(backgroundImage, 
                   getLocalBounds().toFloat(),
                   juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        g.fillAll(juce::Colours::black);
    }
}

void ChannelsComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Position the scrollbar at the bottom
    auto scrollBarHeight = 16;
    horizontalScrollbar.setBounds(bounds.removeFromBottom(scrollBarHeight));
    
    // Set the viewport to take the rest of the space
    viewport.setBounds(bounds);
    
    // Calculate the total width required for all channel strips
    int totalWidth = numChannels * channelStripWidth;
    channelsContainer.setBounds(0, 0, totalWidth, bounds.getHeight());
    
    // Use FlexBox for horizontal layout
    juce::FlexBox stripRow;
    stripRow.flexDirection = juce::FlexBox::Direction::row;
    stripRow.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    stripRow.alignItems = juce::FlexBox::AlignItems::stretch;
    
    // Add each channel strip to the FlexBox
    for (int i = 0; i < numChannels; ++i)
    {
        stripRow.items.add(juce::FlexItem(*channelStrips[i])
                          .withWidth(static_cast<float>(channelStripWidth))
                          .withHeight(static_cast<float>(bounds.getHeight())));
    }
    
    // Perform the layout of channel strips (this will handle positioning)
    stripRow.performLayout(juce::Rectangle<float>(0, 0, totalWidth, bounds.getHeight()));
    
    // Set up scrollbar range and current position
    auto viewportWidth = viewport.getWidth();
    
    if (totalWidth > viewportWidth)
    {
        horizontalScrollbar.setRangeLimits(0.0, totalWidth - viewportWidth);
        horizontalScrollbar.setCurrentRange(viewport.getViewPositionX(), viewportWidth);
    }
    else
    {
        horizontalScrollbar.setRangeLimits(0.0, 0.0);
        horizontalScrollbar.setCurrentRange(0.0, 1.0);
    }
}

void ChannelsComponent::connectToAudioEngine(AudioEngine* engine)
{
    audioEngine = engine;
    if (audioEngine != nullptr)
    {
        // Connect each channel strip to its processor
        for (size_t i = 0; i < channelStrips.size(); ++i)
        {
            if (auto* processor = audioEngine->getChannelProcessor(static_cast<int>(i)))
            {
                channelStrips[i]->connectToProcessor(processor);
            }
        }
    }
    
    // Check channel limits after connecting
    checkChannelLimits();
}

void ChannelsComponent::checkChannelLimits()
{
    // Check subscription limits and enable/disable channels accordingly
    auto& subscription = auralis::SubscriptionManager::getInstance();
    auto plan = subscription.getCurrentPlan();
    
    int channelLimit = 0;
    switch (plan)
    {
        case auralis::Plan::Foundation:
            channelLimit = 8;
            break;
        case auralis::Plan::Flow:
            channelLimit = 16;
            break;
        case auralis::Plan::Pro:
            channelLimit = 32;
            break;
    }
    
    // Enable/disable channels based on limit
    for (int i = 0; i < numChannels; ++i)
    {
        channelStrips[i]->setChannelEnabled(i < channelLimit);
    }
}

void ChannelsComponent::refreshAllChannelStrips()
{
    // Refresh all channel strips to match their processor state
    for (auto& strip : channelStrips)
    {
        strip->refreshParametersFromProcessor();
    }
}

void ChannelsComponent::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
    if (scrollBarThatHasMoved == &horizontalScrollbar)
    {
        viewport.setViewPosition((int) newRangeStart, viewport.getViewPositionY());
    }
} 