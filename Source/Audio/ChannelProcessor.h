#pragma once

#include <JuceHeader.h>
#include "../FX/TrimProcessor.h"
#include "../FX/GateProcessor.h"
#include "../FX/EQProcessor.h"
#include "../FX/CompressorProcessor.h"
#include "TunerProcessor.h"
#include "FXBusProcessor.h"

// Forward declaration to avoid circular dependency
class AudioEngine;

class ChannelProcessor
{
public:
    // Channel type enum
    enum class ChannelType
    {
        Vocal,
        Instrument,
        Drums,
        Other
    };
    
    ChannelProcessor();
    ChannelProcessor(int index, ChannelType type = ChannelType::Other, AudioEngine* engine = nullptr);
    ~ChannelProcessor();
    
    // Audio processing methods
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void releaseResources();
    
    // Parameter control methods
    void setTrimGain(float gainInDecibels);
    void setGateEnabled(bool enabled);
    void setCompressorEnabled(bool enabled);
    void setEqEnabled(bool enabled);
    void setFxSendLevel(float level); // 0.0 to 1.0
    void setTunerEnabled(bool enabled);
    void setTunerStrength(float strength); // 0.0 to 1.0
    void setMuted(bool shouldBeMuted) { muted = shouldBeMuted; }
    void setSolo(bool shouldBeSolo) { solo = shouldBeSolo; }
    
    // New methods for soundcheck corrections
    void setGateThreshold(float thresholdInDb);
    void setEQBandGain(EQProcessor::Band band, float gainInDb);
    void setCompressorRatio(float ratio);
    void setCompressorThreshold(float thresholdInDb);
    
    // Get methods for parameter values
    float getTrimGain() const { return trimGainDecibels; }
    bool isGateEnabled() const { return !gateBypass; }
    bool isCompressorEnabled() const { return !compressorBypass; }
    bool isEqEnabled() const { return !eqBypass; }
    float getFxSendLevel() const { return fxSendLevel; }
    bool isTunerEnabled() const { return !tunerBypass; }
    float getTunerStrength() const { return tuner ? tuner->getStrength() : 0.0f; }
    bool isMuted() const { return muted; }
    bool isSolo() const { return solo; }
    
    // New get methods for soundcheck parameters
    float getGateThreshold() const;
    float getEQBandGain(EQProcessor::Band band) const;
    float getCompressorRatio() const;
    float getCompressorThreshold() const;
    
    // Access methods for channel index and type
    int getChannelIndex() const { return channelIndex; }
    ChannelType getChannelType() const { return channelType; }
    AudioEngine* getAudioEngine() const { return audioEngine; }
    
    // Set methods for channel properties
    void setChannelIndex(int index) { channelIndex = index; }
    void setChannelType(ChannelType type) { channelType = type; }
    void setAudioEngine(AudioEngine* engine) { audioEngine = engine; }
    void setFxBusProcessor(FXBusProcessor* bus) { fxSendBus = bus; }

private:
    // Channel identification
    int channelIndex = -1;
    ChannelType channelType = ChannelType::Other;
    AudioEngine* audioEngine = nullptr;
    
    // Audio processor parameters
    float trimGainDecibels = 0.0f;
    bool gateBypass = false;
    bool compressorBypass = false;
    bool eqBypass = false;
    bool tunerBypass = false;
    float fxSendLevel = 0.0f;
    bool muted = false;
    bool solo = false;
    
    // Individual processors - these are pointers to processors owned by the graph
    TrimProcessor* trim = nullptr;
    GateProcessor* gate = nullptr;
    EQProcessor* eq = nullptr;
    CompressorProcessor* comp = nullptr;
    FXBusProcessor* fxSendBus = nullptr;  // Assigned by RoutingManager
    auralis::TunerProcessor* tuner = nullptr;          // Owned by the graph
    
    // AudioProcessorGraph for connecting the processors
    std::unique_ptr<juce::AudioProcessorGraph> processorGraph;
    juce::AudioProcessorGraph::Node::Ptr inputNode;
    juce::AudioProcessorGraph::Node::Ptr outputNode;
    juce::AudioProcessorGraph::Node::Ptr trimNode;
    juce::AudioProcessorGraph::Node::Ptr gateNode;
    juce::AudioProcessorGraph::Node::Ptr eqNode;
    juce::AudioProcessorGraph::Node::Ptr compNode;
    juce::AudioProcessorGraph::Node::Ptr fxSendNode;
    juce::AudioProcessorGraph::Node::Ptr tunerNode;
    
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    
    // Helper methods
    void createGraph();
    void connectNodes();
    void updateNodeBypass(juce::AudioProcessorGraph::Node::Ptr node, bool bypass);
}; 