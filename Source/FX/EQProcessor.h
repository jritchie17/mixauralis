#pragma once

#include <JuceHeader.h>

class EQProcessor : public juce::AudioProcessor
{
public:
    EQProcessor();
    ~EQProcessor() override;

    // AudioProcessor interface implementations
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    // State management
    const juce::String getName() const override { return "EQProcessor"; }
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
    enum class Band
    {
        LowShelf,   // 80 Hz
        LowMid,     // 300 Hz
        HighMid,    // 3 kHz
        HighShelf   // 8 kHz
    };
    
    void setGain(Band band, float gainInDecibels);
    float getGain(Band band) const;
    
    // Update filter coefficients based on parameters
    void updateFilters();

private:
    // Filter parameters
    static constexpr float lowShelfFrequency = 80.0f;    // Hz
    static constexpr float lowMidFrequency = 300.0f;     // Hz
    static constexpr float highMidFrequency = 3000.0f;   // Hz
    static constexpr float highShelfFrequency = 8000.0f; // Hz
    
    // Gain parameters for each band (in dB, range: -12 to +12)
    float lowShelfGain = 0.0f;
    float lowMidGain = 0.0f;
    float highMidGain = 0.0f;
    float highShelfGain = 0.0f;
    
    // The Q factor for the mid bands
    static constexpr float midQ = 0.7f;
    
    // IIR Filters for stereo processing
    using IIRFilter = juce::dsp::IIR::Filter<float>;
    using IIRCoefs = juce::dsp::IIR::Coefficients<float>;
    
    // Create a stereo filter using two mono filters
    using StereoFilter = juce::dsp::ProcessorChain<IIRFilter, IIRFilter>;
    
    // Four bands of stereo filters: low shelf, low mid, high mid, high shelf
    juce::dsp::ProcessorChain<StereoFilter, StereoFilter, StereoFilter, StereoFilter> processorChain;
    
    // Sample rate for coefficient calculations
    double sampleRate = 44100.0;
}; 