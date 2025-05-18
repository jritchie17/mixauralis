#include "TunerProcessor.h"

namespace auralis
{
    TunerProcessor::TunerProcessor()
    {
    }

    void TunerProcessor::prepareToPlay(double sr, int samplesPerBlock)
    {
        sampleRate = sr;
        dryBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);
        dryBuffer.clear();
        tunedBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);
        tunedBuffer.clear();

        strengthSmoothed.reset(sampleRate, 0.05);
        strengthSmoothed.setCurrentAndTargetValue(strength.load());

        readIndex[0] = readIndex[1] = 0.0f;
    }

    void TunerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        dryBuffer.makeCopyOf(buffer);

        // Detect pitch on first channel using zero crossings
        float detectedPitch = detectPitch(dryBuffer.getReadPointer(0), numSamples);
        float ratio = 1.0f;
        if (detectedPitch > 0.0f)
        {
            const float semitone = std::round(12.0f * std::log2(detectedPitch / 440.0f));
            const float nearestPitch = 440.0f * std::pow(2.0f, semitone / 12.0f);
            ratio = nearestPitch / detectedPitch;
        }

        tunedBuffer.setSize(numChannels, numSamples, false, false, true);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            pitchShiftChannel(dryBuffer.getReadPointer(channel),
                              tunedBuffer.getWritePointer(channel),
                              numSamples, channel, ratio);
        }

        const float currentStrength = strengthSmoothed.getNextValue();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            auto* dryData = dryBuffer.getReadPointer(channel);
            auto* tunedData = tunedBuffer.getReadPointer(channel);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                channelData[sample] = dryData[sample] * (1.0f - currentStrength) +
                                     tunedData[sample] * currentStrength;
            }
        }
    }

    float TunerProcessor::detectPitch(const float* data, int numSamples) const
    {
        if (numSamples <= 1)
            return 0.0f;

        int lastSign = data[0] >= 0.0f ? 1 : -1;
        int crossings = 0;

        for (int i = 1; i < numSamples; ++i)
        {
            int sign = data[i] >= 0.0f ? 1 : -1;
            if (sign != lastSign)
            {
                ++crossings;
                lastSign = sign;
            }
        }

        if (crossings < 2)
            return 0.0f;

        float cycles = static_cast<float>(crossings) / 2.0f;
        return static_cast<float>(sampleRate) * cycles / static_cast<float>(numSamples);
    }

    void TunerProcessor::pitchShiftChannel(const float* input, float* output,
                                           int numSamples, int channel, float ratio)
    {
        float index = readIndex[channel];
        for (int i = 0; i < numSamples; ++i)
        {
            int i0 = static_cast<int>(index) % numSamples;
            int i1 = (i0 + 1) % numSamples;
            float frac = index - static_cast<float>(static_cast<int>(index));

            output[i] = input[i0] * (1.0f - frac) + input[i1] * frac;

            index += ratio;
            while (index >= numSamples)
                index -= numSamples;
        }
        readIndex[channel] = index;
    }
} 