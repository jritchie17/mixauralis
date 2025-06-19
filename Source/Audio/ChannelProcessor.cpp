#include "ChannelProcessor.h"

ChannelProcessor::ChannelProcessor()
    : ChannelProcessor(-1, ChannelType::Other, nullptr)
{
    // All initialization is handled by the delegated constructor
}

ChannelProcessor::ChannelProcessor(int index, ChannelType type, AudioEngine* engine)
    : channelIndex(index), channelType(type), audioEngine(engine)
{
    // Create the processor graph - processors will be created in createGraph()
    processorGraph = std::make_unique<juce::AudioProcessorGraph>();
    
    // Create and connect the processor graph
    createGraph();
    
    // Set up default parameters after graph creation
    trim->setGainLinear(juce::Decibels::decibelsToGain(trimGainDecibels));
    gate->setThreshold(-50.0f);
    gate->setRatio(2.0f);
    gate->setAttack(5.0f);
    gate->setRelease(50.0f);
    
    comp->setThreshold(-18.0f);
    comp->setRatio(3.0f);
    comp->setAttack(10.0f);
    comp->setRelease(150.0f);
    comp->setMakeupGainAuto(true);
    
    // Set the bypass states
    updateNodeBypass(gateNode, gateBypass);
    updateNodeBypass(eqNode, eqBypass);
    updateNodeBypass(compNode, compressorBypass);
    updateNodeBypass(tunerNode, tunerBypass);
}

ChannelProcessor::~ChannelProcessor()
{
    // Clean up resources
    processorGraph->clear();
}

void ChannelProcessor::createGraph()
{
    // Create input and output nodes
    inputNode = processorGraph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));
    
    outputNode = processorGraph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
    
    // Add processor nodes - keep references to them
    auto trimProcessorCopy = std::make_unique<TrimProcessor>();
    trimNode = processorGraph->addNode(std::move(trimProcessorCopy));
    
    auto gateProcessorCopy = std::make_unique<GateProcessor>();
    gateNode = processorGraph->addNode(std::move(gateProcessorCopy));
    
    auto eqProcessorCopy = std::make_unique<EQProcessor>();
    eqNode = processorGraph->addNode(std::move(eqProcessorCopy));
    
    auto compProcessorCopy = std::make_unique<CompressorProcessor>();
    compNode = processorGraph->addNode(std::move(compProcessorCopy));
    
    // Create and add tuner node. The graph will own the processor,
    // but keep a raw pointer for parameter updates.
    tunerNode = processorGraph->addNode(std::make_unique<auralis::TunerProcessor>());
    tuner = static_cast<auralis::TunerProcessor*>(tunerNode->getProcessor());
    
    // Store pointers to processors so we can adjust parameters
    // The graph now owns the processor instances
    trim = dynamic_cast<TrimProcessor*>(trimNode->getProcessor());
    gate = dynamic_cast<GateProcessor*>(gateNode->getProcessor());
    eq = dynamic_cast<EQProcessor*>(eqNode->getProcessor());
    comp = dynamic_cast<CompressorProcessor*>(compNode->getProcessor());
    
    // Connect the nodes in the correct order
    connectNodes();
}

void ChannelProcessor::connectNodes()
{
    // Connect nodes in this order: Input -> Trim -> Gate -> EQ -> Comp -> Tuner -> Output
    
    // Clear any existing connections
    processorGraph->clear();
    
    // Connect input to trim
    for (int channel = 0; channel < 2; ++channel)
    {
        processorGraph->addConnection({ {inputNode->nodeID, channel}, 
                                        {trimNode->nodeID, channel} });
    }
    
    // Connect trim to gate
    for (int channel = 0; channel < 2; ++channel)
    {
        processorGraph->addConnection({ {trimNode->nodeID, channel}, 
                                        {gateNode->nodeID, channel} });
    }
    
    // Connect gate to EQ
    for (int channel = 0; channel < 2; ++channel)
    {
        processorGraph->addConnection({ {gateNode->nodeID, channel}, 
                                        {eqNode->nodeID, channel} });
    }
    
    // Connect EQ to compressor
    for (int channel = 0; channel < 2; ++channel)
    {
        processorGraph->addConnection({ {eqNode->nodeID, channel}, 
                                        {compNode->nodeID, channel} });
    }
    
    // Connect compressor to tuner
    for (int channel = 0; channel < 2; ++channel)
    {
        processorGraph->addConnection({ {compNode->nodeID, channel}, 
                                        {tunerNode->nodeID, channel} });
    }
    
    // Connect tuner to output
    for (int channel = 0; channel < 2; ++channel)
    {
        processorGraph->addConnection({ {tunerNode->nodeID, channel}, 
                                        {outputNode->nodeID, channel} });
    }
}

void ChannelProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = maximumExpectedSamplesPerBlock;
    
    // Set up the graph with the correct sample rate and block size
    processorGraph->setPlayConfigDetails(2, 2, sampleRate, maximumExpectedSamplesPerBlock);
    processorGraph->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
}

void ChannelProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Process the buffer through the graph
    processorGraph->processBlock(buffer, midiMessages);
}

void ChannelProcessor::releaseResources()
{
    processorGraph->releaseResources();
}

void ChannelProcessor::setTrimGain(float gainInDecibels)
{
    trimGainDecibels = gainInDecibels;
    if (trim != nullptr)
    {
        trim->setGainLinear(juce::Decibels::decibelsToGain(trimGainDecibels));
    }
}

void ChannelProcessor::setGateEnabled(bool enabled)
{
    gateBypass = !enabled;
    updateNodeBypass(gateNode, gateBypass);
}

void ChannelProcessor::setCompressorEnabled(bool enabled)
{
    compressorBypass = !enabled;
    updateNodeBypass(compNode, compressorBypass);
}

void ChannelProcessor::setEqEnabled(bool enabled)
{
    eqBypass = !enabled;
    updateNodeBypass(eqNode, eqBypass);
}

void ChannelProcessor::setFxSendLevel(float level)
{
    fxSendLevel = juce::jlimit(0.0f, 1.0f, level);

    // Propagate the level to the assigned FX bus if available
    if (fxSendBus != nullptr)
        fxSendBus->addInputChannel(channelIndex, fxSendLevel);
}

void ChannelProcessor::setTunerEnabled(bool enabled)
{
    tunerBypass = !enabled;
    updateNodeBypass(tunerNode, tunerBypass);
}

void ChannelProcessor::setTunerStrength(float strength)
{
    if (tuner != nullptr)
    {
        tuner->setStrength(strength);
    }
}

void ChannelProcessor::updateNodeBypass(juce::AudioProcessorGraph::Node::Ptr node, bool bypass)
{
    if (node != nullptr && node->getProcessor() != nullptr)
    {
        node->setBypassed(bypass);
    }
}

void ChannelProcessor::setGateThreshold(float thresholdInDb)
{
    if (gate != nullptr)
    {
        gate->setThreshold(thresholdInDb);
    }
}

void ChannelProcessor::setEQBandGain(EQProcessor::Band band, float gainInDb)
{
    if (eq != nullptr)
    {
        eq->setGain(band, gainInDb);
    }
}

void ChannelProcessor::setCompressorRatio(float ratio)
{
    if (comp != nullptr)
    {
        comp->setRatio(ratio);
    }
}

void ChannelProcessor::setCompressorThreshold(float thresholdInDb)
{
    if (comp != nullptr)
    {
        comp->setThreshold(thresholdInDb);
    }
}

float ChannelProcessor::getGateThreshold() const
{
    if (gate != nullptr)
    {
        return gate->getThreshold();
    }
    return -50.0f; // Default value
}

float ChannelProcessor::getEQBandGain(EQProcessor::Band band) const
{
    if (eq != nullptr)
    {
        return eq->getGain(band);
    }
    return 0.0f; // Default value
}

float ChannelProcessor::getCompressorRatio() const
{
    if (comp != nullptr)
    {
        return comp->getRatio();
    }
    return 1.0f; // Default value (no compression)
}

float ChannelProcessor::getCompressorThreshold() const
{
    if (comp != nullptr)
    {
        return comp->getThreshold();
    }
    return 0.0f; // Default value
} 