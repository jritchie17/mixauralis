#include "../UI/SettingsComponent.h"
#include "../Audio/AudioEngine.h"
#include "../Audio/MasterBusProcessor.h"
#include "../MainApp.h"

namespace auralis
{
    SettingsComponent::SettingsComponent()
    {
        // Set component IDs
        accountLabel.setComponentID("accountLabel");
        statusLabel.setComponentID("statusLabel");
        loginButton.setComponentID("loginButton");
        logoutButton.setComponentID("logoutButton");
        platformLabel.setComponentID("platformLabel");
        platformCombo.setComponentID("platformCombo");
        customLufsSlider.setComponentID("customLufsSlider");

        // Setup platform combo
        platformCombo.addItem("YouTube", 1);
        platformCombo.addItem("Facebook", 2);
        platformCombo.addItem("Custom", 3);
        platformCombo.setSelectedId(1, juce::dontSendNotification);

        // Setup LUFS slider
        customLufsSlider.setRange(-24.0, -10.0, 0.1);
        customLufsSlider.setValue(-14.0, juce::dontSendNotification);
        customLufsSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
        customLufsSlider.setVisible(false);
        updateSliderVisibility();  // Ensure initial layout is correct

        // Add components and listeners
        addAndMakeVisible(accountLabel);
        addAndMakeVisible(statusLabel);
        addAndMakeVisible(loginButton);
        addAndMakeVisible(logoutButton);
        addAndMakeVisible(platformLabel);
        addAndMakeVisible(platformCombo);
        addAndMakeVisible(customLufsSlider);

        loginButton.addListener(this);
        logoutButton.addListener(this);
        platformCombo.addListener(this);
        customLufsSlider.addListener(this);

        refreshAccountUI();
    }

    void SettingsComponent::setOnPlanChangeCallback(std::function<void()> callback)
    {
        onPlanChangeCallback = std::move(callback);
    }

    void SettingsComponent::resized()
    {
        auto bounds = getLocalBounds().reduced(10);
        const int rowHeight = 24;
        const int gap = 10;

        // Account section
        accountLabel.setBounds(bounds.removeFromTop(rowHeight));
        statusLabel.setBounds(bounds.removeFromTop(rowHeight));
        
        auto buttonRow = bounds.removeFromTop(rowHeight);
        loginButton.setBounds(buttonRow.removeFromLeft(100));
        logoutButton.setBounds(buttonRow.removeFromLeft(100));

        bounds.removeFromTop(gap);

        // Platform section
        platformLabel.setBounds(bounds.removeFromTop(rowHeight));
        platformCombo.setBounds(bounds.removeFromTop(rowHeight));
        customLufsSlider.setBounds(bounds.removeFromTop(rowHeight));
    }

    void SettingsComponent::refreshAccountUI()
    {
        auto& subManager = SubscriptionManager::getInstance();
        if (subManager.isAuthenticated())
        {
            juce::String planName;
            switch (subManager.getCurrentPlan())
            {
                case Plan::Foundation: planName = "Foundation"; break;
                case Plan::Flow:       planName = "Flow";       break;
                case Plan::Pro:        planName = "Pro";        break;
            }
            statusLabel.setText("Plan: " + planName, juce::dontSendNotification);
            loginButton.setEnabled(false);
            logoutButton.setEnabled(true);
        }
        else
        {
            statusLabel.setText("Not logged in", juce::dontSendNotification);
            loginButton.setEnabled(true);
            logoutButton.setEnabled(false);
        }

        // Notify about plan change
        if (onPlanChangeCallback)
            onPlanChangeCallback();
    }

    void SettingsComponent::buttonClicked(juce::Button* b)
    {
        if (b == &loginButton)
        {
            auto* dialog = new juce::AlertWindow("Login", "Enter your token:", juce::AlertWindow::NoIcon);
            dialog->addTextEditor("token", "");
            dialog->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
            dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
            
            dialog->enterModalState(true, juce::ModalCallbackFunction::create(
                [this, dialog](int result) {
                    if (result == 1)
                    {
                        juce::String token = dialog->getTextEditorContents("token");
                        if (token.isNotEmpty())
                        {
                            SubscriptionManager::getInstance().loginWithToken(token);
                            refreshAccountUI();
                        }
                    }
                    delete dialog;
                }));
        }
        else if (b == &logoutButton)
        {
            SubscriptionManager::getInstance().logout();
            refreshAccountUI();
        }
    }

    void SettingsComponent::updateSliderVisibility()
    {
        const bool custom = (platformCombo.getSelectedId() == 3);
        customLufsSlider.setVisible(custom);
        resized();   // refresh layout
    }

    void SettingsComponent::comboBoxChanged(juce::ComboBox* box)
    {
        if (box == &platformCombo)
        {
            if (auto* app = dynamic_cast<MainApp*>(juce::JUCEApplication::getInstance()))
            {
                if (auto* masterBus = app->getAudioEngine().getMasterBusProcessor())
                {
                    switch (box->getSelectedId())
                    {
                        case 1: // YouTube
                            masterBus->setTargetLufs(-14.0);
                            break;
                        case 2: // Facebook
                            masterBus->setTargetLufs(-16.0);
                            break;
                        case 3: // Custom
                            masterBus->setTargetLufs(customLufsSlider.getValue());
                            break;
                    }
                }
            }
            
            updateSliderVisibility();
        }
    }

    void SettingsComponent::sliderValueChanged(juce::Slider* s)
    {
        if (s == &customLufsSlider)
        {
            if (auto* app = dynamic_cast<MainApp*>(juce::JUCEApplication::getInstance()))
            {
                if (auto* masterBus = app->getAudioEngine().getMasterBusProcessor())
                {
                    masterBus->setTargetLufs(s->getValue());
                }
            }
        }
    }

    void SettingsComponent::visibilityChanged()
    {
        if (isVisible())
            updateSliderVisibility();
    }
} 