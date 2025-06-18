#pragma once

#include <JuceHeader.h>
#include "../UI/LoudnessMeterComponent.h"
#include "TruePeakLimiterProcessor.h"

// Forward declarations for internal processors
class MultibandCompressorProcessor;

enum class StreamTarget { YouTube, Facebook, Custom };

// Define LUFS target constants
static constexpr float kLUFS_Youtube = -14.0f;
static constexpr float kLUFS_Facebook = -16.0f;

class MasterBusProcessor : public juce::AudioProcessor
{
public:
    MasterBusProcessor();
    ~MasterBusProcessor() override;
    
    // AudioProcessor implementation
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    
    // Required AudioProcessor methods (with minimal implementation)
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    
    const juce::String getName() const override { return "MasterBusProcessor"; }
    
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    
    double getTailLengthSeconds() const override { return 0.0; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return {}; }
    void changeProgramName(int index, const juce::String& newName) override {}
    
    void getStateInformation(juce::MemoryBlock& destData) override {}
    void setStateInformation(const void* data, int sizeInBytes) override {}
    
    // Public setters for the master bus
    void setTargetLufs(float targetLUFS) noexcept;
    float getTargetLufs() const noexcept;
    void setCompressorEnabled(bool enabled);
    void setLimiterEnabled(bool enabled);
    void setStreamTarget(StreamTarget target);
    void setMeterTarget(auralis::LoudnessMeterComponent* m) { meter = m; }
    
    // Getters for enabled states
    bool isLimiterEnabled() const { return limiterEnabled; }
    bool isCompressorEnabled() const { return compressorEnabled; }
    
    /**
        Returns the LUFS loudness measured for the last processed audio block.

        The value is calculated using a simple K‑weighted RMS measurement via
        JUCE DSP filters and is updated each time `processBlock` is called.
    */
    float getCurrentLufs() const { return currentLufs; }
    
    // Get the current stream target
    StreamTarget getStreamTarget() const { return currentTarget; }
    
private:
    // Internal processors
    std::unique_ptr<MultibandCompressorProcessor> compressor;
    std::unique_ptr<auralis::TruePeakLimiterProcessor> limiter;
    auralis::LoudnessMeterComponent* meter { nullptr };

    // Filters used for K-weighted loudness measurement
    juce::dsp::IIR::Filter<float> hpFilters[2];
    juce::dsp::IIR::Filter<float> shelfFilters[2];
    juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> hpCoeffs;
    juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> shelfCoeffs;
    
    // Current state
    std::atomic<float> targetLufs{kLUFS_Youtube};
    float currentLufs = -18.0f; // Updated each block
    bool compressorEnabled = true;
    bool limiterEnabled = true;
    StreamTarget currentTarget = StreamTarget::YouTube;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterBusProcessor)
}; 