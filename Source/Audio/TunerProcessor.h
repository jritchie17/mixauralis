#pragma once
#include <JuceHeader.h>

namespace auralis
{
    /**  Basic real-time pitch correction.
         This uses a very naive zero-crossing pitch detector
         and resamples the audio toward the nearest semitone.
         It is intentionally lightweight and not on par with
         professional solutions but improves upon the previous
         placeholder implementation. */
    class TunerProcessor : public juce::AudioProcessor
    {
    public:
        TunerProcessor();
        ~TunerProcessor() override = default;

        //==============================================================================
        const juce::String getName() const override { return "Tuner"; }
        void prepareToPlay (double sampleRate, int samplesPerBlock) override;
        void releaseResources() override {}
        bool isBusesLayoutSupported (const BusesLayout&) const override { return true; }
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
        //==============================================================================
        bool hasEditor() const override { return false; }
        juce::AudioProcessorEditor* createEditor() override { return nullptr; }
        double getTailLengthSeconds() const override { return 0.0; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram (int) override {}
        const juce::String getProgramName (int) override { return {}; }
        void changeProgramName (int, const juce::String&) override {}
        void getStateInformation (juce::MemoryBlock&) override {}
        void setStateInformation (const void*, int) override {}
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }

        //==============================================================================
        void setStrength (float s) noexcept { strength.store (juce::jlimit (0.0f, 1.0f, s)); }
        float getStrength() const noexcept  { return strength.load(); }

    private:
        std::atomic<float> strength { 0.5f };   // 0 = dry, 1 = full-tune
        juce::AudioBuffer<float> dryBuffer;     // Buffer for storing dry signal
        juce::AudioBuffer<float> tunedBuffer;   // Buffer for tuned signal
        std::vector<juce::LagrangeInterpolator> interpolators; // one per channel
        double currentSampleRate = 44100.0;

        float detectPitch (const float* data, int numSamples) const;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TunerProcessor)
    };
} 
