#include "StyleManager.h"

StyleManager& StyleManager::getInstance()
{
    static StyleManager instance;
    return instance;
}

StyleManager::StyleManager()
{
    configureColours();
}

BlackwayLookAndFeel& StyleManager::getLookAndFeel()
{
    return lookAndFeel;
}

void StyleManager::applyGlobalLookAndFeel()
{
    juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
}

void StyleManager::setTheme(Theme newTheme)
{
    if (currentTheme != newTheme)
    {
        currentTheme = newTheme;
        configureColours();
    }
}

StyleManager::Theme StyleManager::getTheme() const
{
    return currentTheme;
}

void StyleManager::configureColours()
{
    if (currentTheme == Theme::Dark)
    {
        lookAndFeel.setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::black);
        lookAndFeel.setColour(juce::Label::textColourId, juce::Colours::white);
    }
    else
    {
        lookAndFeel.setColour(juce::ResizableWindow::backgroundColourId, juce::Colours::white);
        lookAndFeel.setColour(juce::Label::textColourId, juce::Colours::black);
    }
}
