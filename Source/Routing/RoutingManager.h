#pragma once
#include <JuceHeader.h>
#include "../Audio/ChannelProcessor.h"
#include "../Audio/FXBusProcessor.h"

namespace auralis {

class RoutingManager
{
public:
    static RoutingManager& getInstance();
    
    // Initialize with processors
    void initialize(const std::vector<std::unique_ptr<ChannelProcessor>>& channels,
                   const std::vector<std::unique_ptr<FXBusProcessor>>& fxBuses);
    
    // Get the number of channels
    int getNumChannels() const { return numChannels; }
    
    // Get/Set FX bus assignments
    void assignFXBus(int channelIndex, int busIndex);
    int getFXBusAssignment(int channelIndex) const;
    
    // Get/Set physical input assignments
    void assignPhysicalInput(int channelIdx, int deviceChanIdx);
    int getPhysicalInput(int channelIdx) const;
    
    // Get the number of physical inputs
    int getNumPhysicalInputs() const;
    
private:
    RoutingManager() = default;
    ~RoutingManager() = default;
    
    static constexpr int numChannels = 32;
    juce::Array<int> fxBusAssignments;
    juce::Array<int> inputMap;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoutingManager)
}; 

} // namespace auralis 