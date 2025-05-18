#pragma once

#include <JuceHeader.h>
#include "ChannelProcessor.h"
#include "FXBusProcessor.h"
#include "MasterBusProcessor.h"
#include "GroupBusProcessor.h"
#include "../Routing/RoutingManager.h"

class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;
    
    // AudioIODeviceCallback interface implementation
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, 
                                        int numInputChannels,
                                        float* const* outputChannelData, 
                                        int numOutputChannels,
                                        int numSamples,
                                        const juce::AudioIODeviceCallbackContext& context) override;
    
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    
    // Get a ChannelProcessor for a specific channel
    ChannelProcessor* getChannelProcessor(int channelIndex);
    
    // Get the FX bus and master processors
    FXBusProcessor* getFXBusProcessor(int busIndex);
    GroupBusProcessor* getGroupBusProcessor(int busIndex);
    MasterBusProcessor* getMasterBusProcessor();
    
    // Get all group bus processors
    std::vector<GroupBusProcessor*> getAllGroupBusProcessors();
    
    // Get the routing manager
    auralis::RoutingManager& getRoutingManager() { return auralis::RoutingManager::getInstance(); }
    
    // Get the audio device manager
    juce::AudioDeviceManager& getAudioDeviceManager() { return deviceManager; }
    
    // Setup audio device manager
    bool setupAudioDevices();

    /** Save the current audio device state to disk. */
    void saveAudioDeviceState() const;

    /** Restore audio device state from disk if available. */
    void loadAudioDeviceState();
    
    // Set channel send level
    void setChannelSendLevel(int channelIndex, float sendLevel);
    
    // Broadcast parameter changes to update UI
    void broadcastParametersChanged();
    
private:
    static constexpr int numChannels = 32;
    static constexpr int numFXBuses = 3; // Vocal, Instrument, Drum
    static constexpr int numGroupBuses = 4; // Vocals, Instruments, Drums, Speech
    
    std::vector<std::unique_ptr<ChannelProcessor>> channelProcessors;
    std::vector<std::unique_ptr<GroupBusProcessor>> groupBusProcessors;
    std::vector<std::unique_ptr<FXBusProcessor>> fxBusProcessors;
    std::unique_ptr<MasterBusProcessor> masterBusProcessor;
    
    // Test sine wave generator
    std::unique_ptr<juce::AudioProcessor> testSineWave;
    
    juce::AudioDeviceManager deviceManager;
    juce::AudioBuffer<float> tempBuffer;
    juce::AudioBuffer<float> groupBusBuffer;
    juce::AudioBuffer<float> fxBusBuffer;
    juce::AudioBuffer<float> masterBuffer;
    
    double sampleRate = 44100.0;
    int bufferSize = 512;
}; 