#pragma once

#include <JuceHeader.h>
#include "../Source/Audio/AudioEngine.h"
#include "../Source/State/SessionManager.h"
#include "../Source/MainApp.h"

class SessionRoundTripTest : public juce::UnitTest
{
public:
    SessionRoundTripTest() : juce::UnitTest("Session RT", "Auralis") {}

    void runTest() override
    {
        beginTest("Save / Load round trip");

        // 1. Set some channel params
        auto* app = dynamic_cast<MainApp*>(juce::JUCEApplication::getInstance());
        auto& engine = app->getAudioEngine();
        auto* ch0 = engine.getChannelProcessor(0);
        ch0->setTrimGain(0.7f);
        ch0->setFxSendLevel(0.5f);
        ch0->setMuted(true);

        // 2. Save to a temp file
        auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getChildFile("roundtrip.aur");
        expect(auralis::SessionManager::getInstance().saveSession(tmp));

        // 3. Change params
        ch0->setTrimGain(1.0f);
        ch0->setFxSendLevel(0.0f);
        ch0->setMuted(false);

        // 4. Load the file
        expect(auralis::SessionManager::getInstance().loadSession(tmp));

        // 5. Verify restored
        expectWithinAbsoluteError(ch0->getTrimGain(), 0.7f, 0.001f);
        expectWithinAbsoluteError(ch0->getFxSendLevel(), 0.5f, 0.001f);
        expect(ch0->isMuted() == true);
    }

    static SessionRoundTripTest& getInstance()
    {
        static SessionRoundTripTest instance;
        return instance;
    }
}; 