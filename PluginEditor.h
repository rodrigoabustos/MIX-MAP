// Mix Map - editor: one scalable window, five modules left to right.
// By @_rodobustos
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Panels.h"

// Fixed-size design surface (1864 x 440). The editor scales it uniformly (50% - 200%).
class MixMapContent : public juce::Component
{
public:
    static constexpr int baseW = 1864;
    static constexpr int baseH = 440;

    explicit MixMapContent (MixMapProcessor& p);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void refresh();

    KeyPanel      keyPanel;
    ValuesPanel   msPanel;
    ValuesPanel   hzPanel;
    NotesPanel    notesPanel;
    PomodoroPanel pomodoroPanel;
    AlarmOverlay  overlay;
};

class MixMapEditor final : public juce::AudioProcessorEditor,
                           private juce::Timer
{
public:
    explicit MixMapEditor (MixMapProcessor& p);
    ~MixMapEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    MixMapLookAndFeel lnf;          // declared first: destroyed last
    MixMapProcessor&  proc;
    MixMapContent     content;
    bool              fastTimer = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixMapEditor)
};
