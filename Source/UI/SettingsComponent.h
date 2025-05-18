#pragma once
#include <JuceHeader.h>
#include "Subscription/SubscriptionManager.h"

namespace auralis
{
    class SettingsComponent : public juce::Component,
                              private juce::Button::Listener,
                              private juce::ComboBox::Listener,
                              private juce::Slider::Listener
    {
    public:
        SettingsComponent();
        ~SettingsComponent() override = default;

        void resized() override;
        void setOnPlanChangeCallback(std::function<void()> callback);
        
        /** Set the custom LUFS slider value without triggering callbacks. */
        void setCustomLufs(float lufs) { customLufsSlider.setValue(lufs, juce::dontSendNotification); }

    private:
        // ===== Account section =====
        juce::Label          accountLabel   { {}, "Account" };
        juce::Label          statusLabel    { {}, "Not logged in" };
        juce::TextButton     loginButton    { "Login…" };
        juce::TextButton     logoutButton   { "Logout" };

        // ===== Platform / loudness section =====
        juce::Label          platformLabel  { {}, "Stream Platform" };
        juce::ComboBox       platformCombo;
        juce::Slider         customLufsSlider;

        // =========================================================
        void buttonClicked   (juce::Button* b) override;
        void comboBoxChanged (juce::ComboBox* box) override;
        void sliderValueChanged (juce::Slider* s) override;
        void visibilityChanged() override;

        void refreshAccountUI();
        void updateSliderVisibility();

        // Callback for plan changes
        std::function<void()> onPlanChangeCallback;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsComponent)
    };
} 