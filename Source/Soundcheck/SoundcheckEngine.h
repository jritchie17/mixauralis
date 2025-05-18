#pragma once

#include <JuceHeader.h>
#include "../Audio/ChannelProcessor.h"
#include "ToneProfiles.h"
#include <chrono>

// Forward declare the callback class
class SoundcheckAudioCallback;

/**
 * SoundcheckEngine - Analyzes audio input and applies correction settings
 * 
 * Singleton class that handles audio analysis and automatic correction
 * for all channels during the soundcheck process.
 */
class SoundcheckEngine : private juce::Thread
{
public:
    // Singleton pattern
    static SoundcheckEngine& getInstance();
    
    // Delete copy and move constructors and assignment operators
    SoundcheckEngine(const SoundcheckEngine&) = delete;
    SoundcheckEngine& operator=(const SoundcheckEngine&) = delete;
    SoundcheckEngine(SoundcheckEngine&&) = delete;
    SoundcheckEngine& operator=(SoundcheckEngine&&) = delete;
    
    // Destructor
    ~SoundcheckEngine() override;
    
    // Public API for analysis data per channel
    struct ChannelAnalysis {
        float avgRMS = 0.0f;
        float peakLevel = 0.0f;
        float noiseFloor = 0.0f;
        juce::Array<float> measuredMagnitudes;  // 32-band ⅓-octave
        
        // Suggested correction values
        float trimGainSuggestion = 0.0f;
        float gateThresholdSuggestion = -50.0f;
        float eqGainSuggestions[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // For the 4 EQ bands
        float compressorRatioSuggestion = 1.0f;
        
        // Original values (for revert)
        float originalTrimGain = 0.0f;
        float originalGateThreshold = -50.0f;
        float originalEqGains[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float originalCompressorRatio = 1.0f;
    };
    
    // Public API
    void startCheck(int secondsPerChannel = 5);
    void stopCheck();
    bool isRunning() const;
    const ChannelAnalysis& getAnalysis(int channelIndex) const;
    
    // Apply/revert correction settings based on analysis
    void applyCorrections();
    void revertCorrections();
    
    // Set channel processors to control
    void setChannelProcessors(const std::vector<ChannelProcessor*>& processors);
    
    // Make the audio callback class a friend so it can call captureAudio
    friend class SoundcheckAudioCallback;
    
    // Audio capture method - called from the SoundcheckAudioCallback
    void captureAudio(const float* const* inputChannelData, int numChannels, int numSamples);
    
private:
    // Singleton constructor
    SoundcheckEngine();
    
    // Thread implementation
    void run() override;
    
    // Analysis states
    enum class State { idle, analysing, finished };
    State currentState = State::idle;
    
    // Analysis data
    std::vector<ChannelAnalysis> channelAnalyses;
    std::vector<ChannelProcessor*> channelProcessors;
    
    // Analysis parameters
    int analysisLengthSeconds = 5;
    int currentChannelIndex = 0;
    
    // FFT processing
    static constexpr int fftOrder = 11;          // 2^11 = 2048 points
    static constexpr int fftSize = 1 << fftOrder;
    std::unique_ptr<juce::dsp::FFT> fft;
    
    // Audio capture buffer (5 seconds at 48 kHz)
    static constexpr int maxBufferSize = 48000 * 5 * 2; // 5 seconds stereo at 48kHz
    juce::AudioBuffer<float> audioRingBuffer;
    int bufferWritePosition = 0;
    int samplesCollected = 0;
    
    // FFT analysis buffers
    juce::HeapBlock<float> fftTimeDomain;
    juce::HeapBlock<float> fftFrequencyDomain;
    juce::HeapBlock<float> windowBuffer;
    juce::Array<float> analyzedBands;
    
    // Performance monitoring
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
    
    // Helper methods
    void initializeAnalysis();
    void analyzeCurrentChannelBuffer();
    void calculateCorrections(int channelIndex);
    void backupOriginalSettings(int channelIndex);
    void mapFFTToThirdOctaveBands(const float* fftData, juce::Array<float>& bandMagnitudes);
};

// Callback class to capture audio for analysis
class SoundcheckAudioCallback : public juce::AudioIODeviceCallback
{
public:
    SoundcheckAudioCallback(SoundcheckEngine& engine) : soundcheckEngine(engine) {}
    
    void audioDeviceIOCallback(const float* const* inputChannelData,
                               int numInputChannels,
                               float* const* outputChannelData,
                               int numOutputChannels,
                               int numSamples)
    {
        // Pass the input data to the soundcheck engine
        if (soundcheckEngine.isRunning())
        {
            // Only capture if we're actually analyzing
            soundcheckEngine.captureAudio(inputChannelData, numInputChannels, numSamples);
        }
        
        // Pass through all input to output
        for (int channel = 0; channel < juce::jmin(numInputChannels, numOutputChannels); ++channel)
        {
            std::memcpy(outputChannelData[channel], inputChannelData[channel], sizeof(float) * numSamples);
        }
    }
    
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override {}
    void audioDeviceStopped() override {}
    
private:
    SoundcheckEngine& soundcheckEngine;
}; 