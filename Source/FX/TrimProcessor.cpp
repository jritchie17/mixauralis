#include "TrimProcessor.h"

TrimProcessor::TrimProcessor()
    : AudioProcessor (juce::AudioProcessor::BusesProperties()
                      .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // Initialize gainLinear to 1.0 (0 dB)
    gain.setGainLinear(gainLinear);
}

TrimProcessor::~TrimProcessor()
{
}

void TrimProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    // Prepare the DSP gain object
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = maximumExpectedSamplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    
    gain.prepare(spec);
    gain.setGainLinear(gainLinear);
}

void TrimProcessor::releaseResources()
{
    // Nothing to release
}

void TrimProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Process using the gain module
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    gain.process(context);
}

void TrimProcessor::setGainLinear(float newGain)
{
    gainLinear = newGain;
    gain.setGainLinear(gainLinear);
} 