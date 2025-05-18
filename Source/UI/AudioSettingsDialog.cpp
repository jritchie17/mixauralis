#include "AudioSettingsDialog.h"

namespace auralis
{
    AudioSettingsDialog::AudioSettingsDialog (juce::AudioDeviceManager& adm)
        : juce::DialogWindow ("Audio Settings", juce::Colours::darkgrey, true),
          deviceManager (adm)
    {
        // Create selector: wantInput=true, wantOutput=true, showMidi=false
        auto selector = std::make_unique<juce::AudioDeviceSelectorComponent>(
                            deviceManager,
                            /* minInput */   0,  /* maxInput */   256,
                            /* minOutput */  0,  /* maxOutput */  256,
                            /* showMidi */ false,
                            /* includeBuiltInOutputSelector */ false,
                            /* showChannelsAsStereoPairs */ true,
                            /* hideAdvancedOptionsWithButton */ false);

        setContentOwned (selector.release(), true);
        centreWithSize (500, 400);
    }

    void AudioSettingsDialog::closeButtonPressed()
    {
        setVisible (false);   // caller deletes
    }
} 