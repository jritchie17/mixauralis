#include "SubscriptionManager.h"

namespace auralis
{
    SubscriptionManager& SubscriptionManager::getInstance()
    {
        static SubscriptionManager instance;
        return instance;
    }

    bool SubscriptionManager::isAuthenticated() const
    {
        return !token.isEmpty() && 
               juce::Time::getMillisecondCounter() < expiryEpochMs;
    }

    void SubscriptionManager::loginWithToken (const juce::String& jwt)
    {
        if (jwt.isNotEmpty())
        {
            token = jwt;
            currentPlan = Plan::Foundation;
            expiryEpochMs = juce::Time::getMillisecondCounter() + (graceHours * 3600 * 1000);
        }
    }

    void SubscriptionManager::logout()
    {
        token.clear();
        currentPlan = Plan::Foundation;
    }

    Plan SubscriptionManager::getCurrentPlan() const
    {
        return currentPlan;
    }

    void SubscriptionManager::setOfflineGraceHours (int hours)
    {
        graceHours = hours;
    }
} 