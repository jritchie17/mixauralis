#pragma once

#include <JuceHeader.h>

class ReverbProcessor : public juce::AudioProcessor
{
public:
    ReverbProcessor();
    ~ReverbProcessor() override;

    // AudioProcessor interface implementations
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    // State management
    const juce::String getName() const override { return "ReverbProcessor"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

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
    void setRoomSize(float size);
    void setDamping(float damping);
    void setWidth(float width);
    void setWetLevel(float level);
    
    float getRoomSize() const { return parameters.roomSize; }
    float getDamping() const { return parameters.damping; }
    float getWidth() const { return parameters.width; }
    float getWetLevel() const { return parameters.wetLevel; }

private:
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters parameters;
    double sampleRate = 44100.0;
    
    void updateParameters();
}; 