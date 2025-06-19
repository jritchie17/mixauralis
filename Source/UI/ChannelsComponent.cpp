#include "ChannelsComponent.h"
#include "../MainApp.h"
#include "Subscription/SubscriptionManager.h"
#include "../Utils/StyleManager.h"

ChannelsComponent::ChannelsComponent()
{
    // Use global look and feel for consistency
    auto& lf = StyleManager::getInstance().getLookAndFeel();
    setLookAndFeel(&lf);
    
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
        
        addAndMakeVisible(channelStrip.get());
        channelStrips.push_back(std::move(channelStrip));
    }
    
    
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

    // Calculate grid dimensions
    auto itemWidth = getWidth() / 16;
    auto itemHeight = getHeight() / 2;

    for (int i = 0; i < numChannels; ++i)
    {
        auto row = i / 16;
        auto col = i % 16;
        channelStrips[i]->setBounds(col * itemWidth,
                                    row * itemHeight,
                                    itemWidth,
                                    itemHeight);
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
