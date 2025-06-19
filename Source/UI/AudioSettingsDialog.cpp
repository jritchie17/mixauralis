#include "AudioSettingsDialog.h"

namespace auralis
{
    AudioSettingsDialog::AudioSettingsDialog (juce::AudioDeviceManager& adm)
        : juce::DialogWindow ("Audio Settings", juce::Colours::darkgrey, true),
          deviceManager (adm)
    {
        setUsingNativeTitleBar(true);
        setResizable(false, false);
        setLookAndFeel(&lookAndFeel);

        // Create selector: wantInput=true, wantOutput=true, showMidi=false
        selector = std::make_unique<juce::AudioDeviceSelectorComponent>(
                        deviceManager,
                        /* minInput */   0,  /* maxInput */   256,
                        /* minOutput */  0,  /* maxOutput */  256,
                        /* showMidi */ false,
                        /* includeBuiltInOutputSelector */ false,
                        /* showChannelsAsStereoPairs */ true,
                        /* hideAdvancedOptionsWithButton */ false);

        setContentOwned(selector.release(), true);
        centreWithSize(500, 400);
    }

    void AudioSettingsDialog::closeButtonPressed()
    {
        // Persist device state for next launch
        if (auto xml = deviceManager.createStateXml())
        {
            auto file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                            .getChildFile("Auralis/audio_device.xml");
            file.getParentDirectory().createDirectory();
            xml->writeToFile(file, {});
        }

        setVisible(false);   // caller deletes
        setLookAndFeel(nullptr);
    }

    void AudioSettingsDialog::visibilityChanged()
    {
        if (isVisible())
            setLookAndFeel(&lookAndFeel);
    }
}
