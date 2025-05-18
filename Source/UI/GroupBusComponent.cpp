#include "GroupBusComponent.h"

//==============================================================================
// GroupBusRowComponent Implementation
//==============================================================================

GroupBusRowComponent::GroupBusRowComponent(GroupBusProcessor* processor)
    : groupProcessor(processor)
{
    // Set up the bus name label
    addAndMakeVisible(busNameLabel);
    busNameLabel.setText(processor->getBusName(), juce::dontSendNotification);
    busNameLabel.setJustificationType(juce::Justification::centredLeft);
    busNameLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    
    // Set up the output gain slider (fader style)
    addAndMakeVisible(outputGainSlider);
    outputGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    outputGainSlider.setRange(0.0, 1.5, 0.01);
    outputGainSlider.setValue(processor->getOutputGain());
    outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    outputGainSlider.setTextValueSuffix(" dB");
    outputGainSlider.onValueChange = [this] { 
        outputGainSlider.setTextValueSuffix(" dB"); 
    };
    outputGainSlider.setNumDecimalPlacesToDisplay(1);
    outputGainSlider.setDoubleClickReturnValue(true, 1.0);
    outputGainSlider.addListener(this);
    
    // Set up EQ Low Gain knob
    addAndMakeVisible(eqLowGainSlider);
    eqLowGainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    eqLowGainSlider.setRange(-12.0, 12.0, 0.1);
    eqLowGainSlider.setValue(0.0);
    eqLowGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    eqLowGainSlider.setTextValueSuffix(" dB");
    eqLowGainSlider.setDoubleClickReturnValue(true, 0.0);
    eqLowGainSlider.addListener(this);
    
    addAndMakeVisible(eqLowLabel);
    eqLowLabel.setText("Low", juce::dontSendNotification);
    eqLowLabel.setJustificationType(juce::Justification::centred);
    eqLowLabel.attachToComponent(&eqLowGainSlider, false);
    
    // Set up EQ Mid Gain knob
    addAndMakeVisible(eqMidGainSlider);
    eqMidGainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    eqMidGainSlider.setRange(-12.0, 12.0, 0.1);
    eqMidGainSlider.setValue(0.0);
    eqMidGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    eqMidGainSlider.setTextValueSuffix(" dB");
    eqMidGainSlider.setDoubleClickReturnValue(true, 0.0);
    eqMidGainSlider.addListener(this);
    
    addAndMakeVisible(eqMidLabel);
    eqMidLabel.setText("Mid", juce::dontSendNotification);
    eqMidLabel.setJustificationType(juce::Justification::centred);
    eqMidLabel.attachToComponent(&eqMidGainSlider, false);
    
    // Set up EQ High Gain knob
    addAndMakeVisible(eqHighGainSlider);
    eqHighGainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    eqHighGainSlider.setRange(-12.0, 12.0, 0.1);
    eqHighGainSlider.setValue(0.0);
    eqHighGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    eqHighGainSlider.setTextValueSuffix(" dB");
    eqHighGainSlider.setDoubleClickReturnValue(true, 0.0);
    eqHighGainSlider.addListener(this);
    
    addAndMakeVisible(eqHighLabel);
    eqHighLabel.setText("High", juce::dontSendNotification);
    eqHighLabel.setJustificationType(juce::Justification::centred);
    eqHighLabel.attachToComponent(&eqHighGainSlider, false);
    
    // Set up compressor bypass toggle
    addAndMakeVisible(compToggle);
    compToggle.setToggleState(true, juce::dontSendNotification);  // Comp enabled by default
    compToggle.addListener(this);
    
    addAndMakeVisible(compLabel);
    compLabel.setText("Comp", juce::dontSendNotification);
    compLabel.setJustificationType(juce::Justification::centred);
    compLabel.attachToComponent(&compToggle, false);
    
    // Apply custom look and feel
    outputGainSlider.setLookAndFeel(&blackwayLookAndFeel);
    eqLowGainSlider.setLookAndFeel(&blackwayLookAndFeel);
    eqMidGainSlider.setLookAndFeel(&blackwayLookAndFeel);
    eqHighGainSlider.setLookAndFeel(&blackwayLookAndFeel);
    compToggle.setLookAndFeel(&blackwayLookAndFeel);
}

GroupBusRowComponent::~GroupBusRowComponent()
{
    outputGainSlider.setLookAndFeel(nullptr);
    eqLowGainSlider.setLookAndFeel(nullptr);
    eqMidGainSlider.setLookAndFeel(nullptr);
    eqHighGainSlider.setLookAndFeel(nullptr);
    compToggle.setLookAndFeel(nullptr);
}

void GroupBusRowComponent::paint(juce::Graphics& g)
{
    // Fill the background with a slightly highlighted color for better contrast
    auto area = getLocalBounds();
    g.setColour(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId).brighter(0.1f));
    g.fillRect(area);
    
    // Draw a subtle separator line
    g.setColour(juce::Colours::grey.withAlpha(0.3f));
    g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));
}

void GroupBusRowComponent::resized()
{
    auto area = getLocalBounds().reduced(2);
    
    // Layout the controls from left to right
    busNameLabel.setBounds(area.removeFromLeft(100));
    
    // Output gain slider (fader)
    auto gainArea = area.removeFromLeft(100);
    outputGainSlider.setBounds(gainArea.withSizeKeepingCentre(60, gainArea.getHeight() - 20));
    
    // EQ knobs
    auto eqLowArea = area.removeFromLeft(80);
    eqLowGainSlider.setBounds(eqLowArea.withSizeKeepingCentre(70, 90));
    
    auto eqMidArea = area.removeFromLeft(80);
    eqMidGainSlider.setBounds(eqMidArea.withSizeKeepingCentre(70, 90));
    
    auto eqHighArea = area.removeFromLeft(80);
    eqHighGainSlider.setBounds(eqHighArea.withSizeKeepingCentre(70, 90));
    
    // Comp toggle
    auto compArea = area.removeFromLeft(80);
    compToggle.setBounds(compArea.withSizeKeepingCentre(40, 40));
}

void GroupBusRowComponent::sliderValueChanged(juce::Slider* slider)
{
    if (!groupProcessor)
        return;
    
    if (slider == &outputGainSlider)
    {
        groupProcessor->setOutputGain(static_cast<float>(slider->getValue()));
    }
    else if (slider == &eqLowGainSlider)
    {
        groupProcessor->setEQLowGain(static_cast<float>(slider->getValue()));
    }
    else if (slider == &eqMidGainSlider)
    {
        groupProcessor->setEQMidGain(static_cast<float>(slider->getValue()));
    }
    else if (slider == &eqHighGainSlider)
    {
        groupProcessor->setEQHighGain(static_cast<float>(slider->getValue()));
    }
}

void GroupBusRowComponent::buttonClicked(juce::Button* button)
{
    if (!groupProcessor)
        return;
    
    if (button == &compToggle)
    {
        groupProcessor->setCompEnabled(button->getToggleState());
    }
}

//==============================================================================
// GroupBusComponent Implementation
//==============================================================================

GroupBusComponent::GroupBusComponent()
{
    // Set up the headers
    addAndMakeVisible(busNameHeader);
    busNameHeader.setText("Group Bus", juce::dontSendNotification);
    busNameHeader.setFont(juce::Font(14.0f, juce::Font::bold));
    busNameHeader.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(gainHeader);
    gainHeader.setText("Gain", juce::dontSendNotification);
    gainHeader.setFont(juce::Font(14.0f, juce::Font::bold));
    gainHeader.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(lowHeader);
    lowHeader.setText("Low", juce::dontSendNotification);
    lowHeader.setFont(juce::Font(14.0f, juce::Font::bold));
    lowHeader.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(midHeader);
    midHeader.setText("Mid", juce::dontSendNotification);
    midHeader.setFont(juce::Font(14.0f, juce::Font::bold));
    midHeader.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(highHeader);
    highHeader.setText("High", juce::dontSendNotification);
    highHeader.setFont(juce::Font(14.0f, juce::Font::bold));
    highHeader.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(compHeader);
    compHeader.setText("Comp", juce::dontSendNotification);
    compHeader.setFont(juce::Font(14.0f, juce::Font::bold));
    compHeader.setJustificationType(juce::Justification::centred);
    
    // Apply custom look and feel to headers
    busNameHeader.setLookAndFeel(&blackwayLookAndFeel);
    gainHeader.setLookAndFeel(&blackwayLookAndFeel);
    lowHeader.setLookAndFeel(&blackwayLookAndFeel);
    midHeader.setLookAndFeel(&blackwayLookAndFeel);
    highHeader.setLookAndFeel(&blackwayLookAndFeel);
    compHeader.setLookAndFeel(&blackwayLookAndFeel);
}

GroupBusComponent::~GroupBusComponent()
{
    busNameHeader.setLookAndFeel(nullptr);
    gainHeader.setLookAndFeel(nullptr);
    lowHeader.setLookAndFeel(nullptr);
    midHeader.setLookAndFeel(nullptr);
    highHeader.setLookAndFeel(nullptr);
    compHeader.setLookAndFeel(nullptr);
}

void GroupBusComponent::paint(juce::Graphics& g)
{
    // Fill the background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw a header background
    auto headerArea = getLocalBounds().removeFromTop(30);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRect(headerArea);
    
    // Draw a separator line below the headers
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawHorizontalLine(30, 0.0f, static_cast<float>(getWidth()));
}

void GroupBusComponent::resized()
{
    auto area = getLocalBounds();
    
    // Header row
    auto headerArea = area.removeFromTop(30);
    
    // Layout the header labels
    busNameHeader.setBounds(headerArea.removeFromLeft(100));
    gainHeader.setBounds(headerArea.removeFromLeft(100));
    lowHeader.setBounds(headerArea.removeFromLeft(80));
    midHeader.setBounds(headerArea.removeFromLeft(80));
    highHeader.setBounds(headerArea.removeFromLeft(80));
    compHeader.setBounds(headerArea.removeFromLeft(80));
    
    // Layout each group bus row
    int rowHeight = 100;
    for (auto& row : busRows)
    {
        row->setBounds(0, 30 + (rowHeight * (&row - &busRows[0])), getWidth(), rowHeight);
    }
}

void GroupBusComponent::connectToProcessors(std::vector<GroupBusProcessor*> processors)
{
    // Store processor references
    groupProcessors = processors;
    
    // Clear any existing rows
    busRows.clear();
    
    // Create row components for each processor
    for (auto* processor : processors)
    {
        auto row = std::make_unique<GroupBusRowComponent>(processor);
        addAndMakeVisible(*row);
        busRows.push_back(std::move(row));
    }
    
    // Resize to trigger layout update
    resized();
} 