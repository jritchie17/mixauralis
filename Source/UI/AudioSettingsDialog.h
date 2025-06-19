#pragma once
#include <JuceHeader.h>
#include "../Utils/BlackwayLookAndFeel.h"

namespace auralis
{
    /** Modal dialog hosting a JUCE AudioDeviceSelectorComponent allowing the
        user to configure input and output devices.  The current device
        settings are saved and restored automatically. */
    class AudioSettingsDialog : public juce::DialogWindow
    {
    public:
        explicit AudioSettingsDialog (juce::AudioDeviceManager& adm);
        ~AudioSettingsDialog() override = default;

        void closeButtonPressed() override;
        void visibilityChanged() override;

    private:
        juce::AudioDeviceManager& deviceManager;
        juce::AudioDeviceSelectorComponent* selector = nullptr;
        BlackwayLookAndFeel lookAndFeel;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioSettingsDialog)
    };
} 
