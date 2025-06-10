#include "../MainApp.h"
#include "MainComponent.h"
#include "ChannelsComponent.h"
#include "FXBusesComponent.h"
#include "MasterBusComponent.h"
#include "RoutingComponent.h"
#include "../Utils/StyleManager.h"

MainComponent::MainComponent()
{
    juce::Logger::writeToLog("MainComponent constructor start");
    
    try {
        // Use the global look and feel managed by StyleManager
        auto& lf = StyleManager::getInstance().getLookAndFeel();
        setLookAndFeel(&lf);
        tabbedComponent.setLookAndFeel(&lf);
        
        // Load background image
        auto assetsDir = juce::File::getCurrentWorkingDirectory().getChildFile("Assets");
        juce::Logger::writeToLog("Looking for assets in: " + assetsDir.getFullPathName());
        if (!assetsDir.exists())
        {
            juce::Logger::writeToLog("Assets directory not found!");
        }
        else
        {
            auto bgFile = assetsDir.getChildFile("background_01.png");
            if (!bgFile.exists())
            {
                juce::Logger::writeToLog("Background image not found!");
            }
            else
            {
                backgroundImage = juce::ImageCache::getFromFile(bgFile);
                if (!backgroundImage.isValid())
                {
                    juce::Logger::writeToLog("Failed to load background image!");
                }
            }
        }
        
        // Get the global audio engine instance
        juce::Logger::writeToLog("Getting global audio engine...");
        audioEngine = &static_cast<MainApp&>(*juce::JUCEApplication::getInstance()).getAudioEngine();
        if (!audioEngine)
        {
            juce::Logger::writeToLog("Failed to get audio engine!");
            return;
        }
        
        // Get the global master bus processor
        juce::Logger::writeToLog("Getting global master bus processor...");
        masterBusProcessor = audioEngine->getMasterBusProcessor();
        if (!masterBusProcessor)
        {
            juce::Logger::writeToLog("Failed to get master bus processor!");
            return;
        }
        
        // Create components for each tab
        juce::Logger::writeToLog("Creating tab components...");
        channelsTab = std::make_unique<ChannelsComponent>();
        routingTab = std::make_unique<RoutingComponent>();
        fxBusesTab = std::make_unique<FXBusesComponent>();
        masterTab = std::make_unique<MasterBusComponent>(masterBusProcessor);
        settingsTab = std::make_unique<auralis::SettingsComponent>();
        
        if (!channelsTab || !routingTab || !fxBusesTab || !masterTab || !settingsTab)
        {
            juce::Logger::writeToLog("Failed to create one or more tab components!");
            return;
        }
        
        // Connect components to the audio engine
        juce::Logger::writeToLog("Connecting components to audio engine...");
        static_cast<ChannelsComponent*>(channelsTab.get())->connectToAudioEngine(audioEngine);
        static_cast<RoutingComponent*>(routingTab.get())->connectToAudioEngine(audioEngine);
        
        // Connect FX buses and Group buses to their processors
        auto fxBusesComponent = static_cast<FXBusesComponent*>(fxBusesTab.get());
        
        // Get FX processors
        std::vector<FXBusProcessor*> fxProcessors;
        for (int i = 0; i < 3; ++i) // 3 FX buses: Vocal, Instrument, Drum
        {
            auto* processor = audioEngine->getFXBusProcessor(i);
            if (processor)
            {
                fxProcessors.push_back(processor);
            }
            else
            {
                juce::Logger::writeToLog("Failed to get FX processor " + juce::String(i));
            }
        }
        fxBusesComponent->connectToProcessors(fxProcessors);
        
        // Get Group bus processors and connect
        std::vector<GroupBusProcessor*> groupProcessors = audioEngine->getAllGroupBusProcessors();
        fxBusesComponent->connectToGroupBusProcessors(groupProcessors);
        
        // Add tabs to the tabbed component
        juce::Logger::writeToLog("Adding tabs...");
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
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog("Exception in MainComponent constructor: " + juce::String(e.what()));
    }
    catch (...)
    {
        juce::Logger::writeToLog("Unknown exception in MainComponent constructor");
    }
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