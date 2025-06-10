#include "MasterBusProcessor.h"

//==============================================================================
// Internal multiband compressor used by the master bus
//==============================================================================
class MultibandCompressorProcessor
{
public:
    MultibandCompressorProcessor() = default;

    void prepare (double sampleRate, int maximumBlockSize)
    {
        spec.sampleRate       = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (maximumBlockSize);
        spec.numChannels      = 2;

        // Configure filters
        updateFilters (sampleRate);

        for (auto& comp : compressors)
            comp.prepare (spec);
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

    void process (juce::AudioBuffer<float>& buffer)
    {
        if (! isEnabled)
            return;

        const int numSamples = buffer.getNumSamples();

        // Prepare temporary buffers for each band
        juce::AudioBuffer<float> low  (buffer.getNumChannels(), numSamples);
        juce::AudioBuffer<float> mid  (buffer.getNumChannels(), numSamples);
        juce::AudioBuffer<float> high (buffer.getNumChannels(), numSamples);

        low.makeCopyOf  (buffer);
        mid.makeCopyOf  (buffer);
        high.makeCopyOf (buffer);

        // Split into bands using simple second order filters
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* lowData  = low.getWritePointer (ch);
            auto* midData  = mid.getWritePointer (ch);
            auto* highData = high.getWritePointer (ch);

            juce::dsp::AudioBlock<float> lowBlock(&lowData, 1, numSamples);
            juce::dsp::AudioBlock<float> midBlock(&midData, 1, numSamples);
            juce::dsp::AudioBlock<float> highBlock(&highData, 1, numSamples);

            juce::dsp::ProcessContextReplacing<float> lowContext(lowBlock);
            juce::dsp::ProcessContextReplacing<float> midContext(midBlock);
            juce::dsp::ProcessContextReplacing<float> highContext(highBlock);

            lowFilters[ch].process(lowContext);
            highFilters[ch].process(highContext);
            midHighFilters[ch].process(midContext);
            midLowFilters[ch].process(midContext);
        }

        // Run each band through its compressor
        processBand (low,  compressors[0]);
        processBand (mid,  compressors[1]);
        processBand (high, compressors[2]);

        // Sum bands back into the output buffer
        buffer.clear();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            buffer.addFrom (ch, 0, low,  ch, 0, numSamples);
            buffer.addFrom (ch, 0, mid,  ch, 0, numSamples);
            buffer.addFrom (ch, 0, high, ch, 0, numSamples);
        }
    }

    void setEnabled (bool enabled) noexcept  { isEnabled = enabled; }

private:
    void updateFilters (double sampleRate)
    {
        auto lowLP  = juce::dsp::IIR::Coefficients<float>::makeLowPass  (sampleRate, lowCrossoverHz);
        auto highHP = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, highCrossoverHz);
        auto midHP  = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, lowCrossoverHz);
        auto midLP  = juce::dsp::IIR::Coefficients<float>::makeLowPass  (sampleRate, highCrossoverHz);

        for (int ch = 0; ch < 2; ++ch)
        {
            lowFilters[ch].state = *lowLP;
            highFilters[ch].state = *highHP;
            midLowFilters[ch].state = *midLP;
            midHighFilters[ch].state = *midHP;
        }
    }

    static void processBand (juce::AudioBuffer<float>& bandBuffer, juce::dsp::Compressor<float>& comp)
    {
        juce::dsp::AudioBlock<float> block (bandBuffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        comp.process (context);
    }

    //==========================================================================
    juce::dsp::ProcessSpec spec;

    static constexpr float lowCrossoverHz  = 200.0f;
    static constexpr float highCrossoverHz = 2000.0f;

    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                  juce::dsp::IIR::Coefficients<float>>;

    Filter lowFilters [2];
    Filter highFilters[2];
    Filter midLowFilters [2];
    Filter midHighFilters[2];

    juce::dsp::Compressor<float> compressors[3];

    bool isEnabled { true };
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