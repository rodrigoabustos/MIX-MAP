// By @_rodobustos
#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr int stateVersion = 1;
}

MixMapProcessor::MixMapProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    pomodoro.setConfig (settings.pomodoro);
    startTimer (250);
}

MixMapProcessor::~MixMapProcessor()
{
    stopTimer();
}

bool MixMapProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;

    return in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono();
}

juce::AudioProcessorEditor* MixMapProcessor::createEditor()
{
    return new MixMapEditor (*this);
}

void MixMapProcessor::markChanged()
{
    updateHostDisplay (ChangeDetails().withNonParameterStateChanged (true));
}

void MixMapProcessor::applyPomodoroConfig()
{
    settings.pomodoro.clamp();
    pomodoro.setConfig (settings.pomodoro);
}

// ------------------------------------------------------------------- pomodoro
void MixMapProcessor::timerCallback()
{
    const int64_t now = mixmap::nowMs();

    if (pomodoro.update (now))
        startAlarm (now);

    if (! alarmActive.load())
        return;

    const int64_t elapsed = now - alarmStart;

    if (elapsed >= alarmDurationMs)
    {
        alarmActive.store (false);
        return;
    }

    // Window open: the sound repeats every 10 s (3 times in total) until dismissed.
    // Window closed: it sounds once only, because there is nothing to click to stop it.
    if (editorOpen.load() && elapsed >= nextSoundAt)
    {
        playAlarmSound();
        nextSoundAt += alarmRepeatMs;
    }
}

void MixMapProcessor::startAlarm (int64_t now)
{
    alarmStart  = now;
    nextSoundAt = alarmRepeatMs;
    alarmActive.store (true);
    playAlarmSound();
}

void MixMapProcessor::playAlarmSound()
{
   #if JUCE_MAC
    // System sound through the OS (afplay), never through the plugin's audio path.
    const juce::File glass ("/System/Library/Sounds/Glass.aiff");

    if (glass.existsAsFile())
    {
        auto process = std::make_unique<juce::ChildProcess>();

        if (process->start (juce::StringArray { "/usr/bin/afplay", glass.getFullPathName() }, 0))
        {
            soundProcess = std::move (process);
            return;
        }
    }
   #endif

    // Fallback: standard OS alert sound.
    juce::LookAndFeel::getDefaultLookAndFeel().playAlertSound();
}

// ---------------------------------------------------------------------- state
void MixMapProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement xml ("MixMapState");
    xml.setAttribute ("version", stateVersion);
    xml.setAttribute ("root",       settings.rootIndex);
    xml.setAttribute ("flats",      settings.useFlats   ? 1 : 0);
    xml.setAttribute ("octaveLive", settings.octaveLive ? 1 : 0);
    xml.setAttribute ("a4",         settings.a4);
    xml.setAttribute ("bpmMs",      settings.bpmMs);
    xml.setAttribute ("bpmHz",      settings.bpmHz);
    xml.setAttribute ("scale",      settings.scale);
    xml.setAttribute ("workMin",    settings.pomodoro.workMin);
    xml.setAttribute ("shortMin",   settings.pomodoro.shortMin);
    xml.setAttribute ("longMin",    settings.pomodoro.longMin);
    xml.setAttribute ("everyN",     settings.pomodoro.everyN);

    for (int i = 0; i < 8; ++i)
    {
        xml.setAttribute ("note" + juce::String (i), settings.noteText[(size_t) i]);
        xml.setAttribute ("done" + juce::String (i), settings.noteDone[(size_t) i] ? 1 : 0);
    }

    copyXmlToBinary (xml, destData);
}

void MixMapProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml == nullptr || ! xml->hasTagName ("MixMapState"))
        return;

    MixMapSettings s;
    s.rootIndex  = juce::jlimit (0, 11, xml->getIntAttribute ("root", 0));
    s.useFlats   = xml->getIntAttribute ("flats", 0) != 0;
    s.octaveLive = xml->getIntAttribute ("octaveLive", 1) != 0;
    s.a4         = juce::jlimit (400.0, 480.0, xml->getDoubleAttribute ("a4", 440.0));
    s.bpmMs      = mixmap::clampBpm (xml->getDoubleAttribute ("bpmMs", 120.0));
    s.bpmHz      = mixmap::clampBpm (xml->getDoubleAttribute ("bpmHz", 120.0));
    s.scale      = juce::jlimit (0.5, 2.0, xml->getDoubleAttribute ("scale", 1.0));
    s.pomodoro.workMin  = xml->getIntAttribute ("workMin", 25);
    s.pomodoro.shortMin = xml->getIntAttribute ("shortMin", 5);
    s.pomodoro.longMin  = xml->getIntAttribute ("longMin", 15);
    s.pomodoro.everyN   = xml->getIntAttribute ("everyN", 4);
    s.pomodoro.clamp();

    for (int i = 0; i < 8; ++i)
    {
        s.noteText[(size_t) i] = xml->getStringAttribute ("note" + juce::String (i)).substring (0, 120);
        s.noteDone[(size_t) i] = xml->getIntAttribute ("done" + juce::String (i), 0) != 0;
    }

    settings = s;
    pomodoro.setConfig (settings.pomodoro);
}

// Required entry point for the plugin wrapper.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MixMapProcessor();
}
