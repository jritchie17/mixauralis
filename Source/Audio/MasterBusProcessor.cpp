#include "MasterBusProcessor.h"

//==============================================================================
// Internal multiband compressor used by the master bus
//==============================================================================
class MultibandCompressorProcessor
{
public:
    MultibandCompressorProcessor()
    {
        juce::Logger::writeToLog("MultibandCompressorProcessor constructor");
    }
    
    void prepare(double sampleRate, int maximumBlockSize)
    {
        juce::Logger::writeToLog("MultibandCompressorProcessor::prepare: start");
        
        // Update filters and prepare DSP objects
        updateFilters(sampleRate);

        juce::Logger::writeToLog("Preparing compressors and filters");
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maximumBlockSize);
        spec.numChannels = 2;

        // Sanity check - this processor only supports up to stereo
        jassert(spec.numChannels <= 2);
        if (spec.numChannels > 2)
            spec.numChannels = 2;

        // Prepare filter banks
        for (auto& filter : lowFilters)
            filter.prepare(spec);
        for (auto& filter : highFilters)
            filter.prepare(spec);
        for (auto& filter : midLowFilters)
            filter.prepare(spec);
        for (auto& filter : midHighFilters)
            filter.prepare(spec);

        // Prepare compressors
        for (auto& comp : compressors)
            comp.prepare(spec);
        
        juce::Logger::writeToLog("MultibandCompressorProcessor::prepare: end");
    }
    
    void process(juce::AudioBuffer<float>& buffer)
    {
        juce::Logger::writeToLog("MultibandCompressorProcessor::process: start");
        
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        // Only stereo is supported
        jassert(numChannels <= 2);
        if (numChannels > 2)
            return;
        
        juce::Logger::writeToLog("numChannels: " + juce::String(numChannels) + ", numSamples: " + juce::String(numSamples));
        
        // Create temporary buffers for each band
        juce::Logger::writeToLog("Allocating band buffers");
        juce::AudioBuffer<float> lowBand(numChannels, numSamples);
        juce::AudioBuffer<float> midBand(numChannels, numSamples);
        juce::AudioBuffer<float> highBand(numChannels, numSamples);
        
        // Copy input to all bands
        juce::Logger::writeToLog("Copying input to band buffers");
        for (int ch = 0; ch < numChannels; ++ch)
        {
            lowBand.copyFrom(ch, 0, buffer, ch, 0, numSamples);
            midBand.copyFrom(ch, 0, buffer, ch, 0, numSamples);
            highBand.copyFrom(ch, 0, buffer, ch, 0, numSamples);
        }
        
        // Process each channel
        juce::Logger::writeToLog("Starting band split loop");
        for (int ch = 0; ch < numChannels; ++ch)
        {
            juce::Logger::writeToLog("Processing channel: " + juce::String(ch));
            
            // Process filters
            juce::Logger::writeToLog("Processing filters");
            
            // Process low band
            juce::Logger::writeToLog("Processing low filter");
            juce::dsp::AudioBlock<float> lowBlock(lowBand);
            auto lowSingle = lowBlock.getSingleChannelBlock((size_t)ch);
            juce::dsp::ProcessContextReplacing<float> lowContext(lowSingle);
            lowFilters[ch].process(lowContext);
            
            // Process high band
            juce::Logger::writeToLog("Processing high filter");
            juce::dsp::AudioBlock<float> highBlock(highBand);
            auto highSingle = highBlock.getSingleChannelBlock((size_t)ch);
            juce::dsp::ProcessContextReplacing<float> highContext(highSingle);
            highFilters[ch].process(highContext);
            
            // Process mid band
            juce::Logger::writeToLog("Processing mid filters");
            juce::dsp::AudioBlock<float> midBlock(midBand);
            auto midSingle = midBlock.getSingleChannelBlock((size_t)ch);
            juce::dsp::ProcessContextReplacing<float> midContext(midSingle);
            midHighFilters[ch].process(midContext);
            midLowFilters[ch].process(midContext);
        }
        
        // Process each band with its compressor
        juce::Logger::writeToLog("Processing compressors");
        processBand(lowBand, compressors[0]);
        processBand(midBand, compressors[1]);
        processBand(highBand, compressors[2]);
        
        // Mix bands back together
        juce::Logger::writeToLog("Mixing bands");
        buffer.clear();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.addFrom(ch, 0, lowBand,  ch, 0, numSamples);
            buffer.addFrom(ch, 0, midBand,  ch, 0, numSamples);
            buffer.addFrom(ch, 0, highBand, ch, 0, numSamples);
        }
        
        juce::Logger::writeToLog("MultibandCompressorProcessor::process: end");
    }

    void reset()
    {
        for (auto& comp : compressors)
            comp.reset();

        for (auto& filter : lowFilters)
            filter.reset();
        for (auto& filter : highFilters)
            filter.reset();
        for (auto& filter : midLowFilters)
            filter.reset();
        for (auto& filter : midHighFilters)
            filter.reset();
    }

    void setEnabled (bool enabled) noexcept  { isEnabled = enabled; }

private:
    void updateFilters(double sampleRate)
    {
        juce::Logger::writeToLog("updateFilters: Creating filter coefficients");
        lowLP  = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, lowCrossoverHz);
        highHP = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, highCrossoverHz);
        midHP  = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, lowCrossoverHz);
        midLP  = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, highCrossoverHz);

        bool coeffsValid = true;
        if (lowLP == nullptr)
        {
            juce::Logger::writeToLog("updateFilters error: lowLP is null");
            coeffsValid = false;
        }

        if (highHP == nullptr)
        {
            juce::Logger::writeToLog("updateFilters error: highHP is null");
            coeffsValid = false;
        }

        if (midHP == nullptr)
        {
            juce::Logger::writeToLog("updateFilters error: midHP is null");
            coeffsValid = false;
        }

        if (midLP == nullptr)
        {
            juce::Logger::writeToLog("updateFilters error: midLP is null");
            coeffsValid = false;
        }

        if (! coeffsValid)
        {
            juce::Logger::writeToLog("updateFilters: failed to create filter coefficients");
            return;
        }

        jassert(lowLP  != nullptr);
        jassert(highHP != nullptr);
        jassert(midHP  != nullptr);
        jassert(midLP  != nullptr);

        juce::Logger::writeToLog("updateFilters: Assigning coefficients to filters");
        for (int ch = 0; ch < 2; ++ch)
        {
            lowFilters[ch].state = *lowLP;
            highFilters[ch].state = *highHP;
            midLowFilters[ch].state = *midLP;
            midHighFilters[ch].state = *midHP;
            lowFilters[ch].reset();
            highFilters[ch].reset();
            midLowFilters[ch].reset();
            midHighFilters[ch].reset();
        }
        juce::Logger::writeToLog("Low/high crossover: "
            + juce::String(lowCrossoverHz) + " / "
            + juce::String(highCrossoverHz));
        juce::Logger::writeToLog("updateFilters: initialization complete");
        juce::Logger::writeToLog("updateFilters: end");
    }

    static void processBand (juce::AudioBuffer<float>& bandBuffer, juce::dsp::Compressor<float>& comp)
    {
        juce::dsp::AudioBlock<float> block (bandBuffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        comp.process (context);
    }

    juce::dsp::ProcessSpec spec;
    static constexpr float lowCrossoverHz  = 200.0f;
    static constexpr float highCrossoverHz = 2000.0f;
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    Filter lowFilters [2];
    Filter highFilters[2];
    Filter midLowFilters [2];
    Filter midHighFilters[2];
    juce::dsp::Compressor<float> compressors[3];
    bool isEnabled { true };
    juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> lowLP;
    juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> highHP;
    juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> midHP;
    juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> midLP;
};

//==============================================================================
MasterBusProcessor::MasterBusProcessor()
    : AudioProcessor(BusesProperties()
                    .withInput("Input", juce::AudioChannelSet::stereo(), true)
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    compressor = std::make_unique<MultibandCompressorProcessor>();
    limiter = std::make_unique<auralis::TruePeakLimiterProcessor>();
    
    juce::Logger::writeToLog("MasterBusProcessor initialized with LUFS target: " + juce::String(targetLufs));
}

MasterBusProcessor::~MasterBusProcessor()
{
    // No need to explicitly delete managed resources
}

void MasterBusProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    // Prepare internal processors
    compressor->prepare(sampleRate, maximumExpectedSamplesPerBlock);
    limiter->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);

    auto hp  = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 40.0f);
    auto sh  = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 4000.0f,
                                                                  0.7071f,
                                                                  juce::Decibels::decibelsToGain(4.0f));
    for (int ch = 0; ch < 2; ++ch)
    {
        hpFilters[ch].state = *hp;
        shelfFilters[ch].state = *sh;
        hpFilters[ch].reset();
        shelfFilters[ch].reset();
    }

    juce::Logger::writeToLog("MasterBusProcessor prepared with sample rate: " + juce::String(sampleRate));
}

void MasterBusProcessor::releaseResources()
{
    // Reset internal processors
    compressor->reset();
    limiter->releaseResources();
}

void MasterBusProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::Logger::writeToLog("MasterBusProcessor::processBlock: start");
    juce::ScopedNoDenormals noDenormals;
    
    // Process through compressor if enabled
    if (compressorEnabled)
        compressor->process(buffer);
    
    // Process through limiter if enabled
    if (limiterEnabled)
        limiter->processBlock(buffer, midiMessages);
    
    // Push samples to meter if available
    if (meter != nullptr)
        meter->pushSamples(buffer);
    
    // Calculate K-weighted loudness (LUFS)
    juce::AudioBuffer<float> temp (buffer);
    for (int ch = 0; ch < temp.getNumChannels(); ++ch)
    {
        float* channelPtr = temp.getWritePointer(ch);
        float* channels[] = { channelPtr };
        juce::dsp::AudioBlock<float> block(channels, 1, temp.getNumSamples());
        juce::dsp::ProcessContextReplacing<float> context(block);
        hpFilters[ch].process(context);
        shelfFilters[ch].process(context);
    }

    double sumSquares = 0.0;
    for (int ch = 0; ch < temp.getNumChannels(); ++ch)
    {
        const float* data = temp.getReadPointer(ch);
        for (int i = 0; i < temp.getNumSamples(); ++i)
            sumSquares += static_cast<double>(data[i]) * static_cast<double>(data[i]);
    }

    const double meanSquare = sumSquares / (temp.getNumSamples() * temp.getNumChannels());
    const double rms = std::sqrt(meanSquare);
    currentLufs = static_cast<float>(juce::Decibels::gainToDecibels(rms) - 0.691f);
    juce::Logger::writeToLog("MasterBusProcessor::processBlock: end");
}

void MasterBusProcessor::setTargetLufs(float targetLUFS) noexcept
{
    targetLufs = targetLUFS;
    juce::Logger::writeToLog("MasterBusProcessor target LUFS set to: " + juce::String(targetLufs));
}

float MasterBusProcessor::getTargetLufs() const noexcept
{
    return targetLufs;
}

void MasterBusProcessor::setCompressorEnabled(bool enabled)
{
    compressorEnabled = enabled;
    compressor->setEnabled(enabled);
    juce::Logger::writeToLog("MasterBusProcessor compressor enabled: " + juce::String(enabled ? 1 : 0));
}

void MasterBusProcessor::setLimiterEnabled(bool enabled)
{
    limiterEnabled = enabled;
    juce::Logger::writeToLog("MasterBusProcessor limiter enabled: " + juce::String(enabled ? 1 : 0));
}

void MasterBusProcessor::setStreamTarget(StreamTarget target)
{
    currentTarget = target;
    
    // Set appropriate LUFS target based on selected stream
    switch (target)
    {
        case StreamTarget::YouTube:
            setTargetLufs(kLUFS_Youtube);
            break;
            
        case StreamTarget::Facebook:
            setTargetLufs(kLUFS_Facebook);
            break;
            
        case StreamTarget::Custom:
            // Don't change the target LUFS here, as it's set separately for custom
            break;
    }
    
    juce::Logger::writeToLog("MasterBusProcessor stream target set to: " + 
                           juce::String(static_cast<int>(target)));
} 