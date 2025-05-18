#pragma once
#include <JuceHeader.h>

namespace auralis
{
    /**  Simple brick-wall true-peak limiter (placeholder).
         In this header step we declare the interface only. */
    class TruePeakLimiterProcessor : public juce::AudioProcessor
    {
    public:
        TruePeakLimiterProcessor();
        ~TruePeakLimiterProcessor() override = default;

        //==============================================================================
        const juce::String getName() const override { return "TruePeakLimiter"; }
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
        bool isMidiEffect() const override { return false; }

        //==============================================================================
        void setCeiling (float dBFS) noexcept { ceiling.store (dBFS); }
        float getCeiling() const noexcept     { return ceiling.load(); }

    private:
        std::atomic<float> ceiling { -1.0f };  // default −1 dBFS
        juce::dsp::Limiter<float> limiter;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TruePeakLimiterProcessor)
    };
} 