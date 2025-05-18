#include "SoundcheckPanel.h"

SoundcheckPanel::SoundcheckPanel()
    : soundcheckEngine(SoundcheckEngine::getInstance())
{
    // Set up UI components
    setLookAndFeel(&blackwayLookAndFeel);
    
    // Configure buttons
    addAndMakeVisible(startButton);
    startButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a9c3a)); // Green
    startButton.onClick = [this]() { startButtonClicked(); };
    
    addAndMakeVisible(stopButton);
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffc93c3c)); // Red
    stopButton.onClick = [this]() { stopButtonClicked(); };
    stopButton.setEnabled(false);
    
    addAndMakeVisible(applyButton);
    applyButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a9c3a)); // Green
    applyButton.onClick = [this]() { applyButtonClicked(); };
    applyButton.setEnabled(false);
    
    addAndMakeVisible(revertButton);
    revertButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffb28c3c)); // Amber
    revertButton.onClick = [this]() { revertButtonClicked(); };
    revertButton.setEnabled(false);
    
    // Configure labels with better fonts and colors
    auto titleFont = blackwayLookAndFeel.getRobotoFont().withHeight(24.0f).boldened();
    auto labelFont = blackwayLookAndFeel.getRobotoFont().withHeight(16.0f);
    auto statusFont = blackwayLookAndFeel.getRobotoFont().withHeight(14.0f);
    
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Soundcheck Analysis", juce::dontSendNotification);
    titleLabel.setFont(titleFont);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    
    addAndMakeVisible(channelLabel);
    channelLabel.setFont(labelFont);
    channelLabel.setJustificationType(juce::Justification::centred);
    channelLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    
    addAndMakeVisible(statusLabel);
    statusLabel.setFont(statusFont);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    
    // Configure progress bar
    addAndMakeVisible(progressBar);
    progressBar.setTextToDisplay(""); // Remove percentage text
    
    // Configure results table
    addAndMakeVisible(resultsTable);
    resultsTable.setModel(this);
    resultsTable.setHeaderHeight(28);
    resultsTable.setRowHeight(24);
    resultsTable.setMultipleSelectionEnabled(false);
    
    // Set up columns with proportional widths
    columnNames = {"Channel", "Trim (dB)", "Gate Thresh", "EQ Low", "EQ LowMid", "EQ HighMid", "EQ High", "Comp Ratio"};
    columnWidths = {140, 80, 90, 80, 90, 90, 80, 90};
    
    auto& header = resultsTable.getHeader();
    header.setStretchToFitActive(true);
    
    for (int i = 0; i < columnNames.size(); ++i)
    {
        header.addColumn(columnNames[i], i + 1, columnWidths[i], 
                        50, 200, juce::TableHeaderComponent::defaultFlags);
    }
    
    // Set initial visibility for UI mode
    updateUIForMode();
    
    // Set initial alpha
    setAlpha(0.0f);
    
    // Set minimum component size
    setSize(840, 600);
}

SoundcheckPanel::~SoundcheckPanel()
{
    // Stop the timer
    stopTimer();
    
    // Clear look and feel
    setLookAndFeel(nullptr);
}

void SoundcheckPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Draw semi-transparent dark background with gradient
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff1a1a1a), bounds.getTopLeft(),
        juce::Colour(0xff2d2d2d), bounds.getBottomLeft(),
        false));
    g.fillRoundedRectangle(bounds, 12.0f);
    
    // Draw border with subtle glow
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), 12.0f, 2.0f);
    
    // Draw subtle grid lines in results mode
    if (currentMode == DisplayMode::Results)
    {
        g.setColour(juce::Colours::white.withAlpha(0.03f));
        auto tableBounds = resultsTable.getBounds();
        
        // Vertical lines
        for (int i = 1; i < columnNames.size(); ++i)
        {
            int x = tableBounds.getX();
            for (int j = 0; j < i; ++j)
                x += columnWidths[j];
            g.drawVerticalLine(x, tableBounds.getY() + 28, tableBounds.getBottom());
        }
        
        // Horizontal lines
        for (int i = 0; i < totalChannels; ++i)
        {
            int y = tableBounds.getY() + 28 + (i * 24);
            g.drawHorizontalLine(y, tableBounds.getX(), tableBounds.getRight());
        }
    }
}

void SoundcheckPanel::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    
    // Title area
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);
    
    if (currentMode == DisplayMode::Analyzing)
    {
        // Channel label with more space
        channelLabel.setBounds(bounds.removeFromTop(40));
        bounds.removeFromTop(10);
        
        // Progress bar with better height
        progressBar.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(20);
        
        // Status label with more space
        statusLabel.setBounds(bounds.removeFromTop(40));
        bounds.removeFromTop(20);
        
        // Buttons at the bottom with better sizing
        auto buttonArea = bounds.removeFromBottom(40);
        auto buttonWidth = (buttonArea.getWidth() - 20) / 2;
        startButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
        buttonArea.removeFromLeft(20); // Gap between buttons
        stopButton.setBounds(buttonArea);
    }
    else // Results mode
    {
        // Status label
        statusLabel.setBounds(bounds.removeFromTop(30));
        bounds.removeFromTop(20);
        
        // Results table with most of the space
        resultsTable.setBounds(bounds.removeFromTop(bounds.getHeight() - 80));
        bounds.removeFromTop(20);
        
        // Buttons at the bottom
        auto buttonArea = bounds.removeFromBottom(40);
        auto buttonWidth = (buttonArea.getWidth() - 20) / 2;
        applyButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
        buttonArea.removeFromLeft(20); // Gap between buttons
        revertButton.setBounds(buttonArea);
    }
}

void SoundcheckPanel::updateUIForMode()
{
    // Show/hide components based on current mode
    channelLabel.setVisible(currentMode == DisplayMode::Analyzing);
    progressBar.setVisible(currentMode == DisplayMode::Analyzing);
    startButton.setVisible(currentMode == DisplayMode::Analyzing);
    stopButton.setVisible(currentMode == DisplayMode::Analyzing);
    
    resultsTable.setVisible(currentMode == DisplayMode::Results);
    applyButton.setVisible(currentMode == DisplayMode::Results);
    revertButton.setVisible(currentMode == DisplayMode::Results);
    
    // Trigger a resize to update layout
    resized();
}

void SoundcheckPanel::showPanel(bool animate)
{
    // Make sure component is visible
    setVisible(true);
    
    // Bring to front if in a parent component
    if (getParentComponent() != nullptr)
        getParentComponent()->addChildComponent(this);
    
    // Set UI state
    isShowing = true;
    targetAlpha = 1.0f;
    
    // If not animating, set alpha immediately
    if (!animate)
    {
        currentAlpha = targetAlpha;
        setAlpha(currentAlpha);
        return;
    }
    
    // Start timer for animation
    startTimer(30);
}

void SoundcheckPanel::hidePanel(bool animate)
{
    // Set UI state
    isShowing = false;
    targetAlpha = 0.0f;
    
    // If not animating, set alpha immediately
    if (!animate)
    {
        currentAlpha = targetAlpha;
        setAlpha(currentAlpha);
        setVisible(false);
        return;
    }
    
    // Start timer for animation
    startTimer(30);
}

int SoundcheckPanel::getNumRows()
{
    return totalChannels;
}

void SoundcheckPanel::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    // Alternate row colors
    if (rowNumber % 2 == 0)
        g.fillAll(juce::Colours::darkgrey.darker(0.2f));
    else
        g.fillAll(juce::Colours::darkgrey.darker(0.4f));
    
    // Highlight selected row
    if (rowIsSelected)
        g.fillAll(juce::Colours::blue.withAlpha(0.2f));
}

void SoundcheckPanel::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(12.0f));
    
    // Get the analysis data for this row
    const auto& analysis = soundcheckEngine.getAnalysis(rowNumber);
    
    juce::String text;
    juce::Colour cellColor = juce::Colours::white;
    
    // Extract processor for channel name
    ChannelProcessor* processor = nullptr;
    if (rowNumber < totalChannels && rowNumber < static_cast<int>(channelProcessors.size())) {
        processor = channelProcessors[rowNumber];
    }
    
    // Format cell content based on column
    switch (columnId)
    {
        case 1: // Channel name
            if (processor != nullptr) {
                text = "Channel " + juce::String(rowNumber + 1);
            } else {
                text = "Channel " + juce::String(rowNumber + 1);
            }
            break;
            
        case 2: // Trim gain
            text = juce::String(analysis.trimGainSuggestion, 1) + " dB";
            cellColor = getCorrectionColor(std::abs(analysis.trimGainSuggestion));
            break;
            
        case 3: // Gate threshold
            text = juce::String(analysis.gateThresholdSuggestion, 1) + " dB";
            cellColor = juce::Colours::white; // Gate threshold doesn't use color coding
            break;
            
        case 4: // EQ Low
            text = juce::String(analysis.eqGainSuggestions[0], 1) + " dB";
            cellColor = getCorrectionColor(std::abs(analysis.eqGainSuggestions[0]));
            break;
            
        case 5: // EQ Low Mid
            text = juce::String(analysis.eqGainSuggestions[1], 1) + " dB";
            cellColor = getCorrectionColor(std::abs(analysis.eqGainSuggestions[1]));
            break;
            
        case 6: // EQ High Mid
            text = juce::String(analysis.eqGainSuggestions[2], 1) + " dB";
            cellColor = getCorrectionColor(std::abs(analysis.eqGainSuggestions[2]));
            break;
            
        case 7: // EQ High
            text = juce::String(analysis.eqGainSuggestions[3], 1) + " dB";
            cellColor = getCorrectionColor(std::abs(analysis.eqGainSuggestions[3]));
            break;
            
        case 8: // Compressor ratio
            text = juce::String(analysis.compressorRatioSuggestion, 1) + ":1";
            cellColor = analysis.compressorRatioSuggestion > 1.0f ? juce::Colours::orange : juce::Colours::lightgreen;
            break;
            
        default:
            text = "";
            break;
    }
    
    // Set text color based on value
    g.setColour(cellColor);
    
    // Draw text
    g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
}

juce::Colour SoundcheckPanel::getCorrectionColor(float value)
{
    // Return color based on correction magnitude: green (<6dB), yellow (<12dB), red (>=12dB)
    if (value < 0.01f) // Near zero
        return juce::Colours::lightgrey;
    else if (value < 6.0f)
        return juce::Colours::lightgreen;
    else if (value < 12.0f)
        return juce::Colours::yellow;
    else
        return juce::Colours::red;
}

void SoundcheckPanel::timerCallback()
{
    // Handle animation
    if (currentAlpha != targetAlpha)
    {
        // Animate alpha
        currentAlpha = currentAlpha * 0.7f + targetAlpha * 0.3f;
        
        // Check if we're close enough
        if (std::abs(currentAlpha - targetAlpha) < 0.01f)
        {
            currentAlpha = targetAlpha;
            
            // If we've reached 0, hide the component
            if (currentAlpha == 0.0f && !isShowing)
            {
                setVisible(false);
                stopTimer();
                return;
            }
        }
        
        // Apply alpha
        setAlpha(currentAlpha);
    }
    
    // If we're showing and not doing animation, update soundcheck progress
    if (isShowing && currentAlpha == targetAlpha && soundcheckEngine.isRunning())
    {
        // Get the current channel being analyzed from the engine
        int currentChannelIndex = 0;
        for (int i = 0; i < totalChannels; ++i) {
            if (soundcheckEngine.getAnalysis(i).avgRMS > 0.0f) {
                currentChannelIndex = i;
            }
        }
        
        // Update progress based on current channel being analyzed
        currentChannel = currentChannelIndex;
        
        // Update progress
        progress = static_cast<double>(currentChannel) / static_cast<double>(totalChannels);
        
        // Update UI
        channelLabel.setText("Analyzing Channel: " + juce::String(currentChannel + 1), 
                            juce::dontSendNotification);
        
        // Check if soundcheck has finished
        if (!soundcheckEngine.isRunning() && currentChannel >= totalChannels - 1)
        {
            // Update UI
            statusLabel.setText("Soundcheck complete! Review the suggested settings below:", 
                               juce::dontSendNotification);
            progress = 1.0;
            
            // Switch to results mode
            currentMode = DisplayMode::Results;
            updateUIForMode();
            
            // Enable results buttons
            applyButton.setEnabled(true);
            revertButton.setEnabled(true);
            
            // Refresh the table
            resultsTable.updateContent();
        }
        
        progressBar.repaint();
    }
}

void SoundcheckPanel::startButtonClicked()
{
    // Start the soundcheck
    soundcheckEngine.startCheck(5);  // 5 seconds per channel
    
    // Update UI
    statusLabel.setText("Soundcheck in progress...", juce::dontSendNotification);
    channelLabel.setText("Analyzing Channel: 1", juce::dontSendNotification);
    progress = 0.0;
    
    // Update button state
    startButton.setEnabled(false);
    stopButton.setEnabled(true);
    
    // Start timer to update progress
    startTimer(100);
}

void SoundcheckPanel::stopButtonClicked()
{
    // Stop the soundcheck
    soundcheckEngine.stopCheck();
    
    // Update UI
    statusLabel.setText("Soundcheck canceled", juce::dontSendNotification);
    
    // Update button state
    startButton.setEnabled(true);
    stopButton.setEnabled(false);
}

void SoundcheckPanel::applyButtonClicked()
{
    // Apply the corrections
    soundcheckEngine.applyCorrections();
    
    // Update UI
    statusLabel.setText("Corrections applied to all channels", juce::dontSendNotification);
    
    // Update button state
    applyButton.setEnabled(false);
    revertButton.setEnabled(true);
}

void SoundcheckPanel::revertButtonClicked()
{
    // Revert the corrections
    soundcheckEngine.revertCorrections();
    
    // Update UI
    statusLabel.setText("Reverted to original settings", juce::dontSendNotification);
    
    // Update button state
    applyButton.setEnabled(true);
    revertButton.setEnabled(false);
}

void SoundcheckPanel::setChannelProcessors(const std::vector<ChannelProcessor*>& processors)
{
    // Store processors locally
    channelProcessors = processors;
    
    // Pass processors to the soundcheck engine
    soundcheckEngine.setChannelProcessors(processors);
    
    // Update total channels count
    totalChannels = static_cast<int>(processors.size());
} 