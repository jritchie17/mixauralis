#include "AudioEngine.h"

// Helper class for sine wave testing
class SineWaveTestProcessor : public juce::AudioProcessor
{
public:
    SineWaveTestProcessor() 
        : AudioProcessor(juce::AudioProcessor::BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo()))
    {
    }

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        this->sampleRate = sampleRate;
        currentAngle = 0.0f;
        angleIncrement = 2.0f * juce::MathConstants<float>::pi * 60.0f / static_cast<float>(sampleRate);
    }

    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        buffer.clear();
        
        const auto numSamples = buffer.getNumSamples();
        
        // Generate sine wave at 60 Hz
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            
            for (int sample = 0; sample < numSamples; ++sample)
            {
                channelData[sample] = std::sin(currentAngle) * 0.5f; // 0.5 amplitude
                
                // Only increment angle once per sample, not per channel
                if (channel == 0)
                    currentAngle += angleIncrement;
            }
        }
        
        // Reset angle if it gets too big to avoid precision issues
        while (currentAngle >= juce::MathConstants<float>::twoPi)
            currentAngle -= juce::MathConstants<float>::twoPi;
    }

    const juce::String getName() const override { return "SineWaveTest"; }
    
    // Minimal implementations for abstract methods
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    double sampleRate = 44100.0;
    float currentAngle = 0.0f;
    float angleIncrement = 0.0f;
};

AudioEngine::AudioEngine()
{
    // Initialize channel processors
    for (int i = 0; i < numChannels; ++i)
    {
        // Assign default channel types based on index
        ChannelProcessor::ChannelType type;
        
        if (i < 8)
        {
            type = ChannelProcessor::ChannelType::Vocal;
        }
        else if (i < 16)
        {
            type = ChannelProcessor::ChannelType::Instrument;
        }
        else
        {
            type = ChannelProcessor::ChannelType::Drums;
        }
        
        // Create the processor with index, type, and a reference to this engine
        channelProcessors.push_back(std::make_unique<ChannelProcessor>(i, type, this));
    }
    
    // Initialize group bus processors
    groupBusProcessors.push_back(std::make_unique<GroupBusProcessor>(GroupBusProcessor::BusType::Vocals));
    groupBusProcessors.push_back(std::make_unique<GroupBusProcessor>(GroupBusProcessor::BusType::Instruments));
    groupBusProcessors.push_back(std::make_unique<GroupBusProcessor>(GroupBusProcessor::BusType::Drums));
    groupBusProcessors.push_back(std::make_unique<GroupBusProcessor>(GroupBusProcessor::BusType::Speech));
    
    // Initialize FX bus processors
    for (int i = 0; i < numFXBuses; ++i)
    {
        auto busType = static_cast<FXBusProcessor::BusType>(i);
        fxBusProcessors.push_back(std::make_unique<FXBusProcessor>(busType));
    }
    
    // Initialize master bus processor
    masterBusProcessor = std::make_unique<MasterBusProcessor>();
    
    // Initialize routing manager
    getRoutingManager().initialize(channelProcessors, fxBusProcessors);
    
    // Create a test sine wave source on Channel 1
    testSineWave = std::make_unique<SineWaveTestProcessor>();
    
    // Set up the audio device manager
    setupAudioDevices();
}

AudioEngine::~AudioEngine()
{
    deviceManager.removeAudioCallback(this);
    saveAudioDeviceState();
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData, 
                                                 int numInputChannels,
                                                 float* const* outputChannelData, 
                                                 int numOutputChannels,
                                                 int numSamples,
                                                 const juce::AudioIODeviceCallbackContext& context)
{
    // Clear output buffers
    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        std::fill(outputChannelData[channel], outputChannelData[channel] + numSamples, 0.0f);
    }
    
    // Prepare temp buffer
    tempBuffer.setSize(2, numSamples, false, false, true);
    tempBuffer.clear();
    
    // Prepare group bus buffer
    groupBusBuffer.setSize(2, numSamples, false, false, true);
    groupBusBuffer.clear();
    
    // Prepare FX bus buffer
    fxBusBuffer.setSize(2, numSamples, false, false, true);
    fxBusBuffer.clear();
    
    // Prepare master buffer
    masterBuffer.setSize(2, numSamples, false, false, true);
    masterBuffer.clear();
    
    // TESTING: Generate sine wave for Channel 1 (index 0)
    juce::MidiBuffer dummyMidi;
    if (testSineWave)
    {
        testSineWave->processBlock(tempBuffer, dummyMidi);
        channelProcessors[0]->processBlock(tempBuffer, dummyMidi);
    }
    else
    {
        // Normal flow for other channels - copy input to temp buffer
        for (int channel = 0; channel < juce::jmin(2, numInputChannels); ++channel)
        {
            tempBuffer.copyFrom(channel, 0, inputChannelData[channel], numSamples);
        }
        
        // Process each channel
        for (int i = 0; i < numChannels && i < numInputChannels; ++i)
        {
            channelProcessors[i]->processBlock(tempBuffer, dummyMidi);
        }
    }
    
    // Route channels to their respective group buses
    for (int i = 0; i < numChannels; ++i)
    {
        auto* channelProcessor = channelProcessors[i].get();
        
        // Determine which group bus this channel should go to based on channel type
        GroupBusProcessor* targetGroupBus = nullptr;
        
        switch (channelProcessor->getChannelType())
        {
            case ChannelProcessor::ChannelType::Vocal:
                targetGroupBus = groupBusProcessors[0].get(); // Vocals
                break;
            case ChannelProcessor::ChannelType::Instrument:
                targetGroupBus = groupBusProcessors[1].get(); // Instruments
                break;
            case ChannelProcessor::ChannelType::Drums:
                targetGroupBus = groupBusProcessors[2].get(); // Drums
                break;
            case ChannelProcessor::ChannelType::Other:
                targetGroupBus = groupBusProcessors[3].get(); // Speech (mapped to Other)
                break;
            default:
                // Unsupported channel type, skip
                continue;
        }
        
        // Copy the processed channel audio to the group bus buffer for further processing
        if (targetGroupBus)
        {
            // We'll use a separate buffer for each group bus to avoid interference
            tempBuffer.clear();
            
            // Get the processed audio from the channel processor
            // In a real implementation, this would involve more sophisticated routing
            // but for now, we'll just use the global temp buffer
            
            // Process the group bus
            targetGroupBus->processBlock(tempBuffer, dummyMidi);
            
            // Add to the group bus buffer
            groupBusBuffer.addFrom(0, 0, tempBuffer, 0, 0, numSamples);
            groupBusBuffer.addFrom(1, 0, tempBuffer, 1, 0, numSamples);
        }
    }
    
    // Sum the processed group buses to the FX bus
    fxBusBuffer.addFrom(0, 0, groupBusBuffer, 0, 0, numSamples);
    fxBusBuffer.addFrom(1, 0, groupBusBuffer, 1, 0, numSamples);
    
    // Process FX buses
    for (int i = 0; i < numFXBuses; ++i)
    {
        fxBusProcessors[i]->processBlock(fxBusBuffer, dummyMidi);
    }
    
    // Add FX bus output to master
    masterBuffer.addFrom(0, 0, fxBusBuffer, 0, 0, numSamples);
    masterBuffer.addFrom(1, 0, fxBusBuffer, 1, 0, numSamples);
    
    // Process master bus
    masterBusProcessor->processBlock(masterBuffer, dummyMidi);
    
    // Output the master bus to the audio device
    for (int channel = 0; channel < juce::jmin(numOutputChannels, 2); ++channel)
    {
        std::memcpy(outputChannelData[channel], masterBuffer.getReadPointer(channel), sizeof(float) * numSamples);
    }
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    // Get the sample rate and buffer size from the device
    sampleRate = device->getCurrentSampleRate();
    bufferSize = device->getCurrentBufferSizeSamples();
    
    // Prepare the test sine wave source
    if (testSineWave)
    {
        testSineWave->prepareToPlay(sampleRate, bufferSize);
    }
    
    // Prepare all channel processors
    for (auto& processor : channelProcessors)
    {
        processor->prepareToPlay(sampleRate, bufferSize);
    }
    
    // Prepare all group bus processors
    for (auto& processor : groupBusProcessors)
    {
        processor->prepareToPlay(sampleRate, bufferSize);
    }
    
    // Prepare all FX bus processors
    for (auto& processor : fxBusProcessors)
    {
        processor->prepareToPlay(sampleRate, bufferSize);
    }
    
    // Prepare the master bus processor
    masterBusProcessor->prepareToPlay(sampleRate, bufferSize);
    
    // Prepare the buffers
    tempBuffer.setSize(2, bufferSize);
    fxBusBuffer.setSize(2, bufferSize);
    masterBuffer.setSize(2, bufferSize);
}

void AudioEngine::audioDeviceStopped()
{
    // Release resources from the test sine wave
    if (testSineWave)
    {
        testSineWave->releaseResources();
    }
    
    // Release resources from all processors
    for (auto& processor : channelProcessors)
    {
        processor->releaseResources();
    }
    
    for (auto& processor : groupBusProcessors)
    {
        processor->releaseResources();
    }
    
    for (auto& processor : fxBusProcessors)
    {
        processor->releaseResources();
    }
    
    masterBusProcessor->releaseResources();
}

ChannelProcessor* AudioEngine::getChannelProcessor(int channelIndex)
{
    if (channelIndex >= 0 && channelIndex < channelProcessors.size())
    {
        return channelProcessors[channelIndex].get();
    }
    
    return nullptr;
}

FXBusProcessor* AudioEngine::getFXBusProcessor(int busIndex)
{
    if (busIndex >= 0 && busIndex < fxBusProcessors.size())
    {
        return fxBusProcessors[busIndex].get();
    }
    
    return nullptr;
}

GroupBusProcessor* AudioEngine::getGroupBusProcessor(int busIndex)
{
    if (busIndex >= 0 && busIndex < groupBusProcessors.size())
    {
        return groupBusProcessors[busIndex].get();
    }
    
    return nullptr;
}

MasterBusProcessor* AudioEngine::getMasterBusProcessor()
{
    return masterBusProcessor.get();
}

std::vector<GroupBusProcessor*> AudioEngine::getAllGroupBusProcessors()
{
    std::vector<GroupBusProcessor*> processors;
    
    for (auto& processor : groupBusProcessors)
    {
        processors.push_back(processor.get());
    }
    
    return processors;
}

bool AudioEngine::setupAudioDevices()
{
    loadAudioDeviceState();
    deviceManager.addAudioCallback(this);
    return true;
}

void AudioEngine::setChannelSendLevel(int channelIndex, float sendLevel)
{
    if (auto* channel = getChannelProcessor(channelIndex))
    {
        channel->setFxSendLevel(sendLevel);
    }
}

void AudioEngine::broadcastParametersChanged()
{
    // This is a no-op stub for now
    // In the future, this will notify UI components to refresh their displays
    DBG("Broadcasting parameter changes to UI");
}

void AudioEngine::saveAudioDeviceState() const
{
    if (auto xml = deviceManager.createStateXml())
    {
        auto file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("Auralis/audio_device.xml");
        file.getParentDirectory().createDirectory();
        xml->writeToFile(file, {});
    }
}

void AudioEngine::loadAudioDeviceState()
{
    auto file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                    .getChildFile("Auralis/audio_device.xml");

    std::unique_ptr<juce::XmlElement> xml;
    if (file.existsAsFile())
        xml.reset(juce::XmlDocument::parse(file));

    juce::AudioDeviceManager::AudioDeviceSetup cfg;
    juce::String err = deviceManager.initialise(2, 2, xml.get(), true, {}, &cfg);
    if (err.isNotEmpty())
        juce::Logger::writeToLog("Audio init error: " + err);
}
