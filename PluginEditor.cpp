// By @_rodobustos
#include "PluginEditor.h"

//==============================================================================
MixMapContent::MixMapContent (MixMapProcessor& p)
    : keyPanel (p),
      msPanel (p, ValuesPanel::Mode::Milliseconds),
      hzPanel (p, ValuesPanel::Mode::Hertz),
      notesPanel (p),
      pomodoroPanel (p),
      overlay (p)
{
    addAndMakeVisible (keyPanel);
    addAndMakeVisible (msPanel);
    addAndMakeVisible (hzPanel);
    addAndMakeVisible (notesPanel);
    addAndMakeVisible (pomodoroPanel);
    addChildComponent (overlay);       // last child = on top; hidden until the alarm fires

    setSize (baseW, baseH);
}

void MixMapContent::paint (juce::Graphics& g)
{
    g.fillAll (mmcolour::window);

    g.setColour (mmcolour::secondary);
    g.setFont (mmFont (15.0f, true));
    g.drawText ("MIX MAP", juce::Rectangle<float> (18.0f, 6.0f, 200.0f, 24.0f), juce::Justification::centredLeft, false);

    g.setColour (mmcolour::dim);
    g.setFont (mmFont (12.0f));
    g.drawText ("By @_rodobustos", juce::Rectangle<float> (0.0f, (float) baseH - 30.0f, (float) baseW, 22.0f),
                juce::Justification::centred, false);
}

void MixMapContent::resized()
{
    const int y = 34, h = 372, gap = 12;
    int x = 14;

    keyPanel.setBounds (x, y, 736, h);          x += 736 + gap;
    msPanel.setBounds (x, y, 290, h);           x += 290 + gap;
    hzPanel.setBounds (x, y, 290, h);           x += 290 + gap;
    notesPanel.setBounds (x, y, 280, h);        x += 280 + gap;
    pomodoroPanel.setBounds (x, y, getWidth() - 14 - x, h);

    overlay.setBounds (getLocalBounds());
}

void MixMapContent::refresh()
{
    pomodoroPanel.refresh();
}

//==============================================================================
MixMapEditor::MixMapEditor (MixMapProcessor& p)
    : AudioProcessorEditor (&p), proc (p), content (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);

    const double scale = juce::jlimit (0.5, 2.0, proc.settings.scale);
    const int w = juce::roundToInt ((double) MixMapContent::baseW * scale);
    const int h = juce::roundToInt ((double) w * (double) MixMapContent::baseH / (double) MixMapContent::baseW);

    setResizable (true, true);
    setResizeLimits (MixMapContent::baseW / 2, MixMapContent::baseH / 2,
                     MixMapContent::baseW * 2, MixMapContent::baseH * 2);
    getConstrainer()->setFixedAspectRatio ((double) MixMapContent::baseW / (double) MixMapContent::baseH);
    setSize (w, h);

    proc.setEditorOpen (true);
    startTimerHz (10);
}

MixMapEditor::~MixMapEditor()
{
    stopTimer();
    proc.setEditorOpen (false);
    setLookAndFeel (nullptr);
}

void MixMapEditor::paint (juce::Graphics& g)
{
    g.fillAll (mmcolour::window);
}

void MixMapEditor::resized()
{
    const float s = (float) getWidth() / (float) MixMapContent::baseW;
    content.setTransform (juce::AffineTransform::scale (s));
    content.setBounds (0, 0, MixMapContent::baseW, MixMapContent::baseH);
    proc.settings.scale = (double) s;
}

void MixMapEditor::timerCallback()
{
    content.refresh();

    const bool alarm = proc.isAlarmActive();

    if (content.overlay.isVisible() != alarm)
        content.overlay.setVisible (alarm);

    if (alarm)
        content.overlay.repaint();

    if (alarm != fastTimer)          // 30 fps only while the flash is running
    {
        fastTimer = alarm;
        startTimerHz (alarm ? 30 : 10);
    }
}
