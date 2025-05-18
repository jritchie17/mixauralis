#include "EQProcessor.h"

EQProcessor::EQProcessor()
    : AudioProcessor (juce::AudioProcessor::BusesProperties()
                      .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // Initialize with default parameters
}

EQProcessor::~EQProcessor()
{
}

void EQProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    this->sampleRate = sampleRate;
    
    // Prepare the processor chain
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = maximumExpectedSamplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    
    processorChain.prepare(spec);
    
    // Initialize filter coefficients
    updateFilters();
}

void EQProcessor::releaseResources()
{
    // Nothing to release
}

void EQProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Process using the EQ processor chain
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    processorChain.process(context);
}

void EQProcessor::setGain(Band band, float gainInDecibels)
{
    // Clamp gain between -12 and +12 dB
    float clampedGain = juce::jlimit(-12.0f, 12.0f, gainInDecibels);
    
    // Update the appropriate gain parameter
    switch (band)
    {
        case Band::LowShelf:
            lowShelfGain = clampedGain;
            break;
        case Band::LowMid:
            lowMidGain = clampedGain;
            break;
        case Band::HighMid:
            highMidGain = clampedGain;
            break;
        case Band::HighShelf:
            highShelfGain = clampedGain;
            break;
    }
    
    // Update filter coefficients
    updateFilters();
}

float EQProcessor::getGain(Band band) const
{
    switch (band)
    {
        case Band::LowShelf:
            return lowShelfGain;
        case Band::LowMid:
            return lowMidGain;
        case Band::HighMid:
            return highMidGain;
        case Band::HighShelf:
            return highShelfGain;
        default:
            return 0.0f;
    }
}

void EQProcessor::updateFilters()
{
    // Skip if not initialized yet
    if (sampleRate <= 0)
        return;
        
    // Get references to each filter in the chain
    auto& lowShelfBand = processorChain.get<0>();
    auto& lowMidBand = processorChain.get<1>();
    auto& highMidBand = processorChain.get<2>();
    auto& highShelfBand = processorChain.get<3>();
    
    // Create coefficient objects
    auto lowShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate, lowShelfFrequency, midQ, juce::Decibels::decibelsToGain(lowShelfGain));
    
    auto lowMidCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, lowMidFrequency, midQ, juce::Decibels::decibelsToGain(lowMidGain));
    
    auto highMidCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, highMidFrequency, midQ, juce::Decibels::decibelsToGain(highMidGain));
    
    auto highShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, highShelfFrequency, midQ, juce::Decibels::decibelsToGain(highShelfGain));
        
    // Update the filters with new coefficients
    // Left channel (0)
    lowShelfBand.get<0>().coefficients = lowShelfCoeffs;
    lowMidBand.get<0>().coefficients = lowMidCoeffs;
    highMidBand.get<0>().coefficients = highMidCoeffs;
    highShelfBand.get<0>().coefficients = highShelfCoeffs;
    
    // Right channel (1)
    lowShelfBand.get<1>().coefficients = lowShelfCoeffs;
    lowMidBand.get<1>().coefficients = lowMidCoeffs;
    highMidBand.get<1>().coefficients = highMidCoeffs;
    highShelfBand.get<1>().coefficients = highShelfCoeffs;
} 