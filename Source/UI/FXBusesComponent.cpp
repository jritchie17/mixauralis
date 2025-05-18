#include "FXBusesComponent.h"

//==============================================================================
// FXBusRowComponent Implementation
//==============================================================================

FXBusRowComponent::FXBusRowComponent(FXBusProcessor* processor)
    : fxProcessor(processor)
{
    // Set look and feel
    setLookAndFeel(&blackwayLookAndFeel);
    
    // Bus name label
    busNameLabel.setText(fxProcessor->getBusName(), juce::dontSendNotification);
    busNameLabel.setJustificationType(juce::Justification::centredLeft);
    busNameLabel.setFont(blackwayLookAndFeel.getRobotoFont().withHeight(14.0f).boldened());
    busNameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(busNameLabel);
    
    // Reverb wet slider
    reverbWetSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    reverbWetSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    reverbWetSlider.setRange(0.0, 100.0, 0.1);
    reverbWetSlider.setValue(fxProcessor->getReverbWetLevel() * 100.0);
    reverbWetSlider.addListener(this);
    addAndMakeVisible(reverbWetSlider);
    
    reverbLabel.setText("Reverb", juce::dontSendNotification);
    reverbLabel.setJustificationType(juce::Justification::centred);
    reverbLabel.setFont(blackwayLookAndFeel.getRobotoFont().withHeight(12.0f));
    reverbLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(reverbLabel);
    
    // Delay wet slider
    delayWetSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    delayWetSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    delayWetSlider.setRange(0.0, 100.0, 0.1);
    delayWetSlider.setValue(fxProcessor->getDelayWetLevel() * 100.0);
    delayWetSlider.addListener(this);
    addAndMakeVisible(delayWetSlider);
    
    delayLabel.setText("Delay", juce::dontSendNotification);
    delayLabel.setJustificationType(juce::Justification::centred);
    delayLabel.setFont(blackwayLookAndFeel.getRobotoFont().withHeight(12.0f));
    delayLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(delayLabel);
    
    // Bypass toggle
    bypassToggle.setButtonText("");
    bypassToggle.setToggleState(fxProcessor->isBypassed(), juce::dontSendNotification);
    bypassToggle.addListener(this);
    addAndMakeVisible(bypassToggle);
    
    bypassLabel.setText("Bypass", juce::dontSendNotification);
    bypassLabel.setJustificationType(juce::Justification::centred);
    bypassLabel.setFont(blackwayLookAndFeel.getRobotoFont().withHeight(12.0f));
    bypassLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(bypassLabel);
}

FXBusRowComponent::~FXBusRowComponent()
{
    setLookAndFeel(nullptr);
}

void FXBusRowComponent::paint(juce::Graphics& g)
{
    // Use a subtle background for each row
    g.setColour(juce::Colour(0x20ffffff));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(4), 4);
    
    g.setColour(juce::Colour(0x30ffffff));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(4), 4, 1);
}

void FXBusRowComponent::resized()
{
    auto bounds = getLocalBounds().reduced(8);
    
    // Use FlexBox for layout
    juce::FlexBox row;
    row.flexDirection = juce::FlexBox::Direction::row;
    row.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
    row.alignItems = juce::FlexBox::AlignItems::center;
    
    // Bus name takes up about 20% of width
    float nameWidth = bounds.getWidth() * 0.2f;
    row.items.add(juce::FlexItem(busNameLabel).withWidth(nameWidth).withHeight(30.0f));
    
    // Reverb controls take up about 30% of width
    float reverbWidth = bounds.getWidth() * 0.3f;
    juce::FlexBox reverbBox;
    reverbBox.flexDirection = juce::FlexBox::Direction::column;
    reverbBox.justifyContent = juce::FlexBox::JustifyContent::center;
    reverbBox.alignItems = juce::FlexBox::AlignItems::center;
    
    reverbBox.items.add(juce::FlexItem(reverbLabel).withWidth(100.0f).withHeight(20.0f));
    reverbBox.items.add(juce::FlexItem(reverbWetSlider).withWidth(100.0f).withHeight(100.0f));
    
    row.items.add(juce::FlexItem(reverbBox).withWidth(reverbWidth).withHeight(120.0f));
    
    // Delay controls take up about 30% of width
    float delayWidth = bounds.getWidth() * 0.3f;
    juce::FlexBox delayBox;
    delayBox.flexDirection = juce::FlexBox::Direction::column;
    delayBox.justifyContent = juce::FlexBox::JustifyContent::center;
    delayBox.alignItems = juce::FlexBox::AlignItems::center;
    
    delayBox.items.add(juce::FlexItem(delayLabel).withWidth(100.0f).withHeight(20.0f));
    delayBox.items.add(juce::FlexItem(delayWetSlider).withWidth(100.0f).withHeight(100.0f));
    
    row.items.add(juce::FlexItem(delayBox).withWidth(delayWidth).withHeight(120.0f));
    
    // Bypass toggle takes up about 20% of width
    float bypassWidth = bounds.getWidth() * 0.2f;
    juce::FlexBox bypassBox;
    bypassBox.flexDirection = juce::FlexBox::Direction::column;
    bypassBox.justifyContent = juce::FlexBox::JustifyContent::center;
    bypassBox.alignItems = juce::FlexBox::AlignItems::center;
    
    bypassBox.items.add(juce::FlexItem(bypassLabel).withWidth(60.0f).withHeight(20.0f));
    bypassBox.items.add(juce::FlexItem(bypassToggle).withWidth(40.0f).withHeight(40.0f));
    
    row.items.add(juce::FlexItem(bypassBox).withWidth(bypassWidth).withHeight(60.0f));
    
    // Perform layout
    row.performLayout(bounds);
}

void FXBusRowComponent::sliderValueChanged(juce::Slider* slider)
{
    if (fxProcessor == nullptr)
        return;
        
    if (slider == &reverbWetSlider)
    {
        fxProcessor->setReverbWetLevel(static_cast<float>(reverbWetSlider.getValue() / 100.0));
    }
    else if (slider == &delayWetSlider)
    {
        fxProcessor->setDelayWetLevel(static_cast<float>(delayWetSlider.getValue() / 100.0));
    }
}

void FXBusRowComponent::buttonClicked(juce::Button* button)
{
    if (fxProcessor == nullptr)
        return;
        
    if (button == &bypassToggle)
    {
        fxProcessor->setBypass(bypassToggle.getToggleState());
    }
}

//==============================================================================
// FXBusesComponent Implementation
//==============================================================================

FXBusesComponent::FXBusesComponent()
{
    // Apply custom look and feel
    setLookAndFeel(&blackwayLookAndFeel);
    
    // Create the Group Bus Component
    groupBusComponent = std::make_unique<GroupBusComponent>();
    addAndMakeVisible(*groupBusComponent);
    
    // Load background image
    auto assetsDir = juce::File::getCurrentWorkingDirectory().getChildFile("Assets");
    backgroundImage = juce::ImageCache::getFromFile(assetsDir.getChildFile("background_03.png"));
}

FXBusesComponent::~FXBusesComponent()
{
    setLookAndFeel(nullptr);
}

void FXBusesComponent::paint(juce::Graphics& g)
{
    if (backgroundImage.isValid())
    {
        // Draw the background image
        g.drawImage(backgroundImage, 
                   getLocalBounds().toFloat(),
                   juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        g.fillAll(juce::Colours::black);
    }
}

void FXBusesComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // First, allocate space for the Group Bus component at the top
    int groupBusHeight = 430; // Height needed for 4 group buses (100*4 + 30 header)
    if (groupBusComponent != nullptr)
    {
        groupBusComponent->setBounds(bounds.getX(), bounds.getY(), bounds.getWidth(), groupBusHeight);
    }
    
    // Adjust bounds for the FX buses below the Group Bus component
    auto fxBusesBounds = bounds.withTrimmedTop(groupBusHeight + 20); // Add some spacing
    
    // Calculate height for each FX bus row
    int rowHeight = juce::jmin(150, fxBusesBounds.getHeight() / static_cast<int>(busRows.size()));
    
    // Position each FX bus row
    for (int i = 0; i < busRows.size(); ++i)
    {
        busRows[i]->setBounds(fxBusesBounds.getX(), 
                              fxBusesBounds.getY() + i * rowHeight, 
                              fxBusesBounds.getWidth(), 
                              rowHeight);
    }
}

void FXBusesComponent::connectToProcessors(std::vector<FXBusProcessor*> processors)
{
    // Store processor references
    fxProcessors = processors;
    
    // Clear any existing rows
    busRows.clear();
    
    // Create row components for each processor
    for (auto* processor : processors)
    {
        auto row = std::make_unique<FXBusRowComponent>(processor);
        addAndMakeVisible(*row);
        busRows.push_back(std::move(row));
    }
    
    // Resize to trigger layout update
    resized();
}

void FXBusesComponent::connectToGroupBusProcessors(std::vector<GroupBusProcessor*> groupProcessors)
{
    // Connect the group bus component to the processors
    if (groupBusComponent != nullptr)
    {
        groupBusComponent->connectToProcessors(groupProcessors);
    }
    
    // Resize to trigger layout update
    resized();
} 