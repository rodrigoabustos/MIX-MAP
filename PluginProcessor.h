// Mix Map - plugin processor. No audio is processed: the buffer passes through untouched.
// By @_rodobustos
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <memory>
#include "MixMapCore.h"

struct MixMapSettings
{
    int    rootIndex  = 0;        // 0 = C ... 11 = B
    bool   useFlats   = false;
    bool   octaveLive = true;     // true: Live naming (C3 = middle C). false: scientific (C4)
    double a4         = 440.0;
    double bpmMs      = 120.0;
    double bpmHz      = 120.0;
    double scale      = 1.0;      // window scale, 0.5 .. 2.0
    std::array<juce::String, 8> noteText;
    std::array<bool, 8>         noteDone {};
    mixmap::PomodoroConfig      pomodoro;
};

class MixMapProcessor final : public juce::AudioProcessor,
                              private juce::Timer
{
public:
    MixMapProcessor();
    ~MixMapProcessor() override;

    // --- AudioProcessor: pass-through, no processing
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override {}
    bool supportsDoublePrecisionProcessing() const override { return true; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Mix Map"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- shared state (message thread only)
    MixMapSettings settings;
    mixmap::PomodoroEngine pomodoro;

    void markChanged();                    // tell the host the plugin state changed
    void applyPomodoroConfig();            // push settings.pomodoro into the engine

    // --- pomodoro alarm
    bool isAlarmActive() const             { return alarmActive.load(); }
    void dismissAlarm()                    { alarmActive.store (false); }
    void setEditorOpen (bool open)         { editorOpen.store (open); }

    static constexpr int64_t alarmDurationMs = 30000;
    static constexpr int64_t alarmRepeatMs   = 10000;

private:
    void timerCallback() override;
    void startAlarm (int64_t now);
    void playAlarmSound();

    std::atomic<bool> alarmActive { false };
    std::atomic<bool> editorOpen  { false };
    int64_t alarmStart   = 0;
    int64_t nextSoundAt  = 0;      // ms after alarmStart
    std::unique_ptr<juce::ChildProcess> soundProcess;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixMapProcessor)
};
