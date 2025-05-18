#include "SoundcheckEngine.h"
#include <limits>

// Singleton instance implementation
SoundcheckEngine& SoundcheckEngine::getInstance()
{
    static SoundcheckEngine instance;
    return instance;
}

SoundcheckEngine::SoundcheckEngine()
    : juce::Thread("SoundcheckEngine", 7) // Priority 7 as per requirements
{
    // Initialize with 32 channels
    channelAnalyses.resize(32);
    
    // Initialize FFT processor
    fft = std::make_unique<juce::dsp::FFT>(fftOrder);
    
    // Initialize FFT buffers
    fftTimeDomain.allocate(fftSize, true);
    fftFrequencyDomain.allocate(fftSize * 2, true);
    windowBuffer.allocate(fftSize, true);
    
    // Create Hann window for FFT
    for (int i = 0; i < fftSize; ++i)
        windowBuffer[i] = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * i / (fftSize - 1));
    
    // Initialize audio ring buffer (5 sec at 48kHz stereo)
    audioRingBuffer.setSize(2, maxBufferSize);
    audioRingBuffer.clear();
    
    // Initialize analyzed bands array for each channel
    for (auto& analysis : channelAnalyses)
    {
        analysis.measuredMagnitudes.resize(32);
        for (int i = 0; i < 32; ++i)
            analysis.measuredMagnitudes.set(i, -60.0f); // Default to very quiet
    }
}

SoundcheckEngine::~SoundcheckEngine()
{
    // Make sure the thread is stopped
    stopCheck();
    
    // Wait for thread to finish
    stopThread(2000);
}

void SoundcheckEngine::startCheck(int secondsPerChannel)
{
    // Don't start if already running
    if (isRunning())
        return;
    
    // Set analysis parameters
    analysisLengthSeconds = secondsPerChannel;
    currentChannelIndex = 0;
    
    // Start tracking execution time
    startTime = std::chrono::high_resolution_clock::now();
    
    // Initialize analysis data
    initializeAnalysis();
    
    // Set state and start thread
    currentState = State::analysing;
    startThread(juce::Thread::Priority::high); // High priority thread
}

void SoundcheckEngine::initializeAnalysis()
{
    // Reset audio buffer
    bufferWritePosition = 0;
    samplesCollected = 0;
    audioRingBuffer.clear();
    
    // Reset analysis data
    for (auto& analysis : channelAnalyses)
    {
        analysis.avgRMS = 0.0f;
        analysis.peakLevel = 0.0f;
        analysis.noiseFloor = 0.0f;
        
        // Reset FFT buckets
        for (int i = 0; i < analysis.measuredMagnitudes.size(); ++i)
            analysis.measuredMagnitudes.set(i, -60.0f);
        
        // Reset correction suggestions
        analysis.trimGainSuggestion = 0.0f;
        analysis.gateThresholdSuggestion = -50.0f;
        for (int i = 0; i < 4; ++i)
            analysis.eqGainSuggestions[i] = 0.0f;
        analysis.compressorRatioSuggestion = 1.0f;
    }
}

void SoundcheckEngine::stopCheck()
{
    // Only stop if we're analyzing
    if (currentState == State::analysing)
    {
        currentState = State::finished;
        signalThreadShouldExit();
    }
}

bool SoundcheckEngine::isRunning() const
{
    return currentState == State::analysing;
}

const SoundcheckEngine::ChannelAnalysis& SoundcheckEngine::getAnalysis(int channelIndex) const
{
    // Make sure index is valid
    jassert(channelIndex >= 0 && channelIndex < static_cast<int>(channelAnalyses.size()));
    
    // Return analysis data for the specified channel
    return channelAnalyses[channelIndex];
}

void SoundcheckEngine::setChannelProcessors(const std::vector<ChannelProcessor*>& processors)
{
    channelProcessors = processors;
    
    // Resize analysis array to match processor count
    if (channelAnalyses.size() != processors.size())
    {
        channelAnalyses.resize(processors.size());
        for (auto& analysis : channelAnalyses)
        {
            analysis.measuredMagnitudes.resize(32);
            for (int i = 0; i < 32; ++i)
                analysis.measuredMagnitudes.set(i, -60.0f); // Default to very quiet
        }
    }
}

void SoundcheckEngine::captureAudio(const float* const* inputChannelData, int numChannels, int numSamples)
{
    // Check if we're still collecting samples
    if (samplesCollected >= analysisLengthSeconds * 48000) // Assuming 48kHz
        return;
        
    // Get how many channels to process (limited to stereo)
    const int channelsToProcess = juce::jmin(numChannels, 2);
    
    // Copy the samples into the ring buffer
    for (int channel = 0; channel < channelsToProcess; ++channel)
    {
        int bufferIndex = bufferWritePosition;
        for (int i = 0; i < numSamples; ++i)
        {
            audioRingBuffer.setSample(channel, bufferIndex, inputChannelData[channel][i]);
            if (++bufferIndex >= audioRingBuffer.getNumSamples())
                bufferIndex = 0;
        }
    }
    
    // Move write position
    bufferWritePosition = (bufferWritePosition + numSamples) % audioRingBuffer.getNumSamples();
    samplesCollected += numSamples;
}

void SoundcheckEngine::analyzeCurrentChannelBuffer()
{
    // Get the current channel analysis
    auto& analysis = channelAnalyses[currentChannelIndex];
    
    // Calculate RMS and peak levels
    float sumSquared = 0.0f;
    float peak = 0.0f;
    
    // Analyze the samples we've collected
    const int numSamplesToAnalyze = juce::jmin(samplesCollected, audioRingBuffer.getNumSamples());
    
    // RMS analysis
    for (int channel = 0; channel < audioRingBuffer.getNumChannels(); ++channel)
    {
        for (int i = 0; i < numSamplesToAnalyze; ++i)
        {
            const float sample = audioRingBuffer.getSample(channel, i);
            sumSquared += sample * sample;
            peak = juce::jmax(peak, std::abs(sample));
        }
    }
    
    // Store the results
    analysis.avgRMS = std::sqrt(sumSquared / (numSamplesToAnalyze * audioRingBuffer.getNumChannels()));
    analysis.peakLevel = peak;
    
    // Determine noise floor (use bottom 20% of samples)
    juce::Array<float> magnitudes;
    for (int channel = 0; channel < audioRingBuffer.getNumChannels(); ++channel)
    {
        for (int i = 0; i < numSamplesToAnalyze; i += 64) // Sample every 64 samples for efficiency
        {
            magnitudes.add(std::abs(audioRingBuffer.getSample(channel, i)));
        }
    }
    
    // Sort and take the bottom 20%
    magnitudes.sort();
    const int noiseFloorIndex = static_cast<int>(magnitudes.size() * 0.2f);
    analysis.noiseFloor = noiseFloorIndex < magnitudes.size() ? magnitudes[noiseFloorIndex] : 0.0f;
    
    // Perform FFT analysis - divide signal into overlapping windows
    const int hopSize = fftSize / 4; // 75% overlap
    const int numWindows = (numSamplesToAnalyze - fftSize) / hopSize + 1;
    
    // Reset frequency domain averages
    for (int i = 0; i < fftSize; ++i)
        fftFrequencyDomain[i] = 0.0f;
    
    // Process each window
    for (int window = 0; window < numWindows; ++window)
    {
        // Fill time domain buffer with windowed data (average of L+R channels)
        for (int i = 0; i < fftSize; ++i)
        {
            const int sampleIndex = window * hopSize + i;
            if (sampleIndex < numSamplesToAnalyze)
            {
                float sample = 0.0f;
                for (int channel = 0; channel < audioRingBuffer.getNumChannels(); ++channel)
                    sample += audioRingBuffer.getSample(channel, sampleIndex);
                
                sample /= audioRingBuffer.getNumChannels(); // Average the channels
                fftTimeDomain[i] = sample * windowBuffer[i]; // Apply window function
            }
            else
            {
                fftTimeDomain[i] = 0.0f;
            }
        }
        
        // Perform FFT
        fft->performRealOnlyForwardTransform(fftTimeDomain);
        
        // Convert to power spectrum and accumulate
        for (int i = 0; i < fftSize / 2; ++i)
        {
            const float real = fftTimeDomain[i * 2];
            const float imag = fftTimeDomain[i * 2 + 1];
            const float magnitude = std::sqrt(real * real + imag * imag);
            fftFrequencyDomain[i] += magnitude;
        }
    }
    
    // Average the spectrum across all windows
    if (numWindows > 0)
    {
        for (int i = 0; i < fftSize / 2; ++i)
            fftFrequencyDomain[i] /= numWindows;
    }
    
    // Map FFT data to 1/3 octave bands
    mapFFTToThirdOctaveBands(fftFrequencyDomain, analysis.measuredMagnitudes);

    // Classify channel type based on spectral profile
    analysis.suggestedType = classifyChannel(analysis.measuredMagnitudes);

    // Calculate suggested corrections based on reference profiles
    calculateCorrections(currentChannelIndex);
    
    juce::Logger::writeToLog("Analyzing channel " + juce::String(currentChannelIndex) + 
                           " - RMS: " + juce::String(analysis.avgRMS) + 
                           ", Noise floor: " + juce::String(analysis.noiseFloor) + 
                           ", Peak: " + juce::String(analysis.peakLevel));
}

void SoundcheckEngine::mapFFTToThirdOctaveBands(const float* fftData, juce::Array<float>& bandMagnitudes)
{
    // Get the standard 1/3 octave frequencies
    const auto bandFrequencies = getThirdOctaveBandFrequencies();
    
    // Calculate bin frequencies
    const double binWidth = 48000.0 / fftSize; // Assuming 48kHz sample rate
    
    // Reset band magnitudes
    for (int band = 0; band < bandMagnitudes.size(); ++band)
        bandMagnitudes.set(band, 0.0f);
    
    // Count how many FFT bins map to each 1/3 octave band
    juce::Array<int> binCounts;
    binCounts.resize(bandMagnitudes.size());
    binCounts.fill(0);
    
    // Map each FFT bin to its corresponding 1/3 octave band
    for (int bin = 1; bin < fftSize / 2; ++bin) // Skip DC (bin 0)
    {
        const double binFreq = bin * binWidth;
        
        // Find which 1/3 octave band this frequency belongs to
        for (int band = 0; band < bandFrequencies.size(); ++band)
        {
            const double lowerFreq = band > 0 ? std::sqrt(bandFrequencies[band] * bandFrequencies[band - 1]) : 0;
            const double upperFreq = band < bandFrequencies.size() - 1 ? 
                std::sqrt(bandFrequencies[band] * bandFrequencies[band + 1]) : 30000.0;
            
            if (binFreq >= lowerFreq && binFreq < upperFreq)
            {
                // Add bin magnitude to band
                bandMagnitudes.set(band, bandMagnitudes[band] + fftData[bin]);
                binCounts.set(band, binCounts[band] + 1);
                break;
            }
        }
    }
    
    // Average the magnitudes in each band and convert to dB
    for (int band = 0; band < bandMagnitudes.size(); ++band)
    {
        if (binCounts[band] > 0)
        {
            float avg = bandMagnitudes[band] / binCounts[band];
            // Convert to dB, with a minimum of -60dB
            bandMagnitudes.set(band, juce::jmax(-60.0f, juce::Decibels::gainToDecibels(avg)));
        }
        else
        {
            bandMagnitudes.set(band, -60.0f); // Minimum value if no bins in this band
        }
    }
}

void SoundcheckEngine::calculateCorrections(int channelIndex)
{
    // Make sure channel index is valid
    if (channelIndex < 0 || channelIndex >= static_cast<int>(channelAnalyses.size()) || 
        channelIndex >= static_cast<int>(channelProcessors.size()) || 
        channelProcessors[channelIndex] == nullptr)
        return;
    
    auto& analysis = channelAnalyses[channelIndex];
    auto* processor = channelProcessors[channelIndex];
    
    // Convert suggested type to ChannelStripComponent type
    ChannelStripComponent::ChannelType channelType;
    switch (analysis.suggestedType)
    {
        case ChannelProcessor::ChannelType::Vocal:
            channelType = ChannelStripComponent::ChannelType::SingingVocal;
            break;
        case ChannelProcessor::ChannelType::Instrument:
            channelType = ChannelStripComponent::ChannelType::Instrument;
            break;
        case ChannelProcessor::ChannelType::Drums:
            channelType = ChannelStripComponent::ChannelType::Drums;
            break;
        default:
            channelType = ChannelStripComponent::ChannelType::Other;
            break;
    }
    
    // Get reference profile for this channel type
    const auto& refProfile = getProfileFor(channelType);
    
    // Backup original settings before calculating corrections
    backupOriginalSettings(channelIndex);
    
    // 1. Calculate Trim Gain correction
    // Target is reference RMS level - measured RMS
    float measuredRmsDb = juce::Decibels::gainToDecibels(analysis.avgRMS);
    analysis.trimGainSuggestion = refProfile.targetRms - measuredRmsDb;
    analysis.trimGainSuggestion = juce::jlimit(-12.0f, 12.0f, analysis.trimGainSuggestion);
    
    // 2. Calculate Gate Threshold based on noise floor
    float noiseFloorDb = juce::Decibels::gainToDecibels(analysis.noiseFloor);
    analysis.gateThresholdSuggestion = noiseFloorDb + 6.0f; // Set threshold 6dB above noise floor
    analysis.gateThresholdSuggestion = juce::jlimit(-60.0f, -20.0f, analysis.gateThresholdSuggestion);
    
    // 3. Calculate EQ corrections - compare reference and measured magnitudes
    // First, initialize to zeros
    for (int band = 0; band < 4; ++band)
        analysis.eqGainSuggestions[band] = 0.0f;
    
    // Map each band to its closest EQ band and calculate average difference
    juce::Array<float> bandDifferences;
    bandDifferences.resize(4);
    bandDifferences.fill(0.0f);
    
    juce::Array<int> bandCounts;
    bandCounts.resize(4);
    bandCounts.fill(0);
    
    const auto bandFrequencies = getThirdOctaveBandFrequencies();
    for (int i = 0; i < bandFrequencies.size(); ++i)
    {
        if (i < analysis.measuredMagnitudes.size() && i < refProfile.refMagnitudes.size())
        {
            // Calculate difference (positive means we need to boost, negative means cut)
            float diff = refProfile.refMagnitudes[i] - analysis.measuredMagnitudes[i];
            
            // Map to the appropriate EQ band
            int eqBand = getEQBandForFrequency(bandFrequencies[i]);
            
            bandDifferences.set(eqBand, bandDifferences[eqBand] + diff);
            bandCounts.set(eqBand, bandCounts[eqBand] + 1);
        }
    }
    
    // Average the differences for each EQ band
    for (int band = 0; band < 4; ++band)
    {
        if (bandCounts[band] > 0)
        {
            analysis.eqGainSuggestions[band] = bandDifferences[band] / bandCounts[band];
            analysis.eqGainSuggestions[band] = juce::jlimit(-12.0f, 12.0f, analysis.eqGainSuggestions[band]);
        }
    }
    
    // 4. Calculate Compressor settings based on dynamic range
    float dynamicRange = juce::Decibels::gainToDecibels(analysis.peakLevel) - 
                         juce::Decibels::gainToDecibels(analysis.avgRMS);
    
    // Choose compressor ratio based on dynamic range
    if (dynamicRange > 12.0f)
        analysis.compressorRatioSuggestion = 3.0f;  // 3:1 for wide dynamic range
    else if (dynamicRange > 6.0f)
        analysis.compressorRatioSuggestion = 2.0f;  // 2:1 for medium range
    else
        analysis.compressorRatioSuggestion = 1.0f;  // No compression for narrow range
}

void SoundcheckEngine::backupOriginalSettings(int channelIndex)
{
    // Make sure channel index is valid
    if (channelIndex < 0 || channelIndex >= static_cast<int>(channelAnalyses.size()) || 
        channelIndex >= static_cast<int>(channelProcessors.size()) || 
        channelProcessors[channelIndex] == nullptr)
        return;
    
    auto& analysis = channelAnalyses[channelIndex];
    auto* processor = channelProcessors[channelIndex];

    // Store original settings
    analysis.originalType = processor->getChannelType();
    analysis.originalTrimGain = processor->getTrimGain();
    analysis.originalGateThreshold = processor->getGateThreshold();
    
    for (int band = 0; band < 4; ++band)
    {
        analysis.originalEqGains[band] = processor->getEQBandGain(static_cast<EQProcessor::Band>(band));
    }
    
    analysis.originalCompressorRatio = processor->getCompressorRatio();
}

void SoundcheckEngine::applyCorrections()
{
    // Only apply if we have channel processors and soundcheck is finished
    if (channelProcessors.empty() || currentState != State::finished)
        return;
    
    juce::Logger::writeToLog("SoundcheckEngine: Applying corrections to " + 
                            juce::String(channelProcessors.size()) + " channels");
    
    // Apply correction to each channel
    for (size_t i = 0; i < channelProcessors.size() && i < channelAnalyses.size(); ++i)
    {
        // Get channel processor
        auto* processor = channelProcessors[i];
        if (processor == nullptr)
            continue;
        
        // Get analysis data
        const auto& analysis = channelAnalyses[i];

        // Update processor channel type based on analysis
        processor->setChannelType(analysis.suggestedType);

        juce::String typeStr;
        switch (analysis.suggestedType)
        {
            case ChannelProcessor::ChannelType::Vocal:      typeStr = "Vocal"; break;
            case ChannelProcessor::ChannelType::Instrument: typeStr = "Instrument"; break;
            case ChannelProcessor::ChannelType::Drums:      typeStr = "Drums"; break;
            default:                                       typeStr = "Other"; break;
        }
        juce::Logger::writeToLog("Channel " + juce::String(i) + " classified as " + typeStr);

        // Apply trim gain
        processor->setTrimGain(analysis.trimGainSuggestion);
        
        // Apply gate threshold
        processor->setGateThreshold(analysis.gateThresholdSuggestion);
        processor->setGateEnabled(true);
        
        // Apply EQ band gains
        processor->setEqEnabled(true);
        for (int band = 0; band < 4; ++band)
        {
            processor->setEQBandGain(static_cast<EQProcessor::Band>(band), 
                                     analysis.eqGainSuggestions[band]);
        }
        
        // Apply compressor settings
        processor->setCompressorRatio(analysis.compressorRatioSuggestion);
        processor->setCompressorEnabled(analysis.compressorRatioSuggestion > 1.0f);
        
        // Log the settings for debugging
        juce::Logger::writeToLog("Channel " + juce::String(i) + 
                               " - Trim: " + juce::String(analysis.trimGainSuggestion) + " dB" +
                               ", Gate: " + juce::String(analysis.gateThresholdSuggestion) + " dB" +
                               ", Comp Ratio: " + juce::String(analysis.compressorRatioSuggestion) + ":1");
    }
    
    // Switch to idle state
    currentState = State::idle;
}

void SoundcheckEngine::revertCorrections()
{
    // Only revert if we have channel processors
    if (channelProcessors.empty())
        return;
    
    juce::Logger::writeToLog("SoundcheckEngine: Reverting corrections for " + 
                            juce::String(channelProcessors.size()) + " channels");
    
    // Apply original settings to each channel
    for (size_t i = 0; i < channelProcessors.size() && i < channelAnalyses.size(); ++i)
    {
        // Get channel processor
        auto* processor = channelProcessors[i];
        if (processor == nullptr)
            continue;
        
        // Get analysis data
        const auto& analysis = channelAnalyses[i];
        
        // Restore original settings
        processor->setChannelType(analysis.originalType);
        processor->setTrimGain(analysis.originalTrimGain);
        processor->setGateThreshold(analysis.originalGateThreshold);
        
        for (int band = 0; band < 4; ++band)
        {
            processor->setEQBandGain(static_cast<EQProcessor::Band>(band), 
                                    analysis.originalEqGains[band]);
        }
        
        processor->setCompressorRatio(analysis.originalCompressorRatio);
    }
    
    // Switch to idle state
    currentState = State::idle;
}

void SoundcheckEngine::run()
{
    // Process each channel in sequence
    while (!threadShouldExit() && currentChannelIndex < static_cast<int>(channelAnalyses.size()))
    {
        // Get current channel to analyze
        auto& analysis = channelAnalyses[currentChannelIndex];
        
        // Reset audio capture buffer
        bufferWritePosition = 0;
        samplesCollected = 0;
        audioRingBuffer.clear();
        
        // Wait for audio to be collected
        while (!threadShouldExit() && samplesCollected < analysisLengthSeconds * 48000)
        {
            wait(100); // Check every 100ms
        }
        
        if (threadShouldExit())
            break;
            
        // Analyze the collected audio
        analyzeCurrentChannelBuffer();
        
        // Move to next channel
        ++currentChannelIndex;
    }
    
    // Set state to finished if we completed all channels
    if (!threadShouldExit())
    {
        currentState = State::finished;
        
        // Check elapsed time
        auto endTime = std::chrono::high_resolution_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        juce::Logger::writeToLog("SoundcheckEngine: Analysis complete for all channels in " + 
                                juce::String(elapsedMs / 1000.0) + " seconds");
        
        // Assert if it took too long
        jassert(elapsedMs < 60000); // Must complete in under 60 seconds
    }
}

ChannelProcessor::ChannelType SoundcheckEngine::classifyChannel(const juce::Array<float>& magnitudes) const
{
    using StripType = ChannelStripComponent::ChannelType;

    struct TypeMap { StripType strip; ChannelProcessor::ChannelType proc; };
    const TypeMap types[] = {
        {StripType::SingingVocal,   ChannelProcessor::ChannelType::Vocal},
        {StripType::Speech,         ChannelProcessor::ChannelType::Vocal},
        {StripType::Instrument,     ChannelProcessor::ChannelType::Instrument},
        {StripType::Drums,          ChannelProcessor::ChannelType::Drums},
        {StripType::Other,          ChannelProcessor::ChannelType::Other}
    };

    ChannelProcessor::ChannelType best = ChannelProcessor::ChannelType::Other;
    float bestScore = std::numeric_limits<float>::max();

    for (const auto& t : types)
    {
        const auto& profile = getProfileFor(t.strip);
        float score = 0.0f;
        const int n = juce::jmin(magnitudes.size(), profile.refMagnitudes.size());
        for (int i = 0; i < n; ++i)
        {
            const float diff = profile.refMagnitudes[i] - magnitudes[i];
            score += diff * diff;
        }

        if (score < bestScore)
        {
            bestScore = score;
            best = t.proc;
        }
    }

    return best;
}
