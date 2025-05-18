#include "RoutingMatrixComponent.h"

RoutingMatrixComponent::RoutingMatrixComponent() {}

void RoutingMatrixComponent::setRoutingManager(auralis::RoutingManager* manager)
{
    routingManager = manager;
    refreshMatrix();
}

void RoutingMatrixComponent::refreshMatrix()
{
    inputLabels.clear();
    channelLabels.clear();
    toggles.clear();

    if (routingManager == nullptr)
    {
        repaint();
        return;
    }

    numInputs = routingManager->getNumPhysicalInputs();
    auto plan = auralis::SubscriptionManager::getInstance().getCurrentPlan();
    numChannels = juce::jmin(routingManager->getNumChannels(),
                             auralis::RoutingManager::getMaxChannelsForPlan(plan));

    for (int j = 0; j < numInputs; ++j)
    {
        auto* lbl = new juce::Label();
        lbl->setText("In " + juce::String(j + 1), juce::dontSendNotification);
        lbl->setJustificationType(juce::Justification::centred);
        inputLabels.add(lbl);
        addAndMakeVisible(lbl);
    }

    for (int i = 0; i < numChannels; ++i)
    {
        auto* rowLabel = new juce::Label();
        rowLabel->setText("Ch " + juce::String(i + 1), juce::dontSendNotification);
        rowLabel->setJustificationType(juce::Justification::centredRight);
        channelLabels.add(rowLabel);
        addAndMakeVisible(rowLabel);

        for (int j = 0; j < numInputs; ++j)
        {
            auto* toggle = new juce::ToggleButton();
            toggle->setClickingTogglesState(true);
            int row = i, col = j;
            toggle->onClick = [this, row, col]() { toggleButtonClicked(row, col); };
            toggles.add(toggle);
            addAndMakeVisible(toggle);
        }
    }

    // Restore states from manager
    for (int i = 0; i < numChannels; ++i)
    {
        int phys = routingManager->getPhysicalInput(i);
        if (phys >= 0 && phys < numInputs)
            toggles[i * numInputs + phys]->setToggleState(true, juce::dontSendNotification);
    }

    resized();
    repaint();
}

void RoutingMatrixComponent::toggleButtonClicked(int row, int col)
{
    if (routingManager == nullptr)
        return;

    routingManager->assignPhysicalInput(row, col);

    for (int j = 0; j < numInputs; ++j)
        toggles[row * numInputs + j]->setToggleState(j == col, juce::dontSendNotification);
}

void RoutingMatrixComponent::resized()
{
    const int labelHeight = 20;
    const int rowLabelWidth = 60;
    const int cellWidth = 40;
    const int cellHeight = 24;

    int startX = rowLabelWidth;
    int startY = labelHeight;

    for (int j = 0; j < numInputs; ++j)
        if (auto* lbl = inputLabels[j])
            lbl->setBounds(startX + j * cellWidth, 0, cellWidth, labelHeight);

    for (int i = 0; i < numChannels; ++i)
    {
        if (auto* lbl = channelLabels[i])
            lbl->setBounds(0, startY + i * cellHeight, rowLabelWidth, cellHeight);

        for (int j = 0; j < numInputs; ++j)
        {
            if (auto* t = toggles[i * numInputs + j])
                t->setBounds(startX + j * cellWidth, startY + i * cellHeight, cellWidth, cellHeight);
        }
    }

    setSize(startX + numInputs * cellWidth, startY + numChannels * cellHeight);
}

void RoutingMatrixComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::transparentBlack);
}

