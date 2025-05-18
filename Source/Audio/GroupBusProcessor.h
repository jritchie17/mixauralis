#pragma once

#include <JuceHeader.h>

// Forward declarations
class BusEQProcessor;
class BusGlueCompressorProcessor;

class GroupBusProcessor : public juce::AudioProcessor
{
public:
    enum class BusType
    {
        Vocals,
        Instruments,
        Drums,
        Speech
    };

    GroupBusProcessor(BusType type);
    ~GroupBusProcessor() override;

    // AudioProcessor interface implementations
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    // State management
    const juce::String getName() const override { return "GroupBusProcessor"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Programs
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return {}; }
    void changeProgramName(int index, const juce::String& newName) override {}

    // State handling
    void getStateInformation(juce::MemoryBlock& destData) override {}
    void setStateInformation(const void* data, int sizeInBytes) override {}

    // Group Bus Controls
    void setEQLowGain(float dB);
    void setEQMidGain(float dB);
    void setEQHighGain(float dB);
    void setCompEnabled(bool enabled);
    void setEQEnabled(bool enabled);
    
    // Getters for EQ and compressor settings
    float getEQLowGain() const;
    float getEQMidGain() const;
    float getEQHighGain() const;
    bool isEQEnabled() const { return eqEnabled; }
    bool isCompEnabled() const { return compEnabled; }
    
    // Output gain control
    void setOutputGain(float gain);
    float getOutputGain() const;
    
    // Access to the bus type
    BusType getBusType() const { return busType; }
    
    // Get the name of the bus
    juce::String getBusName() const;

private:
    BusType busType;
    
    // Internal processors
    std::unique_ptr<BusEQProcessor> eqProcessor;
    std::unique_ptr<BusGlueCompressorProcessor> compProcessor;
    
    // State
    bool eqEnabled = true;
    bool compEnabled = true;
    float outputGain = 1.0f;
    
    // Output gain processor
    juce::dsp::Gain<float> gainProcessor;
    
    double sampleRate = 44100.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GroupBusProcessor)
};

//==============================================================================
// Bus EQ Processor (3-band EQ: Low, Mid, High)
//==============================================================================

class BusEQProcessor : public juce::AudioProcessor
{
public:
    BusEQProcessor();
    ~BusEQProcessor() override;

    // AudioProcessor interface implementations
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    // State management
    const juce::String getName() const override { return "BusEQProcessor"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Programs
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return {}; }
    void changeProgramName(int index, const juce::String& newName) override {}

    // State handling
    void getStateInformation(juce::MemoryBlock& destData) override {}
    void setStateInformation(const void* data, int sizeInBytes) override {}

    // Parameter control methods
    void setLowGain(float gainInDecibels);
    void setMidGain(float gainInDecibels);
    void setHighGain(float gainInDecibels);
    
    float getLowGain() const { return lowShelfGain; }
    float getMidGain() const { return midPeakGain; }
    float getHighGain() const { return highShelfGain; }
    
    // Update filter coefficients based on parameters
    void updateFilters();

private:
    // Filter parameters
    static constexpr float lowShelfFrequency = 100.0f;  // Hz
    static constexpr float midPeakFrequency = 900.0f;   // Hz
    static constexpr float highShelfFrequency = 8000.0f; // Hz
    
    // Gain parameters for each band (in dB, range: -12 to +12)
    float lowShelfGain = 0.0f;
    float midPeakGain = 0.0f;
    float highShelfGain = 0.0f;
    
    // The Q factor for the mid peak
    static constexpr float midQ = 0.7f;
    
    // IIR Filters for stereo processing
    using IIRFilter = juce::dsp::IIR::Filter<float>;
    using IIRCoefs = juce::dsp::IIR::Coefficients<float>;
    
    // Create a stereo filter using two mono filters
    using StereoFilter = juce::dsp::ProcessorChain<IIRFilter, IIRFilter>;
    
    // Three bands of stereo filters: low shelf, mid peak, high shelf
    juce::dsp::ProcessorChain<StereoFilter, StereoFilter, StereoFilter> processorChain;
    
    // Sample rate for coefficient calculations
    double sampleRate = 44100.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BusEQProcessor)
};

//==============================================================================
// Bus Glue Compressor Processor
//==============================================================================

class BusGlueCompressorProcessor : public juce::AudioProcessor
{
public:
    BusGlueCompressorProcessor();
    ~BusGlueCompressorProcessor() override;

    // AudioProcessor interface implementations
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    // State management
    const juce::String getName() const override { return "BusGlueCompressorProcessor"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Programs
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return {}; }
    void changeProgramName(int index, const juce::String& newName) override {}

    // State handling
    void getStateInformation(juce::MemoryBlock& destData) override {}
    void setStateInformation(const void* data, int sizeInBytes) override {}

    // Parameter access (read-only as these are fixed for bus glue compression)
    float getThreshold() const { return thresholdInDb; }
    float getRatio() const { return ratio; }
    float getAttack() const { return attackMs; }
    float getRelease() const { return releaseMs; }
    float getGainReduction() const; // Returns current gain reduction in dB

private:
    // Fixed compressor parameters for bus glue
    float thresholdInDb = -20.0f;  // Threshold (dBFS)
    float ratio = 2.0f;            // Ratio (2:1)
    float attackMs = 10.0f;        // Attack (ms)
    float releaseMs = 200.0f;      // Release (ms)
    float makeupGainDb = 1.0f;     // Small makeup gain (dB)
    
    // Compressor DSP processor
    juce::dsp::Compressor<float> compressor;
    
    // Makeup gain processor
    juce::dsp::Gain<float> makeupGain;
    
    // To store the current gain reduction amount
    float currentGainReduction = 0.0f;
    
    double sampleRate = 44100.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BusGlueCompressorProcessor)
}; 