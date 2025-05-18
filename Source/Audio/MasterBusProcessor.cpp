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

            lowFilters[ch].processSamples      (lowData, numSamples);
            highFilters[ch].processSamples     (highData, numSamples);

            midHighFilters[ch].processSamples  (midData, numSamples);
            midLowFilters[ch].processSamples   (midData, numSamples);
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
            *lowFilters[ch].coefficients      = *lowLP;
            *highFilters[ch].coefficients     = *highHP;
            *midLowFilters[ch].coefficients   = *midLP;
            *midHighFilters[ch].coefficients  = *midHP;
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
    
    // For now, we're just simulating LUFS measurement
    // In a real implementation, we'd calculate LUFS from the buffer here
    
    // Just a simple placeholder implementation to show changes in the UI
    // Simulating some variation in the measured LUFS
    currentLufs = targetLufs + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 2.0f;
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