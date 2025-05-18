#include "RoutingManager.h"
#include "../MainApp.h"

namespace auralis {

RoutingManager& RoutingManager::getInstance()
{
    static RoutingManager instance;
    return instance;
}

void RoutingManager::initialize(const std::vector<std::unique_ptr<ChannelProcessor>>& channels,
                              const std::vector<std::unique_ptr<FXBusProcessor>>& fxBuses)
{
    // Initialize FX bus assignments based on channel types
    fxBusAssignments.resize(channels.size());
    
    for (size_t i = 0; i < channels.size(); ++i)
    {
        const auto& channel = channels[i];
        switch (channel->getChannelType())
        {
            case ChannelProcessor::ChannelType::Vocal:
                fxBusAssignments.set(i, 0); // Vocal FX bus
                break;
            case ChannelProcessor::ChannelType::Instrument:
                fxBusAssignments.set(i, 1); // Instrument FX bus
                break;
            case ChannelProcessor::ChannelType::Drums:
                fxBusAssignments.set(i, 2); // Drum FX bus
                break;
            default:
                fxBusAssignments.set(i, -1); // No FX bus
                break;
        }
    }
    
    // Initialize input map to unassigned (-1)
    inputMap.resize(channels.size());
    for (int i = 0; i < inputMap.size(); ++i)
        inputMap.set(i, -1);
}

void RoutingManager::assignFXBus(int channelIndex, int busIndex)
{
    if (channelIndex < 0 || channelIndex >= numChannels)
        return;
        
    // Resize the assignments array if needed
    if (fxBusAssignments.size() <= channelIndex)
    {
        const int oldSize = fxBusAssignments.size();
        fxBusAssignments.resize(channelIndex + 1);
        // Initialize new elements to -1
        for (int i = oldSize; i < fxBusAssignments.size(); ++i)
            fxBusAssignments.set(i, -1);
    }
        
    fxBusAssignments.set(channelIndex, busIndex);
}

int RoutingManager::getFXBusAssignment(int channelIndex) const
{
    if (channelIndex < 0 || channelIndex >= numChannels || channelIndex >= fxBusAssignments.size())
        return -1;
        
    return fxBusAssignments[channelIndex];
}

void RoutingManager::assignPhysicalInput(int channelIdx, int deviceChanIdx)
{
    if (channelIdx < 0 || channelIdx >= numChannels)
        return;
        
    // Resize the map if needed
    if (inputMap.size() <= channelIdx)
    {
        const int oldSize = inputMap.size();
        inputMap.resize(channelIdx + 1);
        // Initialize new elements to -1
        for (int i = oldSize; i < inputMap.size(); ++i)
            inputMap.set(i, -1);
    }
        
    inputMap.set(channelIdx, deviceChanIdx);
}

int RoutingManager::getPhysicalInput(int channelIdx) const
{
    if (channelIdx < 0 || channelIdx >= numChannels || channelIdx >= inputMap.size())
        return -1;
        
    return inputMap[channelIdx];
}

int RoutingManager::getNumPhysicalInputs() const
{
    if (auto* app = dynamic_cast<MainApp*>(juce::JUCEApplication::getInstance()))
    {
        auto* device = app->getAudioEngine().getAudioDeviceManager().getCurrentAudioDevice();
        return device != nullptr ? device->getInputChannelNames().size() : 0;
    }
    return 0;
}

} // namespace auralis 