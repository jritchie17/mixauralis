#include "ReverbProcessor.h"

ReverbProcessor::ReverbProcessor()
    : AudioProcessor (juce::AudioProcessor::BusesProperties()
                     .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // Initialize reverb with default parameters
    parameters.roomSize = 0.5f;
    parameters.damping = 0.4f;
    parameters.width = 1.0f;
    parameters.wetLevel = 0.25f;
    parameters.dryLevel = 1.0f; // Always pass through dry signal
    
    // Apply parameters to the reverb processor
    reverb.setParameters(parameters);
}

ReverbProcessor::~ReverbProcessor()
{
}

void ReverbProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    this->sampleRate = sampleRate;
    
    // Prepare the DSP processor
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = maximumExpectedSamplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    
    reverb.prepare(spec);

    roomSizeSmoothed.reset(sampleRate, 0.05);
    roomSizeSmoothed.setCurrentAndTargetValue(parameters.roomSize);
    dampingSmoothed.reset(sampleRate, 0.05);
    dampingSmoothed.setCurrentAndTargetValue(parameters.damping);
    widthSmoothed.reset(sampleRate, 0.05);
    widthSmoothed.setCurrentAndTargetValue(parameters.width);
    wetLevelSmoothed.reset(sampleRate, 0.05);
    wetLevelSmoothed.setCurrentAndTargetValue(parameters.wetLevel);

    // Apply current parameters
    updateParameters();
}

void ReverbProcessor::releaseResources()
{
    // Nothing specific to release
}

void ReverbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    
    // Update smoothed parameters for this block
    roomSizeSmoothed.skip(numSamples);
    dampingSmoothed.skip(numSamples);
    widthSmoothed.skip(numSamples);
    wetLevelSmoothed.skip(numSamples);

    parameters.roomSize = roomSizeSmoothed.getCurrentValue();
    parameters.damping = dampingSmoothed.getCurrentValue();
    parameters.width = widthSmoothed.getCurrentValue();
    parameters.wetLevel = wetLevelSmoothed.getCurrentValue();

    updateParameters();

    // Process audio through the reverb
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    reverb.process(context);
}

void ReverbProcessor::setRoomSize(float size)
{
    parameters.roomSize = juce::jlimit(0.0f, 1.0f, size);
    roomSizeSmoothed.setTargetValue(parameters.roomSize);
}

void ReverbProcessor::setDamping(float damping)
{
    parameters.damping = juce::jlimit(0.0f, 1.0f, damping);
    dampingSmoothed.setTargetValue(parameters.damping);
}

void ReverbProcessor::setWidth(float width)
{
    parameters.width = juce::jlimit(0.0f, 1.0f, width);
    widthSmoothed.setTargetValue(parameters.width);
}

void ReverbProcessor::setWetLevel(float level)
{
    parameters.wetLevel = juce::jlimit(0.0f, 1.0f, level);
    wetLevelSmoothed.setTargetValue(parameters.wetLevel);
}

void ReverbProcessor::updateParameters()
{
    reverb.setParameters(parameters);
} 