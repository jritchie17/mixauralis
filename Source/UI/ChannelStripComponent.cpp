#include "ChannelStripComponent.h"
#include "LevelMeter.h"
#include "ChannelsComponent.h"
#include "../Audio/AudioEngine.h"
#include "../Utils/StyleManager.h"

ChannelStripComponent::ChannelStripComponent(int index, ChannelsComponent* parent)
    : parentComponent(parent), channelIndex(index)
{
    // Use the global look and feel
    setLookAndFeel(&StyleManager::getInstance().getLookAndFeel());
    
    // Load background image
    auto assetsDir = juce::File::getCurrentWorkingDirectory().getChildFile("Assets");
    auto bgSvgFile = assetsDir.getChildFile("background_02.svg");
    
    if (bgSvgFile.existsAsFile())
    {
        std::unique_ptr<juce::Drawable> svgDrawable = juce::Drawable::createFromSVGFile(bgSvgFile);
        if (svgDrawable != nullptr)
        {
            // Convert the SVG to an image
            backgroundImage = juce::Image(juce::Image::ARGB, kStandardWidth, 600, true);
            juce::Graphics g(backgroundImage);
            svgDrawable->drawWithin(g, juce::Rectangle<float>(0, 0, kStandardWidth, 600), 
                                  juce::RectanglePlacement::stretchToFit, 1.0f);
        }
    }
    else
    {
        // Fallback if SVG not found
        backgroundImage = juce::ImageCache::getFromFile(assetsDir.getChildFile("background_02.png"));
    }
    
    // Channel index label setup
    indexLabel.setText("--", juce::dontSendNotification);
    indexLabel.setJustificationType(juce::Justification::centredLeft);
    indexLabel.setFont(StyleManager::getInstance().getLookAndFeel().getRobotoFont().withHeight(14.0f).boldened());
    indexLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(indexLabel);
    
    // Name Label setup
    nameLabel.setText(channelName, juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centredLeft);
    nameLabel.setFont(StyleManager::getInstance().getLookAndFeel().getRobotoFont().withHeight(12.0f).boldened());
    nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(nameLabel);
    
    // Trim Dial setup
    trimDial.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    trimDial.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    trimDial.setRange(-24.0, 24.0, 0.1);
    trimDial.setValue(0.0);
    trimDial.addListener(this);
    addAndMakeVisible(trimDial);
    
    trimLabel.setText("Trim", juce::dontSendNotification);
    trimLabel.setJustificationType(juce::Justification::centred);
    trimLabel.setFont(StyleManager::getInstance().getLookAndFeel().getRobotoFont().withHeight(10.0f));
    trimLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(trimLabel);
    
    trimValueLabel.setText("0 dB", juce::dontSendNotification);
    trimValueLabel.setJustificationType(juce::Justification::centred);
    trimValueLabel.setFont(StyleManager::getInstance().getLookAndFeel().getRobotoFont().withHeight(10.0f));
    trimValueLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(trimValueLabel);
    
    // Gate Toggle setup
    gateToggle.setButtonText("");
    gateToggle.addListener(this);
    addAndMakeVisible(gateToggle);
    
    gateLabel.setText("Gate", juce::dontSendNotification);
    gateLabel.setJustificationType(juce::Justification::centred);
    gateLabel.setFont(StyleManager::getInstance().getLookAndFeel().getRobotoFont().withHeight(10.0f));
    gateLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(gateLabel);
    
    // Compressor Toggle setup
    compToggle.setButtonText("");
    compToggle.addListener(this);
    addAndMakeVisible(compToggle);
    
    compLabel.setText("Comp", juce::dontSendNotification);
    compLabel.setJustificationType(juce::Justification::centred);
    compLabel.setFont(StyleManager::getInstance().getLookAndFeel().getRobotoFont().withHeight(10.0f));
    compLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(compLabel);
    
    // EQ Button setup
    eqButton.setButtonText("EQ");
    eqButton.addListener(this);
    addAndMakeVisible(eqButton);
    
    // FX Send Dial setup
    fxSendDial.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    fxSendDial.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    fxSendDial.setRange(0.0, 100.0, 0.1);
    fxSendDial.setValue(0.0);
    fxSendDial.addListener(this);
    addAndMakeVisible(fxSendDial);
    
    fxSendLabel.setText("FX Send", juce::dontSendNotification);
    fxSendLabel.setJustificationType(juce::Justification::centred);
    fxSendLabel.setFont(StyleManager::getInstance().getLookAndFeel().getRobotoFont().withHeight(10.0f));
    fxSendLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(fxSendLabel);
    
    fxSendValueLabel.setText("0 %", juce::dontSendNotification);
    fxSendValueLabel.setJustificationType(juce::Justification::centred);
    fxSendValueLabel.setFont(StyleManager::getInstance().getLookAndFeel().getRobotoFont().withHeight(10.0f));
    fxSendValueLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(fxSendValueLabel);
    
    // Tuner Toggle setup (only visible for vocal channels)
    tunerToggle.setButtonText("");
    tunerToggle.addListener(this);
    tunerToggle.setVisible(false); // Initially hidden
    addAndMakeVisible(tunerToggle);
    
    tunerLabel.setText("Tuner", juce::dontSendNotification);
    tunerLabel.setJustificationType(juce::Justification::centred);
    tunerLabel.setFont(StyleManager::getInstance().getLookAndFeel().getRobotoFont().withHeight(10.0f));
    tunerLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    tunerLabel.setVisible(false); // Initially hidden
    addAndMakeVisible(tunerLabel);
    
    // Tuner Dial setup
    tunerDial.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    tunerDial.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    tunerDial.setRange(0.0, 1.0, 0.01);
    tunerDial.setValue(0.5);
    tunerDial.addListener(this);
    tunerDial.setVisible(false); // Initially hidden
    addAndMakeVisible(tunerDial);
    
    tunerValueLabel.setText("50 %", juce::dontSendNotification);
    tunerValueLabel.setJustificationType(juce::Justification::centred);
    tunerValueLabel.setFont(StyleManager::getInstance().getLookAndFeel().getRobotoFont().withHeight(10.0f));
    tunerValueLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    tunerValueLabel.setVisible(false); // Initially hidden
    addAndMakeVisible(tunerValueLabel);
    
    // Mute/Solo buttons
    auto muteIcon = juce::ImageCache::getFromFile(assetsDir.getChildFile("BlackwayFX/icons/mute.png"));
    auto soloIcon = juce::ImageCache::getFromFile(assetsDir.getChildFile("BlackwayFX/icons/solo.png"));
    
    muteButton.setImages(false, true, true,
                        muteIcon, 1.0f, {},
                        juce::Image(), 0.7f, {},
                        juce::Image(), 0.5f, {});
    muteButton.addListener(this);
    addAndMakeVisible(muteButton);
    
    soloButton.setImages(false, true, true,
                        soloIcon, 1.0f, {},
                        juce::Image(), 0.7f, {},
                        juce::Image(), 0.5f, {});
    soloButton.addListener(this);
    addAndMakeVisible(soloButton);
    
    // Level Meter setup - use the new meter component
    levelMeter = std::make_unique<LevelMeter>();
    addAndMakeVisible(*levelMeter);
    
    // Load the type icon
    loadTypeIcon();
    
    // Set up tooltips
    setupTooltips();
    
    // Enable keyboard focus
    setWantsKeyboardFocus(true);
    
    // Set fixed width for the channel strip
    setSize(kStandardWidth, 500); // Height will be determined by parent
    
    // Set up the input combo box
    inputCombo.setJustificationType(juce::Justification::centred);
    inputCombo.addListener(this);
    addAndMakeVisible(inputCombo);
}

ChannelStripComponent::~ChannelStripComponent()
{
    setLookAndFeel(nullptr);
}

void ChannelStripComponent::paint(juce::Graphics& g)
{
    if (!isEnabled)
        g.setOpacity(0.4f);

    auto bounds = getLocalBounds();
    
    if (backgroundImage.isValid())
    {
        // Draw the background image
        g.drawImage(backgroundImage,
                   bounds.toFloat(),
                   juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        // Fallback to solid color if no image
        g.fillAll(juce::Colours::darkgrey);
    }
    
    // Draw keyboard focus outline
    if (hasKeyboardFocus(false))
    {
        g.setColour(juce::Colours::cyan);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1).toFloat(), 2.0f, 2.0f);
    }
    
    // Draw the coloured band at the top
    auto bandColour = getChannelColourBand();
    g.setColour(bandColour);
    g.fillRect(bounds.getX(), bounds.getY(), bounds.getWidth(), 4);
    
    // Draw channel type icon
    if (typeIcon.isValid())
    {
        g.drawImage(typeIcon, 
                    bounds.getX() + 8, bounds.getY() + 8, 24, 24,
                    0, 0, typeIcon.getWidth(), typeIcon.getHeight(),
                    false);
    }
    
    // Draw the meter tick marks or color zones
    auto meterBounds = levelMeter->getBounds();
    
    // Draw tick marks (faint lines for dB levels)
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    
    // -6 dB mark (approximately 0.5 of the meter height)
    int minus6dBY = meterBounds.getY() + meterBounds.getHeight() * 0.5f;
    g.drawLine(meterBounds.getX(), minus6dBY, meterBounds.getRight(), minus6dBY, 1.0f);
    
    // -24 dB mark (approximately 0.75 of the meter height)
    int minus24dBY = meterBounds.getY() + meterBounds.getHeight() * 0.75f;
    g.drawLine(meterBounds.getX(), minus24dBY, meterBounds.getRight(), minus24dBY, 1.0f);
    
    // -60 dB mark (approximately 0.95 of the meter height)
    int minus60dBY = meterBounds.getY() + meterBounds.getHeight() * 0.95f;
    g.drawLine(meterBounds.getX(), minus60dBY, meterBounds.getRight(), minus60dBY, 1.0f);
    
    // Draw selection highlight if selected
    if (selected)
    {
        g.setColour(juce::Colours::orange.withAlpha(0.3f));
        g.fillRect(bounds);
    }
}

void ChannelStripComponent::resized()
{
    auto bounds = getLocalBounds().reduced(kStandardPadding);
    
    // Use FlexBox for layout
    juce::FlexBox mainColumn;
    mainColumn.flexDirection = juce::FlexBox::Direction::column;
    mainColumn.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
    mainColumn.alignItems = juce::FlexBox::AlignItems::center;
    
    // Channel Name and index at the top
    auto headerHeight = 32;
    auto headerArea = bounds.removeFromTop(headerHeight);
    
    // Position index and name labels 
    indexLabel.setBounds(headerArea.getX() + 32, headerArea.getY(), 30, 24);
    nameLabel.setBounds(indexLabel.getRight() + 2, headerArea.getY(), headerArea.getWidth() - 34, 24);
    
    // Position mute/solo buttons at the bottom
    auto bottomArea = bounds.removeFromBottom(kToggleSize);
    muteButton.setBounds(bottomArea.getX(), bottomArea.getY(), kToggleSize, kToggleSize);
    soloButton.setBounds(bottomArea.getX() + kToggleSize + 4, bottomArea.getY(), kToggleSize, kToggleSize);
    
    // Trim dial with fixed size
    auto controlWidth = kControlSize;
    mainColumn.items.add(juce::FlexItem(trimDial).withHeight(controlWidth).withWidth(controlWidth));
    mainColumn.items.add(juce::FlexItem(trimLabel).withHeight(14.0f).withWidth(60.0f));
    mainColumn.items.add(juce::FlexItem(trimValueLabel).withHeight(14.0f).withWidth(60.0f)
                         .withMargin(juce::FlexItem::Margin(0, 0, kStandardPadding, 0)));
    
    // Gate toggle
    mainColumn.items.add(juce::FlexItem(gateToggle).withHeight(kToggleSize).withWidth(kToggleSize));
    mainColumn.items.add(juce::FlexItem(gateLabel).withHeight(14.0f).withWidth(60.0f)
                         .withMargin(juce::FlexItem::Margin(0, 0, kStandardPadding, 0)));
    
    // Comp toggle
    mainColumn.items.add(juce::FlexItem(compToggle).withHeight(kToggleSize).withWidth(kToggleSize));
    mainColumn.items.add(juce::FlexItem(compLabel).withHeight(14.0f).withWidth(60.0f)
                         .withMargin(juce::FlexItem::Margin(0, 0, kStandardPadding, 0)));
    
    // EQ button
    mainColumn.items.add(juce::FlexItem(eqButton).withHeight(24.0f).withWidth(bounds.getWidth() - 2 * kStandardPadding)
                         .withMargin(juce::FlexItem::Margin(0, 0, kStandardPadding, 0)));
    
    // FX Send dial with fixed size
    mainColumn.items.add(juce::FlexItem(fxSendDial).withHeight(controlWidth).withWidth(controlWidth));
    mainColumn.items.add(juce::FlexItem(fxSendLabel).withHeight(14.0f).withWidth(60.0f));
    mainColumn.items.add(juce::FlexItem(fxSendValueLabel).withHeight(14.0f).withWidth(60.0f)
                         .withMargin(juce::FlexItem::Margin(0, 0, kStandardPadding, 0)));
    
    // Tuner toggle and dial (only visible for vocal channels)
    mainColumn.items.add(juce::FlexItem(tunerToggle).withHeight(kToggleSize).withWidth(kToggleSize));
    mainColumn.items.add(juce::FlexItem(tunerLabel).withHeight(14.0f).withWidth(60.0f));
    mainColumn.items.add(juce::FlexItem(tunerDial).withHeight(kControlSize).withWidth(kControlSize));
    mainColumn.items.add(juce::FlexItem(tunerValueLabel).withHeight(14.0f).withWidth(60.0f)
                         .withMargin(juce::FlexItem::Margin(0, 0, kStandardPadding, 0)));
    
    // Calculate total height of all items (excluding level meter)
    float totalHeight = 0.0f;
    for (auto& item : mainColumn.items)
        totalHeight += item.height + item.margin.top + item.margin.bottom;
    
    // Position the level meter along the side
    auto meterWidth = 6; // 6px width as per spec
    levelMeter->setBounds(bounds.getRight() - meterWidth - kStandardPadding, 
                         headerArea.getBottom() + kStandardPadding,
                         meterWidth, 
                         bounds.getHeight() - headerArea.getHeight() - bottomArea.getHeight() - 3 * kStandardPadding);
    
    // Place the input combo at the top
    inputCombo.setBounds(bounds.removeFromTop(20));
    
    // Place the tuner dial
    auto dialSize = 60;
    auto dialBounds = bounds.removeFromTop(dialSize);
    tunerDial.setBounds(dialBounds.withSizeKeepingCentre(dialSize, dialSize));
    
    // Perform layout of main column
    mainColumn.performLayout(bounds.withTrimmedTop(headerHeight));
}

void ChannelStripComponent::setChannelName(const juce::String& name)
{
    channelName = name;
    
    // Update the name label
    nameLabel.setText(channelName, juce::dontSendNotification);
}

void ChannelStripComponent::setChannelIndex(int index)
{
    channelIndex = index;
    
    // Format the channel number with leading zeros
    indexLabel.setText(juce::String::formatted("%02d", index + 1), juce::dontSendNotification);
}

void ChannelStripComponent::setChannelType(ChannelType type)
{
    channelType = type;
    
    // Update visibility of tuner controls based on channel type
    bool isVocalChannel = (type == ChannelType::SingingVocal);
    tunerToggle.setVisible(isVocalChannel);
    tunerLabel.setVisible(isVocalChannel);
    tunerDial.setVisible(isVocalChannel);
    tunerValueLabel.setVisible(isVocalChannel);
    
    // Load the appropriate type icon
    loadTypeIcon();
    
    resized(); // Update layout
    repaint(); // Repaint to show the color band
}

void ChannelStripComponent::connectToProcessor(ChannelProcessor* processor)
{
    channelProcessor = processor;
    
    // Update UI to reflect processor state
    if (channelProcessor != nullptr)
    {
        trimDial.setValue(channelProcessor->getTrimGain(), juce::dontSendNotification);
        gateToggle.setToggleState(channelProcessor->isGateEnabled(), juce::dontSendNotification);
        compToggle.setToggleState(channelProcessor->isCompressorEnabled(), juce::dontSendNotification);
        // EQ button doesn't have a toggle state
        fxSendDial.setValue(channelProcessor->getFxSendLevel() * 100.0, juce::dontSendNotification);
        tunerToggle.setToggleState(channelProcessor->isTunerEnabled(), juce::dontSendNotification);
        
        // Update value labels
        updateValueLabels();
    }
}

void ChannelStripComponent::buttonClicked(juce::Button* button)
{
    if (channelProcessor == nullptr)
        return;
        
    if (button == &gateToggle)
    {
        channelProcessor->setGateEnabled(gateToggle.getToggleState());
    }
    else if (button == &compToggle)
    {
        channelProcessor->setCompressorEnabled(compToggle.getToggleState());
    }
    else if (button == &eqButton)
    {
        // In a real app, this would open an EQ editor window
        channelProcessor->setEqEnabled(!channelProcessor->isEqEnabled());
    }
    else if (button == &tunerToggle)
    {
        channelProcessor->setTunerEnabled(tunerToggle.getToggleState());
    }
    else if (button == &muteButton)
    {
        if (channelProcessor != nullptr)
        {
            bool newState = !channelProcessor->isMuted();
            channelProcessor->setMuted(newState);
            updateIconColours();
        }
    }
    else if (button == &soloButton)
    {
        if (channelProcessor != nullptr)
        {
            bool newState = !channelProcessor->isSolo();
            channelProcessor->setSolo(newState);
            updateIconColours();
        }
    }
}

void ChannelStripComponent::sliderValueChanged(juce::Slider* slider)
{
    if (channelProcessor == nullptr)
        return;
        
    if (slider == &trimDial)
    {
        float gainInDb = static_cast<float>(slider->getValue());
        channelProcessor->setTrimGain(gainInDb);
        trimValueLabel.setText(juce::String(gainInDb, 1) + " dB", juce::dontSendNotification);
    }
    else if (slider == &fxSendDial)
    {
        float level = static_cast<float>(slider->getValue());
        channelProcessor->setFxSendLevel(level / 100.0f);
        fxSendValueLabel.setText(juce::String(level, 1) + " %", juce::dontSendNotification);
    }
    else if (slider == &tunerDial)
    {
        float strength = static_cast<float>(slider->getValue());
        channelProcessor->setTunerStrength(strength / 100.0f);
        tunerValueLabel.setText(juce::String(strength, 1) + " %", juce::dontSendNotification);
    }
}

void ChannelStripComponent::mouseDown(const juce::MouseEvent& event)
{
    // Notify parent component of selection
    if (parentComponent != nullptr)
    {
        setSelected(true);
    }
}

void ChannelStripComponent::setSelected(bool shouldBeSelected)
{
    if (selected != shouldBeSelected)
    {
        selected = shouldBeSelected;
        repaint(); // Trigger repaint to show/hide selection outline
    }
}

void ChannelStripComponent::updateValueLabels()
{
    // Update value labels based on current control values
    if (channelProcessor != nullptr)
    {
        trimValueLabel.setText(juce::String(int(trimDial.getValue())) + " dB", juce::dontSendNotification);
        fxSendValueLabel.setText(juce::String(int(fxSendDial.getValue())) + " %", juce::dontSendNotification);
    }
}

void ChannelStripComponent::refreshParametersFromProcessor()
{
    if (channelProcessor != nullptr)
    {
        juce::Logger::writeToLog("Refreshing UI for channel " + juce::String(channelIndex) + " from processor");
        
        // Update all controls to match processor state
        trimDial.setValue(channelProcessor->getTrimGain(), juce::dontSendNotification);
        juce::Logger::writeToLog("Trim gain set to: " + juce::String(channelProcessor->getTrimGain()));
        
        gateToggle.setToggleState(channelProcessor->isGateEnabled(), juce::dontSendNotification);
        compToggle.setToggleState(channelProcessor->isCompressorEnabled(), juce::dontSendNotification);
        eqButton.setToggleState(channelProcessor->isEqEnabled(), juce::dontSendNotification);
        fxSendDial.setValue(channelProcessor->getFxSendLevel() * 100.0, juce::dontSendNotification);
        tunerToggle.setToggleState(channelProcessor->isTunerEnabled(), juce::dontSendNotification);
        
        // Update value labels
        updateValueLabels();
        
        // Force a repaint to show the changes
        repaint();
    }
}

void ChannelStripComponent::setupTooltips()
{
    // Set up tooltips for the controls
    trimDial.setTooltip("Adjust input gain (-12 dB to +12 dB)");
    gateToggle.setTooltip("Noise gate (cuts background bleed)");
    compToggle.setTooltip("Compressor (smooths dynamics)");
    eqButton.setTooltip("Open 4-band EQ");
    fxSendDial.setTooltip("Amount sent to reverb/delay bus");
    tunerToggle.setTooltip("Subtle vocal pitch-correction");
}

void ChannelStripComponent::loadTypeIcon()
{
    // Load the appropriate icon based on the channel type
    auto assetsDir = juce::File::getCurrentWorkingDirectory().getChildFile("Assets/BlackwayFX/icons");
    
    juce::String iconName;
    
    switch (channelType)
    {
        case ChannelType::SingingVocal:
            iconName = "vocal_icon.svg";
            break;
        case ChannelType::Instrument:
            iconName = "instrument_icon.svg";
            break;
        case ChannelType::Drums:
            iconName = "drums_icon.svg";
            break;
        case ChannelType::Speech:
            iconName = "speech_icon.svg";
            break;
        default:
            iconName = "misc_icon.svg";
            break;
    }
    
    auto svgFile = assetsDir.getChildFile(iconName);
    if (svgFile.existsAsFile())
    {
        std::unique_ptr<juce::Drawable> svgDrawable = juce::Drawable::createFromSVGFile(svgFile);
        if (svgDrawable != nullptr)
        {
            // Convert the SVG to a 24x24 image
            typeIcon = juce::Image(juce::Image::ARGB, 24, 24, true);
            juce::Graphics g(typeIcon);
            svgDrawable->drawWithin(g, juce::Rectangle<float>(0, 0, 24, 24), 
                                  juce::RectanglePlacement::centred, 1.0f);
        }
    }
    
    // If icon not found, create a default colored square as a fallback
    if (!typeIcon.isValid())
    {
        typeIcon = juce::Image(juce::Image::RGB, 24, 24, true);
        juce::Graphics g(typeIcon);
        
        switch (channelType)
        {
            case ChannelType::SingingVocal:
                g.fillAll(juce::Colour(0xFF26C6DA)); // Teal
                break;
            case ChannelType::Instrument:
                g.fillAll(juce::Colour(0xFF7E57C2)); // Purple
                break;
            case ChannelType::Drums:
                g.fillAll(juce::Colour(0xFFFF8A65)); // Orange
                break;
            case ChannelType::Speech:
                g.fillAll(juce::Colour(0xFFFFEB3B)); // Yellow
                break;
            default:
                g.fillAll(juce::Colours::grey);
                break;
        }
    }
}

juce::Colour ChannelStripComponent::getChannelColourBand() const
{
    // Return the appropriate color based on channel type
    switch (channelType)
    {
        case ChannelType::SingingVocal:
            return juce::Colour::fromString("FF26C6DA"); // Teal
        case ChannelType::Instrument:
            return juce::Colour::fromString("FF7E57C2"); // Purple
        case ChannelType::Drums:
            return juce::Colour::fromString("FFFF8A65"); // Orange
        case ChannelType::Speech:
            return juce::Colour::fromString("FFFFEB3B"); // Yellow
        default:
            return juce::Colours::grey;
    }
}

void ChannelStripComponent::setChannelEnabled(bool enabled)
{
    if (isEnabled != enabled)
    {
        isEnabled = enabled;
        updateComponentEnablement();
        repaint();
    }
}

void ChannelStripComponent::updateComponentEnablement()
{
    // Update all interactive components
    trimDial.setEnabled(isEnabled);
    gateToggle.setEnabled(isEnabled);
    compToggle.setEnabled(isEnabled);
    eqButton.setEnabled(isEnabled);
    fxSendDial.setEnabled(isEnabled);
    tunerToggle.setEnabled(isEnabled);
    muteButton.setEnabled(isEnabled);
    soloButton.setEnabled(isEnabled);
}

void ChannelStripComponent::updateIconColours()
{
    if (channelProcessor != nullptr)
    {
        muteButton.setAlpha(channelProcessor->isMuted() ? 1.0f : 0.4f);
        soloButton.setAlpha(channelProcessor->isSolo() ? 1.0f : 0.4f);
    }
}

void ChannelStripComponent::setInputChoices(const juce::StringArray& names)
{
    inputCombo.clear();
    inputCombo.addItemList(names, 1);
}

void ChannelStripComponent::comboBoxChanged(juce::ComboBox* box)
{
    if (box == &inputCombo)
    {
        int phys = inputCombo.getSelectedId() - 1;   // IDs start at 1
        auralis::RoutingManager::getInstance()
                .assignPhysicalInput(channelIndex, phys);
    }
} 