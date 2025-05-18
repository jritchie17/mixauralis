#pragma once
#include <JuceHeader.h>
#include "Audio/AudioEngine.h"
#include "State/SessionManager.h"
#include "UI/AudioSettingsDialog.h"

class MainWindow;
class SessionRoundTripTest;

class MainApp : public juce::JUCEApplication,
                public juce::MenuBarModel
{
public:
    MainApp() = default;
    ~MainApp() override = default;

    const juce::String getApplicationName() override { return "Auralis"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String& commandLine) override;
    void shutdown() override;
    void systemRequestedQuit() override;
    void anotherInstanceStarted(const juce::String& commandLine) override;
    
    // Provide access to the audio engine
    AudioEngine& getAudioEngine() { return audioEngine; }
    
    // Get the main window
    MainWindow* getMainWindow() { return mainWindow.get(); }

    enum CommandIDs
    {
        newSession = 1,
        openSession,
        saveSession,
        saveSessionAs,
        quit,
        undo,
        redo,
        showSettings,
        runTests,
        audioSettings = 11001
    };

    // MenuBarModel implementation
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

    // ApplicationCommandTarget implementation
    juce::ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; }
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo& info) override;

private:
    std::unique_ptr<MainWindow> mainWindow;
    AudioEngine audioEngine;
    juce::ApplicationCommandManager commandManager;

    void createMenuBarModel();
    void runUnitTests();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainApp)
};
