#pragma once

#include <JuceHeader.h>
#include "FXBusProcessor.h"
#include "ChannelProcessor.h"
#include "Subscription/SubscriptionManager.h"

/**
 * RoutingManager - Handles routing between channels, FX buses, and the master bus
 * 
 * This class manages the connections between different audio components in the mixer,
 * particularly for FX send/return routing.
 */
class RoutingManager
{
public:
    RoutingManager();
    ~RoutingManager();
    
    // Initialize with references to processors
    void initialize(const std::vector<std::unique_ptr<ChannelProcessor>>& channelProcessors,
                    const std::vector<std::unique_ptr<FXBusProcessor>>& fxBusProcessors);
    
    // Set the send level from a channel to an FX bus
    // This would typically be called when a user adjusts an FX Send knob
    void setChannelSendLevel(int channelIndex, float sendLevel);
    
    // Determine which FX bus a channel should send to based on its type
    FXBusProcessor::BusType getFXBusTypeForChannel(int channelIndex) const;

    // Check if more channels can be added based on subscription plan
    bool canAddMoreChannels() const;

    // Helper to get max channels for a plan
    static int getMaxChannelsForPlan(auralis::Plan plan);
    
private:
    // References to the audio processors
    const std::vector<std::unique_ptr<ChannelProcessor>>* channelProcessors = nullptr;
    const std::vector<std::unique_ptr<FXBusProcessor>>* fxBusProcessors = nullptr;
    
    // Mapping of channel indices to their FX bus types
    std::map<int, FXBusProcessor::BusType> channelToFxBusMap;
    
    // Helper to find the right FX bus processor
    FXBusProcessor* findFxBusProcessor(FXBusProcessor::BusType busType) const;
}; 