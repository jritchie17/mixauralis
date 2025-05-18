#include "GroupBusProcessor.h"

//==============================================================================
// GroupBusProcessor Implementation
//==============================================================================

GroupBusProcessor::GroupBusProcessor(BusType type)
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo())
                     .withOutput("Output", juce::AudioChannelSet::stereo())),
      busType(type)
{
    // Create the internal processors
    eqProcessor = std::make_unique<BusEQProcessor>();
    compProcessor = std::make_unique<BusGlueCompressorProcessor>();
    
    // Both are enabled by default
    eqEnabled = true;
    compEnabled = true;
}

GroupBusProcessor::~GroupBusProcessor()
{
}

void GroupBusProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    this->sampleRate = sampleRate;
    
    // Prepare internal processors
    eqProcessor->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
    compProcessor->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
    
    // Prepare gain processor
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = maximumExpectedSamplesPerBlock;
    spec.numChannels = 2;
    
    gainProcessor.prepare(spec);
    gainProcessor.setGainLinear(outputGain);
}

void GroupBusProcessor::releaseResources()
{
    eqProcessor->releaseResources();
    compProcessor->releaseResources();
}

void GroupBusProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Process through the chain: EQ -> Compressor -> Gain
    
    if (eqEnabled)
    {
        eqProcessor->processBlock(buffer, midiMessages);
    }
    
    if (compEnabled)
    {
        compProcessor->processBlock(buffer, midiMessages);
    }
    
    // Apply output gain
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    gainProcessor.process(context);
}

void GroupBusProcessor::setEQLowGain(float dB)
{
    if (eqProcessor)
    {
        eqProcessor->setLowGain(dB);
        
        // Debug output for verification
        DBG("Group Bus " + getBusName() + " - EQ Low Gain set to " + juce::String(dB) + " dB");
    }
}

void GroupBusProcessor::setEQMidGain(float dB)
{
    if (eqProcessor)
    {
        eqProcessor->setMidGain(dB);
        
        // Debug output for verification
        DBG("Group Bus " + getBusName() + " - EQ Mid Gain set to " + juce::String(dB) + " dB");
    }
}

void GroupBusProcessor::setEQHighGain(float dB)
{
    if (eqProcessor)
    {
        eqProcessor->setHighGain(dB);
        
        // Debug output for verification
        DBG("Group Bus " + getBusName() + " - EQ High Gain set to " + juce::String(dB) + " dB");
    }
}

void GroupBusProcessor::setCompEnabled(bool enabled)
{
    compEnabled = enabled;
    
    // Debug output for verification
    if (!enabled)
        DBG("Group Bus " + getBusName() + " - Comp bypassed");
    else
        DBG("Group Bus " + getBusName() + " - Comp enabled");
}

void GroupBusProcessor::setEQEnabled(bool enabled)
{
    eqEnabled = enabled;
    
    // Debug output for verification
    if (!enabled)
        DBG("Group Bus " + getBusName() + " - EQ bypassed");
    else
        DBG("Group Bus " + getBusName() + " - EQ enabled");
}

void GroupBusProcessor::setOutputGain(float gain)
{
    outputGain = gain;
    gainProcessor.setGainLinear(gain);
    
    // Debug output
    DBG("Group Bus " + getBusName() + " - Output gain set to " + juce::String(juce::Decibels::gainToDecibels(gain)) + " dB");
}

float GroupBusProcessor::getOutputGain() const
{
    return outputGain;
}

float GroupBusProcessor::getEQLowGain() const
{
    return eqProcessor ? eqProcessor->getLowGain() : 0.0f;
}

float GroupBusProcessor::getEQMidGain() const
{
    return eqProcessor ? eqProcessor->getMidGain() : 0.0f;
}

float GroupBusProcessor::getEQHighGain() const
{
    return eqProcessor ? eqProcessor->getHighGain() : 0.0f;
}

juce::String GroupBusProcessor::getBusName() const
{
    switch (busType)
    {
        case BusType::Vocals:      return "Vocals";
        case BusType::Instruments: return "Instruments";
        case BusType::Drums:       return "Drums";
        case BusType::Speech:      return "Speech";
        default:                   return "Unknown";
    }
}

//==============================================================================
// BusEQProcessor Implementation
//==============================================================================

BusEQProcessor::BusEQProcessor()
    : AudioProcessor(BusesProperties()
                    .withInput("Input", juce::AudioChannelSet::stereo())
                    .withOutput("Output", juce::AudioChannelSet::stereo()))
{
    // Initialize with default values
    updateFilters();
}

BusEQProcessor::~BusEQProcessor()
{
}

void BusEQProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    this->sampleRate = sampleRate;
    
    // Update the filters with the new sample rate
    updateFilters();
    
    // Prepare the processor chain
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = maximumExpectedSamplesPerBlock;
    spec.numChannels = 2;
    
    processorChain.prepare(spec);
}

void BusEQProcessor::releaseResources()
{
    // Nothing to do here
}

void BusEQProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    
    // Process through the entire chain
    processorChain.process(context);
}

void BusEQProcessor::setLowGain(float gainInDecibels)
{
    // Limit the gain range
    lowShelfGain = juce::jlimit(-12.0f, 12.0f, gainInDecibels);
    updateFilters();
}

void BusEQProcessor::setMidGain(float gainInDecibels)
{
    // Limit the gain range
    midPeakGain = juce::jlimit(-12.0f, 12.0f, gainInDecibels);
    updateFilters();
}

void BusEQProcessor::setHighGain(float gainInDecibels)
{
    // Limit the gain range
    highShelfGain = juce::jlimit(-12.0f, 12.0f, gainInDecibels);
    updateFilters();
}

void BusEQProcessor::updateFilters()
{
    // Create coefficients for low shelf filter (100 Hz)
    auto lowShelfCoefs = IIRCoefs::makeLowShelf(sampleRate, lowShelfFrequency, 0.707f, juce::Decibels::decibelsToGain(lowShelfGain));
    
    // Create coefficients for mid peak filter (900 Hz)
    auto midPeakCoefs = IIRCoefs::makePeakFilter(sampleRate, midPeakFrequency, midQ, juce::Decibels::decibelsToGain(midPeakGain));
    
    // Create coefficients for high shelf filter (8 kHz)
    auto highShelfCoefs = IIRCoefs::makeHighShelf(sampleRate, highShelfFrequency, 0.707f, juce::Decibels::decibelsToGain(highShelfGain));
    
    // Get the filters from the processor chain and update their coefficients
    auto& lowShelfL = processorChain.get<0>().get<0>();
    auto& lowShelfR = processorChain.get<0>().get<1>();
    auto& midPeakL = processorChain.get<1>().get<0>();
    auto& midPeakR = processorChain.get<1>().get<1>();
    auto& highShelfL = processorChain.get<2>().get<0>();
    auto& highShelfR = processorChain.get<2>().get<1>();
    
    // Update the coefficients for each filter
    *lowShelfL.coefficients = *lowShelfCoefs;
    *lowShelfR.coefficients = *lowShelfCoefs;
    *midPeakL.coefficients = *midPeakCoefs;
    *midPeakR.coefficients = *midPeakCoefs;
    *highShelfL.coefficients = *highShelfCoefs;
    *highShelfR.coefficients = *highShelfCoefs;
}

//==============================================================================
// BusGlueCompressorProcessor Implementation
//==============================================================================

BusGlueCompressorProcessor::BusGlueCompressorProcessor()
    : AudioProcessor(BusesProperties()
                    .withInput("Input", juce::AudioChannelSet::stereo())
                    .withOutput("Output", juce::AudioChannelSet::stereo()))
{
    // Initialize compressor with fixed glue settings (2:1 ratio, 10ms attack, 200ms release)
    compressor.setThreshold(thresholdInDb);
    compressor.setRatio(ratio);
    compressor.setAttack(attackMs);
    compressor.setRelease(releaseMs);
    
    // Set up makeup gain
    makeupGain.setGainDecibels(makeupGainDb);
}

BusGlueCompressorProcessor::~BusGlueCompressorProcessor()
{
}

void BusGlueCompressorProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    this->sampleRate = sampleRate;
    
    // Prepare the processors
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = maximumExpectedSamplesPerBlock;
    spec.numChannels = 2;
    
    compressor.prepare(spec);
    makeupGain.prepare(spec);
}

void BusGlueCompressorProcessor::releaseResources()
{
    // Nothing to do here
}

void BusGlueCompressorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Store a copy of the input levels before compression for gain reduction calculation
    float inputLevel = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        inputLevel += buffer.getMagnitude(channel, 0, buffer.getNumSamples());
    }
    inputLevel /= buffer.getNumChannels();
    
    // Process through compressor
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    compressor.process(context);
    
    // Measure gain reduction by comparing input and output levels
    float outputLevel = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        outputLevel += buffer.getMagnitude(channel, 0, buffer.getNumSamples());
    }
    outputLevel /= buffer.getNumChannels();
    
    if (inputLevel > 0.0f && outputLevel > 0.0f)
    {
        currentGainReduction = juce::Decibels::gainToDecibels(outputLevel / inputLevel);
        if (currentGainReduction > 0.0f)
            currentGainReduction = 0.0f; // Ensure gain reduction is negative or 0
    }
    else
    {
        currentGainReduction = 0.0f; // No gain reduction if levels are too low
    }
    
    // Apply makeup gain
    makeupGain.process(context);
}

float BusGlueCompressorProcessor::getGainReduction() const
{
    return currentGainReduction;
} 