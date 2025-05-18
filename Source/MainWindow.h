#pragma once

#include <JuceHeader.h>
#include "UI/MainComponent.h"
#include "MainApp.h"

class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow() : juce::DocumentWindow ("Auralis",
                                        juce::Colours::black,
                                        juce::DocumentWindow::allButtons)
    {
        // Create and set the main content component
        mainComponent = std::make_unique<MainComponent>();
        setContentOwned(mainComponent.get(), true);
        
        // Set the menu bar model
        auto* app = dynamic_cast<MainApp*>(juce::JUCEApplication::getInstance());
        if (app != nullptr)
        {
            setMenuBar(app);
        }
        
        centreWithSize(1024, 768);
        setVisible(true);
        setResizable(true, true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

    /** Provide access to the main component for other modules. */
    MainComponent* getMainComponent() const { return mainComponent.get(); }
    
private:
    std::unique_ptr<MainComponent> mainComponent;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
}; 