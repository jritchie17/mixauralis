#pragma once
#include <JuceHeader.h>
#include "../Routing/RoutingManager.h"
#include "../Subscription/SubscriptionManager.h"

class RoutingMatrixComponent : public juce::Component
{
public:
    RoutingMatrixComponent();

    void setRoutingManager(auralis::RoutingManager* manager);
    void refreshMatrix();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    auralis::RoutingManager* routingManager = nullptr;
    int numInputs = 0;
    int numChannels = 0;

    juce::OwnedArray<juce::Label> inputLabels;
    juce::OwnedArray<juce::Label> channelLabels;
    juce::OwnedArray<juce::ToggleButton> toggles;

    void toggleButtonClicked(int row, int col);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoutingMatrixComponent)
};
