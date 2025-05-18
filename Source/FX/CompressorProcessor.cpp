#include "CompressorProcessor.h"

CompressorProcessor::CompressorProcessor()
    : AudioProcessor (juce::AudioProcessor::BusesProperties()
                      .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // Initialize compressor with default parameters
    compressor.setThreshold(thresholdInDb);
    compressor.setRatio(ratio);
    compressor.setAttack(attackMs);
    compressor.setRelease(releaseMs);
    
    // Calculate initial auto makeup gain
    if (autoMakeupGain)
    {
        makeupGainDb = calculateAutoMakeupGain(thresholdInDb, ratio);
        makeupGain.setGainDecibels(makeupGainDb);
    }
}

CompressorProcessor::~CompressorProcessor()
{
}

void CompressorProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    this->sampleRate = sampleRate;
    
    // Prepare the DSP processors
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = maximumExpectedSamplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    
    compressor.prepare(spec);
    makeupGain.prepare(spec);
    
    // Update parameters that might depend on sample rate
    compressor.setAttack(attackMs);
    compressor.setRelease(releaseMs);
}

void CompressorProcessor::releaseResources()
{
    // Nothing to release
}

void CompressorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Process audio through the compressor
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    
    // Apply compression
    compressor.process(context);
    
    // Apply makeup gain
    makeupGain.process(context);
}

void CompressorProcessor::setThreshold(float thresholdInDb)
{
    this->thresholdInDb = thresholdInDb;
    compressor.setThreshold(thresholdInDb);
    
    // Update auto makeup gain if enabled
    if (autoMakeupGain)
    {
        updateAutoMakeupGain();
    }
}

void CompressorProcessor::setRatio(float ratio)
{
    this->ratio = ratio;
    compressor.setRatio(ratio);
    
    // Update auto makeup gain if enabled
    if (autoMakeupGain)
    {
        updateAutoMakeupGain();
    }
}

void CompressorProcessor::setAttack(float attackInMs)
{
    this->attackMs = attackInMs;
    compressor.setAttack(attackInMs);
}

void CompressorProcessor::setRelease(float releaseInMs)
{
    this->releaseMs = releaseInMs;
    compressor.setRelease(releaseInMs);
}

void CompressorProcessor::setMakeupGainAuto(bool isAuto)
{
    autoMakeupGain = isAuto;
    
    if (autoMakeupGain)
    {
        // If auto is enabled, calculate and apply the makeup gain
        updateAutoMakeupGain();
    }
}

void CompressorProcessor::setMakeupGain(float gainInDb)
{
    if (!autoMakeupGain)
    {
        // Only apply manual makeup gain if auto is disabled
        makeupGainDb = gainInDb;
        makeupGain.setGainDecibels(makeupGainDb);
    }
}

void CompressorProcessor::updateAutoMakeupGain()
{
    makeupGainDb = calculateAutoMakeupGain(thresholdInDb, ratio);
    makeupGain.setGainDecibels(makeupGainDb);
}

float CompressorProcessor::calculateAutoMakeupGain(float thresholdInDb, float ratio)
{
    // A simple formula for auto makeup gain:
    // Assumes you want to compensate for gain reduction at the threshold.
    // As ratio increases, more makeup gain is needed.
    // This is a common formula: gain = -threshold * (1 - 1/ratio)
    
    return -thresholdInDb * (1.0f - (1.0f / ratio));
} 