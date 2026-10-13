// Mix Map - dark look, macOS-style palette and shared widgets.
// By @_rodobustos
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "MixMapCore.h"

namespace mmcolour
{
    const juce::Colour window    { 0xff1c1c1e };
    const juce::Colour panel     { 0xff2c2c2e };
    const juce::Colour field     { 0xff3a3a3c };
    const juce::Colour border    { 0xff48484a };
    const juce::Colour text      { 0xfff2f2f7 };
    const juce::Colour secondary { 0xff98989d };
    const juce::Colour dim       { 0xff636366 };
    const juce::Colour accent    { 0xff0a84ff };

    const juce::Colour root      { 0xff30d158 };   // green
    const juce::Colour fifth     { 0xffffd60a };   // yellow
    const juce::Colour minor7    { 0xffff9f0a };   // orange
    const juce::Colour tritone   { 0xffff453a };   // red
}

inline juce::String utf8 (const std::string& s) { return juce::String::fromUTF8 (s.c_str()); }

inline juce::Font mmFont (float size, bool bold = false)
{
    const juce::Font f { juce::FontOptions { size } };
    return bold ? f.boldened() : f;
}

class MixMapLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MixMapLookAndFeel()
    {
        using namespace mmcolour;
        setColour (juce::ResizableWindow::backgroundColourId, window);

        setColour (juce::Label::textColourId, text);
        setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textWhenEditingColourId, text);
        setColour (juce::Label::backgroundWhenEditingColourId, field);
        setColour (juce::Label::outlineWhenEditingColourId, accent);

        setColour (juce::TextEditor::backgroundColourId, field);
        setColour (juce::TextEditor::textColourId, text);
        setColour (juce::TextEditor::outlineColourId, border);
        setColour (juce::TextEditor::focusedOutlineColourId, accent);
        setColour (juce::TextEditor::highlightColourId, accent.withAlpha (0.45f));
        setColour (juce::TextEditor::highlightedTextColourId, text);
        setColour (juce::CaretComponent::caretColourId, text);

        setColour (juce::ComboBox::backgroundColourId, field);
        setColour (juce::ComboBox::textColourId, text);
        setColour (juce::ComboBox::outlineColourId, border);
        setColour (juce::ComboBox::arrowColourId, secondary);
        setColour (juce::ComboBox::focusedOutlineColourId, accent);

        setColour (juce::PopupMenu::backgroundColourId, panel);
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

        setColour (juce::TextButton::buttonColourId, field);
        setColour (juce::TextButton::buttonOnColourId, accent);
        setColour (juce::TextButton::textColourOffId, text);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);

        setColour (juce::ToggleButton::textColourId, text);
        setColour (juce::ToggleButton::tickColourId, accent);
        setColour (juce::ToggleButton::tickDisabledColourId, dim);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        return mmFont (juce::jmin (13.0f, (float) buttonHeight * 0.55f), true);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override            { return mmFont (12.5f); }
    juce::Font getPopupMenuFont() override                           { return mmFont (13.0f); }
};

// Editable number: double-click to type, drag vertically or use the wheel to change.
class NumberBox : public juce::Label
{
public:
    std::function<void (double)> onValueChange;

    NumberBox()
    {
        setEditable (false, true, false);   // double-click edits
        setJustificationType (juce::Justification::centred);
        setColour (juce::Label::backgroundColourId, mmcolour::field);
        setColour (juce::Label::outlineColourId, mmcolour::border);
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        setFont (mmFont (13.0f, true));
    }

    void configure (double minV, double maxV, double wheelStep, double dragStepPerPixel, bool integerOnly = false)
    {
        minValue = minV;  maxValue = maxV;  step = wheelStep;  dragStep = dragStepPerPixel;  integers = integerOnly;
    }

    double getValue() const { return value; }

    void setValue (double v, bool notify)
    {
        v = juce::jlimit (minValue, maxValue, v);
        v = integers ? std::round (v) : std::round (v * 100.0) / 100.0;   // matches the 2 displayed decimals
        value = v;
        setText (utf8 (mixmap::formatTrimmed (value)), juce::dontSendNotification);
        if (notify && onValueChange)
            onValueChange (value);
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        dragStartValue = value;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (isBeingEdited() || e.getDistanceFromDragStart() < 3)
            return;

        setValue (dragStartValue - (double) e.getDistanceFromDragStartY() * dragStep, true);
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        if (juce::approximatelyEqual (w.deltaY, 0.0f))
            return;

        setValue (value + (w.deltaY > 0.0f ? step : -step), true);
    }

protected:
    void textWasEdited() override
    {
        const double parsed = mixmap::parseNumber (getText().toStdString(), value);
        setValue (parsed, true);
    }

private:
    double minValue = 0.0, maxValue = 1000.0, step = 1.0, dragStep = 0.25, value = 0.0, dragStartValue = 0.0;
    bool integers = false;
};

// Base for the five modules: rounded panel with a small caps title.
class PanelBase : public juce::Component
{
public:
    explicit PanelBase (juce::String panelTitle) : title (std::move (panelTitle)) {}

    void paint (juce::Graphics& g) override
    {
        g.setColour (mmcolour::panel);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f);

        g.setColour (mmcolour::secondary);
        g.setFont (mmFont (12.0f, true));
        g.drawText (title, 14, 9, getWidth() - 28, 18, juce::Justification::centredLeft, false);

        paintBody (g);
    }

protected:
    virtual void paintBody (juce::Graphics&) {}
    juce::String title;
};
