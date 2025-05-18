#pragma once

#include <JuceHeader.h>
#include "../Audio/ChannelProcessor.h"
#include "../Utils/BlackwayLookAndFeel.h"

// Forward declaration for cross-referencing
class ChannelsComponent;

class ChannelStripComponent : public juce::Component,
                              public juce::Button::Listener,
                              public juce::Slider::Listener,
                              public juce::ComboBox::Listener
{
public:
    enum class ChannelType
    {
        SingingVocal,
        Instrument,
        Drums,
        Speech,
        Other
    };
    
    // Standard width for channel strips as per requirements
    static constexpr int kStandardWidth = 140;
    static constexpr int kStandardPadding = 6;
    static constexpr int kControlSize = 64; // Standard 64x64 for rotary knobs
    static constexpr int kToggleSize = 24;  // Standard 24x24 for toggles

    ChannelStripComponent(int index = 0, ChannelsComponent* parent = nullptr);
    ~ChannelStripComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void setChannelName(const juce::String& name);
    void setChannelType(ChannelType type);
    void setChannelIndex(int index);
    void setChannelEnabled(bool enabled);
    
    // Connect to a channel processor
    void connectToProcessor(ChannelProcessor* processor);
    
    // Notification of channel selection
    void mouseDown(const juce::MouseEvent& event) override;
    bool isSelected() const { return selected; }
    void setSelected(bool shouldBeSelected);
    void setEnabled(bool shouldBeEnabled);
    
    // Button and slider listener callbacks
    void buttonClicked(juce::Button* button) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    
    // Get the channel index and processor
    int getChannelIndex() const { return channelIndex; }
    ChannelProcessor* getChannelProcessor() const { return channelProcessor; }
    
    // Refresh all parameters from the processor
    void refreshParametersFromProcessor();

    void setInputChoices(const juce::StringArray& names);
    
    // Update input selection
    void setSelectedInput(int physicalInput, juce::NotificationType notification = juce::sendNotification)
    {
        inputCombo.setSelectedId(physicalInput + 1, notification);
    }

private:
    ChannelType channelType = ChannelType::Other;
    juce::String channelName = "Unassigned";
    int channelIndex = 0;
    bool selected = false;
    bool isEnabled = true;
    
    // Pointer to parent component for selection notification
    ChannelsComponent* parentComponent = nullptr;
    
    // Custom look and feel
    BlackwayLookAndFeel blackwayLookAndFeel;
    
    // UI Components
    juce::Label nameLabel;
    juce::Label indexLabel;  // For displaying channel number
    juce::Image typeIcon;    // Channel type icon
    
    juce::Slider trimDial;
    juce::Label trimLabel;
    juce::Label trimValueLabel;  // Display value in dB
    
    juce::ToggleButton gateToggle;
    juce::Label gateLabel;
    
    juce::ToggleButton compToggle;
    juce::Label compLabel;
    
    juce::TextButton eqButton;
    
    juce::Slider fxSendDial;
    juce::Label fxSendLabel;
    juce::Label fxSendValueLabel;  // Display value in %
    
    juce::ToggleButton tunerToggle;
    juce::Label tunerLabel;
    
    juce::Slider tunerDial;        // New tuner strength knob
    juce::Label tunerValueLabel;   // Display value in %
    
    juce::ImageButton muteButton;
    juce::ImageButton soloButton;
    
    juce::Slider fader;
    juce::ComboBox inputCombo;
    
    std::unique_ptr<juce::Component> levelMeter;
    
    // Background image
    juce::Image backgroundImage;
    
    // Linked processor
    ChannelProcessor* channelProcessor = nullptr;
    
    // Helper methods
    void updateValueLabels();
    void setupTooltips();
    void loadTypeIcon();
    juce::Colour getChannelColourBand() const;
    void updateComponentEnablement();
    void updateIconColours();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStripComponent)
}; 