#pragma once

#include <JuceHeader.h>

class GateProcessor : public juce::AudioProcessor
{
public:
    GateProcessor();
    ~GateProcessor() override;

    // AudioProcessor interface implementations
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    // State management
    const juce::String getName() const override { return "GateProcessor"; }
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
    void setRatio(float newRatio);
    void setAttack(float attackInMs);
    void setRelease(float releaseInMs);
    
    float getThreshold() const { return thresholdInDb; }
    float getRatio() const { return ratio; }
    float getAttack() const { return attackMs; }
    float getRelease() const { return releaseMs; }

private:
    // Gate parameters
    float thresholdInDb = -50.0f;  // Default threshold in dBFS
    float ratio = 2.0f;            // Default ratio (2:1)
    float attackMs = 5.0f;         // Default attack time in milliseconds
    float releaseMs = 50.0f;       // Default release time in milliseconds
    
    float levelEnvelopePerChannel[2] = { 0.0f, 0.0f };  // Envelope followers for each channel
    double sampleRate = 44100.0;
    
    // Coefficient calculations
    float calculateAttackCoefficient(float attackTimeMs, double sampleRate);
    float calculateReleaseCoefficient(float releaseTimeMs, double sampleRate);
    
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    
    // Process helpers
    float applyEnvelope(float input, int channel);
    float calculateGain(float rmsLevel);
}; 