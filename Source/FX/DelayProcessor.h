#pragma once

#include <JuceHeader.h>

class DelayProcessor : public juce::AudioProcessor
{
public:
    DelayProcessor();
    ~DelayProcessor() override;

    // AudioProcessor interface implementations
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    // State management
    const juce::String getName() const override { return "DelayProcessor"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

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
    void setDelayTimeMs(float delayMs);
    void setFeedback(float feedback);
    void setWetLevel(float wetLevel);
    
    float getDelayTimeMs() const { return delayTimeMs; }
    float getFeedback() const { return feedbackLevel; }
    float getWetLevel() const { return wetLevel; }

private:
    // DSP components
    static constexpr auto maxDelayTimeMs = 800.0f;
    using DelayLineType = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>;
    
    DelayLineType delayLine;
    float delayTimeMs = 350.0f;    // Default 350ms
    float feedbackLevel = 0.35f;   // Default 0.35 feedback
    float wetLevel = 0.20f;        // Default 0.20 wet mix
    
    double sampleRate = 44100.0;
    float lastInputSample[2] = {0.0f, 0.0f};  // For feedback calculation

    // Smoothed parameters for click-free modulation
    juce::SmoothedValue<float> delayTimeMsSmoothed;
    juce::SmoothedValue<float> feedbackSmoothed;
    juce::SmoothedValue<float> wetLevelSmoothed;
}; 