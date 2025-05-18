#include "DelayProcessor.h"

DelayProcessor::DelayProcessor()
    : AudioProcessor (juce::AudioProcessor::BusesProperties()
                     .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // Initialize delay line
    delayLine.setMaximumDelayInSamples(static_cast<int>(maxDelayTimeMs * 48.0f)); // Support up to 48kHz
}

DelayProcessor::~DelayProcessor()
{
}

void DelayProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    this->sampleRate = sampleRate;
    
    // Reset delay state
    lastInputSample[0] = 0.0f;
    lastInputSample[1] = 0.0f;
    
    // Prepare the delay line
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = maximumExpectedSamplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    
    delayLine.prepare(spec);
    
    // Set the initial delay time
    delayLine.setDelay(delayTimeMs * 0.001f * sampleRate);
}

void DelayProcessor::releaseResources()
{
    // Nothing specific to release
}

void DelayProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Make a dry copy for later mixing
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);
    
    // Process each channel
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Get the current input sample
            float inputSample = channelData[sample];
            
            // Read from the delay line
            float delaySample = delayLine.popSample(channel);
            
            // Calculate the output sample with feedback
            float outputSample = inputSample + (delaySample * feedbackLevel);
            
            // Push the output sample back into the delay line
            delayLine.pushSample(channel, outputSample);
            
            // Mix the delayed signal with the original signal
            channelData[sample] = dryBuffer.getSample(channel, sample) + (delaySample * wetLevel);
            
            // Store the input sample for the next iteration
            lastInputSample[channel] = inputSample;
        }
    }
}

void DelayProcessor::setDelayTimeMs(float delayMs)
{
    delayTimeMs = juce::jlimit(10.0f, maxDelayTimeMs, delayMs);
    
    // Update the delay time in the delay line
    if (sampleRate > 0)
        delayLine.setDelay(delayTimeMs * 0.001f * sampleRate);
}

void DelayProcessor::setFeedback(float feedback)
{
    feedbackLevel = juce::jlimit(0.0f, 0.9f, feedback);
}

void DelayProcessor::setWetLevel(float wet)
{
    wetLevel = juce::jlimit(0.0f, 1.0f, wet);
} 