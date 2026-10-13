// Mix Map - the five modules (KEY, NOTE VALUES, RATE, MIX NOTES, POMODORO) and the alarm overlay.
// By @_rodobustos
#pragma once

#include <array>
#include "Look.h"
#include "PluginProcessor.h"

//==============================================================================
// Module 1 - KEY: root note -> Hz of root, P5, b7, #4 across octaves C0..C10
class KeyPanel : public PanelBase
{
public:
    explicit KeyPanel (MixMapProcessor& p) : PanelBase ("KEY"), proc (p)
    {
        addAndMakeVisible (rootBox);
        addAndMakeVisible (accidentalButton);
        addAndMakeVisible (a4Box);
        addAndMakeVisible (conventionBox);

        rebuildRootItems();
        rootBox.setScrollWheelEnabled (false);
        rootBox.onChange = [this]
        {
            proc.settings.rootIndex = rootBox.getSelectedId() - 1;
            proc.markChanged();
            repaint();
        };

        accidentalButton.setClickingTogglesState (false);
        accidentalButton.onClick = [this]
        {
            proc.settings.useFlats = ! proc.settings.useFlats;
            updateAccidentalText();
            rebuildRootItems();
            proc.markChanged();
            repaint();
        };
        updateAccidentalText();

        a4Box.configure (400.0, 480.0, 1.0, 0.1);
        a4Box.setValue (proc.settings.a4, false);
        a4Box.onValueChange = [this] (double v)
        {
            proc.settings.a4 = v;
            proc.markChanged();
            repaint();
        };

        conventionBox.addItem ("Octaves: Live (C3 = middle C)", 1);
        conventionBox.addItem ("Octaves: Scientific (C4 = middle C)", 2);
        conventionBox.setSelectedId (proc.settings.octaveLive ? 1 : 2, juce::dontSendNotification);
        conventionBox.setScrollWheelEnabled (false);
        conventionBox.onChange = [this]
        {
            proc.settings.octaveLive = conventionBox.getSelectedId() == 1;
            proc.markChanged();
            repaint();
        };
    }

    void resized() override
    {
        rootBox.setBounds (14, 34, 96, 26);
        accidentalButton.setBounds (118, 34, 40, 26);
        a4Box.setBounds (200, 34, 64, 26);
        conventionBox.setBounds (306, 34, 260, 26);
    }

protected:
    void paintBody (juce::Graphics& g) override
    {
        using namespace mmcolour;
        const auto& st = proc.settings;

        g.setColour (secondary);
        g.setFont (mmFont (12.0f));
        g.drawText ("A4", juce::Rectangle<float> (172.0f, 34.0f, 26.0f, 26.0f), juce::Justification::centredRight, false);
        g.drawText ("Hz", juce::Rectangle<float> (268.0f, 34.0f, 28.0f, 26.0f), juce::Justification::centredLeft, false);

        const float x0 = 14.0f, labelW = 56.0f, y0 = 76.0f, headH = 22.0f, rowH = 62.0f;
        const float colW = ((float) getWidth() - 14.0f - (x0 + labelW)) / (float) mixmap::numOctaves;

        g.setFont (mmFont (11.0f));
        g.setColour (secondary);
        g.drawText ("Octave", juce::Rectangle<float> (x0, y0, labelW, headH), juce::Justification::centredLeft, false);

        for (int c = 0; c < mixmap::numOctaves; ++c)
        {
            const int label = mixmap::displayedOctave (mixmap::firstOctave + c, st.octaveLive);
            g.drawText (juce::String (label),
                        juce::Rectangle<float> (x0 + labelW + (float) c * colW, y0, colW, headH),
                        juce::Justification::centred, false);
        }

        g.setColour (border);
        g.drawHorizontalLine ((int) (y0 + headH), x0, (float) getWidth() - 14.0f);

        const juce::Colour rowColour[4] = { root, fifth, minor7, tritone };

        for (int row = 0; row < 4; ++row)
        {
            const float y = y0 + headH + (float) row * rowH;

            g.setColour (rowColour[row]);
            g.setFont (mmFont (13.0f, true));
            g.drawText (utf8 (mixmap::intervalLabel (row)),
                        juce::Rectangle<float> (x0, y, labelW, rowH), juce::Justification::centredLeft, false);

            for (int c = 0; c < mixmap::numOctaves; ++c)
            {
                const auto cell = mixmap::keyCell (st.rootIndex, mixmap::firstOctave + c, row,
                                                   st.a4, st.useFlats, st.octaveLive);
                const float x = x0 + labelW + (float) c * colW;

                if (cell.audible)
                {
                    g.setColour (rowColour[row]);
                    g.setFont (mmFont (12.5f, true));
                    g.drawText (utf8 (mixmap::formatHzKey (cell.hz)),
                                juce::Rectangle<float> (x, y + 12.0f, colW, 18.0f), juce::Justification::centred, false);

                    g.setColour (secondary);
                    g.setFont (mmFont (10.5f));
                    g.drawText (utf8 (cell.noteName),
                                juce::Rectangle<float> (x, y + 31.0f, colW, 16.0f), juce::Justification::centred, false);
                }
                else
                {
                    g.setColour (dim);
                    g.setFont (mmFont (12.5f));
                    g.drawText (utf8 (mixmap::dashSymbol()),
                                juce::Rectangle<float> (x, y, colW, rowH), juce::Justification::centred, false);
                }
            }

            g.setColour (border.withAlpha (0.5f));
            g.drawHorizontalLine ((int) (y + rowH), x0, (float) getWidth() - 14.0f);
        }

        g.setColour (dim);
        g.setFont (mmFont (10.5f));
        g.drawText ("Equal temperament. Notes above 20 kHz are dimmed.",
                    juce::Rectangle<float> (x0, 350.0f, 500.0f, 16.0f), juce::Justification::centredLeft, false);
    }

private:
    void updateAccidentalText()
    {
        accidentalButton.setButtonText (utf8 (proc.settings.useFlats ? mixmap::flatSymbol() : mixmap::sharpSymbol()));
    }

    void rebuildRootItems()
    {
        rootBox.clear (juce::dontSendNotification);
        for (int pc = 0; pc < 12; ++pc)
            rootBox.addItem (utf8 (mixmap::pitchClassName (pc, proc.settings.useFlats)), pc + 1);
        rootBox.setSelectedId (proc.settings.rootIndex + 1, juce::dontSendNotification);
    }

    MixMapProcessor& proc;
    juce::ComboBox   rootBox, conventionBox;
    juce::TextButton accidentalButton;
    NumberBox        a4Box;
};

//==============================================================================
// Modules 2 and 3 - tempo tables: note values in ms, or the same values as rates in Hz
class ValuesPanel : public PanelBase
{
public:
    enum class Mode { Milliseconds, Hertz };

    ValuesPanel (MixMapProcessor& p, Mode m)
        : PanelBase (m == Mode::Milliseconds ? "NOTE VALUES (ms)" : "RATE (Hz)"), proc (p), mode (m)
    {
        addAndMakeVisible (bpmBox);
        addAndMakeVisible (tapButton);

        bpmBox.configure (20.0, 999.0, 1.0, 0.25);
        bpmBox.setValue (bpmRef(), false);
        bpmBox.onValueChange = [this] (double v)
        {
            bpmRef() = v;
            proc.markChanged();
            repaint();
        };

        tapButton.setButtonText ("TAP");
        tapButton.onClick = [this]
        {
            const double bpm = tapper.tap (mixmap::nowMs());
            if (bpm > 0.0)
                bpmBox.setValue (bpm, true);
        };
    }

    void resized() override
    {
        bpmBox.setBounds (52, 34, 84, 26);
        tapButton.setBounds (144, 34, 56, 26);
    }

protected:
    void paintBody (juce::Graphics& g) override
    {
        using namespace mmcolour;

        g.setColour (secondary);
        g.setFont (mmFont (12.0f));
        g.drawText ("BPM", juce::Rectangle<float> (14.0f, 34.0f, 36.0f, 26.0f), juce::Justification::centredLeft, false);

        static const char* const names[mixmap::numNoteValues] = { "Whole", "Half", "Quarter", "8th", "16th", "32nd", "64th" };
        static const char* const heads[3] = { "Straight", "Dotted", "Triplet" };
        static const mixmap::Modifier mods[3] = { mixmap::Modifier::Straight, mixmap::Modifier::Dotted, mixmap::Modifier::Triplet };

        const float x0 = 14.0f, labelW = 78.0f, y0 = 76.0f, headH = 22.0f, rowH = 36.0f;
        const float colW = ((float) getWidth() - 14.0f - (x0 + labelW)) / 3.0f;

        g.setFont (mmFont (11.0f));
        for (int c = 0; c < 3; ++c)
            g.drawText (heads[c], juce::Rectangle<float> (x0 + labelW + (float) c * colW, y0, colW, headH),
                        juce::Justification::centred, false);

        g.setColour (border);
        g.drawHorizontalLine ((int) (y0 + headH), x0, (float) getWidth() - 14.0f);

        for (int row = 0; row < mixmap::numNoteValues; ++row)
        {
            const float y = y0 + headH + (float) row * rowH;
            const int denom = mixmap::noteDenominator (row);

            if (row % 2 == 1)
            {
                g.setColour (field.withAlpha (0.35f));
                g.fillRect (juce::Rectangle<float> (x0 - 4.0f, y, (float) getWidth() - 2.0f * (x0 - 4.0f), rowH));
            }

            g.setColour (text);
            g.setFont (mmFont (12.0f, true));
            g.drawText (names[row], juce::Rectangle<float> (x0, y, labelW, rowH), juce::Justification::centredLeft, false);
            g.setColour (secondary);
            g.setFont (mmFont (10.5f));
            g.drawText ("1/" + juce::String (denom), juce::Rectangle<float> (x0, y, labelW - 4.0f, rowH),
                        juce::Justification::centredRight, false);

            for (int c = 0; c < 3; ++c)
            {
                const double ms = mixmap::noteMs (denom, bpmRef(), mods[c]);
                const std::string value = mode == Mode::Milliseconds ? mixmap::formatMs (ms)
                                                                     : mixmap::formatHzRate (mixmap::msToHz (ms));
                g.setColour (text);
                g.setFont (mmFont (12.0f));
                g.drawText (utf8 (value), juce::Rectangle<float> (x0 + labelW + (float) c * colW, y, colW, rowH),
                            juce::Justification::centred, false);
            }
        }

        g.setColour (dim);
        g.setFont (mmFont (10.5f));
        g.drawText (mode == Mode::Milliseconds ? "Whole note = 240000 / BPM ms. Dotted x1.5, triplet x2/3."
                                               : "Hz = 1000 / ms. For LFO and modulation rates.",
                    juce::Rectangle<float> (x0, 354.0f, (float) getWidth() - 28.0f, 14.0f),
                    juce::Justification::centredLeft, false);
    }

private:
    double& bpmRef() { return mode == Mode::Milliseconds ? proc.settings.bpmMs : proc.settings.bpmHz; }

    MixMapProcessor& proc;
    Mode             mode;
    NumberBox        bpmBox;
    juce::TextButton tapButton;
    mixmap::TapTempo tapper;
};

//==============================================================================
// Module 4 - MIX NOTES: 8 lines with checkbox, saved with the Live set
class NotesPanel : public PanelBase
{
public:
    explicit NotesPanel (MixMapProcessor& p) : PanelBase ("MIX NOTES"), proc (p)
    {
        for (int i = 0; i < 8; ++i)
        {
            auto& row = rows[(size_t) i];

            addAndMakeVisible (row.check);
            addAndMakeVisible (row.editor);
            addAndMakeVisible (row.strike);

            row.strike.owner = &row;
            row.strike.setInterceptsMouseClicks (false, false);

            row.check.setButtonText ({});
            row.check.setToggleState (proc.settings.noteDone[(size_t) i], juce::dontSendNotification);
            row.check.onClick = [this, i]
            {
                proc.settings.noteDone[(size_t) i] = rows[(size_t) i].check.getToggleState();
                refreshRow (i);
                proc.markChanged();
            };

            auto& ed = row.editor;
            ed.setMultiLine (false);
            ed.setReturnKeyStartsNewLine (false);
            ed.setScrollbarsShown (false);
            ed.setJustification (juce::Justification::centredLeft);
            ed.setIndents (6, 0);
            ed.setFont (row.font);
            ed.setInputRestrictions (120);
            ed.setTextToShowWhenEmpty ("Note " + juce::String (i + 1), mmcolour::dim);
            ed.setText (proc.settings.noteText[(size_t) i], juce::dontSendNotification);
            ed.onTextChange = [this, i]
            {
                proc.settings.noteText[(size_t) i] = rows[(size_t) i].editor.getText();
                rows[(size_t) i].strike.repaint();
            };
            ed.onReturnKey = [this, i]
            {
                rows[(size_t) i].editor.giveAwayKeyboardFocus();
                proc.markChanged();
            };
            ed.onEscapeKey = [this, i] { rows[(size_t) i].editor.giveAwayKeyboardFocus(); };
            ed.onFocusLost = [this] { proc.markChanged(); };

            refreshRow (i);
        }
    }

    void resized() override
    {
        for (int i = 0; i < 8; ++i)
        {
            auto& row = rows[(size_t) i];
            const int y = 38 + i * 40;
            row.check.setBounds (10, y + 2, 26, 26);
            row.editor.setBounds (40, y, getWidth() - 40 - 12, 30);
            row.strike.setBounds (row.editor.getBounds());
        }
    }

private:
    struct Row;

    struct Strike : public juce::Component
    {
        Row* owner = nullptr;
        void paint (juce::Graphics& g) override;
    };

    struct Row
    {
        juce::ToggleButton check;
        juce::TextEditor   editor;
        Strike             strike;
        juce::Font         font { mmFont (13.0f) };
    };

    void refreshRow (int i)
    {
        auto& row = rows[(size_t) i];
        const bool done = row.check.getToggleState();
        const auto colour = done ? mmcolour::dim : mmcolour::text;
        row.editor.setColour (juce::TextEditor::textColourId, colour);
        row.editor.applyColourToAllText (colour, true);
        row.strike.repaint();
    }

    MixMapProcessor&    proc;
    std::array<Row, 8>  rows;
};

inline void NotesPanel::Strike::paint (juce::Graphics& g)
{
    if (owner == nullptr || ! owner->check.getToggleState())
        return;

    const auto text = owner->editor.getText();
    if (text.isEmpty())
        return;

    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText (owner->font, text, 0.0f, 0.0f);
    const float textW = glyphs.getBoundingBox (0, -1, true).getWidth();
    const float maxW  = (float) getWidth() - 14.0f;

    g.setColour (mmcolour::secondary);
    g.drawHorizontalLine (getHeight() / 2, 7.0f, 7.0f + juce::jmin (textW, maxW));
}

//==============================================================================
// Module 5 - POMODORO (manual phases, long break every N cycles)
class PomodoroPanel : public PanelBase
{
public:
    explicit PomodoroPanel (MixMapProcessor& p) : PanelBase ("POMODORO"), proc (p)
    {
        addAndMakeVisible (startButton);
        addAndMakeVisible (resetButton);
        addAndMakeVisible (skipButton);
        for (auto* b : { &workBox, &shortBox, &longBox, &everyBox })
            addAndMakeVisible (b);

        resetButton.setButtonText ("Reset");
        skipButton.setButtonText ("Skip");

        startButton.onClick = [this]
        {
            const int64_t now = mixmap::nowMs();
            if (proc.pomodoro.state() == mixmap::RunState::Running)
                proc.pomodoro.pause (now);
            else
            {
                proc.pomodoro.start (now);
                proc.dismissAlarm();
            }
            refresh();
        };
        resetButton.onClick = [this] { proc.pomodoro.reset(); proc.dismissAlarm(); refresh(); };
        skipButton.onClick  = [this] { proc.pomodoro.skip();  proc.dismissAlarm(); refresh(); };

        setupBox (workBox,  1.0, 999.0, 1.0, 0.2, proc.settings.pomodoro.workMin,  [this] (int v) { proc.settings.pomodoro.workMin  = v; });
        setupBox (shortBox, 1.0, 999.0, 1.0, 0.2, proc.settings.pomodoro.shortMin, [this] (int v) { proc.settings.pomodoro.shortMin = v; });
        setupBox (longBox,  1.0, 999.0, 1.0, 0.2, proc.settings.pomodoro.longMin,  [this] (int v) { proc.settings.pomodoro.longMin  = v; });
        setupBox (everyBox, 1.0, 99.0,  1.0, 0.1, proc.settings.pomodoro.everyN,   [this] (int v) { proc.settings.pomodoro.everyN   = v; });

        refresh();
    }

    void refresh()
    {
        const auto state = proc.pomodoro.state();
        startButton.setButtonText (state == mixmap::RunState::Running ? "Pause"
                                   : state == mixmap::RunState::Paused ? "Resume" : "Start");
        repaint();
    }

    void resized() override
    {
        const int w = getWidth();
        startButton.setBounds (14, 146, w - 28, 32);
        resetButton.setBounds (14, 184, (w - 36) / 2, 28);
        skipButton.setBounds (14 + (w - 36) / 2 + 8, 184, (w - 36) / 2, 28);

        const int boxX = w - 14 - 56;
        for (int i = 0; i < 4; ++i)
        {
            juce::Component* b = i == 0 ? (juce::Component*) &workBox : i == 1 ? (juce::Component*) &shortBox
                               : i == 2 ? (juce::Component*) &longBox : (juce::Component*) &everyBox;
            b->setBounds (boxX, 246 + i * 30, 56, 24);
        }
    }

protected:
    void paintBody (juce::Graphics& g) override
    {
        using namespace mmcolour;
        const auto now = mixmap::nowMs();
        const auto& eng = proc.pomodoro;

        const juce::Colour phaseColour = eng.phase() == mixmap::Phase::Work ? text
                                       : eng.phase() == mixmap::Phase::ShortBreak ? root : accent;

        g.setColour (phaseColour);
        g.setFont (mmFont (46.0f, true));
        g.drawText (utf8 (mixmap::formatClock (eng.remainingMs (now))),
                    juce::Rectangle<float> (0.0f, 36.0f, (float) getWidth(), 62.0f), juce::Justification::centred, false);

        juce::String phaseText = eng.phase() == mixmap::Phase::Work ? "WORK - Cycle " + juce::String (eng.cycle()) + "/" + juce::String (eng.config().everyN)
                               : eng.phase() == mixmap::Phase::ShortBreak ? "SHORT BREAK" : "LONG BREAK";
        g.setColour (secondary);
        g.setFont (mmFont (12.0f, true));
        g.drawText (phaseText, juce::Rectangle<float> (0.0f, 100.0f, (float) getWidth(), 18.0f), juce::Justification::centred, false);

        juce::String status;
        if (eng.state() == mixmap::RunState::Running)      status = "Running";
        else if (eng.state() == mixmap::RunState::Paused)  status = "Paused";
        else if (eng.awaitingStart())                      status = eng.lastFinished() == mixmap::Phase::Work ? "Work done. Press Start." : "Break done. Press Start.";
        else                                               status = "Ready";

        g.setFont (mmFont (11.5f));
        g.setColour (eng.awaitingStart() ? minor7 : dim);
        g.drawText (status, juce::Rectangle<float> (0.0f, 120.0f, (float) getWidth(), 18.0f), juce::Justification::centred, false);

        g.setColour (border);
        g.drawHorizontalLine (224, 14.0f, (float) getWidth() - 14.0f);

        g.setColour (secondary);
        g.setFont (mmFont (11.0f, true));
        g.drawText ("SETTINGS (minutes)", juce::Rectangle<float> (14.0f, 228.0f, (float) getWidth() - 28.0f, 16.0f),
                    juce::Justification::centredLeft, false);

        g.setFont (mmFont (12.0f));
        const char* labels[4] = { "Work", "Short break", "Long break", "Every N cycles" };
        for (int i = 0; i < 4; ++i)
            g.drawText (labels[i], juce::Rectangle<float> (14.0f, (float) (246 + i * 30), (float) getWidth() - 14.0f - 62.0f - 14.0f, 24.0f),
                        juce::Justification::centredLeft, false);
    }

private:
    void setupBox (NumberBox& box, double mn, double mx, double step, double drag, int initial, std::function<void (int)> assign)
    {
        box.configure (mn, mx, step, drag, true);
        box.setValue ((double) initial, false);
        box.onValueChange = [this, assign] (double v)
        {
            assign ((int) v);
            proc.applyPomodoroConfig();
            proc.markChanged();
            refresh();
        };
    }

    MixMapProcessor& proc;
    juce::TextButton startButton, resetButton, skipButton;
    NumberBox        workBox, shortBox, longBox, everyBox;
};

//==============================================================================
// Pulsing flash over the whole window when a phase ends. Click anywhere to dismiss.
class AlarmOverlay : public juce::Component
{
public:
    explicit AlarmOverlay (MixMapProcessor& p) : proc (p) { setVisible (false); }

    void paint (juce::Graphics& g) override
    {
        const double t = juce::Time::getMillisecondCounterHiRes() / 1000.0;
        const float pulse = 0.5f + 0.5f * (float) std::sin (t * juce::MathConstants<double>::twoPi * 1.5);

        g.fillAll (mmcolour::minor7.withAlpha (0.18f + 0.34f * pulse));

        const bool workDone = proc.pomodoro.lastFinished() == mixmap::Phase::Work;
        g.setColour (juce::Colours::white);
        g.setFont (mmFont (40.0f, true));
        g.drawText (workDone ? "WORK DONE - TAKE A BREAK" : "BREAK DONE - BACK TO WORK",
                    getLocalBounds().withTrimmedBottom (50), juce::Justification::centred, false);

        g.setFont (mmFont (16.0f));
        g.drawText ("Click anywhere to dismiss", getLocalBounds().withTrimmedTop (70), juce::Justification::centred, false);
    }

    void mouseDown (const juce::MouseEvent&) override { proc.dismissAlarm(); }

private:
    MixMapProcessor& proc;
};
