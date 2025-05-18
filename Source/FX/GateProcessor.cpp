#include "GateProcessor.h"

GateProcessor::GateProcessor()
    : AudioProcessor (juce::AudioProcessor::BusesProperties()
                      .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // Initialize attack and release coefficients
    attackCoeff = calculateAttackCoefficient(attackMs, sampleRate);
    releaseCoeff = calculateReleaseCoefficient(releaseMs, sampleRate);
}

GateProcessor::~GateProcessor()
{
}

void GateProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    this->sampleRate = sampleRate;
    
    // Recalculate coefficients with the new sample rate
    attackCoeff = calculateAttackCoefficient(attackMs, sampleRate);
    releaseCoeff = calculateReleaseCoefficient(releaseMs, sampleRate);
    
    // Reset envelopes
    levelEnvelopePerChannel[0] = 0.0f;
    levelEnvelopePerChannel[1] = 0.0f;
}

void GateProcessor::releaseResources()
{
    // Nothing to release
}

void GateProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Process each channel independently
    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);
        
        // Process each sample
        for (int i = 0; i < numSamples; ++i)
        {
            // Apply envelope follower to get the RMS level
            float rmsLevel = applyEnvelope(channelData[i] * channelData[i], channel);
            
            // Calculate the gain based on the RMS level and gate settings
            float gain = calculateGain(rmsLevel);
            
            // Apply the gain to the sample
            channelData[i] *= gain;
        }
    }
}

float GateProcessor::applyEnvelope(float inputSquared, int channel)
{
    // Apply envelope follower to track the input signal level
    float& envelope = levelEnvelopePerChannel[channel];
    
    // Choose attack or release coefficient based on whether input is above current envelope
    if (inputSquared > envelope)
    {
        // Signal is rising, use attack time
        envelope = attackCoeff * (envelope - inputSquared) + inputSquared;
    }
    else
    {
        // Signal is falling, use release time
        envelope = releaseCoeff * (envelope - inputSquared) + inputSquared;
    }
    
    // Return RMS value
    return std::sqrt(envelope);
}

float GateProcessor::calculateGain(float rmsLevel)
{
    // Convert threshold from dB to linear
    float thresholdLinear = juce::Decibels::decibelsToGain(thresholdInDb);
    
    // Calculate the gain to apply
    if (rmsLevel < thresholdLinear)
    {
        // Below threshold - apply downward expansion/gate
        float dbBelow = juce::Decibels::gainToDecibels(rmsLevel) - thresholdInDb;
        float gainDb = dbBelow * (1.0f - (1.0f / ratio));
        
        // Convert back to linear gain
        return juce::Decibels::decibelsToGain(gainDb);
    }
    else
    {
        // Above threshold - pass signal unchanged
        return 1.0f;
    }
}

float GateProcessor::calculateAttackCoefficient(float attackTimeMs, double sampleRate)
{
    // Convert attack time from ms to seconds
    float attackTimeSec = attackTimeMs / 1000.0f;
    
    // Calculate coefficient for a smoothed attack
    return std::exp(-1.0f / (sampleRate * attackTimeSec));
}

float GateProcessor::calculateReleaseCoefficient(float releaseTimeMs, double sampleRate)
{
    // Convert release time from ms to seconds
    float releaseTimeSec = releaseTimeMs / 1000.0f;
    
    // Calculate coefficient for a smoothed release
    return std::exp(-1.0f / (sampleRate * releaseTimeSec));
}

void GateProcessor::setThreshold(float thresholdInDb)
{
    this->thresholdInDb = thresholdInDb;
}

void GateProcessor::setRatio(float newRatio)
{
    this->ratio = newRatio;
}

void GateProcessor::setAttack(float attackInMs)
{
    this->attackMs = attackInMs;
    attackCoeff = calculateAttackCoefficient(attackMs, sampleRate);
}

void GateProcessor::setRelease(float releaseInMs)
{
    this->releaseMs = releaseInMs;
    releaseCoeff = calculateReleaseCoefficient(releaseMs, sampleRate);
} 