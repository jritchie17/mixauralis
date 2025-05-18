#pragma once
#include <JuceHeader.h>

namespace auralis
{
    enum class Plan { Foundation, Flow, Pro };

    class SubscriptionManager
    {
    public:
        static SubscriptionManager& getInstance();

        bool  isAuthenticated() const;
        Plan  getCurrentPlan() const;

        void  loginWithToken (const juce::String& jwt);
        void  logout();
        void  setOfflineGraceHours (int hours);

    private:
        SubscriptionManager() = default;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubscriptionManager)

        juce::String  token;
        juce::int64   expiryEpochMs = 0;
        Plan          currentPlan   = Plan::Foundation;
        int           graceHours    = 168;
    };
} 