#pragma once

#include <JuceHeader.h>
#include "../UI/ChannelStripComponent.h"

/**
 * ToneProfile - A reference frequency profile for different channel types
 * 
 * Contains a reference magnitude curve (32-band 1/3-octave), target RMS level,
 * and noise gate threshold for specific channel types.
 */
struct ToneProfile
{
    juce::Array<float> refMagnitudes;   // 32-band ⅓-octave
    float targetRms = -18.0f;           // dBFS
    float gateThreshold = -50.0f;       // dBFS
};

/**
 * Returns the reference ToneProfile for a specific channel type
 * 
 * @param type The channel type (SingingVocal, Instrument, etc.)
 * @return The reference ToneProfile for the specified channel type
 */
inline const ToneProfile& getProfileFor(ChannelStripComponent::ChannelType type)
{
    using ChType = ChannelStripComponent::ChannelType;
    
    // Static profiles for each channel type
    static const std::map<ChType, ToneProfile> profiles = {
        {ChType::SingingVocal, []() {
            ToneProfile profile;
            // Target range for singing vocals: 80 Hz to 12 kHz with presence boost at 3-5 kHz
            profile.refMagnitudes = {
                -24.0f, -18.0f, -12.0f, -6.0f, -3.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // 20-200 Hz
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, +1.0f, +2.0f, +3.0f,        // 200-2kHz
                +4.0f, +4.0f, +3.0f, +2.0f, +1.0f, 0.0f, -1.0f, -3.0f, -6.0f, -9.0f,  // 2k-20kHz
                -12.0f, -18.0f                                                         // >20kHz
            };
            profile.targetRms = -18.0f;
            profile.gateThreshold = -45.0f;
            return profile;
        }()},
        
        {ChType::Instrument, []() {
            ToneProfile profile;
            // Profile for general instruments with balanced frequency response
            profile.refMagnitudes = {
                -18.0f, -12.0f, -8.0f, -4.0f, -2.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // 20-200 Hz
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,           // 200-2kHz
                0.0f, 0.0f, 0.0f, 0.0f, -1.0f, -2.0f, -3.0f, -4.0f, -5.0f, -6.0f,     // 2k-20kHz
                -8.0f, -12.0f                                                          // >20kHz
            };
            profile.targetRms = -16.0f;
            profile.gateThreshold = -50.0f;
            return profile;
        }()},
        
        {ChType::Drums, []() {
            ToneProfile profile;
            // Profile for drums with emphasis on low and high frequencies
            profile.refMagnitudes = {
                -6.0f, -3.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, -2.0f, -3.0f,     // 20-200 Hz
                -4.0f, -3.0f, -2.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,        // 200-2kHz
                +1.0f, +2.0f, +3.0f, +3.0f, +2.0f, +1.0f, 0.0f, -1.0f, -3.0f, -6.0f,   // 2k-20kHz
                -9.0f, -12.0f                                                           // >20kHz
            };
            profile.targetRms = -14.0f;
            profile.gateThreshold = -40.0f;
            return profile;
        }()},
        
        {ChType::Speech, []() {
            ToneProfile profile;
            // Profile for speech with emphasis on midrange clarity
            profile.refMagnitudes = {
                -30.0f, -24.0f, -18.0f, -12.0f, -9.0f, -6.0f, -3.0f, -1.0f, 0.0f, 0.0f, // 20-200 Hz
                0.0f, 0.0f, 0.0f, +1.0f, +2.0f, +3.0f, +3.0f, +2.0f, +1.0f, 0.0f,       // 200-2kHz
                -1.0f, -2.0f, -4.0f, -6.0f, -9.0f, -12.0f, -15.0f, -18.0f, -21.0f, -24.0f, // 2k-20kHz
                -27.0f, -30.0f                                                           // >20kHz
            };
            profile.targetRms = -16.0f;
            profile.gateThreshold = -40.0f;
            return profile;
        }()},
        
        {ChType::Other, []() {
            ToneProfile profile;
            // Generic flat profile for unknown sources
            profile.refMagnitudes = {
                -6.0f, -5.0f, -4.0f, -3.0f, -2.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,     // 20-200 Hz
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,           // 200-2kHz
                0.0f, 0.0f, 0.0f, 0.0f, -1.0f, -2.0f, -3.0f, -4.0f, -5.0f, -6.0f,     // 2k-20kHz
                -7.0f, -8.0f                                                           // >20kHz
            };
            profile.targetRms = -18.0f;
            profile.gateThreshold = -50.0f;
            return profile;
        }()}
    };
    
    // Return the requested profile or the "Other" profile if not found
    auto it = profiles.find(type);
    if (it != profiles.end()) {
        return it->second;
    }
    
    return profiles.at(ChType::Other);
}

// Helper function to get band frequencies for the 32-band ⅓-octave spectrum
inline juce::Array<float> getThirdOctaveBandFrequencies()
{
    // Standard 1/3 octave band center frequencies (Hz)
    return {
        20.0f, 25.0f, 31.5f, 40.0f, 50.0f, 63.0f, 80.0f, 100.0f, 125.0f, 160.0f,
        200.0f, 250.0f, 315.0f, 400.0f, 500.0f, 630.0f, 800.0f, 1000.0f, 1250.0f, 1600.0f,
        2000.0f, 2500.0f, 3150.0f, 4000.0f, 5000.0f, 6300.0f, 8000.0f, 10000.0f, 12500.0f, 16000.0f,
        20000.0f, 25000.0f
    };
}

// Helper to map frequencies to corresponding EQ bands
inline int getEQBandForFrequency(float frequency)
{
    using Band = EQProcessor::Band;
    
    if (frequency < 200.0f) {
        return static_cast<int>(Band::LowShelf);  // Low shelf (centered around 80 Hz)
    } else if (frequency < 1000.0f) {
        return static_cast<int>(Band::LowMid);    // Low mid (centered around 300 Hz)
    } else if (frequency < 5000.0f) {
        return static_cast<int>(Band::HighMid);   // High mid (centered around 3 kHz)
    } else {
        return static_cast<int>(Band::HighShelf); // High shelf (centered around 8 kHz)
    }
} 