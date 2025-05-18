#include "RoutingComponent.h"

// Custom ToolbarItemFactory for creating our toolbar items
class RoutingToolbarItemFactory : public juce::ToolbarItemFactory
{
public:
    RoutingToolbarItemFactory(RoutingComponent& routingComponent)
        : owner(routingComponent)
    {
    }
    
    // Item IDs
    enum ItemIDs
    {
        SoundcheckButton = 1
    };
    
    // Return the number of item types
    int getNumItems()
    {
        return 1;
    }
    
    // Return the ID for each item type
    int getItemID(int index)
    {
        return SoundcheckButton;
    }
    
    // Create a new toolbar item for the given ID
    void getAllToolbarItemIds(juce::Array<int>& ids)
    {
        ids.add(SoundcheckButton);
    }
    
    // Create the item from the ID
    void getDefaultItemSet(juce::Array<int>& ids)
    {
        ids.add(SoundcheckButton);
    }
    
    // Create the actual toolbar button
    juce::ToolbarItemComponent* createItem(int itemId)
    {
        if (itemId == SoundcheckButton)
        {
            // Load SVG from Assets directory
            auto assetsDir = juce::File::getCurrentWorkingDirectory().getChildFile("Assets");
            auto svgFile = assetsDir.getChildFile("soundcheck.svg");
            
            std::unique_ptr<juce::Drawable> icon;
            if (svgFile.existsAsFile())
            {
                icon = juce::Drawable::createFromSVGFile(svgFile);
            }
            else
            {
                // Fallback if SVG file not found
                icon = std::make_unique<juce::DrawableRectangle>();
                static_cast<juce::DrawableRectangle*>(icon.get())->setFill(juce::Colours::white);
                static_cast<juce::DrawableRectangle*>(icon.get())->setRectangle(juce::Rectangle<float>(0, 0, 24, 24));
            }
            
            auto button = new juce::ToolbarButton(itemId, "Soundcheck", std::move(icon), nullptr);
            button->setTooltip("Run automatic soundcheck analysis");
            
            // Set up button callback to the owner component
            button->onClick = [this, itemId]() { owner.handleToolbarButtonClicked(itemId); };
            
            return button;
        }
        
        return nullptr;
    }
    
private:
    RoutingComponent& owner;
};

//==============================================================================
RoutingComponent::RoutingComponent()
{
    // Set up basic component properties
    setLookAndFeel(&blackwayLookAndFeel);
    
    // Load background image
    auto assetsDir = juce::File::getCurrentWorkingDirectory().getChildFile("Assets");
    backgroundImage = juce::ImageCache::getFromFile(assetsDir.getChildFile("background_02.png"));
    
    // Create toolbar
    addAndMakeVisible(toolbar);
    toolbarFactory = std::make_unique<RoutingToolbarItemFactory>(*this);
    toolbar.addDefaultItems(*toolbarFactory);
    
    // Set toolbar style
    toolbar.setVertical(false);
    toolbar.setColour(juce::Toolbar::backgroundColourId, juce::Colours::transparentBlack);
    toolbar.setColour(juce::Toolbar::buttonMouseOverBackgroundColourId, juce::Colours::white.withAlpha(0.1f));
    toolbar.setColour(juce::Toolbar::buttonMouseDownBackgroundColourId, juce::Colours::white.withAlpha(0.2f));
    
    // Create soundcheck panel (but don't make visible yet)
    soundcheckPanel = std::make_unique<SoundcheckPanel>();
    addChildComponent(soundcheckPanel.get());

    // Routing matrix
    routingMatrix = std::make_unique<RoutingMatrixComponent>();
    addAndMakeVisible(routingMatrix.get());
    
    addAndMakeVisible(soundcheckButton);
    addAndMakeVisible(autoMapButton);
    
    soundcheckButton.addListener(this);
    autoMapButton.addListener(this);
    
    // Create channel strips
    for (int i = 0; i < numChannels; ++i)
    {
        auto* strip = new ChannelStripComponent(i);
        strip->setChannelName("Channel " + juce::String(i + 1));
        channelStrips.add(strip);
        addAndMakeVisible(strip);
    }
    
    // Set up input choices for each channel strip
    juce::StringArray inputs;
    const int nInputs = auralis::RoutingManager::getInstance().getNumPhysicalInputs();
    for (int i = 0; i < nInputs; ++i)
        inputs.add("Input " + juce::String(i + 1));
    for (auto* strip : channelStrips)
    {
        strip->setInputChoices(inputs);
        // Update combo selection based on routing manager
        auto& rm = auralis::RoutingManager::getInstance();
        int phys = rm.getPhysicalInput(strip->getChannelIndex());
        strip->setSelectedInput(phys, juce::dontSendNotification);
    }
}

RoutingComponent::~RoutingComponent()
{
    // Clean up
    setLookAndFeel(nullptr);
}

void RoutingComponent::paint(juce::Graphics& g)
{
    // Paint background
    if (backgroundImage.isValid())
    {
        g.drawImage(backgroundImage, getLocalBounds().toFloat(), 
                    juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        g.fillAll(juce::Colours::black);
    }
    
}

void RoutingComponent::resized()
{
    auto bounds = getLocalBounds();
    auto buttonHeight = 30;
    auto buttonWidth = 120;
    auto padding = 10;
    
    // Place buttons in top-right corner
    soundcheckButton.setBounds(bounds.getRight() - 2 * (buttonWidth + padding), 
                              padding, 
                              buttonWidth, 
                              buttonHeight);
                              
    autoMapButton.setBounds(bounds.getRight() - (buttonWidth + padding), 
                           padding, 
                           buttonWidth, 
                           buttonHeight);
    
    // Position toolbar at the top-right
    toolbar.setBounds(getWidth() - 200, 0, 200, 40);

    // Position routing matrix below toolbar
    auto matrixBounds = bounds.reduced(20);
    matrixBounds.removeFromTop(50); // space for buttons
    routingMatrix->setBounds(matrixBounds);
    
    // Center the soundcheck panel
    auto centerX = (getWidth() - soundcheckPanel->getWidth()) / 2;
    auto centerY = (getHeight() - soundcheckPanel->getHeight()) / 2;
    soundcheckPanel->setBounds(centerX, centerY, 
                              soundcheckPanel->getWidth(), soundcheckPanel->getHeight());
}

void RoutingComponent::connectToAudioEngine(AudioEngine* engine)
{
    audioEngine = engine;
    
    // If we have an engine and it has channel processors, pass them to the soundcheck panel
    if (audioEngine != nullptr)
    {
        // Convert channel processors to raw pointers
        std::vector<ChannelProcessor*> processors;
        for (int i = 0; i < 32; ++i)
        {
            processors.push_back(audioEngine->getChannelProcessor(i));
        }

        soundcheckPanel->setChannelProcessors(processors);

        routingMatrix->setRoutingManager(&audioEngine->getRoutingManager());
        routingMatrix->refreshMatrix();
    }
}

void RoutingComponent::handleToolbarButtonClicked(int toolbarItemId)
{
    // Check which button was clicked
    if (toolbarItemId == RoutingToolbarItemFactory::SoundcheckButton)
    {
        soundcheckButtonClicked();
    }
}

void RoutingComponent::soundcheckButtonClicked()
{
    // Show the soundcheck panel
    soundcheckPanel->showPanel(true);
}

void RoutingComponent::buttonClicked(juce::Button* button)
{
    if (button == &soundcheckButton)
    {
        // ... existing soundcheck code ...
    }
    else if (button == &autoMapButton)
    {
        auto& rm = auralis::RoutingManager::getInstance();
        const int n = rm.getNumChannels();
        for (int i = 0; i < n; ++i)
            rm.assignPhysicalInput(i, i);   // 1-to-1 mapping
            
        // Update combo selections
        for (auto* strip : channelStrips)
        {
            int phys = rm.getPhysicalInput(strip->getChannelIndex());
            strip->setSelectedInput(phys, juce::dontSendNotification);
        }

        routingMatrix->refreshMatrix();

        refreshChannelLabels();   // existing method to update UI
        DBG("Auto-mapped " << n << " inputs");
    }
}

void RoutingComponent::refreshChannelLabels()
{
    // For now, just trigger a repaint
    repaint();
} 