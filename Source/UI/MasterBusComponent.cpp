#include "MasterBusComponent.h"

MasterBusComponent::MasterBusComponent(MasterBusProcessor* processor)
    : masterProcessor(processor)
{
    juce::Logger::writeToLog("MasterBusComponent constructor start");
    
    // Set up the look and feel
    setLookAndFeel(&blackwayLookAndFeel);
    
    // Load background image
    auto assetsDir = juce::File::getCurrentWorkingDirectory().getChildFile("Assets");
    backgroundImage = juce::ImageCache::getFromFile(assetsDir.getChildFile("background_02.png"));
    
    // Set up the level meter
    addAndMakeVisible(levelMeter);
    
    // Output meter label
    outputMeterLabel.setText("OUTPUT", juce::dontSendNotification);
    outputMeterLabel.setFont(blackwayLookAndFeel.getRobotoFont().withHeight(14.0f).boldened());
    outputMeterLabel.setJustificationType(juce::Justification::centred);
    outputMeterLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(outputMeterLabel);
    
    // Current LUFS display
    currentLufsLabel.setText("CURRENT: -18.0 LUFS", juce::dontSendNotification);
    currentLufsLabel.setFont(blackwayLookAndFeel.getRobotoFont().withHeight(14.0f));
    currentLufsLabel.setJustificationType(juce::Justification::centredLeft);
    currentLufsLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(currentLufsLabel);
    
    // Target LUFS display
    targetLufsLabel.setText("TARGET: -14.0 LUFS", juce::dontSendNotification);
    targetLufsLabel.setFont(blackwayLookAndFeel.getRobotoFont().withHeight(14.0f));
    targetLufsLabel.setJustificationType(juce::Justification::centredLeft);
    targetLufsLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(targetLufsLabel);
    
    // Set up effect toggles
    compressorToggle.setButtonText("COMPRESSOR");
    compressorToggle.setToggleState(true, juce::dontSendNotification);
    compressorToggle.onClick = [this] { masterProcessor->setCompressorEnabled(compressorToggle.getToggleState()); };
    addAndMakeVisible(compressorToggle);
    
    limiterToggle.setButtonText("LIMITER");
    limiterToggle.setToggleState(true, juce::dontSendNotification);
    limiterToggle.onClick = [this] { masterProcessor->setLimiterEnabled(limiterToggle.getToggleState()); };
    addAndMakeVisible(limiterToggle);
    
    // Set up target selection buttons
    youtubeButton.setButtonText("YouTube (-14 LUFS)");
    youtubeButton.setClickingTogglesState(true);
    youtubeButton.setRadioGroupId(1);
    youtubeButton.setToggleState(true, juce::dontSendNotification);
    youtubeButton.onClick = [this] {
        if (youtubeButton.getToggleState()) {
            masterProcessor->setStreamTarget(StreamTarget::YouTube);
            customLufsSlider.setVisible(false);
            customLufsLabel.setVisible(false);
            updateLufsDisplay();
        }
    };
    addAndMakeVisible(youtubeButton);
    
    facebookButton.setButtonText("Facebook (-16 LUFS)");
    facebookButton.setClickingTogglesState(true);
    facebookButton.setRadioGroupId(1);
    facebookButton.onClick = [this] {
        if (facebookButton.getToggleState()) {
            masterProcessor->setStreamTarget(StreamTarget::Facebook);
            customLufsSlider.setVisible(false);
            customLufsLabel.setVisible(false);
            updateLufsDisplay();
        }
    };
    addAndMakeVisible(facebookButton);
    
    customButton.setButtonText("Custom LUFS");
    customButton.setClickingTogglesState(true);
    customButton.setRadioGroupId(1);
    customButton.onClick = [this] {
        if (customButton.getToggleState()) {
            masterProcessor->setStreamTarget(StreamTarget::Custom);
            customLufsSlider.setVisible(true);
            customLufsLabel.setVisible(true);
            masterProcessor->setTargetLufs(static_cast<float>(customLufsSlider.getValue()));
            updateLufsDisplay();
        }
    };
    addAndMakeVisible(customButton);
    
    // Set up custom LUFS slider
    customLufsSlider.setRange(-24.0, -10.0, 0.1);
    customLufsSlider.setValue(-18.0);
    customLufsSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    customLufsSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    customLufsSlider.onValueChange = [this] {
        masterProcessor->setTargetLufs(static_cast<float>(customLufsSlider.getValue()));
        updateLufsDisplay();
    };
    customLufsSlider.setVisible(false);
    addAndMakeVisible(customLufsSlider);
    
    // Set up custom LUFS label
    customLufsLabel.setText("Custom LUFS", juce::dontSendNotification);
    customLufsLabel.setFont(blackwayLookAndFeel.getRobotoFont().withHeight(14.0f));
    customLufsLabel.setJustificationType(juce::Justification::centredLeft);
    customLufsLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    customLufsLabel.setVisible(false);
    addAndMakeVisible(customLufsLabel);
    
    // Set up loudness meter
    meter = std::make_unique<auralis::LoudnessMeterComponent>();
    addAndMakeVisible(*meter);
    
    // Set meter target in master bus
    masterProcessor->setMeterTarget(meter.get());
    
    // Start timer for updates
    startTimerHz(2);
    
    juce::Logger::writeToLog("MasterBusComponent constructor end");
}

MasterBusComponent::~MasterBusComponent()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void MasterBusComponent::paint(juce::Graphics& g)
{
    if (backgroundImage.isValid())
    {
        g.drawImage(backgroundImage, 
                   getLocalBounds().toFloat(),
                   juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        g.fillAll(juce::Colours::black);
    }
    
    // Draw section headers
    g.setColour(juce::Colours::white);
    g.setFont(blackwayLookAndFeel.getRobotoFont().withHeight(16.0f).boldened());
    g.drawText("MASTER BUS", 20, 10, 200, 30, juce::Justification::left);
    g.drawText("TARGET LOUDNESS", 20, 240, 200, 30, juce::Justification::left);
    g.drawText("PROCESSING", 20, 360, 200, 30, juce::Justification::left);
    
    // Draw separator lines
    g.setColour(juce::Colours::grey);
    g.drawLine(20, 40, getWidth() - 20, 40, 1.0f);
    g.drawLine(20, 270, getWidth() - 20, 270, 1.0f);
    g.drawLine(20, 390, getWidth() - 20, 390, 1.0f);
}

void MasterBusComponent::resized()
{
    auto area = getLocalBounds().reduced(20);
    auto topSection = area.removeFromTop(200);
    
    // Position the level meter
    auto meterArea = topSection.removeFromRight(80);
    outputMeterLabel.setBounds(meterArea.removeFromTop(25));
    levelMeter.setBounds(meterArea.reduced(0, 10));
    
    // Position the LUFS display
    auto lufsArea = topSection.removeFromTop(100).reduced(0, 10);
    currentLufsLabel.setBounds(lufsArea.removeFromTop(30));
    targetLufsLabel.setBounds(lufsArea.removeFromTop(30));
    
    // Position the loudness meter
    auto loudnessMeterArea = topSection.removeFromRight(60);
    meter->setBounds(loudnessMeterArea);
    
    // Target selection area
    auto targetArea = area.removeFromTop(120);
    youtubeButton.setBounds(targetArea.removeFromTop(30).reduced(0, 5));
    facebookButton.setBounds(targetArea.removeFromTop(30).reduced(0, 5));
    customButton.setBounds(targetArea.removeFromTop(30).reduced(0, 5));
    
    // Custom LUFS control
    auto customArea = targetArea.removeFromTop(30);
    customLufsLabel.setBounds(customArea.removeFromLeft(100));
    customLufsSlider.setBounds(customArea);
    
    // Processing toggles
    auto processingArea = area.removeFromTop(80);
    compressorToggle.setBounds(processingArea.removeFromTop(30).reduced(0, 5));
    limiterToggle.setBounds(processingArea.removeFromTop(30).reduced(0, 5));
}

void MasterBusComponent::updateLufsDisplay()
{
    if (masterProcessor)
    {
        // Update current LUFS display
        float currentLufs = masterProcessor->getCurrentLufs();
        currentLufsLabel.setText("CURRENT: " + juce::String(currentLufs, 1) + " LUFS", 
                               juce::dontSendNotification);
        
        // Update target LUFS display based on selected target
        targetLufsLabel.setText("TARGET: " + juce::String(masterProcessor->getStreamTarget() == StreamTarget::Custom 
                                           ? static_cast<float>(customLufsSlider.getValue())
                                           : masterProcessor->getStreamTarget() == StreamTarget::YouTube 
                                             ? kLUFS_Youtube : kLUFS_Facebook, 1) + " LUFS", 
                              juce::dontSendNotification);
    }
}

void MasterBusComponent::timerCallback()
{
    // Update the LUFS display
    if (masterProcessor)
    {
        currentLufsLabel.setText(juce::String(masterProcessor->getTargetLufs(), 1) + " LUFS", 
                               juce::dontSendNotification);
    }
} 