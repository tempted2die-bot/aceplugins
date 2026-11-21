// SonarWraith - parameter table (no JUCE dependency; shared by plugin + test harness)
#pragma once
#include <vector>

namespace sw
{
enum class PType { Float, Int, Choice, Bool };

// X(id, display name, type, min, max, default, skewCentre(-1 = linear), choices, unit)
#define SW_PARAMS(X) \
    /* ---------- OSCILLATOR ---------- */ \
    X(oscWave,   "Wave",        Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(oscFold,   "Fold",        Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(oscOct,    "Octave",      Int,  -3.f,   2.f,     0.f,    -1.f,  "", "oct") \
    X(oscFine,   "Fine",        Float,-100.f, 100.f,   0.f,    -1.f,  "", "ct") \
    X(uniVoices, "Unison",      Int,   1.f,   4.f,     1.f,    -1.f,  "", "") \
    X(uniDetune, "Detune",      Float, 0.f,   1.f,     0.2f,   -1.f,  "", "") \
    X(subLvl,    "Sub",         Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(noiseLvl,  "Noise",       Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    /* ---------- FM + PING ---------- */ \
    X(fmRatio,   "FM Ratio",    Float, 0.25f, 16.f,    2.f,     2.f,  "", "x") \
    X(fmAmt,     "FM Amt",      Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(pingAmt,   "Ping",        Float,-48.f,  48.f,    0.f,    -1.f,  "", "st") \
    X(pingDecay, "Ping Time",   Float, 1.f,   2000.f,  60.f,  100.f,  "", "ms") \
    /* ---------- BUBBLER ---------- */ \
    X(bubRate,   "Rate",        Float, 0.f,   32.f,    0.f,     6.f,  "", "Hz") \
    X(bubChance, "Chance",      Float, 0.f,   1.f,     0.7f,   -1.f,  "", "") \
    X(bubRand,   "Scatter",     Float, 0.f,   24.f,    0.f,    -1.f,  "", "st") \
    X(bubGate,   "Gate",        Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    /* ---------- FILTER ---------- */ \
    X(fltType,   "Type",        Choice,0.f,   3.f,     0.f,    -1.f,  "LP|BP|HP|NOTCH", "") \
    X(fltCut,    "Cutoff",      Float, 20.f,  20000.f, 8000.f, 1000.f,"", "Hz") \
    X(fltRes,    "Reso",        Float, 0.f,   1.f,     0.2f,   -1.f,  "", "") \
    X(fltEnv,    "Env Amt",     Float,-1.f,   1.f,     0.f,    -1.f,  "", "") \
    X(fltDecay,  "Env Decay",   Float, 1.f,   4000.f,  250.f, 200.f,  "", "ms") \
    /* ---------- AMP ---------- */ \
    X(atk,       "Attack",      Float, 0.f,   5000.f,  1.f,   100.f,  "", "ms") \
    X(dec,       "Decay",       Float, 1.f,   8000.f,  400.f, 300.f,  "", "ms") \
    X(sus,       "Sustain",     Float, 0.f,   1.f,     0.8f,   -1.f,  "", "") \
    X(rel,       "Release",     Float, 1.f,   10000.f, 250.f, 500.f,  "", "ms") \
    /* ---------- LFO ---------- */ \
    X(lfoShape,  "Shape",       Choice,0.f,   5.f,     0.f,    -1.f,  "SINE|TRI|SAW|SQUARE|S&H|DRIFT", "") \
    X(lfoRate,   "Rate",        Float, 0.02f, 40.f,    2.f,     3.f,  "", "Hz") \
    X(lfoSync,   "Sync",        Bool,  0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(lfoDiv,    "Division",    Choice,0.f,   9.f,     3.f,    -1.f,  "1/1|1/2|1/4|1/8|1/16|1/32|1/4T|1/8T|1/16T|1/8D", "") \
    X(lfoPitch,  "> Pitch",     Float, 0.f,   24.f,    0.f,     3.f,  "", "st") \
    X(lfoCut,    "> Cutoff",    Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    /* ---------- VOICE ---------- */ \
    X(polyMode,  "Mode",        Choice,0.f,   2.f,     0.f,    -1.f,  "POLY|MONO|LEGATO", "") \
    X(glide,     "Glide",       Float, 0.f,   2000.f,  0.f,   150.f,  "", "ms") \
    X(voiceVol,  "Volume",      Float, 0.f,   1.f,     0.6f,   -1.f,  "", "") \
    /* ---------- FX: DRIVE ---------- */ \
    X(drvOn,     "Drive On",    Bool,  0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(drvMode,   "Mode",        Choice,0.f,   4.f,     0.f,    -1.f,  "TANH|HARD|FOLD|RECTIFY|BLOWN", "") \
    X(drvAmt,    "Drive",       Float, 0.f,   1.f,     0.35f,  -1.f,  "", "") \
    X(drvTone,   "Tone",        Float, 0.f,   1.f,     0.8f,   -1.f,  "", "") \
    X(drvMix,    "Mix",         Float, 0.f,   1.f,     1.f,    -1.f,  "", "") \
    /* ---------- FX: CRUSH ---------- */ \
    X(crsOn,     "Crush On",    Bool,  0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(crsBits,   "Bits",        Float, 1.f,   16.f,    16.f,   -1.f,  "", "bit") \
    X(crsRate,   "Rate",        Float, 200.f, 48000.f, 48000.f,6000.f,"", "Hz") \
    X(crsMix,    "Mix",         Float, 0.f,   1.f,     1.f,    -1.f,  "", "") \
    /* ---------- FX: GLITCH ---------- */ \
    X(glOn,      "Glitch On",   Bool,  0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(glDiv,     "Grid",        Choice,0.f,   4.f,     2.f,    -1.f,  "1/4|1/8|1/16|1/32|1/64", "") \
    X(glChance,  "Chance",      Float, 0.f,   1.f,     0.35f,  -1.f,  "", "") \
    X(glRev,     "Reverse",     Float, 0.f,   1.f,     0.25f,  -1.f,  "", "") \
    X(glPitch,   "Warp",        Float, 0.f,   1.f,     0.15f,  -1.f,  "", "") \
    X(glMix,     "Mix",         Float, 0.f,   1.f,     1.f,    -1.f,  "", "") \
    /* ---------- FX: SONAR DELAY ---------- */ \
    X(dlyOn,     "Delay On",    Bool,  0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(dlyDiv,    "Time",        Choice,0.f,   8.f,     5.f,    -1.f,  "1/32|1/16|1/16T|1/8|1/8T|1/8D|1/4|1/4D|1/2", "") \
    X(dlyPP,     "Ping-Pong",   Bool,  0.f,   1.f,     1.f,    -1.f,  "", "") \
    X(dlyFb,     "Feedback",    Float, 0.f,   1.f,     0.45f,  -1.f,  "", "") \
    X(dlyTone,   "Tone",        Float, 0.f,   1.f,     0.55f,  -1.f,  "", "") \
    X(dlyWarble, "Warble",      Float, 0.f,   1.f,     0.15f,  -1.f,  "", "") \
    X(dlyMix,    "Mix",         Float, 0.f,   1.f,     0.3f,   -1.f,  "", "") \
    /* ---------- FX: ATMOS REVERB ---------- */ \
    X(revOn,     "Atmos On",    Bool,  0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(revSize,   "Size",        Float, 0.f,   1.f,     0.65f,  -1.f,  "", "") \
    X(revDamp,   "Damp",        Float, 0.f,   1.f,     0.45f,  -1.f,  "", "") \
    X(revShimmer,"Shimmer",     Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(revMix,    "Mix",         Float, 0.f,   1.f,     0.3f,   -1.f,  "", "") \
    /* ---------- MASTER ---------- */ \
    X(width,     "Width",       Float, 0.f,   2.f,     1.f,    -1.f,  "", "") \
    X(outGain,   "Output",      Float,-24.f,  12.f,    0.f,    -1.f,  "", "dB")

namespace pid
{
    enum : int
    {
#define SW_X_ENUM(id, name, type, mn, mx, df, cn, ch, un) id,
        SW_PARAMS(SW_X_ENUM)
#undef SW_X_ENUM
        COUNT
    };
}

struct ParamDef
{
    const char* id;
    const char* name;
    PType type;
    float min, max, def, centre;
    const char* choices;
    const char* unit;
};

inline const std::vector<ParamDef>& paramDefs()
{
    static const std::vector<ParamDef> defs = {
#define SW_X_DEF(id, name, type, mn, mx, df, cn, ch, un) { #id, name, PType::type, mn, mx, df, cn, ch, un },
        SW_PARAMS(SW_X_DEF)
#undef SW_X_DEF
    };
    return defs;
}

struct Params
{
    float v[pid::COUNT];
    Params() { resetToDefaults(); }
    void resetToDefaults()
    {
        const auto& d = paramDefs();
        for (int i = 0; i < pid::COUNT; ++i) v[i] = d[(size_t) i].def;
    }
    float f(int i) const { return v[i]; }
    int   n(int i) const { return (int) (v[i] + 0.5f); }   // ints / choices (non-negative)
    int   sn(int i) const { return (int) (v[i] < 0 ? v[i] - 0.5f : v[i] + 0.5f); } // signed int
    bool  b(int i) const { return v[i] > 0.5f; }
};
} // namespace sw
