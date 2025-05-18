#include "FXBusProcessor.h"

FXBusProcessor::FXBusProcessor(BusType type)
    : busType(type)
{
    // Create the processor graph
    processorGraph = std::make_unique<juce::AudioProcessorGraph>();
    
    // Create the effect processors
    reverb = std::make_unique<ReverbProcessor>();
    delay = std::make_unique<DelayProcessor>();
    
    // Set default wet levels
    reverb->setWetLevel(reverbWetLevel);
    delay->setWetLevel(delayWetLevel);
}

FXBusProcessor::~FXBusProcessor()
{
    releaseResources();
}

void FXBusProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = maximumExpectedSamplesPerBlock;
    
    // Reset the processors
    reverb->releaseResources();
    delay->releaseResources();
    
    // Configure the processor graph
    processorGraph->setPlayConfigDetails(2, 2, sampleRate, maximumExpectedSamplesPerBlock);
    processorGraph->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
    
    // Create input summing node with node ID 1
    auto audioInputNodeProcessor = std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
                                          juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);
    inputNode = processorGraph->addNode(std::move(audioInputNodeProcessor),
                                      juce::AudioProcessorGraph::NodeID(1));
    
    // Create output node with node ID 2
    auto audioOutputNodeProcessor = std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
                                           juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);
    outputNode = processorGraph->addNode(std::move(audioOutputNodeProcessor),
                                       juce::AudioProcessorGraph::NodeID(2));
    
    // Add the reverb processor node with node ID 3
    reverb->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
    reverbNode = processorGraph->addNode(std::move(reverb),
                                       juce::AudioProcessorGraph::NodeID(3));
    
    // Add the delay processor node with node ID 4
    delay->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
    delayNode = processorGraph->addNode(std::move(delay),
                                      juce::AudioProcessorGraph::NodeID(4));
    
    // Set up the node connections
    updateConnections();
    
    // Debug output
    juce::Logger::writeToLog("FXBusProcessor: " + getBusName() + " prepared with " + 
                           juce::String(sampleRate) + "Hz sample rate");
}

void FXBusProcessor::updateConnections()
{
    // Clear all existing connections
    processorGraph->clear();
    
    // If bypassed, connect input directly to output
    if (bypassed)
    {
        for (int channel = 0; channel < 2; ++channel)
        {
            processorGraph->addConnection({ { inputNode->nodeID, channel }, 
                                          { outputNode->nodeID, channel } });
        }
        return;
    }
    
    // Connect nodes in series: Input -> Reverb -> Delay -> Output
    for (int channel = 0; channel < 2; ++channel)
    {
        // Connect input to reverb (or delay if reverb is bypassed)
        if (!reverbBypass)
        {
            processorGraph->addConnection({ { inputNode->nodeID, channel }, 
                                          { reverbNode->nodeID, channel } });
            
            // Connect reverb to delay (or output if delay is bypassed)
            if (!delayBypass)
            {
                processorGraph->addConnection({ { reverbNode->nodeID, channel }, 
                                              { delayNode->nodeID, channel } });
                
                // Connect delay to output
                processorGraph->addConnection({ { delayNode->nodeID, channel }, 
                                              { outputNode->nodeID, channel } });
            }
            else
            {
                // If delay is bypassed, connect reverb directly to output
                processorGraph->addConnection({ { reverbNode->nodeID, channel }, 
                                              { outputNode->nodeID, channel } });
            }
        }
        else if (!delayBypass)
        {
            // If reverb is bypassed but delay is active
            processorGraph->addConnection({ { inputNode->nodeID, channel }, 
                                          { delayNode->nodeID, channel } });
            
            // Connect delay to output
            processorGraph->addConnection({ { delayNode->nodeID, channel }, 
                                          { outputNode->nodeID, channel } });
        }
        else
        {
            // If both effects are bypassed, connect input directly to output
            processorGraph->addConnection({ { inputNode->nodeID, channel }, 
                                          { outputNode->nodeID, channel } });
        }
    }
}

void FXBusProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Process audio through the processor graph
    processorGraph->processBlock(buffer, midiMessages);
}

void FXBusProcessor::releaseResources()
{
    if (processorGraph != nullptr)
    {
        processorGraph->releaseResources();
    }
}

void FXBusProcessor::setReverbEnabled(bool enabled)
{
    reverbBypass = !enabled;
    juce::Logger::writeToLog(getBusName() + " reverb " + (enabled ? "enabled" : "disabled"));
    
    // Update the processor graph connections
    updateConnections();
}

void FXBusProcessor::setDelayEnabled(bool enabled)
{
    delayBypass = !enabled;
    juce::Logger::writeToLog(getBusName() + " delay " + (enabled ? "enabled" : "disabled"));
    
    // Update the processor graph connections
    updateConnections();
}

void FXBusProcessor::setReverbWetLevel(float level)
{
    reverbWetLevel = juce::jlimit(0.0f, 1.0f, level);
    
    // Update the reverb parameters
    if (reverbNode != nullptr && reverbNode->getProcessor() != nullptr)
    {
        if (auto* reverbProcessor = dynamic_cast<ReverbProcessor*>(reverbNode->getProcessor()))
        {
            reverbProcessor->setWetLevel(reverbWetLevel);
        }
    }
    
    juce::Logger::writeToLog(getBusName() + " reverb wet level: " + juce::String(reverbWetLevel));
}

void FXBusProcessor::setDelayWetLevel(float level)
{
    delayWetLevel = juce::jlimit(0.0f, 1.0f, level);
    
    // Update the delay parameters
    if (delayNode != nullptr && delayNode->getProcessor() != nullptr)
    {
        if (auto* delayProcessor = dynamic_cast<DelayProcessor*>(delayNode->getProcessor()))
        {
            delayProcessor->setWetLevel(delayWetLevel);
        }
    }
    
    juce::Logger::writeToLog(getBusName() + " delay wet level: " + juce::String(delayWetLevel));
}

void FXBusProcessor::setBypass(bool shouldBypass)
{
    bypassed = shouldBypass;
    juce::Logger::writeToLog(getBusName() + (bypassed ? " bypassed" : " active"));
    
    // Update connections
    updateConnections();
}

void FXBusProcessor::addInputChannel(int channelIndex, float sendLevel)
{
    // Store the send level for this channel
    channelSendLevels[channelIndex] = juce::jlimit(0.0f, 1.0f, sendLevel);
    
    juce::Logger::writeToLog(getBusName() + " added channel " + juce::String(channelIndex) + 
                           " with send level " + juce::String(sendLevel));
    
    // In a complete implementation, this would connect a send node from the channel 
    // processor to this bus's input summing node, with the send level applied
}

juce::String FXBusProcessor::getBusName() const
{
    switch (busType)
    {
        case BusType::VocalFX:
            return "Vocal FX";
        case BusType::InstrumentFX:
            return "Instrument FX";
        case BusType::DrumFX:
            return "Drum FX";
        default:
            return "Unknown FX";
    }
} 