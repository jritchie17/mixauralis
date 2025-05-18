#pragma once

#include <JuceHeader.h>

class CompressorProcessor : public juce::AudioProcessor
{
public:
    CompressorProcessor();
    ~CompressorProcessor() override;

    // AudioProcessor interface implementations
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    // State management
    const juce::String getName() const override { return "CompressorProcessor"; }
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
    void setThreshold(float thresholdInDb);
    void setRatio(float ratio);
    void setAttack(float attackInMs);
    void setRelease(float releaseInMs);
    void setMakeupGainAuto(bool isAuto);
    void setMakeupGain(float gainInDb);
    
    float getThreshold() const { return thresholdInDb; }
    float getRatio() const { return ratio; }
    float getAttack() const { return attackMs; }
    float getRelease() const { return releaseMs; }
    bool isMakeupGainAuto() const { return autoMakeupGain; }
    float getMakeupGain() const { return makeupGainDb; }

private:
    // Compressor parameters
    float thresholdInDb = -18.0f;  // Default threshold (dBFS)
    float ratio = 3.0f;            // Default ratio (3:1)
    float attackMs = 10.0f;        // Default attack (ms)
    float releaseMs = 150.0f;      // Default release (ms)
    float makeupGainDb = 0.0f;     // Default makeup gain (dB)
    bool autoMakeupGain = true;    // Default automatic makeup gain
    
    // Compressor DSP processor
    juce::dsp::Compressor<float> compressor;
    
    // Makeup gain processor for when auto is disabled
    juce::dsp::Gain<float> makeupGain;
    
    // For auto-makeup calculation
    void updateAutoMakeupGain();
    float calculateAutoMakeupGain(float thresholdInDb, float ratio);
    
    double sampleRate = 44100.0;
}; 