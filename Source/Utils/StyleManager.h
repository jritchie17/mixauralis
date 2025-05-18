#pragma once

#include <JuceHeader.h>
#include "BlackwayLookAndFeel.h"

class StyleManager
{
public:
    enum class Theme { Dark, Light };

    static StyleManager& getInstance();

    BlackwayLookAndFeel& getLookAndFeel();

    void applyGlobalLookAndFeel();

    void setTheme(Theme newTheme);
    Theme getTheme() const;

private:
    StyleManager();
    void configureColours();

    Theme currentTheme { Theme::Dark };
    BlackwayLookAndFeel lookAndFeel;
};
