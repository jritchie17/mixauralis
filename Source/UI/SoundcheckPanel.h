#pragma once

#include <JuceHeader.h>
#include "../Soundcheck/SoundcheckEngine.h"
#include "../Utils/BlackwayLookAndFeel.h"

class SoundcheckPanel : public juce::Component,
                        private juce::Timer,
                        private juce::TableListBoxModel
{
public:
    SoundcheckPanel();
    ~SoundcheckPanel() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Show/hide the panel with animation
    void showPanel(bool animate = true);
    void hidePanel(bool animate = true);
    
    // Set the channel processors to be corrected
    void setChannelProcessors(const std::vector<ChannelProcessor*>& processors);
    
    // TableListBoxModel overrides for the summary table
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    
private:
    // UI components
    BlackwayLookAndFeel blackwayLookAndFeel;
    
    // Labels
    juce::Label titleLabel;
    juce::Label channelLabel { {}, "Analyzing Channel: --" };
    juce::Label statusLabel { {}, "Ready to start soundcheck" };
    
    // Buttons
    juce::TextButton startButton { "Start Soundcheck" };
    juce::TextButton stopButton { "Stop" };
    juce::TextButton applyButton { "Apply Changes" };
    juce::TextButton revertButton { "Revert Changes" };
    
    // Progress indicator
    juce::ProgressBar progressBar { progress };
    double progress { 0.0 };
    
    // Results table
    juce::TableListBox resultsTable;
    juce::StringArray columnNames;
    std::vector<int> columnWidths;
    
    // UI state management
    enum class DisplayMode { Analyzing, Results };
    DisplayMode currentMode { DisplayMode::Analyzing };
    void updateUIForMode();
    
    // Helper method to get cell color based on correction value
    juce::Colour getCorrectionColor(float value);
    
    // Callback for timer
    void timerCallback() override;
    
    // Button click callbacks
    void startButtonClicked();
    void stopButtonClicked();
    void applyButtonClicked();
    void revertButtonClicked();
    
    // References to the SoundcheckEngine
    SoundcheckEngine& soundcheckEngine;
    
    // Channel processors
    std::vector<ChannelProcessor*> channelProcessors;
    
    // Current analysis state
    int currentChannel { 0 };
    int totalChannels { 32 };
    
    // UI state
    bool isShowing { false };
    float targetAlpha { 0.0f };
    float currentAlpha { 0.0f };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundcheckPanel)
}; 