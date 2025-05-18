#pragma once
#include <JuceHeader.h>

namespace auralis
{
    /**  Serialises the entire mixer state (routing, channel params,
         bus params, master settings) to a single JSON file and
         restores it on load.  Only declaration in this step. */
    class SessionManager
    {
    public:
        static SessionManager& getInstance();

        /** Save the current session to disk.  Returns false on I/O failure. */
        bool saveSession (const juce::File& fileToWrite) const;

        /** Load a session from disk.  Returns false if file corrupt. */
        bool loadSession (const juce::File& fileToRead);

    private:
        SessionManager() = default;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SessionManager)

        // Helpers (to be implemented in .cpp)
        juce::var  createChannelJson      (int channelIdx) const;
        juce::var  createGroupBusJson     (const juce::String& name) const;
        juce::var  createMasterBusJson()  const;
        bool       applyChannelJson       (const juce::var& v, int channelIdx);
        bool       applyGroupBusJson      (const juce::var& v);
        bool       applyMasterBusJson     (const juce::var& v);
    };
} 