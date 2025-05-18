#include "TruePeakLimiterProcessor.h"

namespace auralis
{
    TruePeakLimiterProcessor::TruePeakLimiterProcessor()
        : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    {
    }

    void TruePeakLimiterProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = getTotalNumInputChannels();

        limiter.prepare (spec);
        limiter.setRelease (100.0f);
    }

    void TruePeakLimiterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
    {
        limiter.setThreshold (ceiling.load());
        juce::dsp::AudioBlock<float> block (buffer);
        limiter.process (juce::dsp::ProcessContextReplacing<float>(block));
    }
} 