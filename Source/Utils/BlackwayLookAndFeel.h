#pragma once

#include <JuceHeader.h>

class BlackwayLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BlackwayLookAndFeel()
    {
        // Load toggle button images (for Gate, Comp, Tuner toggles)
        auto assetsDir = getAssetsDirectory();
        
        switchOnImage = juce::ImageCache::getFromFile(assetsDir.getChildFile("BlackwayFX/toggles/switch_horizontal_on.png"));
        switchOffImage = juce::ImageCache::getFromFile(assetsDir.getChildFile("BlackwayFX/toggles/switch_horizontal_off.png"));
        
        // Load button images (for EQ, Mute, Solo buttons)
        squareButtonOnImage = juce::ImageCache::getFromFile(assetsDir.getChildFile("button_square_small_on.png"));
        squareButtonOffImage = juce::ImageCache::getFromFile(assetsDir.getChildFile("button_square_small_off.png"));
        rectButtonOnImage = juce::ImageCache::getFromFile(assetsDir.getChildFile("button_rectangular_small_on.png"));
        rectButtonOffImage = juce::ImageCache::getFromFile(assetsDir.getChildFile("button_rectangular_small_off.png"));
        
        // Load background for channel strip
        backgroundImage = juce::ImageCache::getFromFile(assetsDir.getChildFile("background_02.png"));
        
        // Load knob frames for dials (trim and fx send)
        loadKnobImages();
        
        // Level meter background
        meterImage = juce::ImageCache::getFromFile(assetsDir.getChildFile("meter_vu.png"));
        
        // Load font
        auto fontFile = assetsDir.getChildFile("BlackwayFX/fonts/Roboto-Bold.ttf");
        if (fontFile.existsAsFile())
        {
            juce::MemoryBlock fontData;
            if (fontFile.loadFileAsData(fontData))
            {
                auto* fontDataPtr = static_cast<const void*>(fontData.getData());
                auto fontDataSize = static_cast<size_t>(fontData.getSize());
                
                if (fontDataPtr != nullptr && fontDataSize > 0)
                {
                    auto typeface = juce::Typeface::createSystemTypefaceFor(fontDataPtr, fontDataSize);
                    if (typeface != nullptr)
                    {
                        robotoFont = juce::Font(typeface);
                    }
                }
            }
        }
        
        // Set default colors for tabs and UI elements
        setColour(juce::TabbedComponent::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::TabbedButtonBar::tabOutlineColourId, juce::Colour(0xff333333));
        setColour(juce::TabbedButtonBar::frontOutlineColourId, juce::Colours::orange);
        setColour(juce::TabbedButtonBar::tabTextColourId, juce::Colours::lightgrey);
        setColour(juce::TabbedButtonBar::frontTextColourId, juce::Colours::white);
        
        // Set font for all text components
        if (robotoFont.getTypefacePtr() != nullptr)
        {
            juce::Typeface::Ptr typeface = robotoFont.getTypefacePtr();
            setDefaultSansSerifTypeface(typeface);
        }
        
        // Button text colors
        setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        
        // Progress bar colors
        setColour(juce::ProgressBar::backgroundColourId, juce::Colours::darkgrey.darker());
        setColour(juce::ProgressBar::foregroundColourId, juce::Colour(0xff2a9c3a));
        
        // Table header colors
        setColour(juce::TableHeaderComponent::textColourId, juce::Colours::white);
        setColour(juce::TableHeaderComponent::backgroundColourId, juce::Colours::darkgrey.darker(0.3f));
        setColour(juce::TableHeaderComponent::outlineColourId, juce::Colours::grey);
        
        // Table colors
        setColour(juce::ListBox::backgroundColourId, juce::Colours::darkgrey.darker(0.6f));
        setColour(juce::ListBox::outlineColourId, juce::Colours::grey);
        setColour(juce::ListBox::textColourId, juce::Colours::white);
    }
    
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, 
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        const bool isOn = button.getToggleState();
        const juce::Image& imageToUse = isOn ? switchOnImage : switchOffImage;
        
        if (imageToUse.isValid())
        {
            g.drawImage(imageToUse, 
                        0, 0, button.getWidth(), button.getHeight(),
                        0, 0, imageToUse.getWidth(), imageToUse.getHeight(),
                        false);
        }
        else
        {
            // Fallback to default if image not found
            LookAndFeel_V4::drawToggleButton(g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        }
    }
    
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
        auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
                                        .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);
        
        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);
        
        g.setColour(baseColour);
        
        if (button.isConnectedOnLeft() || button.isConnectedOnRight())
        {
            juce::Path path;
            path.addRoundedRectangle(bounds.getX(), bounds.getY(),
                                   bounds.getWidth(), bounds.getHeight(),
                                   4.0f, 4.0f,
                                   !button.isConnectedOnLeft(),  // curveTopLeft
                                   !button.isConnectedOnRight(), // curveTopRight
                                   !button.isConnectedOnLeft(),  // curveBottomLeft
                                   !button.isConnectedOnRight()  // curveBottomRight
                                   );
            g.fillPath(path);
            
            g.setColour(button.findColour(juce::ComboBox::outlineColourId));
            g.strokePath(path, juce::PathStrokeType(1.0f));
        }
        else
        {
            g.fillRoundedRectangle(bounds, 4.0f);
            
            if (button.isEnabled())
                g.setColour(button.findColour(juce::ComboBox::outlineColourId));
            else
                g.setColour(juce::Colours::darkgrey);
                
            g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        }
    }
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        if (knobFrames.size() > 0)
        {
            // Calculate which frame to use based on slider value
            int frameIndex = juce::jlimit(0, static_cast<int>(knobFrames.size()) - 1,
                                        static_cast<int>(sliderPosProportional * (knobFrames.size() - 1)));
                                        
            g.drawImage(knobFrames[frameIndex], 
                        x, y, width, height,
                        0, 0, knobFrames[frameIndex].getWidth(), knobFrames[frameIndex].getHeight(),
                        false);
        }
        else
        {
            // Fallback to default if images not loaded
            LookAndFeel_V4::drawRotarySlider(g, x, y, width, height, 
                                            sliderPosProportional, rotaryStartAngle,
                                            rotaryEndAngle, slider);
        }
    }
    
    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g, bool isMouseOver, bool isMouseDown) override
    {
        auto area = button.getActiveArea();
        auto baseColour = button.getTabBackgroundColour();
        
        if (button.getToggleState())
            baseColour = baseColour.contrasting(0.1f);
        
        if (isMouseOver)
            baseColour = baseColour.brighter(0.1f);
        
        if (isMouseDown)
            baseColour = baseColour.darker(0.1f);
            
        // Draw tab background
        g.setColour(baseColour);
        g.fillRect(area);
        
        // Draw a highlight on active tab
        if (button.getToggleState())
        {
            g.setColour(juce::Colours::orange);
            g.fillRect(area.getX(), area.getBottom() - 3, area.getWidth(), 3);
        }
        
        // Draw tab text
        auto textColour = button.getToggleState() ? 
                          juce::Colours::white :
                          juce::Colours::lightgrey;
        
        g.setColour(textColour);
        g.setFont(getTabButtonFont(button, 16.0f));
        
        g.drawText(button.getButtonText(), 
                  area.reduced(2), 
                  juce::Justification::centred,
                  true);
    }
    
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        return robotoFont.withHeight(juce::jmin(16.0f, buttonHeight * 0.6f)).boldened();
    }
    
    juce::Font getLabelFont(juce::Label& label) override
    {
        return robotoFont.withHeight(label.getFont().getHeight());
    }
    
    juce::Font getTabButtonFont(juce::TabBarButton&, float height) override
    {
        return robotoFont.withHeight(height).boldened();
    }
    
    juce::Image getBackgroundImage() const { return backgroundImage; }
    juce::Image getMeterImage() const { return meterImage; }
    juce::Font getRobotoFont() const { return robotoFont; }
    
private:
    juce::File getAssetsDirectory() const
    {
        // Use a proper path that respects the application location
        // For now, use a relative path from the application directory
        auto appDataDir = juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory);
        auto assetsDir = appDataDir.getChildFile("Auralis/Assets");
        
        // If the directory doesn't exist yet (development environment), fall back to local path
        if (!assetsDir.exists())
        {
            assetsDir = juce::File::getCurrentWorkingDirectory().getChildFile("Assets");
        }
        
        return assetsDir;
    }
    
    void loadKnobImages()
    {
        // Load knob frames from the BlackwayFX/knobs directory
        auto assetsDir = getAssetsDirectory();
        juce::File knobDirectory = assetsDir.getChildFile("BlackwayFX/knobs");
        
        if (knobDirectory.exists() && knobDirectory.isDirectory())
        {
            juce::Array<juce::File> knobFiles;
            knobDirectory.findChildFiles(knobFiles, juce::File::findFiles, false, "*.png");
            
            // Sort files numerically
            knobFiles.sort();
            
            for (auto& file : knobFiles)
            {
                juce::Image frame = juce::ImageCache::getFromFile(file);
                if (frame.isValid())
                {
                    knobFrames.add(frame);
                }
            }
        }
        
        // If we couldn't load the frames from BlackwayFX, try the original location
        if (knobFrames.isEmpty())
        {
            juce::File oldKnobDirectory = assetsDir.getChildFile("PNG Oneshots 128 frames/knob_small_scale_linear");
            
            if (oldKnobDirectory.exists() && oldKnobDirectory.isDirectory())
            {
                juce::Array<juce::File> knobFiles;
                oldKnobDirectory.findChildFiles(knobFiles, juce::File::findFiles, false, "*.png");
                
                // Sort files numerically
                knobFiles.sort();
                
                for (auto& file : knobFiles)
                {
                    juce::Image frame = juce::ImageCache::getFromFile(file);
                    if (frame.isValid())
                    {
                        knobFrames.add(frame);
                    }
                }
            }
        }
        
        // If we still couldn't load any frames, we'll load a fallback
        if (knobFrames.isEmpty())
        {
            juce::Image fallbackKnob = juce::ImageFileFormat::loadFrom(assetsDir.getChildFile("button_quare_big_off.png"));
            if (fallbackKnob.isValid())
            {
                knobFrames.add(fallbackKnob);
            }
        }
    }
    
    juce::Image switchOnImage, switchOffImage;
    juce::Image squareButtonOnImage, squareButtonOffImage;
    juce::Image rectButtonOnImage, rectButtonOffImage;
    juce::Image backgroundImage;
    juce::Image meterImage;
    juce::Array<juce::Image> knobFrames;
    juce::Font robotoFont { "Roboto Bold", 12.0f, juce::Font::bold };
}; 