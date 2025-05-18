#include "RoutingManager.h"

RoutingManager::RoutingManager()
{
    // Initialize defaults
}

RoutingManager::~RoutingManager()
{
    // Clean up
}

void RoutingManager::initialize(const std::vector<std::unique_ptr<ChannelProcessor>>& channelProcs,
                               const std::vector<std::unique_ptr<FXBusProcessor>>& fxBusProcs)
{
    // Store references
    channelProcessors = &channelProcs;
    fxBusProcessors = &fxBusProcs;
    
    // Create initial mapping of channels to FX buses
    for (size_t i = 0; i < channelProcs.size(); ++i)
    {
        auto channelType = channelProcs[i]->getChannelType();
        
        // Assign each channel to the appropriate FX bus based on its type
        FXBusProcessor::BusType busType;
        
        if (channelType == ChannelProcessor::ChannelType::Vocal)
        {
            busType = FXBusProcessor::BusType::VocalFX;
        }
        else if (channelType == ChannelProcessor::ChannelType::Instrument)
        {
            busType = FXBusProcessor::BusType::InstrumentFX;
        }
        else
        {
            busType = FXBusProcessor::BusType::DrumFX; // Default for other channel types
        }
        
        channelToFxBusMap[static_cast<int>(i)] = busType;
        
        // Log the initial mapping
        juce::Logger::writeToLog("Channel " + juce::String(i) + " mapped to FX bus: " +
                               juce::String(static_cast<int>(busType)));
    }
}

void RoutingManager::setChannelSendLevel(int channelIndex, float sendLevel)
{
    if (channelProcessors == nullptr || fxBusProcessors == nullptr)
    {
        juce::Logger::writeToLog("RoutingManager not initialized");
        return;
    }
    
    // Validate channel index
    if (channelIndex < 0 || channelIndex >= static_cast<int>(channelProcessors->size()))
    {
        juce::Logger::writeToLog("Invalid channel index: " + juce::String(channelIndex));
        return;
    }
    
    // Get the FX bus type for this channel
    auto busType = getFXBusTypeForChannel(channelIndex);
    
    // Find the corresponding FX bus processor
    auto* fxBus = findFxBusProcessor(busType);
    if (fxBus == nullptr)
    {
        juce::Logger::writeToLog("FX bus not found for channel " + juce::String(channelIndex));
        return;
    }
    
    // Update the send level
    fxBus->addInputChannel(channelIndex, sendLevel);
    
    // Also update the send level in the channel processor
    if (channelIndex >= 0 && channelIndex < static_cast<int>(channelProcessors->size()))
    {
        (*channelProcessors)[channelIndex]->setFxSendLevel(sendLevel);
    }
    
    juce::Logger::writeToLog("Set channel " + juce::String(channelIndex) + 
                           " send level to " + juce::String(sendLevel) + 
                           " for FX bus " + fxBus->getBusName());
}

FXBusProcessor::BusType RoutingManager::getFXBusTypeForChannel(int channelIndex) const
{
    auto it = channelToFxBusMap.find(channelIndex);
    if (it != channelToFxBusMap.end())
    {
        return it->second;
    }
    
    // Default to VocalFX if not found
    return FXBusProcessor::BusType::VocalFX;
}

FXBusProcessor* RoutingManager::findFxBusProcessor(FXBusProcessor::BusType busType) const
{
    if (fxBusProcessors == nullptr)
        return nullptr;
    
    // Find the FX bus with the matching type
    for (const auto& fxBus : *fxBusProcessors)
    {
        if (fxBus->getBusType() == busType)
        {
            return fxBus.get();
        }
    }
    
    return nullptr;
}

bool RoutingManager::canAddMoreChannels() const
{
    if (channelProcessors == nullptr)
        return true; // No channels yet, so we can add some
        
    int currentChannelCount = static_cast<int>(channelProcessors->size());
    auto plan = auralis::SubscriptionManager::getInstance().getCurrentPlan();
    return currentChannelCount < getMaxChannelsForPlan(plan);
}

int RoutingManager::getMaxChannelsForPlan(auralis::Plan plan)
{
    switch (plan)
    {
        case auralis::Plan::Foundation: return 32;
        case auralis::Plan::Flow:       return 48;
        case auralis::Plan::Pro:        return 64;
    }
    return 32; // Default to Foundation plan limit
} 