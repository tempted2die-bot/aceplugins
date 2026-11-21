// WRAITHCORE - parameter table (single source of truth, no JUCE dependency)
#pragma once
#include "FxCatalog.h"
#include <vector>
#include <string>
#include <cmath>

namespace wc
{
enum class PType { Float, Int, Choice, Bool };

// X(id, display name, type, min, max, default, skewCentre(-1 = linear), choices, unit)
#define WC_PARAMS(X) \
    /* ---------- OSC A ---------- */ \
    X(oscWave,   "Wave",        Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(oscFold,   "Fold",        Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(oscOct,    "Octave",      Int,  -3.f,   2.f,     0.f,    -1.f,  "", "oct") \
    X(oscFine,   "Fine",        Float,-100.f, 100.f,   0.f,    -1.f,  "", "ct") \
    X(oscALvl,   "Level",       Float, 0.f,   1.f,     1.f,    -1.f,  "", "") \
    X(uniVoices, "Unison",      Int,   1.f,   4.f,     1.f,    -1.f,  "", "") \
    X(uniDetune, "Detune",      Float, 0.f,   1.f,     0.2f,   -1.f,  "", "") \
    X(subLvl,    "Sub",         Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(noiseLvl,  "Noise",       Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    /* ---------- OSC B ---------- */ \
    X(obLvl,     "Level",       Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(obWave,    "Wave",        Float, 0.f,   1.f,     0.66f,  -1.f,  "", "") \
    X(obOct,     "Octave",      Int,  -3.f,   2.f,     0.f,    -1.f,  "", "oct") \
    X(obSemi,    "Semi",        Int, -12.f,  12.f,     0.f,    -1.f,  "", "st") \
    X(obFine,    "Fine",        Float,-100.f, 100.f,   7.f,    -1.f,  "", "ct") \
    X(obMode,    "Mode",        Choice,0.f,   2.f,     0.f,    -1.f,  "ADD|RING|SYNC", "") \
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
    X(fltType,   "Type",        Choice,0.f,   6.f,     0.f,    -1.f,  "LP12|LP24|BAND|HIGH|NOTCH|COMB|ACID", "") \
    X(fltCut,    "Cutoff",      Float, 20.f,  20000.f, 8000.f, 1000.f,"", "Hz") \
    X(fltRes,    "Reso",        Float, 0.f,   1.f,     0.2f,   -1.f,  "", "") \
    X(fltEnv,    "Env Amt",     Float,-1.f,   1.f,     0.f,    -1.f,  "", "") \
    X(fltDecay,  "Env Decay",   Float, 1.f,   4000.f,  250.f, 200.f,  "", "ms") \
    X(fltDrive,  "Drive",       Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(fltKey,    "Key Track",   Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    /* ---------- AMP ---------- */ \
    X(atk,       "Attack",      Float, 0.f,   5000.f,  1.f,   100.f,  "", "ms") \
    X(dec,       "Decay",       Float, 1.f,   8000.f,  400.f, 300.f,  "", "ms") \
    X(sus,       "Sustain",     Float, 0.f,   1.f,     0.8f,   -1.f,  "", "") \
    X(rel,       "Release",     Float, 1.f,   10000.f, 250.f, 500.f,  "", "ms") \
    /* ---------- VOICE LFO ---------- */ \
    X(lfoShape,  "Shape",       Choice,0.f,   5.f,     0.f,    -1.f,  "SINE|TRI|SAW|SQUARE|S&H|DRIFT", "") \
    X(lfoRate,   "Rate",        Float, 0.02f, 40.f,    2.f,     3.f,  "", "Hz") \
    X(lfoSync,   "Sync",        Bool,  0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(lfoDiv,    "Division",    Choice,0.f,   9.f,     3.f,    -1.f,  "1/1|1/2|1/4|1/8|1/16|1/32|1/4T|1/8T|1/16T|1/8D", "") \
    X(lfoPitch,  "> Pitch",     Float, 0.f,   24.f,    0.f,     3.f,  "", "st") \
    X(lfoCut,    "> Cutoff",    Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    /* ---------- VOICE ---------- */ \
    X(polyMode,  "Mode",        Choice,0.f,   2.f,     0.f,    -1.f,  "POLY|MONO|LEGATO", "") \
    X(glide,     "Glide",       Float, 0.f,   2000.f,  0.f,   150.f,  "", "ms") \
    X(voiceVol,  "Volume",      Float, 0.f,   1.f,     0.8f,   -1.f,  "", "") \
    /* ---------- MASTER ---------- */ \
    X(width,     "Width",       Float, 0.f,   2.f,     1.f,    -1.f,  "", "") \
    X(outGain,   "Output",      Float,-24.f,  12.f,    0.f,    -1.f,  "", "dB") \
    X(bendRange, "Bend Range",  Int,   1.f,   24.f,    2.f,    -1.f,  "", "st") \
    X(limiter,   "Safety Clip", Bool,  0.f,   1.f,     1.f,    -1.f,  "", "") \
    /* ---------- LAYERS ---------- */ \
    X(q8Lvl,     "808 Level",   Float, 0.f,   1.f,     0.65f,  -1.f,  "", "") \
    X(synthLvl,  "Synth Level", Float, 0.f,   1.f,     0.8f,   -1.f,  "", "") \
    X(splitNote, "Split",       Float, 0.f,   96.f,    48.f,   -1.f,  "", "note") \
    X(subMono,   "Sub Mono",    Bool,  0.f,   1.f,     0.f,    -1.f,  "", "") \
    /* ---------- 808 QUAKE ---------- */ \
    X(q8On,      "808 On",      Bool,  0.f,   1.f,     1.f,    -1.f,  "", "") \
    X(q8Oct,     "Octave",      Int,  -3.f,   1.f,    -1.f,    -1.f,  "", "oct") \
    X(q8Shape,   "Shape",       Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(q8Punch,   "Punch",       Float, 0.f,   48.f,    17.f,   -1.f,  "", "st") \
    X(q8PunchTime,"Punch Time", Float, 5.f,   800.f,   90.f,  100.f,  "", "ms") \
    X(q8Click,   "Click",       Float, 0.f,   1.f,     0.15f,  -1.f,  "", "") \
    X(q8Phase,   "Spread",      Float, 0.f,   180.f,   0.f,    -1.f,  "", "deg") \
    X(q8DrvMode, "Dirt Mode",   Choice,0.f,   4.f,     0.f,    -1.f,  "CLIP|TUBE|FOLD|FUZZ|SHRED", "") \
    X(q8Drive,   "Dirt",        Float, 0.f,   1.f,     0.45f,  -1.f,  "", "") \
    X(q8Fold,    "Fold",        Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(q8Asym,    "Asym",        Float, 0.f,   1.f,     0.2f,   -1.f,  "", "") \
    X(q8Tone,    "Tone",        Float, 0.f,   1.f,     0.75f,  -1.f,  "", "") \
    X(q8SubKeep, "Clean Sub",   Float, 0.f,   1.f,     0.35f,  -1.f,  "", "") \
    X(q8Knock,   "Knock",       Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(q8Cut,     "Cutoff",      Float, 20.f,  20000.f, 12000.f,1000.f,"", "Hz") \
    X(q8Res,     "Reso",        Float, 0.f,   1.f,     0.1f,   -1.f,  "", "") \
    X(q8Fm,      "FM Growl",    Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(q8FmRatio, "FM Ratio",    Float, 0.5f,  8.f,     1.f,     2.f,  "", "x") \
    X(q8Vowel,   "Vowel",       Float, 0.f,   1.f,     0.3f,   -1.f,  "", "") \
    X(q8GrowlRes,"Formant Q",   Float, 0.f,   1.f,     0.5f,   -1.f,  "", "") \
    X(q8GrowlAmt,"Growl",       Float, 0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(q8Decay,   "Decay",       Float, 10.f,  20000.f, 4000.f,1500.f, "", "ms") \
    X(q8Sus,     "Sustain",     Float, 0.f,   1.f,     1.f,    -1.f,  "", "") \
    X(q8Rel,     "Release",     Float, 5.f,   5000.f,  120.f, 300.f,  "", "ms") \
    X(q8Mode,    "Mode",        Choice,0.f,   2.f,     2.f,    -1.f,  "POLY|MONO|LEGATO", "") \
    X(q8Glide,   "Slide",       Float, 0.f,   1000.f,  60.f,  120.f,  "", "ms") \
    X(q8WobOn,   "Wobble On",   Bool,  0.f,   1.f,     0.f,    -1.f,  "", "") \
    X(q8WobDiv,  "Rate",        Choice,0.f,   11.f,    4.f,    -1.f,  "1/1|1/2|1/4|1/4T|1/8|1/8T|1/16|1/16T|1/32|1/2D|1/4D|1/8D", "") \
    X(q8WobShape,"Shape",       Choice,0.f,   6.f,     0.f,    -1.f,  "SINE|TRI|SAW DN|SAW UP|SQUARE|YOI|RANDOM", "") \
    X(q8WobTarget,"Target",     Choice,0.f,   5.f,     0.f,    -1.f,  "FILTER|GROWL|FM|FOLD|PITCH|ALL", "") \
    X(q8WobDepth,"Depth",       Float, 0.f,   1.f,     0.7f,   -1.f,  "", "") \
    X(q8WobRetrig,"Retrig",     Bool,  0.f,   1.f,     1.f,    -1.f,  "", "")

namespace pid
{
    enum : int
    {
#define WC_X_ENUM(id, name, type, mn, mx, df, cn, ch, un) id,
        WC_PARAMS(WC_X_ENUM)
#undef WC_X_ENUM
        BASE_COUNT
    };
}

// ---------------- generated parameter blocks ----------------
constexpr int kSlots = 10;            // lane A = slots 0-4, lane B = slots 5-9
constexpr int kSlotsPerLane = 5;
enum SlotField { SF_TYPE, SF_ON, SF_MIX, SF_P0, SF_P1, SF_P2, SF_P3, SF_COUNT };
constexpr int SLOT_BASE = pid::BASE_COUNT;
constexpr int slotId (int s, int f) { return SLOT_BASE + s * SF_COUNT + f; }

constexpr int RACK_BASE = SLOT_BASE + kSlots * SF_COUNT;
enum RackField { R_ROUTING, R_XOVER, R_LANEA_LVL, R_LANEB_LVL, R_LANEA_ON, R_LANEB_ON, R_MIX, R_COUNT };
constexpr int rackId (int f) { return RACK_BASE + f; }

constexpr int MOD_BASE = RACK_BASE + R_COUNT;
enum ModField { M_MACRO1, M_MACRO2, M_MACRO3, M_MACRO4,
                M_LFOA_RATE, M_LFOA_SYNC, M_LFOA_DIV, M_LFOA_SHAPE,
                M_LFOB_RATE, M_LFOB_SYNC, M_LFOB_DIV, M_LFOB_SHAPE,
                M_ENV_ATK, M_ENV_REL, M_ENV_GAIN, M_COUNT };
constexpr int modId (int f) { return MOD_BASE + f; }

constexpr int MTX_BASE = MOD_BASE + M_COUNT;
constexpr int kMtxRows = 8;
enum MtxField { MX_SRC, MX_DST, MX_AMT, MX_COUNT };
constexpr int mtxId (int row, int f) { return MTX_BASE + row * MX_COUNT + f; }

constexpr int NUM_PARAMS = MTX_BASE + kMtxRows * MX_COUNT;

// modulation sources
enum ModSource { MS_NONE, MS_MACRO1, MS_MACRO2, MS_MACRO3, MS_MACRO4, MS_LFOA, MS_LFOB, MS_ENV, MS_MODWHEEL, MS_VELOCITY, MS_RANDOM, MS_COUNT };
inline const char* modSourceName (int s)
{
    static const char* n[] = { "---", "MACRO 1 BRIGHT", "MACRO 2 DIRT", "MACRO 3 SPACE", "MACRO 4 CHAOS", "LFO A", "LFO B", "ENV FOLLOW", "MOD WHEEL", "VELOCITY", "NOTE RANDOM" };
    return n[std::clamp (s, 0, (int) MS_COUNT - 1)];
}
inline const char* macroName (int m) { static const char* n[] = { "BRIGHT", "DIRT", "SPACE", "CHAOS" }; return n[std::clamp (m, 0, 3)]; }

// modulation targets: 0 = none, 1..50 = FX slot params (P1-P4, MIX per slot), then fixed params
constexpr int kSlotTargets = kSlots * 5;
inline const std::vector<std::pair<int, const char*>>& fixedTargets()
{
    static const std::vector<std::pair<int, const char*>> t = {
        { pid::fltCut, "SYN CUTOFF" }, { pid::fltRes, "SYN RESO" }, { pid::oscWave, "SYN WAVE" }, { pid::oscFold, "SYN FOLD" },
        { pid::fmAmt, "SYN FM" }, { pid::obLvl, "OSC B LEVEL" }, { pid::obWave, "OSC B WAVE" }, { pid::pingAmt, "SYN PING" },
        { pid::bubRate, "BUBBLE RATE" }, { pid::lfoPitch, "SYN VIBRATO" }, { pid::fltDrive, "FILTER DRIVE" },
        { pid::q8Cut, "808 CUTOFF" }, { pid::q8Drive, "808 DIRT" }, { pid::q8Vowel, "808 VOWEL" }, { pid::q8Fm, "808 FM" },
        { pid::q8WobDepth, "WOBBLE DEPTH" }, { pid::q8Punch, "808 PUNCH" }, { pid::q8Fold, "808 FOLD" },
        { pid::synthLvl, "SYNTH LEVEL" }, { pid::q8Lvl, "808 LEVEL" },
        { rackId (R_MIX), "RACK MIX" }, { rackId (R_LANEA_LVL), "LANE A LEVEL" }, { rackId (R_LANEB_LVL), "LANE B LEVEL" },
    };
    return t;
}
inline int numTargets() { return 1 + kSlotTargets + (int) fixedTargets().size(); }
inline std::string slotLabel (int s) { return std::string (s < kSlotsPerLane ? "A" : "B") + std::to_string ((s % kSlotsPerLane) + 1); }
// param index a target points at (-1 for none). Slot targets map P1-P4 -> SF_P0..SF_P3, 5th -> SF_MIX
inline int targetParam (int t)
{
    if (t <= 0) return -1;
    if (t <= kSlotTargets) { const int s = (t - 1) / 5, k = (t - 1) % 5; return slotId (s, k < 4 ? SF_P0 + k : SF_MIX); }
    const int f = t - 1 - kSlotTargets;
    return f < (int) fixedTargets().size() ? fixedTargets()[(size_t) f].first : -1;
}
inline int targetForSlot (int s, int field) { return 1 + s * 5 + (field == SF_MIX ? 4 : field - SF_P0); }

//==============================================================================
struct ParamDef
{
    std::string id, name;
    PType type;
    float min, max, def, centre;
    std::string choices; // '|' separated
    std::string unit;
};

inline std::string joinFxNames()
{
    std::string s;
    for (int t = 0; t < FX_COUNT; ++t) { if (t) s += "|"; s += fxInfo (t).name; }
    return s;
}

inline const std::vector<ParamDef>& paramDefs()
{
    static const std::vector<ParamDef> defs = []
    {
        std::vector<ParamDef> d = {
#define WC_X_DEF(id, name, type, mn, mx, df, cn, ch, un) { #id, name, PType::type, mn, mx, df, cn, ch, un },
            WC_PARAMS(WC_X_DEF)
#undef WC_X_DEF
        };
        const std::string fxNames = joinFxNames();
        for (int s = 0; s < kSlots; ++s)
        {
            const std::string L = slotLabel (s);
            d.push_back ({ "fx" + L + "type", "FX " + L + " Module", PType::Choice, 0.f, (float) (FX_COUNT - 1), 0.f, -1.f, fxNames, "" });
            d.push_back ({ "fx" + L + "on",   "FX " + L + " On",     PType::Bool,   0.f, 1.f, 1.f, -1.f, "", "" });
            d.push_back ({ "fx" + L + "mix",  "FX " + L + " Mix",    PType::Float,  0.f, 1.f, 1.f, -1.f, "", "" });
            for (int k = 0; k < 4; ++k)
                d.push_back ({ "fx" + L + "p" + std::to_string (k + 1), "FX " + L + " P" + std::to_string (k + 1), PType::Float, 0.f, 1.f, 0.5f, -1.f, "", "fx" });
        }
        d.push_back ({ "rackRouting", "Rack Routing",  PType::Choice, 0.f, 2.f, 0.f, -1.f, "SERIAL A>B|PARALLEL A+B|SPLIT LOW>A HIGH>B", "" });
        d.push_back ({ "rackXover",   "Rack Crossover",PType::Float,  40.f, 2000.f, 180.f, 250.f, "", "Hz" });
        d.push_back ({ "laneALvl",    "Lane A Level",  PType::Float,  0.f, 1.5f, 1.f, -1.f, "", "" });
        d.push_back ({ "laneBLvl",    "Lane B Level",  PType::Float,  0.f, 1.5f, 1.f, -1.f, "", "" });
        d.push_back ({ "laneAOn",     "Lane A On",     PType::Bool,   0.f, 1.f, 1.f, -1.f, "", "" });
        d.push_back ({ "laneBOn",     "Lane B On",     PType::Bool,   0.f, 1.f, 1.f, -1.f, "", "" });
        d.push_back ({ "rackMix",     "Rack Mix",      PType::Float,  0.f, 1.f, 1.f, -1.f, "", "" });

        for (int m = 0; m < 4; ++m)
            d.push_back ({ "macro" + std::to_string (m + 1), std::string ("Macro ") + std::to_string (m + 1) + " " + macroName (m), PType::Float, 0.f, 1.f, 0.f, -1.f, "", "" });
        const std::string lfoShapes = "SINE|TRI|SAW UP|SAW DN|SQUARE|S&H|SMOOTH RND";
        const std::string divs = "4 BAR|2 BAR|1/1|1/2|1/4|1/8|1/16|1/32|1/4T|1/8T|1/16T|1/4D|1/8D";
        for (int l = 0; l < 2; ++l)
        {
            const std::string L = l == 0 ? "A" : "B";
            d.push_back ({ "lfo" + L + "Rate",  "LFO " + L + " Rate",  PType::Float,  0.02f, 30.f, 1.f, 2.f, "", "Hz" });
            d.push_back ({ "lfo" + L + "Sync",  "LFO " + L + " Sync",  PType::Bool,   0.f, 1.f, 1.f, -1.f, "", "" });
            d.push_back ({ "lfo" + L + "Div",   "LFO " + L + " Division", PType::Choice, 0.f, 12.f, 4.f, -1.f, divs, "" });
            d.push_back ({ "lfo" + L + "Shape", "LFO " + L + " Shape", PType::Choice, 0.f, 6.f, 0.f, -1.f, lfoShapes, "" });
        }
        d.push_back ({ "envAtk",  "Env Follow Attack",  PType::Float, 0.1f, 200.f, 5.f, 20.f, "", "ms" });
        d.push_back ({ "envRel",  "Env Follow Release", PType::Float, 5.f, 2000.f, 150.f, 200.f, "", "ms" });
        d.push_back ({ "envGain", "Env Follow Gain",    PType::Float, -12.f, 24.f, 6.f, -1.f, "", "dB" });

        std::string src, dst;
        for (int s = 0; s < MS_COUNT; ++s) { if (s) src += "|"; src += modSourceName (s); }
        dst = "---";
        for (int s = 0; s < kSlots; ++s)
            for (int k = 0; k < 5; ++k)
                dst += "|" + slotLabel (s) + (k < 4 ? " P" + std::to_string (k + 1) : std::string (" MIX"));
        for (auto& t : fixedTargets()) dst += std::string ("|") + t.second;
        for (int r = 0; r < kMtxRows; ++r)
        {
            const std::string R = std::to_string (r + 1);
            d.push_back ({ "mod" + R + "src", "Mod " + R + " Source", PType::Choice, 0.f, (float) (MS_COUNT - 1), 0.f, -1.f, src, "" });
            d.push_back ({ "mod" + R + "dst", "Mod " + R + " Target", PType::Choice, 0.f, (float) (numTargets() - 1), 0.f, -1.f, dst, "" });
            d.push_back ({ "mod" + R + "amt", "Mod " + R + " Amount", PType::Float, -1.f, 1.f, 0.f, -1.f, "", "bip" });
        }
        return d;
    }();
    return defs;
}

// JUCE-compatible normalisation (NormalisableRange with skew-for-centre)
inline float skewOf (const ParamDef& d)
{
    if (d.centre <= 0.f) return 1.f;
    return std::log (0.5f) / std::log ((d.centre - d.min) / (d.max - d.min));
}
inline float toNorm (const ParamDef& d, float v)
{
    float p = (v - d.min) / (d.max - d.min);
    p = p < 0.f ? 0.f : (p > 1.f ? 1.f : p);
    const float sk = skewOf (d);
    return sk == 1.f ? p : std::pow (p, sk);
}
inline float fromNorm (const ParamDef& d, float n)
{
    n = n < 0.f ? 0.f : (n > 1.f ? 1.f : n);
    const float sk = skewOf (d);
    if (sk != 1.f && n > 0.f) n = std::exp (std::log (n) / sk);
    return d.min + (d.max - d.min) * n;
}

struct Params
{
    float v[NUM_PARAMS];
    Params() { resetToDefaults(); }
    void resetToDefaults()
    {
        const auto& d = paramDefs();
        for (int i = 0; i < NUM_PARAMS; ++i) v[i] = d[(size_t) i].def;
    }
    float f (int i) const { return v[i]; }
    int   n (int i) const { return (int) (v[i] + 0.5f); }
    int   sn (int i) const { return (int) (v[i] < 0 ? v[i] - 0.5f : v[i] + 0.5f); }
    bool  b (int i) const { return v[i] > 0.5f; }
};
} // namespace wc
