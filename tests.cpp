// Mix Map - core unit tests. Plain C++, no framework.
// Build: c++ -std=c++17 Tests/tests.cpp -I Source/Core -o mixmap_tests && ./mixmap_tests
#include "MixMapCore.h"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace mixmap;

static int failures = 0;
static int checks   = 0;

#define CHECK_NEAR(actual, expected, tol)                                                          \
    do {                                                                                           \
        ++checks;                                                                                  \
        const double a_ = (actual), e_ = (expected);                                               \
        if (! (std::abs (a_ - e_) <= (tol)))                                                       \
        { ++failures; std::printf ("FAIL line %d: %s = %.6f, expected %.6f\n", __LINE__, #actual, a_, e_); } \
    } while (0)

#define CHECK_TRUE(cond)                                                                           \
    do { ++checks; if (! (cond)) { ++failures; std::printf ("FAIL line %d: %s\n", __LINE__, #cond); } } while (0)

#define CHECK_STR(actual, expected)                                                                \
    do {                                                                                           \
        ++checks;                                                                                  \
        const std::string a_ = (actual), e_ = (expected);                                          \
        if (a_ != e_) { ++failures; std::printf ("FAIL line %d: %s = '%s', expected '%s'\n", __LINE__, #actual, a_.c_str(), e_.c_str()); } \
    } while (0)

static std::string S (const char* a, const char* b = "") { return std::string (a) + b; }

int main()
{
    // ---- module 1: frequencies (A4 = 440, equal temperament)
    CHECK_NEAR (midiToHz (69, 440.0), 440.0, 1e-9);
    CHECK_NEAR (midiToHz (rootMidi (0, 0), 440.0),  16.351598, 1e-5);     // C0
    CHECK_NEAR (midiToHz (rootMidi (0, 4), 440.0), 261.625565, 1e-5);     // C4 sci = C3 Live
    CHECK_NEAR (midiToHz (rootMidi (0, 9), 440.0), 8372.018090, 1e-4);    // C9
    CHECK_NEAR (midiToHz (rootMidi (0, 10), 440.0), 16744.036179, 1e-3);  // C10
    CHECK_NEAR (midiToHz (rootMidi (11, 9), 440.0), 15804.265640, 1e-3);  // B9
    CHECK_NEAR (midiToHz (69, 432.0), 432.0, 1e-9);                       // custom A4

    // ---- module 1: cells, audibility, naming
    {
        auto c = keyCell (0, 10, 1, 440.0, false, true);       // fifth above C10 = G10
        CHECK_NEAR (c.hz, 25087.707903, 1e-3);
        CHECK_TRUE (! c.audible);
        auto r = keyCell (0, 10, 0, 440.0, false, true);       // C10 itself
        CHECK_TRUE (r.audible);
        auto b = keyCell (11, 4, 1, 440.0, false, false);      // B4 -> fifth F#5 (scientific)
        CHECK_STR (b.noteName, S (pitchClassName (6, false).c_str(), "5"));
        CHECK_NEAR (b.hz, 740.0 * 1.0, 1.0);                   // F#5 ~ 739.99
        auto m7 = keyCell (11, 4, 2, 440.0, false, false);     // B4 -> minor 7th A5
        CHECK_STR (m7.noteName, "A5");
        CHECK_NEAR (m7.hz, 880.0, 1e-9);
        auto live = keyCell (0, 4, 0, 440.0, false, true);     // Live convention: middle C = C3
        CHECK_STR (live.noteName, "C3");
        auto sci = keyCell (0, 4, 0, 440.0, false, false);
        CHECK_STR (sci.noteName, "C4");
        auto tri = keyCell (0, 4, 3, 440.0, true, false);      // C4 -> augmented 4th, flats -> Gb4
        CHECK_STR (tri.noteName, std::string ("G") + flatSymbol() + "4");
    }

    // ---- note names: never E#, B#, Cb, Fb
    for (int pc = 0; pc < 12; ++pc)
    {
        for (bool flats : { false, true })
        {
            const std::string n = pitchClassName (pc, flats);
            CHECK_TRUE (n != std::string ("E") + sharpSymbol());
            CHECK_TRUE (n != std::string ("B") + sharpSymbol());
            CHECK_TRUE (n != std::string ("C") + flatSymbol());
            CHECK_TRUE (n != std::string ("F") + flatSymbol());
        }
    }
    CHECK_STR (pitchClassName (1, false), std::string ("C") + sharpSymbol());
    CHECK_STR (pitchClassName (1, true),  std::string ("D") + flatSymbol());
    CHECK_STR (pitchClassName (10, true), std::string ("B") + flatSymbol());

    // ---- modules 2 and 3: tempo maths
    CHECK_NEAR (noteMs (4, 120.0, Modifier::Straight), 500.0, 1e-9);        // quarter @120
    CHECK_NEAR (noteMs (4, 120.0, Modifier::Dotted),   750.0, 1e-9);
    CHECK_NEAR (noteMs (4, 120.0, Modifier::Triplet),  333.333333, 1e-5);
    CHECK_NEAR (noteMs (1, 120.0, Modifier::Straight), 2000.0, 1e-9);       // whole
    CHECK_NEAR (noteMs (2, 120.0, Modifier::Straight), 1000.0, 1e-9);       // half
    CHECK_NEAR (noteMs (8, 120.0, Modifier::Straight), 250.0, 1e-9);
    CHECK_NEAR (noteMs (16, 120.0, Modifier::Straight), 125.0, 1e-9);
    CHECK_NEAR (noteMs (32, 120.0, Modifier::Straight), 62.5, 1e-9);
    CHECK_NEAR (noteMs (64, 120.0, Modifier::Straight), 31.25, 1e-9);
    CHECK_NEAR (msToHz (500.0), 2.0, 1e-12);
    CHECK_NEAR (msToHz (noteMs (64, 999.0, Modifier::Triplet)), 399.6, 1e-9);   // max rate (999 BPM)
    CHECK_NEAR (noteMs (64, 999.0, Modifier::Triplet), 2.502503, 1e-6);
    CHECK_NEAR (msToHz (noteMs (64, 1000.0, Modifier::Triplet)), 400.0, 1e-9);
    CHECK_NEAR (msToHz (noteMs (1, 20.0, Modifier::Straight)), 1.0 / 12.0, 1e-12);  // 0.083 Hz
    CHECK_NEAR (noteMs (1, 20.0, Modifier::Dotted), 18000.0, 1e-9);
    CHECK_TRUE (noteDenominator (0) == 1 && noteDenominator (6) == 64);
    CHECK_NEAR (clampBpm (5.0), 20.0, 0.0);
    CHECK_NEAR (clampBpm (5000.0), 999.0, 0.0);

    // ---- formatting
    CHECK_STR (formatHzKey (16.351598), "16.35");
    CHECK_STR (formatHzKey (999.994), "999.99");
    CHECK_STR (formatHzKey (1000.04), "1,000.0");
    CHECK_STR (formatHzKey (16744.036), "16,744.0");
    CHECK_STR (formatHzKey (8372.018), "8,372.0");
    CHECK_STR (formatMs (333.333333), "333.33");
    CHECK_STR (formatMs (18000.0), "18000.00");
    CHECK_STR (formatHzRate (1.0 / 12.0), "0.083");
    CHECK_STR (formatHzRate (400.0), "400.000");
    CHECK_STR (formatTrimmed (120.0), "120");
    CHECK_STR (formatTrimmed (128.5), "128.5");
    CHECK_STR (formatTrimmed (99.99), "99.99");
    CHECK_STR (formatTrimmed (440.0), "440");
    CHECK_NEAR (parseNumber ("128,5", 0.0), 128.5, 1e-12);
    CHECK_NEAR (parseNumber ("128.5", 0.0), 128.5, 1e-12);
    CHECK_NEAR (parseNumber ("abc", 77.0), 77.0, 0.0);
    CHECK_STR (formatClock (25 * 60 * 1000), "25:00");
    CHECK_STR (formatClock (1), "00:01");
    CHECK_STR (formatClock (0), "00:00");
    CHECK_STR (formatClock (61 * 1000), "01:01");

    // ---- tap tempo
    {
        TapTempo t;
        CHECK_TRUE (t.tap (1000) <= 0.0);
        CHECK_NEAR (t.tap (1500), 120.0, 1e-9);     // 500 ms interval
        CHECK_NEAR (t.tap (2000), 120.0, 1e-9);
        CHECK_TRUE (t.tap (10000) <= 0.0);          // > 2 s gap resets, one tap again
        CHECK_NEAR (t.tap (10600), 100.0, 1e-9);    // 600 ms
        // more than 7 taps: only the last 6 intervals count
        TapTempo u;
        int64_t time = 0;
        double bpm = 0;
        for (int i = 0; i < 12; ++i) { bpm = u.tap (time); time += (i < 5 ? 1000 : 500); }
        CHECK_NEAR (bpm, 120.0, 1e-9);
    }

    // ---- pomodoro engine
    {
        PomodoroEngine p;
        PomodoroConfig cfg;           // 25 / 5 / 15 / 4
        p.setConfig (cfg);
        const int64_t W = 25LL * 60000, SB = 5LL * 60000, LB = 15LL * 60000;
        int64_t t = 1000;

        CHECK_TRUE (p.phase() == Phase::Work && p.cycle() == 1 && p.state() == RunState::Idle);
        CHECK_TRUE (p.remainingMs (t) == W);

        for (int c = 1; c <= 4; ++c)
        {
            CHECK_TRUE (p.phase() == Phase::Work);
            CHECK_TRUE (p.cycle() == c);
            p.start (t);
            CHECK_TRUE (p.state() == RunState::Running);
            CHECK_TRUE (! p.update (t + W - 1));                 // not yet
            CHECK_TRUE (p.update (t + W));                       // finishes exactly once
            CHECK_TRUE (! p.update (t + W + 5000));              // does not fire again
            t += W;
            CHECK_TRUE (p.state() == RunState::Idle && p.awaitingStart());
            CHECK_TRUE (p.lastFinished() == Phase::Work);
            if (c < 4)
            {
                CHECK_TRUE (p.phase() == Phase::ShortBreak);
                p.start (t + 10000);                              // manual: waits for the user
                CHECK_TRUE (p.update (t + 10000 + SB));
                t += 10000 + SB;
            }
            else
            {
                CHECK_TRUE (p.phase() == Phase::LongBreak);
                CHECK_TRUE (p.remainingMs (t) == LB);
            }
        }
        p.start (t);
        CHECK_TRUE (p.update (t + LB));
        CHECK_TRUE (p.phase() == Phase::Work && p.cycle() == 1);   // cycle counter restarts after the long break

        // pause / resume keeps remaining time
        PomodoroEngine q;
        q.setConfig (cfg);
        q.start (0);
        q.pause (10 * 60000);
        CHECK_TRUE (q.state() == RunState::Paused);
        CHECK_TRUE (q.remainingMs (999999999) == 15LL * 60000);
        q.start (50 * 60000);
        CHECK_TRUE (q.remainingMs (50 * 60000) == 15LL * 60000);
        CHECK_TRUE (! q.update (64 * 60000));
        CHECK_TRUE (q.update (65 * 60000));

        // skip: next phase, stopped, no alarm flag
        PomodoroEngine s;
        s.setConfig (cfg);
        s.start (0);
        s.skip();
        CHECK_TRUE (s.phase() == Phase::ShortBreak && s.state() == RunState::Idle && ! s.awaitingStart());

        // reset
        s.reset();
        CHECK_TRUE (s.phase() == Phase::Work && s.cycle() == 1 && s.state() == RunState::Idle);

        // everyN = 1: every work session is followed by a long break
        PomodoroEngine e;
        PomodoroConfig c1; c1.everyN = 1;
        e.setConfig (c1);
        e.start (0);
        CHECK_TRUE (e.update (W));
        CHECK_TRUE (e.phase() == Phase::LongBreak);

        // config clamping
        PomodoroConfig bad; bad.workMin = 0; bad.shortMin = -3; bad.longMin = 5000; bad.everyN = 0;
        bad.clamp();
        CHECK_TRUE (bad.workMin == 1 && bad.shortMin == 1 && bad.longMin == 999 && bad.everyN == 1);

        // changing duration while idle updates the displayed time
        PomodoroEngine d;
        d.setConfig (cfg);
        PomodoroConfig c50 = cfg; c50.workMin = 50;
        d.setConfig (c50);
        CHECK_TRUE (d.remainingMs (0) == 50LL * 60000);
    }

    std::printf ("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
