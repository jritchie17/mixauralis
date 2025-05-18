#include "SessionManager.h"
#include "../Audio/AudioEngine.h"
#include "../Audio/ChannelProcessor.h"
#include "../Audio/GroupBusProcessor.h"
#include "../Audio/MasterBusProcessor.h"
#include "../FX/EQProcessor.h"
#include "../MainApp.h"
#include "../MainWindow.h"
#include "../UI/MainComponent.h"
#include "../UI/ChannelsComponent.h"

namespace auralis
{
    // Static instance of AudioEngine
    static AudioEngine& getAudioEngine()
    {
        static AudioEngine instance;
        return instance;
    }

    SessionManager& SessionManager::getInstance()
    {
        static SessionManager instance;
        return instance;
    }

    bool SessionManager::saveSession(const juce::File& fileToWrite) const
    {
        auto& engine = getAudioEngine();
        auto root = std::make_unique<juce::DynamicObject>();
        
        // Save channels
        juce::Array<juce::var> channelsArray;
        for (int i = 0; i < 32; ++i) // Use constant from AudioEngine
        {
            if (auto* channel = engine.getChannelProcessor(i))
            {
                auto channelJson = createChannelJson(i);
                juce::Logger::writeToLog("Channel " + juce::String(i) + " JSON: " + juce::JSON::toString(channelJson));
                channelsArray.add(channelJson);
            }
        }
        root->setProperty("channels", channelsArray);
        
        // Save group busses
        juce::Array<juce::var> groupBussesArray;
        for (int i = 0; i < 4; ++i) // Use constant from AudioEngine
        {
            if (auto* bus = engine.getGroupBusProcessor(i))
            {
                groupBussesArray.add(createGroupBusJson(bus->getBusName()));
            }
        }
        root->setProperty("groupBusses", groupBussesArray);
        
        // Save master bus
        if (auto* master = engine.getMasterBusProcessor())
        {
            root->setProperty("masterBus", createMasterBusJson());
        }
        
        // Convert to JSON string and save
        juce::var rootVar(root.release());  // Transfer ownership to var
        juce::String json = juce::JSON::toString(rootVar, true);
        juce::Logger::writeToLog("Full session JSON: " + json);
        return fileToWrite.replaceWithText(json);
    }

    bool SessionManager::loadSession(const juce::File& fileToRead)
    {
        auto json = fileToRead.loadFileAsString();
        juce::Logger::writeToLog("Loading JSON: " + json);
        auto parsed = juce::JSON::parse(json);
        
        if (!parsed.isObject())
        {
            juce::Logger::writeToLog("Failed to parse session file: not a JSON object");
            return false;
        }
            
        auto root = parsed.getDynamicObject();
        if (!root)
        {
            juce::Logger::writeToLog("Failed to parse session file: null root object");
            return false;
        }
            
        // Apply channel settings
        if (auto* channels = root->getProperty("channels").getArray())
        {
            for (int i = 0; i < channels->size(); ++i)
            {
                if (!applyChannelJson(channels->getReference(i), i))
                {
                    juce::Logger::writeToLog("Failed to apply channel " + juce::String(i) + " settings");
                    return false;
                }
            }
        }
        
        // Apply group bus settings
        if (auto* groupBusses = root->getProperty("groupBusses").getArray())
        {
            for (int i = 0; i < groupBusses->size(); ++i)
            {
                if (!applyGroupBusJson(groupBusses->getReference(i)))
                {
                    juce::Logger::writeToLog("Failed to apply group bus settings");
                    return false;
                }
            }
        }
        
        // Apply master bus settings
        if (auto masterBus = root->getProperty("masterBus"))
        {
            if (!applyMasterBusJson(masterBus))
            {
                juce::Logger::writeToLog("Failed to apply master bus settings");
                return false;
            }
        }
        
        // Get the main window to refresh the UI
        if (auto* app = dynamic_cast<MainApp*>(juce::JUCEApplication::getInstance()))
        {
            if (auto* mainWindow = app->getMainWindow())
            {
                if (auto* mainComponent = dynamic_cast<MainComponent*>(mainWindow->getContentComponent()))
                {
                    // Find the channels component and refresh it
                    if (auto* channelsComponent = dynamic_cast<ChannelsComponent*>(mainComponent->findChildWithID("channelsTab")))
                    {
                        channelsComponent->refreshAllChannelStrips();
                    }
                }
            }
        }
        
        juce::Logger::writeToLog("Session restored successfully");
        return true;
    }

    juce::var SessionManager::createChannelJson(int channelIdx) const
    {
        auto& engine = getAudioEngine();
        auto* channel = engine.getChannelProcessor(channelIdx);
        if (!channel)
            return {};
            
        auto channelObj = std::make_unique<juce::DynamicObject>();
        
        // Basic channel properties
        channelObj->setProperty("index", channelIdx);
        channelObj->setProperty("type", static_cast<int>(channel->getChannelType()));
        
        // Trim processor
        float trimGain = channel->getTrimGain();
        juce::Logger::writeToLog("Saving channel " + juce::String(channelIdx) + " trim gain: " + juce::String(trimGain));
        channelObj->setProperty("trimGain", trimGain);
        
        // Gate processor
        channelObj->setProperty("gateThreshold", channel->getGateThreshold());
        channelObj->setProperty("gateEnabled", channel->isGateEnabled());
        
        // EQ processor
        channelObj->setProperty("eqLowGain", channel->getEQBandGain(EQProcessor::Band::LowShelf));
        channelObj->setProperty("eqMidGain", channel->getEQBandGain(EQProcessor::Band::LowMid));
        channelObj->setProperty("eqHighGain", channel->getEQBandGain(EQProcessor::Band::HighShelf));
        channelObj->setProperty("eqEnabled", channel->isEqEnabled());
        
        // Compressor processor
        channelObj->setProperty("compRatio", channel->getCompressorRatio());
        channelObj->setProperty("compThreshold", channel->getCompressorThreshold());
        channelObj->setProperty("compEnabled", channel->isCompressorEnabled());
        
        // FX Send
        channelObj->setProperty("fxSendLevel", channel->getFxSendLevel());
        channelObj->setProperty("mute", channel->isMuted());
        channelObj->setProperty("solo", channel->isSolo());
        
        // Tuner processor
        channelObj->setProperty("tunerEnabled", channel->isTunerEnabled());
        channelObj->setProperty("tunerStrength", channel->getTunerStrength());
        
        return juce::var(channelObj.release());
    }

    juce::var SessionManager::createGroupBusJson(const juce::String& name) const
    {
        auto& engine = getAudioEngine();
        for (int i = 0; i < 4; ++i)
        {
            if (auto* bus = engine.getGroupBusProcessor(i))
            {
                if (bus->getBusName() == name)
                {
                    auto busObj = std::make_unique<juce::DynamicObject>();
                    busObj->setProperty("name", name);
                    
                    // EQ settings
                    busObj->setProperty("eqLowGain", bus->getEQLowGain());
                    busObj->setProperty("eqMidGain", bus->getEQMidGain());
                    busObj->setProperty("eqHighGain", bus->getEQHighGain());
                    busObj->setProperty("eqEnabled", bus->isEQEnabled());
                    
                    // Compressor settings
                    busObj->setProperty("compEnabled", bus->isCompEnabled());
                    
                    // Output gain
                    busObj->setProperty("outputGain", bus->getOutputGain());
                    
                    return juce::var(busObj.release());
                }
            }
        }
        return {};
    }

    juce::var SessionManager::createMasterBusJson() const
    {
        auto& engine = getAudioEngine();
        auto* master = engine.getMasterBusProcessor();
        if (!master)
            return {};
            
        auto masterObj = std::make_unique<juce::DynamicObject>();
        
        // Save target LUFS and platform
        masterObj->setProperty("targetLufs", master->getTargetLufs());
        masterObj->setProperty("platform", static_cast<int>(master->getStreamTarget()));
        masterObj->setProperty("customLufs", master->getTargetLufs()); // Store current LUFS as custom value
        
        // Save processor states
        masterObj->setProperty("compressorEnabled", master->isCompressorEnabled());
        masterObj->setProperty("limiterEnabled", master->isLimiterEnabled());
        
        return juce::var(masterObj.release());
    }

    bool SessionManager::applyChannelJson(const juce::var& v, int channelIdx)
    {
        if (!v.isObject())
        {
            juce::Logger::writeToLog("Channel JSON is not an object");
            return false;
        }
            
        auto& engine = getAudioEngine();
        auto* channel = engine.getChannelProcessor(channelIdx);
        if (!channel)
        {
            juce::Logger::writeToLog("Failed to get channel processor for index " + juce::String(channelIdx));
            return false;
        }
            
        // Apply trim gain
        if (v.hasProperty("trimGain"))
        {
            auto gain = static_cast<float>(v["trimGain"]);
            juce::Logger::writeToLog("Loading channel " + juce::String(channelIdx) + " trim gain: " + juce::String(gain));
            channel->setTrimGain(gain);
            juce::Logger::writeToLog("After setTrimGain, current value is: " + juce::String(channel->getTrimGain()));
        }
            
        // Apply gate settings
        if (v.hasProperty("gateThreshold"))
        {
            auto threshold = static_cast<float>(v["gateThreshold"]);
            juce::Logger::writeToLog("Setting channel " + juce::String(channelIdx) + " gate threshold to: " + juce::String(threshold));
            channel->setGateThreshold(threshold);
        }
        if (v.hasProperty("gateEnabled"))
        {
            auto enabled = static_cast<bool>(v["gateEnabled"]);
            juce::Logger::writeToLog(juce::String("Setting channel ") + juce::String(channelIdx) + " gate enabled to: " + juce::String(enabled ? "true" : "false"));
            channel->setGateEnabled(enabled);
        }
            
        // Apply EQ settings
        if (v.hasProperty("eqLowGain"))
        {
            auto gain = static_cast<float>(v["eqLowGain"]);
            juce::Logger::writeToLog("Setting channel " + juce::String(channelIdx) + " EQ low gain to: " + juce::String(gain));
            channel->setEQBandGain(EQProcessor::Band::LowShelf, gain);
        }
        if (v.hasProperty("eqMidGain"))
        {
            auto gain = static_cast<float>(v["eqMidGain"]);
            juce::Logger::writeToLog("Setting channel " + juce::String(channelIdx) + " EQ mid gain to: " + juce::String(gain));
            channel->setEQBandGain(EQProcessor::Band::LowMid, gain);
        }
        if (v.hasProperty("eqHighGain"))
        {
            auto gain = static_cast<float>(v["eqHighGain"]);
            juce::Logger::writeToLog("Setting channel " + juce::String(channelIdx) + " EQ high gain to: " + juce::String(gain));
            channel->setEQBandGain(EQProcessor::Band::HighShelf, gain);
        }
        if (v.hasProperty("eqEnabled"))
        {
            auto enabled = static_cast<bool>(v["eqEnabled"]);
            juce::Logger::writeToLog(juce::String("Setting channel ") + juce::String(channelIdx) + " EQ enabled to: " + juce::String(enabled ? "true" : "false"));
            channel->setEqEnabled(enabled);
        }
            
        // Apply compressor settings
        if (v.hasProperty("compRatio"))
        {
            auto ratio = static_cast<float>(v["compRatio"]);
            juce::Logger::writeToLog("Setting channel " + juce::String(channelIdx) + " compressor ratio to: " + juce::String(ratio));
            channel->setCompressorRatio(ratio);
        }
        if (v.hasProperty("compThreshold"))
        {
            auto threshold = static_cast<float>(v["compThreshold"]);
            juce::Logger::writeToLog("Setting channel " + juce::String(channelIdx) + " compressor threshold to: " + juce::String(threshold));
            channel->setCompressorThreshold(threshold);
        }
        if (v.hasProperty("compEnabled"))
        {
            auto enabled = static_cast<bool>(v["compEnabled"]);
            juce::Logger::writeToLog(juce::String("Setting channel ") + juce::String(channelIdx) + " compressor enabled to: " + juce::String(enabled ? "true" : "false"));
            channel->setCompressorEnabled(enabled);
        }
            
        // Apply FX send level
        if (v.hasProperty("fxSendLevel"))
        {
            auto level = static_cast<float>(v["fxSendLevel"]);
            juce::Logger::writeToLog("Setting channel " + juce::String(channelIdx) + " FX send level to: " + juce::String(level));
            channel->setFxSendLevel(level);
        }
            
        // Apply mute
        if (v.hasProperty("mute"))
        {
            auto mute = static_cast<bool>(v["mute"]);
            juce::Logger::writeToLog(juce::String("Setting channel ") + juce::String(channelIdx) + " mute to: " + juce::String(mute ? "true" : "false"));
            channel->setMuted(mute);
        }
            
        // Apply solo
        if (v.hasProperty("solo"))
        {
            auto solo = static_cast<bool>(v["solo"]);
            juce::Logger::writeToLog(juce::String("Setting channel ") + juce::String(channelIdx) + " solo to: " + juce::String(solo ? "true" : "false"));
            channel->setSolo(solo);
        }
            
        // Apply tuner settings
        if (v.hasProperty("tunerEnabled"))
        {
            auto enabled = static_cast<bool>(v["tunerEnabled"]);
            juce::Logger::writeToLog(juce::String("Setting channel ") + juce::String(channelIdx) + " tuner enabled to: " + juce::String(enabled ? "true" : "false"));
            channel->setTunerEnabled(enabled);
        }
        if (v.hasProperty("tunerStrength"))
        {
            auto strength = static_cast<float>(v["tunerStrength"]);
            juce::Logger::writeToLog("Setting channel " + juce::String(channelIdx) + " tuner strength to: " + juce::String(strength));
            channel->setTunerStrength(strength);
        }
            
        return true;
    }

    bool SessionManager::applyGroupBusJson(const juce::var& v)
    {
        if (!v.isObject())
        {
            juce::Logger::writeToLog("Group bus JSON is not an object");
            return false;
        }
            
        auto& engine = getAudioEngine();
        juce::String name = v["name"].toString();
        
        for (int i = 0; i < 4; ++i)
        {
            if (auto* bus = engine.getGroupBusProcessor(i))
            {
                if (bus->getBusName() == name)
                {
                    // Apply EQ settings
                    if (v.hasProperty("eqLowGain"))
                    {
                        auto gain = static_cast<float>(v["eqLowGain"]);
                        juce::Logger::writeToLog("Setting group bus " + name + " EQ low gain to: " + juce::String(gain));
                        bus->setEQLowGain(gain);
                    }
                    if (v.hasProperty("eqMidGain"))
                    {
                        auto gain = static_cast<float>(v["eqMidGain"]);
                        juce::Logger::writeToLog("Setting group bus " + name + " EQ mid gain to: " + juce::String(gain));
                        bus->setEQMidGain(gain);
                    }
                    if (v.hasProperty("eqHighGain"))
                    {
                        auto gain = static_cast<float>(v["eqHighGain"]);
                        juce::Logger::writeToLog("Setting group bus " + name + " EQ high gain to: " + juce::String(gain));
                        bus->setEQHighGain(gain);
                    }
                    if (v.hasProperty("eqEnabled"))
                    {
                        auto enabled = static_cast<bool>(v["eqEnabled"]);
                        juce::Logger::writeToLog(juce::String("Setting group bus ") + name + " EQ enabled to: " + juce::String(enabled ? "true" : "false"));
                        bus->setEQEnabled(enabled);
                    }
                    
                    // Apply compressor settings
                    if (v.hasProperty("compEnabled"))
                    {
                        auto enabled = static_cast<bool>(v["compEnabled"]);
                        juce::Logger::writeToLog(juce::String("Setting group bus ") + name + " compressor enabled to: " + juce::String(enabled ? "true" : "false"));
                        bus->setCompEnabled(enabled);
                    }
                    
                    // Apply output gain
                    if (v.hasProperty("outputGain"))
                    {
                        auto gain = static_cast<float>(v["outputGain"]);
                        juce::Logger::writeToLog("Setting group bus " + name + " output gain to: " + juce::String(gain));
                        bus->setOutputGain(gain);
                    }
                    
                    return true;
                }
            }
        }
        
        juce::Logger::writeToLog("Failed to find group bus with name: " + name);
        return false;
    }

    bool SessionManager::applyMasterBusJson(const juce::var& v)
    {
        auto& engine = getAudioEngine();
        auto* master = engine.getMasterBusProcessor();
        if (!master)
            return false;
            
        // Restore target LUFS and platform
        if (v.hasProperty("targetLufs"))
            master->setTargetLufs(static_cast<float>(v["targetLufs"]));
        
        if (v.hasProperty("platform"))
        {
            auto platform = static_cast<StreamTarget>(static_cast<int>(v["platform"]));
            master->setStreamTarget(platform);
            
            // If platform is Custom and we have a custom LUFS value, restore it
            if (platform == StreamTarget::Custom && v.hasProperty("customLufs"))
            {
                const float custom = static_cast<float>(v["customLufs"]);
                master->setTargetLufs(custom);

                // Propagate the value to the SettingsComponent slider if available
                if (auto* app = dynamic_cast<MainApp*>(juce::JUCEApplication::getInstance()))
                {
                    if (auto* mainWin = app->getMainWindow())
                    {
                        if (auto* mainComp = mainWin->getMainComponent())
                        {
                            if (auto* settings = mainComp->getSettingsComponent())
                                settings->setCustomLufs(custom);
                        }
                    }
                }
            }
        }
        
        // Restore processor states
        if (v.hasProperty("compressorEnabled"))
            master->setCompressorEnabled(static_cast<bool>(v["compressorEnabled"]));
        
        if (v.hasProperty("limiterEnabled"))
            master->setLimiterEnabled(static_cast<bool>(v["limiterEnabled"]));
        
        return true;
    }
} 