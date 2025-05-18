#include "MainApp.h"
#include "UI/MainComponent.h"
#include "MainWindow.h"
#include "Tests/SessionRoundTripTest.h"
#include "Utils/StyleManager.h"

namespace {
    void logToStderr(const juce::String& message)
    {
        std::cerr << message << std::endl;
        std::cerr.flush();
    }

    class TestRunnerWindow : public juce::DocumentWindow
    {
    public:
        TestRunnerWindow() : DocumentWindow("Unit Tests", juce::Colours::lightgrey, DocumentWindow::closeButton)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new juce::TextEditor(), true);
            
            auto* editor = dynamic_cast<juce::TextEditor*>(getContentComponent());
            editor->setMultiLine(true);
            editor->setReadOnly(true);
            editor->setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
            
            centreWithSize(600, 400);
            setVisible(true);
            toFront(true);
        }
        
        void closeButtonPressed() override
        {
            delete this;
        }
        
        void addText(const juce::String& text)
        {
            if (auto* editor = dynamic_cast<juce::TextEditor*>(getContentComponent()))
            {
                editor->moveCaretToEnd();
                editor->insertTextAtCaret(text + "\n");
            }
        }
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestRunnerWindow)
    };

    class TestLogger : public juce::Logger
    {
    public:
        TestLogger(TestRunnerWindow& w) : window(w) {}
        
        void logMessage(const juce::String& message) override
        {
            window.addText(message);
        }
        
    private:
        TestRunnerWindow& window;
    };
}

void MainApp::initialise(const juce::String& commandLine)
{
    logToStderr("Creating main window");

    // Apply global look and feel so all components share the same style
    StyleManager::getInstance().applyGlobalLookAndFeel();

    // Register commands
    commandManager.registerAllCommandsForTarget(this);
    
    mainWindow = std::make_unique<MainWindow>();
    createMenuBarModel();
    logToStderr("Main window created");
}

void MainApp::shutdown()
{
    logToStderr("Shutting down main window");
    juce::MenuBarModel::setMacMainMenu(nullptr);
    mainWindow = nullptr;
    logToStderr("Main window reset");
}

void MainApp::systemRequestedQuit()
{
    juce::JUCEApplication::quit();
}

void MainApp::anotherInstanceStarted(const juce::String& commandLine)
{
}

juce::StringArray MainApp::getMenuBarNames()
{
    return { "File", "Edit", "View", "Debug" };
}

juce::PopupMenu MainApp::getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName)
{
    juce::PopupMenu menu;

    if (menuName == "File")
    {
        menu.addCommandItem(&commandManager, CommandIDs::newSession);
        menu.addCommandItem(&commandManager, CommandIDs::openSession);
        menu.addCommandItem(&commandManager, CommandIDs::saveSession);
        menu.addCommandItem(&commandManager, CommandIDs::saveSessionAs);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::audioSettings);
        menu.addSeparator();
        menu.addCommandItem(&commandManager, CommandIDs::quit);
    }
    else if (menuName == "Edit")
    {
        menu.addCommandItem(&commandManager, CommandIDs::undo);
        menu.addCommandItem(&commandManager, CommandIDs::redo);
    }
    else if (menuName == "View")
    {
        menu.addCommandItem(&commandManager, CommandIDs::showSettings);
    }
    else if (menuName == "Debug")
    {
        #if JUCE_ENABLE_UNIT_TESTS
        menu.addCommandItem(&commandManager, CommandIDs::runTests);
        #endif
    }

    return menu;
}

void MainApp::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    logToStderr("Menu item selected: " + juce::String(menuItemID));
}

void MainApp::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    commands.addArray({
        CommandIDs::newSession,
        CommandIDs::openSession,
        CommandIDs::saveSession,
        CommandIDs::saveSessionAs,
        CommandIDs::quit,
        CommandIDs::undo,
        CommandIDs::redo,
        CommandIDs::showSettings,
        CommandIDs::audioSettings,
        #if JUCE_ENABLE_UNIT_TESTS
        CommandIDs::runTests
        #endif
    });
}

void MainApp::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    switch (commandID)
    {
        case CommandIDs::newSession:
            result.setInfo("New Session", "Create a new session", "File", 0);
            break;
        case CommandIDs::openSession:
            result.setInfo("Open Session", "Open an existing session", "File", 0);
            break;
        case CommandIDs::saveSession:
            result.setInfo("Save Session", "Save the current session", "File", 0);
            break;
        case CommandIDs::saveSessionAs:
            result.setInfo("Save Session As", "Save the current session with a new name", "File", 0);
            break;
        case CommandIDs::quit:
            result.setInfo("Quit", "Quit the application", "File", 0);
            break;
        case CommandIDs::undo:
            result.setInfo("Undo", "Undo the last action", "Edit", 0);
            break;
        case CommandIDs::redo:
            result.setInfo("Redo", "Redo the last undone action", "Edit", 0);
            break;
        case CommandIDs::showSettings:
            result.setInfo("Show Settings", "Show the settings panel", "View", 0);
            break;
        case CommandIDs::audioSettings:
            result.setInfo("Audio Settings...", "Configure audio device settings", "File", 0);
            break;
        #if JUCE_ENABLE_UNIT_TESTS
        case CommandIDs::runTests:
            result.setInfo("Run Tests", "Run all unit tests", "Debug", 0);
            break;
        #endif
    }
}

bool MainApp::perform(const juce::ApplicationCommandTarget::InvocationInfo& info)
{
    switch (info.commandID)
    {
        case CommandIDs::saveSession:
        {
            logToStderr("Creating save file chooser");
            
            auto chooser = std::make_shared<juce::FileChooser>("Save Session", juce::File{}, "*.aur");
            auto flags = juce::FileBrowserComponent::saveMode | 
                        juce::FileBrowserComponent::canSelectFiles;
                        
            logToStderr("Launching save file chooser");
            
            chooser->launchAsync(flags, [chooser](const juce::FileChooser& fc)
            {
                logToStderr("Save file chooser completed");
                auto file = fc.getResult();
                if (file != juce::File{})
                {
                    logToStderr("Saving session to: " + file.getFullPathName());
                    try
                    {
                        auralis::SessionManager::getInstance().saveSession(file);
                        logToStderr("Session saved successfully");
                    }
                    catch (const std::exception& e)
                    {
                        logToStderr("Error saving session: " + juce::String(e.what()));
                    }
                }
                else
                {
                    logToStderr("No file selected for save");
                }
            });
            return true;
        }
        case CommandIDs::openSession:
        {
            logToStderr("Creating load file chooser");
            
            auto chooser = std::make_shared<juce::FileChooser>("Load Session", juce::File{}, "*.aur");
            auto flags = juce::FileBrowserComponent::openMode | 
                        juce::FileBrowserComponent::canSelectFiles;
                        
            logToStderr("Launching load file chooser");
            
            chooser->launchAsync(flags, [chooser](const juce::FileChooser& fc)
            {
                logToStderr("Load file chooser completed");
                auto file = fc.getResult();
                if (file != juce::File{})
                {
                    logToStderr("Loading session from: " + file.getFullPathName());
                    try
                    {
                        auralis::SessionManager::getInstance().loadSession(file);
                        logToStderr("Session loaded successfully");
                    }
                    catch (const std::exception& e)
                    {
                        logToStderr("Error loading session: " + juce::String(e.what()));
                    }
                }
                else
                {
                    logToStderr("No file selected for load");
                }
            });
            return true;
        }
        case CommandIDs::quit:
            logToStderr("Quit requested");
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
            return true;
        case CommandIDs::audioSettings:
        {
            static std::unique_ptr<auralis::AudioSettingsDialog> dlg;
            if (!dlg)
                dlg = std::make_unique<auralis::AudioSettingsDialog>(audioEngine.getAudioDeviceManager());

            dlg->setVisible(true);
            dlg->toFront(true);
            return true;
        }
        #if JUCE_ENABLE_UNIT_TESTS
        case CommandIDs::runTests:
            logToStderr("Running unit tests");
            {
                auto* window = new TestRunnerWindow();
                auto logger = std::make_unique<TestLogger>(*window);
                auto* oldLogger = juce::Logger::getCurrentLogger();
                juce::Logger::setCurrentLogger(logger.get());
                
                juce::UnitTestRunner runner;
                runner.setAssertOnFailure(false);
                runner.runAllTests();
                
                juce::Logger::setCurrentLogger(oldLogger);
            }
            return true;
        #endif
        default:
            return false;
    }
}

void MainApp::createMenuBarModel()
{
    setMacMainMenu(this);
}

// This creates the application instance
START_JUCE_APPLICATION(MainApp)
