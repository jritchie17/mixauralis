#pragma once
#include <JuceHeader.h>

namespace auralis
{
    /**  Displays short-term LUFS, true-peak in dBFS, and a bar meter. */
    class LoudnessMeterComponent : public juce::Component,
                                   private juce::Timer
    {
    public:
        LoudnessMeterComponent();
        ~LoudnessMeterComponent() override = default;

        /** Call once per audio block from the master bus. */
        void pushSamples (const juce::AudioBuffer<float>& buffer);

        void paint    (juce::Graphics&) override;
        void resized  () override {}

    private:
        void timerCallback() override;   // UI refresh 10 Hz

        // Ring buffer for short-term LUFS calc (400 ms window)
        juce::AudioBuffer<float> lufsBuffer;
        int   lufsWritePos   = 0;
        float currentLufs    = -100.0f;
        float currentTruePk  = -100.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoudnessMeterComponent)
    };
} 