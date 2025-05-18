#include "MainComponent.h"
#include "ChannelsComponent.h"
#include "FXBusesComponent.h"
#include "MasterBusComponent.h"
#include "RoutingComponent.h"
#include "../Utils/StyleManager.h"

MainComponent::MainComponent()
{
    juce::Logger::writeToLog("MainComponent constructor start");
    
    // Use the global look and feel managed by StyleManager
    auto& lf = StyleManager::getInstance().getLookAndFeel();
    setLookAndFeel(&lf);
    tabbedComponent.setLookAndFeel(&lf);
    
    // Load background image
    auto assetsDir = juce::File::getCurrentWorkingDirectory().getChildFile("Assets");
    backgroundImage = juce::ImageCache::getFromFile(assetsDir.getChildFile("background_01.png"));
    
    // Create audio engine for processors
    audioEngine = std::make_unique<AudioEngine>();
    
    // Create master bus processor
    masterBusProcessor = std::make_unique<MasterBusProcessor>();
    
    // Create components for each tab
    channelsTab = std::make_unique<ChannelsComponent>();
    routingTab = std::make_unique<RoutingComponent>();
    fxBusesTab = std::make_unique<FXBusesComponent>();
    masterTab = std::make_unique<MasterBusComponent>(masterBusProcessor.get());
    settingsTab = std::make_unique<auralis::SettingsComponent>();
    
    // Connect components to the audio engine
    static_cast<ChannelsComponent*>(channelsTab.get())->connectToAudioEngine(audioEngine.get());
    static_cast<RoutingComponent*>(routingTab.get())->connectToAudioEngine(audioEngine.get());
    
    // Connect FX buses and Group buses to their processors
    auto fxBusesComponent = static_cast<FXBusesComponent*>(fxBusesTab.get());
    
    // Get FX processors
    std::vector<FXBusProcessor*> fxProcessors;
    for (int i = 0; i < 3; ++i) // 3 FX buses: Vocal, Instrument, Drum
    {
        fxProcessors.push_back(audioEngine->getFXBusProcessor(i));
    }
    fxBusesComponent->connectToProcessors(fxProcessors);
    
    // Get Group bus processors and connect
    std::vector<GroupBusProcessor*> groupProcessors = audioEngine->getAllGroupBusProcessors();
    fxBusesComponent->connectToGroupBusProcessors(groupProcessors);
    
    // Add tabs to the tabbed component
    tabbedComponent.addTab("Routing", juce::Colours::black, routingTab.get(), false);
    channelsTab->setComponentID("channelsTab");
    tabbedComponent.addTab("Channels", juce::Colours::black, channelsTab.get(), false);
    tabbedComponent.addTab("FX Buses", juce::Colours::black, fxBusesTab.get(), false);
    tabbedComponent.addTab("Master", juce::Colours::black, masterTab.get(), false);
    tabbedComponent.addTab("Settings", juce::Colours::black, settingsTab.get(), false);
    
    // Select Channels tab by default
    tabbedComponent.setCurrentTabIndex(1);
    
    // Style the tab bar
    tabbedComponent.setTabBarDepth(30);
    tabbedComponent.setOutline(0);
    tabbedComponent.setIndent(0);
    
    addAndMakeVisible(tabbedComponent);
    
    setSize(1280, 720); // Set to minimum size as per requirements
    
    juce::Logger::writeToLog("MainComponent constructor done");
}

MainComponent::~MainComponent()
{
    juce::Logger::writeToLog("MainComponent destructor");
    tabbedComponent.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void MainComponent::paint(juce::Graphics& g)
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

void MainComponent::resized()
{
    tabbedComponent.setBounds(getLocalBounds());
} 