#pragma once
#include <JuceHeader.h>

namespace auralis
{
    /** Simple modal window that will host the JUCE AudioDeviceSelectorComponent.
        Implementation will follow in a later step. */
    class AudioSettingsDialog : public juce::DialogWindow
    {
    public:
        explicit AudioSettingsDialog (juce::AudioDeviceManager& adm);
        ~AudioSettingsDialog() override = default;

        void closeButtonPressed() override;

    private:
        juce::AudioDeviceManager& deviceManager;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioSettingsDialog)
    };
} 