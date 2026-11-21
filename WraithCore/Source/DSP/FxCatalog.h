// WRAITHCORE - FX module catalog: names, parameter labels, value mapping and display text.
// Shared by the DSP (to decode knob values) and the UI (labels / readouts). No JUCE dependency.
#pragma once
#include <string>
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace wc
{
enum FxType
{
    FX_NONE = 0,
    FX_DISTORT, FX_CLIPPER, FX_CRUSH, FX_BLOWN, FX_SUBHARM, FX_DATAMOSH,
    FX_FILTER, FX_FORMANT, FX_EQ, FX_RESONATOR,
    FX_OTT, FX_PUMP, FX_GATE,
    FX_DELAY, FX_REVERB, FX_REVERSE, FX_CHORUS, FX_PHASER,
    FX_STUTTER, FX_TAPESTOP, FX_GRAINS, FX_FREQSHIFT, FX_RINGMOD, FX_PITCH,
    FX_TAPE, FX_STEREO,
    FX_COUNT
};

struct FxInfo
{
    const char* name;
    const char* group;
    const char* blurb;
    const char* p[4];
    float def[4];
    float defMix;
};

inline const FxInfo& fxInfo (int t)
{
    static const FxInfo table[FX_COUNT] = {
        { "EMPTY",      "",          "",                                           { "", "", "", "" },                         { .5f, .5f, .5f, .5f }, 1.f },
        { "DISTORT",    "DRIVE",     "6-mode waveshaper, 2x oversampled",          { "MODE", "DRIVE", "TONE", "BIAS" },        { .08f, .4f, .8f, 0.f }, 1.f },
        { "CLIPPER",    "DRIVE",     "loudness clipper, 2x oversampled",           { "DRIVE", "KNEE", "CEILING", "OUTPUT" },   { .3f, .5f, 1.f, .667f }, 1.f },
        { "BITCRUSH",   "DRIVE",     "bit depth + jittered sample rate",           { "BITS", "RATE", "JITTER", "NOISE" },      { .5f, .75f, 0.f, 0.f }, 1.f },
        { "BLOWN SPKR", "DRIVE",     "torn speaker cone with buzzing rattle",      { "BLOW", "RATTLE", "SIZE", "RESO" },       { .5f, .3f, .5f, .3f }, 1.f },
        { "SUBHARM",    "DRIVE",     "octave-divider sub generator",               { "SUB", "OCTAVE", "TONE", "DRIVE" },       { .5f, .25f, .4f, .2f }, 1.f },
        { "DATAMOSH",   "GLITCH",    "bit-flips, stuck buffers, shuffled memory",  { "CORRUPT", "CHUNK", "HOLD", "BITS" },     { .3f, .4f, .3f, .3f }, 1.f },
        { "FILTER",     "TONE",      "LP/BP/HP/notch/comb, drive",                 { "TYPE", "CUTOFF", "RESO", "DRIVE" },      { .07f, .7f, .2f, 0.f }, 1.f },
        { "FORMANT",    "TONE",      "talking vowel filter with synced LFO",       { "VOWEL", "Q", "RATE", "DEPTH" },          { .3f, .5f, .5f, .5f }, 1.f },
        { "EQ",         "TONE",      "3-band shelf / peak EQ",                     { "LOW", "MID", "HIGH", "MID FREQ" },       { .5f, .5f, .5f, .5f }, 1.f },
        { "RESONATOR",  "TONE",      "tuned metal combs (chords)",                 { "NOTE", "DECAY", "DAMP", "CHORD" },       { .4f, .6f, .4f, .1f }, .5f },
        { "OTT",        "DYNAMICS",  "3-band upward + downward smash",             { "DEPTH", "TIME", "UPWARD", "DOWNWARD" },  { .5f, .5f, 1.f, 1.f }, 1.f },
        { "PUMP",       "DYNAMICS",  "tempo-synced sidechain duck",                { "RATE", "DEPTH", "CURVE", "HOLD" },       { .5f, .7f, .5f, .2f }, 1.f },
        { "TRANCEGATE", "DYNAMICS",  "16-step rhythmic gate",                      { "RATE", "PATTERN", "DEPTH", "SMOOTH" },   { .375f, .03f, 1.f, .3f }, 1.f },
        { "DELAY",      "SPACE",     "tempo ping-pong with tape warble",           { "TIME", "FEEDBACK", "TONE", "WARBLE" },   { .61f, .45f, .55f, .15f }, .3f },
        { "REVERB",     "SPACE",     "8-line FDN with shimmer",                    { "SIZE", "DAMP", "SHIMMER", "LOW CUT" },   { .65f, .45f, 0.f, .6f }, .3f },
        { "REVERSE",    "SPACE",     "backwards-playing delay",                    { "LENGTH", "FEEDBACK", "TONE", "SMEAR" },  { .5f, .3f, .7f, .3f }, .5f },
        { "CHORUS",     "SPACE",     "1-4 voice ensemble",                         { "RATE", "DEPTH", "VOICES", "WIDTH" },     { .35f, .4f, .4f, .8f }, .5f },
        { "PHASER",     "SPACE",     "2-12 stage phaser",                          { "RATE", "DEPTH", "FEEDBACK", "STAGES" },  { .3f, .7f, .5f, .5f }, 1.f },
        { "STUTTER",    "GLITCH",    "synced buffer repeats, reverse, warp",       { "GRID", "CHANCE", "REVERSE", "WARP" },    { .5f, .35f, .25f, .15f }, 1.f },
        { "TAPESTOP",   "GLITCH",    "tape stops, dropouts, DJ spinbacks",         { "GRID", "STOP", "GAP", "SPINBACK" },      { .375f, .3f, .2f, .1f }, 1.f },
        { "GRAINS",     "GLITCH",    "granular cloud with random freezes",         { "SIZE", "DENSITY", "SCATTER", "FREEZE" }, { .45f, .5f, .3f, 0.f }, .5f },
        { "FREQ SHIFT", "GLITCH",    "Hilbert frequency shifter (barber-pole)",    { "SHIFT", "FEEDBACK", "LFO RATE", "LFO AMT" }, { .55f, .3f, .2f, 0.f }, .5f },
        { "RING MOD",   "GLITCH",    "metallic ring modulator with drift",         { "FREQ", "DRIFT", "SHAPE", "TONE" },       { .4f, .2f, 0.f, .8f }, .5f },
        { "PITCH",      "GLITCH",    "granular pitch shifter",                     { "SEMIS", "FINE", "FEEDBACK", "WINDOW" },  { .755f, .5f, 0.f, .4f }, .5f },
        { "LO-FI TAPE", "CHARACTER", "wow, flutter, age, dropouts",                { "WOW", "FLUTTER", "AGE", "DROPOUTS" },    { .3f, .2f, .3f, 0.f }, 1.f },
        { "STEREO",     "CHARACTER", "width, haas, mono-below, pan",               { "WIDTH", "HAAS", "MONO <", "PAN" },       { .5f, 0.f, 0.f, .5f }, 1.f },
    };
    return table[std::clamp (t, 0, FX_COUNT - 1)];
}

// ---------- value mapping helpers (normalised 0..1 -> real units) ----------
inline float fxLin (float v, float a, float b) { return a + (b - a) * v; }
inline float fxLog (float v, float a, float b) { return a * std::pow (b / a, v); }
inline int   fxStep (float v, int n) { return std::clamp ((int) (v * (float) n), 0, n - 1); }

// stepped parameters: number of steps (0 = continuous) and their names
inline int fxSteps (int t, int pi)
{
    switch (t)
    {
        case FX_DISTORT:   return pi == 0 ? 6 : 0;
        case FX_SUBHARM:   return pi == 1 ? 2 : 0;
        case FX_FILTER:    return pi == 0 ? 7 : 0;
        case FX_FORMANT:   return pi == 2 ? 9 : 0;
        case FX_RESONATOR: return pi == 0 ? 61 : (pi == 3 ? 6 : 0);
        case FX_PUMP:      return pi == 0 ? 5 : 0;
        case FX_GATE:      return pi == 0 ? 4 : (pi == 1 ? 16 : 0);
        case FX_DELAY:     return pi == 0 ? 9 : 0;
        case FX_REVERSE:   return pi == 0 ? 5 : 0;
        case FX_CHORUS:    return pi == 2 ? 4 : 0;
        case FX_PHASER:    return pi == 3 ? 5 : 0;
        case FX_STUTTER:   return pi == 0 ? 5 : 0;
        case FX_TAPESTOP:  return pi == 0 ? 4 : 0;
        case FX_PITCH:     return pi == 0 ? 49 : 0;
        default:           return 0;
    }
}

inline const char* const* fxDivNames (int set)
{
    static const char* delay[]   = { "1/32", "1/16", "1/16T", "1/8", "1/8T", "1/8D", "1/4", "1/4D", "1/2" };
    static const char* grid5[]   = { "1/4", "1/8", "1/16", "1/32", "1/64" };
    static const char* pump[]    = { "1/1", "1/2", "1/4", "1/8", "1/16" };
    static const char* gate[]    = { "1/8", "1/16", "1/32", "1/16T" };
    static const char* stop[]    = { "1/2", "1/4", "1/8", "1/16" };
    static const char* rev[]     = { "1/8", "1/4", "1/2", "1 BAR", "2 BAR" };
    static const char* formant[] = { "OFF", "1/1", "1/2", "1/4", "1/8", "1/16", "1/4T", "1/8T", "1/16T" };
    switch (set) { case 0: return delay; case 1: return grid5; case 2: return pump; case 3: return gate;
                   case 4: return stop; case 5: return rev; default: return formant; }
}
// beats for those division tables
inline double fxDivBeats (int set, int i)
{
    static const double delay[]   = { 0.125, 0.25, 1.0 / 6, 0.5, 1.0 / 3, 0.75, 1.0, 1.5, 2.0 };
    static const double grid5[]   = { 1, 0.5, 0.25, 0.125, 0.0625 };
    static const double pump[]    = { 4, 2, 1, 0.5, 0.25 };
    static const double gate[]    = { 0.5, 0.25, 0.125, 1.0 / 6 };
    static const double stop[]    = { 2, 1, 0.5, 0.25 };
    static const double rev[]     = { 0.5, 1, 2, 4, 8 };
    static const double formant[] = { 0, 4, 2, 1, 0.5, 0.25, 2.0 / 3, 1.0 / 3, 1.0 / 6 };
    switch (set) { case 0: return delay[i]; case 1: return grid5[i]; case 2: return pump[i]; case 3: return gate[i];
                   case 4: return stop[i]; case 5: return rev[i]; default: return formant[i]; }
}

inline const char* gatePatternName (int i)
{
    static const char* n[] = { "STRAIGHT", "OFFBEAT", "GALLOP", "TRIPLET", "BROKEN", "STUTTER", "HALF", "SYNCO",
                               "MORSE", "BOUNCE", "ROLL", "DOTTED", "SPARSE", "DENSE", "TRAP", "CHAOS" };
    return n[std::clamp (i, 0, 15)];
}
inline unsigned gatePattern (int i)
{
    static const unsigned m[] = { 0xAAAA, 0x5555, 0xB6DB, 0x9249, 0xD5A9, 0xFAFA, 0xF0F0, 0x9A9A,
                                  0xE8E2, 0xCCCC, 0xFFF0, 0x9292, 0x8080, 0xEEEE, 0xA8AA, 0xD3B6 };
    return m[std::clamp (i, 0, 15)];
}

inline std::string fxFmt (const char* f, double v) { char b[48]; std::snprintf (b, sizeof (b), f, v); return b; }
inline std::string fxHz (float hz) { return hz >= 1000.f ? fxFmt ("%.2f kHz", hz / 1000.f) : fxFmt (hz < 10.f ? "%.2f Hz" : "%.0f Hz", hz); }
inline std::string fxPct (float v) { return fxFmt ("%.0f%%", v * 100.f); }
inline std::string fxDb (float d) { return fxFmt ("%+.1f dB", d); }
inline std::string noteName (int midi)
{
    static const char* n[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return std::string (n[((midi % 12) + 12) % 12]) + std::to_string (midi / 12 - 1);
}

// Human readable value for the UI / host
inline std::string fxText (int t, int pi, float v)
{
    switch (t)
    {
        case FX_DISTORT:
            if (pi == 0) { static const char* m[] = { "TANH", "HARD", "FOLD", "RECTIFY", "BLOWN", "TUBE" }; return m[fxStep (v, 6)]; }
            if (pi == 1) return fxFmt ("%.1f dB", v * 42.f);
            if (pi == 2) return fxHz (600.f * std::exp2 (v * 5.06f));
            return fxPct (v);
        case FX_CLIPPER:
            if (pi == 0) return fxFmt ("+%.1f dB", v * 24.f);
            if (pi == 1) return v < 0.5f ? fxFmt ("SOFT %.0f", (1.f - v * 2.f) * 100.f) : fxFmt ("HARD %.0f", (v * 2.f - 1.f) * 100.f);
            if (pi == 2) return fxFmt ("%.1f dB", fxLin (v, -12.f, 0.f));
            return fxDb (fxLin (v, -12.f, 6.f));
        case FX_CRUSH:
            if (pi == 0) return fxFmt ("%.1f bit", 1.f + v * 15.f);
            if (pi == 1) return fxHz (fxLog (v, 200.f, 48000.f));
            return fxPct (v);
        case FX_BLOWN:
            if (pi == 2) return v < 0.33f ? "PHONE" : (v < 0.66f ? "RADIO" : "CABINET");
            return fxPct (v);
        case FX_SUBHARM:
            if (pi == 1) return fxStep (v, 2) == 0 ? "-1 OCT" : "-2 OCT";
            if (pi == 2) return fxHz (fxLog (v, 40.f, 400.f));
            return fxPct (v);
        case FX_DATAMOSH:
            if (pi == 1) return fxFmt ("%.0f ms", fxLog (v, 2.f, 250.f));
            if (pi == 3) return fxFmt ("%.0f bits", 1.f + v * 11.f);
            return fxPct (v);
        case FX_FILTER:
            if (pi == 0) { static const char* m[] = { "LP 12", "LP 24", "BAND", "HIGH", "NOTCH", "COMB +", "COMB -" }; return m[fxStep (v, 7)]; }
            if (pi == 1) return fxHz (fxLog (v, 20.f, 20000.f));
            return fxPct (v);
        case FX_FORMANT:
            if (pi == 0) { static const char* m[] = { "U", "O", "A", "E", "I" }; return m[std::clamp ((int) std::lround (v * 4.f), 0, 4)]; }
            if (pi == 2) return fxDivNames (6)[fxStep (v, 9)];
            return fxPct (v);
        case FX_EQ:
            if (pi == 3) return fxHz (fxLog (v, 150.f, 6800.f));
            return fxDb (fxLin (v, -15.f, 15.f));
        case FX_RESONATOR:
            if (pi == 0) return noteName (24 + fxStep (v, 61));
            if (pi == 3) { static const char* m[] = { "UNISON", "OCTAVE", "FIFTH", "MINOR", "MAJOR", "CLUSTER" }; return m[fxStep (v, 6)]; }
            return fxPct (v);
        case FX_OTT:
            if (pi == 1) return fxFmt ("%.2fx", fxLog (v, 0.2f, 3.f));
            return fxPct (v);
        case FX_PUMP:
            if (pi == 0) return fxDivNames (2)[fxStep (v, 5)];
            return fxPct (v);
        case FX_GATE:
            if (pi == 0) return fxDivNames (3)[fxStep (v, 4)];
            if (pi == 1) return gatePatternName (fxStep (v, 16));
            return fxPct (v);
        case FX_DELAY:
            if (pi == 0) return fxDivNames (0)[fxStep (v, 9)];
            if (pi == 2) return fxHz (400.f * std::exp2 (v * 5.6f));
            return fxPct (v);
        case FX_REVERB:
            if (pi == 3) return fxHz (fxLog (v, 20.f, 600.f));
            return fxPct (v);
        case FX_REVERSE:
            if (pi == 0) return fxDivNames (5)[fxStep (v, 5)];
            if (pi == 2) return fxHz (500.f * std::exp2 (v * 5.3f));
            return fxPct (v);
        case FX_CHORUS:
            if (pi == 0) return fxHz (fxLog (v, 0.05f, 8.f));
            if (pi == 2) return std::to_string (1 + fxStep (v, 4)) + (fxStep (v, 4) == 0 ? " VOICE" : " VOICES");
            return fxPct (v);
        case FX_PHASER:
            if (pi == 0) return fxHz (fxLog (v, 0.02f, 10.f));
            if (pi == 2) return fxFmt ("%+.0f%%", (v * 2.f - 1.f) * 95.f);
            if (pi == 3) { static const int s[] = { 2, 4, 6, 8, 12 }; return std::to_string (s[fxStep (v, 5)]) + " STG"; }
            return fxPct (v);
        case FX_STUTTER:
            if (pi == 0) return fxDivNames (1)[fxStep (v, 5)];
            return fxPct (v);
        case FX_TAPESTOP:
            if (pi == 0) return fxDivNames (4)[fxStep (v, 4)];
            return fxPct (v);
        case FX_GRAINS:
            if (pi == 0) return fxFmt ("%.0f ms", fxLog (v, 10.f, 400.f));
            if (pi == 1) return fxFmt ("%.1f /s", fxLog (v, 1.f, 60.f));
            return fxPct (v);
        case FX_FREQSHIFT:
            if (pi == 0) { const float s = v * 2.f - 1.f; return fxFmt ("%+.1f Hz", (s < 0 ? -1.f : 1.f) * s * s * 1000.f); }
            if (pi == 2) return fxHz (fxLin (v, 0.f, 8.f));
            if (pi == 3) return fxFmt ("+/-%.0f Hz", v * 500.f);
            return fxPct (v);
        case FX_RINGMOD:
            if (pi == 0) return fxHz (fxLog (v, 20.f, 5000.f));
            if (pi == 3) return fxHz (fxLog (v, 300.f, 20000.f));
            return fxPct (v);
        case FX_PITCH:
            if (pi == 0) return fxFmt ("%+.0f st", (double) (fxStep (v, 49) - 24));
            if (pi == 1) return fxFmt ("%+.0f ct", (v * 2.f - 1.f) * 50.f);
            if (pi == 3) return fxFmt ("%.0f ms", fxLin (v, 20.f, 120.f));
            return fxPct (v);
        case FX_STEREO:
            if (pi == 0) return fxFmt ("%.0f%%", v * 200.f);
            if (pi == 1) return fxFmt ("%.1f ms", v * 25.f);
            if (pi == 2) return v < 0.01f ? "OFF" : fxHz (fxLog (v, 20.f, 500.f));
            if (pi == 3) { const float p = v * 2.f - 1.f; return std::abs (p) < 0.01f ? "C" : fxFmt (p < 0 ? "L%.0f" : "R%.0f", std::abs (p) * 100.f); }
            return fxPct (v);
        default:
            return fxPct (v);
    }
}
} // namespace wc
