#pragma once

#include <JuceHeader.h>
#include "../FX/ReverbProcessor.h"
#include "../FX/DelayProcessor.h"

class FXBusProcessor
{
public:
    enum class BusType
    {
        VocalFX,
        InstrumentFX,
        DrumFX
    };

    // Default constructor
    FXBusProcessor() : FXBusProcessor(BusType::VocalFX) {}
    
    FXBusProcessor(BusType type);
    ~FXBusProcessor();
    
    // Audio processing methods
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void releaseResources();
    
    // Parameter control methods
    void setReverbEnabled(bool enabled);
    void setDelayEnabled(bool enabled);
    void setReverbWetLevel(float level); // 0.0 to 1.0
    void setDelayWetLevel(float level);  // 0.0 to 1.0
    void setBypass(bool shouldBypass);
    
    // Channel routing
    void addInputChannel(int channelIndex, float sendLevel);
    
    // Getters
    bool isReverbEnabled() const { return !reverbBypass; }
    bool isDelayEnabled() const { return !delayBypass; }
    float getReverbWetLevel() const { return reverbWetLevel; }
    float getDelayWetLevel() const { return delayWetLevel; }
    bool isBypassed() const { return bypassed; }
    BusType getBusType() const { return busType; }
    juce::String getBusName() const;

private:
    BusType busType;
    
    // Processor graph and nodes
    std::unique_ptr<juce::AudioProcessorGraph> processorGraph;
    juce::AudioProcessorGraph::Node::Ptr inputNode;   // Input summing node
    juce::AudioProcessorGraph::Node::Ptr outputNode;  // Output node
    
    // Effect processors (owned by the graph after preparation)
    std::unique_ptr<ReverbProcessor> reverb;
    std::unique_ptr<DelayProcessor> delay;
    juce::AudioProcessorGraph::Node::Ptr reverbNode;
    juce::AudioProcessorGraph::Node::Ptr delayNode;
    
    // Internal parameters
    bool reverbBypass = false;
    bool delayBypass = false;
    float reverbWetLevel = 0.5f;
    float delayWetLevel = 0.5f;
    bool bypassed = false;
    
    // Map of channel indices to send levels
    std::map<int, float> channelSendLevels;
    
    // Updates connections in the processor graph
    void updateConnections();
    
    // Audio settings
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
}; 