// Mix Map - core calculations. No JUCE dependency, so it can be unit tested anywhere.
// By @_rodobustos
#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

namespace mixmap
{
    // UTF-8 musical symbols. Kept as constants (never inside hex-escaped literals
    // followed by digits, which would swallow the digit as part of the escape).
    inline const char* sharpSymbol() { return "\xE2\x99\xAF"; }  // U+266F
    inline const char* flatSymbol()  { return "\xE2\x99\xAD"; }  // U+266D
    inline const char* dashSymbol()  { return "\xE2\x80\x94"; }  // U+2014

    constexpr double maxAudibleHz = 20000.0;
    constexpr int    firstOctave  = 0;   // scientific octave of the first column (C0 = 16.35 Hz)
    constexpr int    lastOctave   = 10;  // scientific octave of the last column  (C10 = 16744 Hz)
    constexpr int    numOctaves   = lastOctave - firstOctave + 1;  // 11

    // ------------------------------------------------------------------ clock
    inline int64_t nowMs()
    {
        using namespace std::chrono;
        return duration_cast<milliseconds> (steady_clock::now().time_since_epoch()).count();
    }

    // ------------------------------------------------------------- formatting
    inline std::string formatFixed (double v, int decimals)
    {
        std::ostringstream os;
        os.imbue (std::locale::classic());
        os << std::fixed << std::setprecision (decimals) << v;
        return os.str();
    }

    inline std::string withThousands (const std::string& s)
    {
        const auto dot = s.find ('.');
        const std::string intPart = s.substr (0, dot);
        const std::string rest    = dot == std::string::npos ? "" : s.substr (dot);
        std::string out;
        int count = 0;
        for (int i = (int) intPart.size() - 1; i >= 0; --i)
        {
            out.insert (out.begin(), intPart[(size_t) i]);
            if (++count % 3 == 0 && i > 0 && intPart[(size_t) i - 1] != '-')
                out.insert (out.begin(), ',');
        }
        return out + rest;
    }

    // Module 1 (KEY): < 1000 Hz -> 2 decimals; >= 1000 Hz -> 1 decimal + thousands separator.
    inline std::string formatHzKey (double hz)
    {
        if (hz < 1000.0)
            return formatFixed (hz, 2);
        return withThousands (formatFixed (hz, 1));
    }

    // Module 2: milliseconds, 2 decimals.
    inline std::string formatMs (double ms)   { return formatFixed (ms, 2); }

    // Module 3: Hz, always 3 decimals (very low rates at slow tempos need them).
    inline std::string formatHzRate (double hz) { return formatFixed (hz, 3); }

    // BPM / A4: up to 2 decimals, trailing zeros removed ("120", "128.5", "99.99").
    inline std::string formatTrimmed (double v)
    {
        std::string s = formatFixed (v, 2);
        while (! s.empty() && s.back() == '0') s.pop_back();
        if (! s.empty() && s.back() == '.') s.pop_back();
        return s;
    }

    // Accepts "128.5" and "128,5".
    inline double parseNumber (std::string s, double fallback)
    {
        std::replace (s.begin(), s.end(), ',', '.');
        try
        {
            size_t used = 0;
            const double v = std::stod (s, &used);
            return used == 0 ? fallback : v;
        }
        catch (...) { return fallback; }
    }

    // ------------------------------------------------------------ module 1: key
    inline std::string pitchClassName (int pc, bool flats)
    {
        static const char* const names[12] = { "C", "", "D", "", "E", "F", "", "G", "", "A", "", "B" };
        pc = ((pc % 12) + 12) % 12;
        if (names[pc][0] != '\0')
            return names[pc];

        // Black keys: 1=C#/Db 3=D#/Eb 6=F#/Gb 8=G#/Ab 10=A#/Bb
        static const char letterSharp[12] = { 0, 'C', 0, 'D', 0, 0, 'F', 0, 'G', 0, 'A', 0 };
        static const char letterFlat [12] = { 0, 'D', 0, 'E', 0, 0, 'G', 0, 'A', 0, 'B', 0 };
        std::string s (1, flats ? letterFlat[pc] : letterSharp[pc]);
        s += flats ? flatSymbol() : sharpSymbol();
        return s;
    }

    inline double midiToHz (int midi, double a4)
    {
        return a4 * std::pow (2.0, (midi - 69) / 12.0);
    }

    // Scientific pitch notation: C4 = MIDI 60 = middle C = 261.63 Hz.
    // Octave of the root column -> MIDI of the root (C of that octave + pitch class).
    inline int rootMidi (int rootPc, int scientificOctave)
    {
        return 12 * (scientificOctave + 1) + rootPc;
    }

    // Ableton Live labels middle C as C3, i.e. one octave below scientific notation.
    inline int displayedOctave (int scientificOctave, bool liveConvention)
    {
        return scientificOctave - (liveConvention ? 1 : 0);
    }

    struct Interval { const char* label; int semitones; };

    // Row order in the KEY table: root (green), perfect fifth (yellow),
    // minor seventh (orange), augmented fourth (red).
    inline int intervalSemitones (int row)
    {
        static const int s[4] = { 0, 7, 10, 6 };
        return s[std::clamp (row, 0, 3)];
    }

    inline std::string intervalLabel (int row)
    {
        switch (row)
        {
            case 0:  return "Root";
            case 1:  return "P5";
            case 2:  return std::string (flatSymbol()) + "7";
            default: return std::string (sharpSymbol()) + "4";
        }
    }

    struct KeyCell
    {
        double      hz;
        int         midi;
        bool        audible;     // hz <= 20 kHz
        std::string noteName;    // e.g. "F#5" (octave per chosen convention)
    };

    inline KeyCell keyCell (int rootPc, int scientificOctave, int row, double a4, bool flats, bool liveConvention)
    {
        const int midi = rootMidi (rootPc, scientificOctave) + intervalSemitones (row);
        const double hz = midiToHz (midi, a4);
        const int sciOct = midi / 12 - 1;
        std::string name = pitchClassName (midi % 12, flats) + std::to_string (displayedOctave (sciOct, liveConvention));
        return { hz, midi, hz <= maxAudibleHz, name };
    }

    // --------------------------------------------------- modules 2 and 3: tempo
    enum class Modifier { Straight, Dotted, Triplet };

    constexpr int numNoteValues = 7;

    inline int noteDenominator (int index)           // 0..6 -> 1,2,4,8,16,32,64
    {
        return 1 << std::clamp (index, 0, numNoteValues - 1);
    }

    inline double modifierFactor (Modifier m)
    {
        switch (m)
        {
            case Modifier::Dotted:   return 1.5;
            case Modifier::Triplet:  return 2.0 / 3.0;
            case Modifier::Straight: return 1.0;
        }
        return 1.0;
    }

    // Whole note = 4 beats = 240000 / BPM ms.
    inline double noteMs (int denominator, double bpm, Modifier m)
    {
        return 240000.0 / (bpm * (double) denominator) * modifierFactor (m);
    }

    inline double msToHz (double ms) { return 1000.0 / ms; }

    inline double clampBpm (double bpm) { return std::clamp (bpm, 20.0, 999.0); }

    // ---------------------------------------------------------------- tap tempo
    class TapTempo
    {
    public:
        // Returns the new BPM, or a value <= 0 if there are not enough taps yet.
        double tap (int64_t nowMs)
        {
            if (! taps.empty() && nowMs - taps.back() > resetAfterMs)
                taps.clear();

            taps.push_back (nowMs);
            if (taps.size() > maxTaps)
                taps.erase (taps.begin());

            if (taps.size() < 2)
                return 0.0;

            const double avg = (double) (taps.back() - taps.front()) / (double) (taps.size() - 1);
            if (avg <= 0.0)
                return 0.0;

            return clampBpm (60000.0 / avg);
        }

        void reset() { taps.clear(); }

    private:
        static constexpr size_t  maxTaps      = 7;     // 7 taps = 6 intervals
        static constexpr int64_t resetAfterMs = 2000;
        std::vector<int64_t> taps;
    };

    // ----------------------------------------------------------------- pomodoro
    struct PomodoroConfig
    {
        int workMin = 25, shortMin = 5, longMin = 15, everyN = 4;

        void clamp()
        {
            workMin  = std::clamp (workMin,  1, 999);
            shortMin = std::clamp (shortMin, 1, 999);
            longMin  = std::clamp (longMin,  1, 999);
            everyN   = std::clamp (everyN,   1, 99);
        }
    };

    enum class Phase    { Work, ShortBreak, LongBreak };
    enum class RunState { Idle, Running, Paused };

    class PomodoroEngine
    {
    public:
        void setConfig (PomodoroConfig c) { c.clamp(); cfg = c; }
        const PomodoroConfig& config() const { return cfg; }

        Phase    phase() const         { return currentPhase; }
        int      cycle() const         { return currentCycle; }
        RunState state() const         { return runState; }
        bool     awaitingStart() const { return waiting; }          // a phase just finished, next one is ready
        Phase    lastFinished() const  { return finishedPhase; }

        int64_t phaseDurationMs (Phase p) const
        {
            const int minutes = p == Phase::Work ? cfg.workMin : p == Phase::ShortBreak ? cfg.shortMin : cfg.longMin;
            return (int64_t) minutes * 60 * 1000;
        }

        int64_t remainingMs (int64_t now) const
        {
            switch (runState)
            {
                case RunState::Idle:    return phaseDurationMs (currentPhase);
                case RunState::Paused:  return pausedRemaining;
                case RunState::Running: return std::max<int64_t> (0, endTime - now);
            }
            return 0;
        }

        void start (int64_t now)
        {
            if (runState == RunState::Running)
                return;

            const int64_t remaining = runState == RunState::Paused ? pausedRemaining : phaseDurationMs (currentPhase);
            endTime  = now + remaining;
            runState = RunState::Running;
            waiting  = false;
        }

        void pause (int64_t now)
        {
            if (runState != RunState::Running)
                return;

            pausedRemaining = std::max<int64_t> (0, endTime - now);
            runState = RunState::Paused;
        }

        // Returns true exactly once, when a running phase reaches zero.
        bool update (int64_t now)
        {
            if (runState != RunState::Running || now < endTime)
                return false;

            finishedPhase = currentPhase;
            advance();
            runState = RunState::Idle;
            waiting  = true;
            return true;
        }

        void skip()
        {
            advance();
            runState = RunState::Idle;
            waiting  = false;
        }

        void reset()
        {
            currentPhase = Phase::Work;
            currentCycle = 1;
            runState     = RunState::Idle;
            waiting      = false;
        }

    private:
        void advance()
        {
            switch (currentPhase)
            {
                case Phase::Work:
                    currentPhase = currentCycle >= cfg.everyN ? Phase::LongBreak : Phase::ShortBreak;
                    break;
                case Phase::ShortBreak:
                    ++currentCycle;
                    currentPhase = Phase::Work;
                    break;
                case Phase::LongBreak:
                    currentCycle = 1;
                    currentPhase = Phase::Work;
                    break;
            }
        }

        PomodoroConfig cfg;
        Phase    currentPhase    = Phase::Work;
        Phase    finishedPhase   = Phase::Work;
        RunState runState        = RunState::Idle;
        int      currentCycle    = 1;
        bool     waiting         = false;
        int64_t  endTime         = 0;
        int64_t  pausedRemaining = 0;
    };

    inline std::string formatClock (int64_t ms)
    {
        const int64_t totalSec = (ms + 999) / 1000;   // round up so it never shows 00:00 while running
        std::ostringstream os;
        os.imbue (std::locale::classic());
        os << std::setfill ('0') << std::setw (2) << (totalSec / 60) << ':' << std::setw (2) << (totalSec % 60);
        return os.str();
    }
}
