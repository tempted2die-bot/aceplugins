// WRAITHCORE - factory preset library (folders + reference-modelled sounds)
#pragma once
#include "ParamDefs.h"
#include <vector>
#include <string>
#include <utility>
#include "PresetTrims.h"

namespace wc
{
using PV = std::vector<std::pair<int, float>>;

enum PresetCat { CAT_SPLIT, CAT_808, CAT_BASS, CAT_LEAD, CAT_PLUCK, CAT_KEYS, CAT_PAD, CAT_ARP, CAT_FX, CAT_COUNT };
inline const char* catName (int c)
{
    static const char* n[] = { "808 + SYNTH", "808", "BASS", "LEAD", "PLUCK", "KEYS + BELLS", "PADS + ATMOS", "ARP + SEQ", "FX + NOISE" };
    return n[std::clamp (c, 0, CAT_COUNT - 1)];
}

struct Preset { std::string name; int cat; PV values; };

// ---------- knob helpers (real units -> normalised FX values) ----------
inline float st (int i, int n) { return ((float) i + 0.5f) / (float) n; }
inline float hz2n (float lo, float hi, float hz) { return std::log (hz / lo) / std::log (hi / lo); }

struct PB
{
    PV v; bool rowUsed[kMtxRows] {}; bool slotUsed[kSlots] {};
    PB& s (int id, float val) { v.push_back ({ id, val }); return *this; }
    PB& add (const PV& o) { v.insert (v.end(), o.begin(), o.end()); return *this; }
    PB& synthOnly() { return s (pid::q8On, 0).s (pid::splitNote, 0); }
    PB& only808() { return s (pid::synthLvl, 0).s (pid::splitNote, 0); }
    PB& fx (int slot, int type, float a, float b, float c, float d, float mix = -1.f)
    {
        slotUsed[slot] = true;
        s (slotId (slot, SF_TYPE), (float) type).s (slotId (slot, SF_ON), 1)
            .s (slotId (slot, SF_P0), a).s (slotId (slot, SF_P1), b).s (slotId (slot, SF_P2), c).s (slotId (slot, SF_P3), d)
            .s (slotId (slot, SF_MIX), mix < 0.f ? fxInfo (type).defMix : mix);
        types[slot] = type;
        return *this;
    }
    PB& mod (int row, int src, int dst, float amt) { rowUsed[row] = true; return s (mtxId (row, MX_SRC), (float) src).s (mtxId (row, MX_DST), (float) dst).s (mtxId (row, MX_AMT), amt); }
    PB& lfoA (int div, int shape) { return s (modId (M_LFOA_SYNC), 1).s (modId (M_LFOA_DIV), (float) div).s (modId (M_LFOA_SHAPE), (float) shape); }
    PB& lfoB (int div, int shape) { return s (modId (M_LFOB_SYNC), 1).s (modId (M_LFOB_DIV), (float) div).s (modId (M_LFOB_SHAPE), (float) shape); }
    PB& routing (int r) { return s (rackId (R_ROUTING), (float) r); }

    // ----- FX shorthands -----
    PB& dist (int sl, int mode, float drive, float tone, float bias = 0.f, float mix = 1.f) { return fx (sl, FX_DISTORT, st (mode, 6), drive, tone, bias, mix); }
    PB& clip (int sl, float driveDb = 3.f, float knee = 0.6f, float ceilDb = -0.3f, float outDb = 0.f) { return fx (sl, FX_CLIPPER, driveDb / 24.f, knee, (ceilDb + 12.f) / 12.f, (outDb + 12.f) / 18.f, 1.f); }
    PB& crush (int sl, float bits, float rate, float jitter = 0.f, float noise = 0.f, float mix = 1.f) { return fx (sl, FX_CRUSH, (bits - 1.f) / 15.f, hz2n (200.f, 48000.f, rate), jitter, noise, mix); }
    PB& blown (int sl, float blow, float rattle, float size, float reso, float mix = 1.f) { return fx (sl, FX_BLOWN, blow, rattle, size, reso, mix); }
    PB& subh (int sl, float lvl, int oct, float toneHz, float drive) { return fx (sl, FX_SUBHARM, lvl, st (oct, 2), hz2n (40.f, 400.f, toneHz), drive, 1.f); }
    PB& mosh (int sl, float corrupt, float chunkMs, float hold, float bits, float mix = 1.f) { return fx (sl, FX_DATAMOSH, corrupt, hz2n (2.f, 250.f, chunkMs), hold, bits, mix); }
    PB& filt (int sl, int type, float cutHz, float res, float drive = 0.f) { return fx (sl, FX_FILTER, st (type, 7), hz2n (20.f, 20000.f, cutHz), res, drive, 1.f); }
    PB& formant (int sl, float vowel, float q, int rate, float depth, float mix = 1.f) { return fx (sl, FX_FORMANT, vowel, q, st (rate, 9), depth, mix); }
    PB& eq (int sl, float lo, float mid, float hi, float midHz = 800.f) { return fx (sl, FX_EQ, (lo + 15.f) / 30.f, (mid + 15.f) / 30.f, (hi + 15.f) / 30.f, hz2n (150.f, 6800.f, midHz), 1.f); }
    PB& reson (int sl, int note, float decay, float damp, int chord, float mix = 0.5f) { return fx (sl, FX_RESONATOR, st (note - 24, 61), decay, damp, st (chord, 6), mix); }
    PB& ott (int sl, float depth, float time = 0.5f, float up = 1.f, float down = 1.f) { return fx (sl, FX_OTT, depth, time, up, down, 1.f); }
    PB& pump (int sl, int rate, float depth, float curve = 0.5f, float hold = 0.2f) { return fx (sl, FX_PUMP, st (rate, 5), depth, curve, hold, 1.f); }
    PB& gate (int sl, int rate, int pattern, float depth, float smooth = 0.25f) { return fx (sl, FX_GATE, st (rate, 4), st (pattern, 16), depth, smooth, 1.f); }
    PB& delay (int sl, int div, float fb, float tone, float warble, float mix) { return fx (sl, FX_DELAY, st (div, 9), fb, tone, warble, mix); }
    PB& reverb (int sl, float size, float damp, float shimmer, float mix, float lowCut = 150.f) { return fx (sl, FX_REVERB, size, damp, shimmer, hz2n (20.f, 600.f, lowCut), mix); }
    PB& reverse (int sl, int len, float fb, float tone, float smear, float mix = 0.5f) { return fx (sl, FX_REVERSE, st (len, 5), fb, tone, smear, mix); }
    PB& chorus (int sl, float rateHz, float depth, int voices, float width, float mix = 0.5f) { return fx (sl, FX_CHORUS, hz2n (0.05f, 8.f, rateHz), depth, st (voices - 1, 4), width, mix); }
    PB& phaser (int sl, float rateHz, float depth, float fb, int stages, float mix = 1.f) { return fx (sl, FX_PHASER, hz2n (0.02f, 10.f, rateHz), depth, 0.5f + fb * 0.5f, st (stages, 5), mix); }
    PB& stutter (int sl, int grid, float chance, float rev, float warp, float mix = 1.f) { return fx (sl, FX_STUTTER, st (grid, 5), chance, rev, warp, mix); }
    PB& tapestop (int sl, int grid, float stop, float gapP, float spin, float mix = 1.f) { return fx (sl, FX_TAPESTOP, st (grid, 4), stop, gapP, spin, mix); }
    PB& grains (int sl, float sizeMs, float dens, float scatter, float freeze, float mix = 0.5f) { return fx (sl, FX_GRAINS, hz2n (10.f, 400.f, sizeMs), hz2n (1.f, 60.f, dens), scatter, freeze, mix); }
    PB& fshift (int sl, float hz, float fb, float lfoHz, float lfoAmtHz, float mix = 0.5f)
    {
        const float sgn = hz < 0.f ? -1.f : 1.f, sv = sgn * std::sqrt (std::fabs (hz) / 1000.f);
        return fx (sl, FX_FREQSHIFT, (sv + 1.f) * 0.5f, fb, lfoHz / 8.f, lfoAmtHz / 500.f, mix);
    }
    PB& ring (int sl, float hz, float drift, float shape, float toneHz, float mix = 0.5f) { return fx (sl, FX_RINGMOD, hz2n (20.f, 5000.f, hz), drift, shape, hz2n (300.f, 20000.f, toneHz), mix); }
    PB& pitch (int sl, int semisV, float fb, float windowMs = 60.f, float mix = 0.5f) { return fx (sl, FX_PITCH, st (semisV + 24, 49), 0.5f, fb, (windowMs - 20.f) / 100.f, mix); }
    PB& tape (int sl, float wow, float flutter, float age, float drop, float mix = 1.f) { return fx (sl, FX_TAPE, wow, flutter, age, drop, mix); }
    PB& stereo (int sl, float widthPct, float haasMs, float monoBelowHz, float pan = 0.f)
    {
        return fx (sl, FX_STEREO, widthPct / 200.f, haasMs / 25.f, monoBelowHz <= 0.f ? 0.f : hz2n (20.f, 500.f, monoBelowHz), (pan + 1.f) * 0.5f, 1.f);
    }

    int types[kSlots] {};

    // default macros so BRIGHT / DIRT / SPACE / CHAOS always do something useful
    PV finish()
    {
        auto freeRow = [this]() { for (int r = 0; r < kMtxRows; ++r) if (! rowUsed[r]) return r; return -1; };
        auto rowAdd = [&] (int src, int dst, float amt) { const int r = freeRow(); if (r >= 0) mod (r, src, dst, amt); };
        auto fixedT = [] (int param) { const auto& f = fixedTargets(); for (size_t i = 0; i < f.size(); ++i) if (f[i].first == param) return 1 + kSlotTargets + (int) i; return 0; };
        auto findSlot = [this] (std::initializer_list<int> ts) { for (int sl = 0; sl < kSlots; ++sl) for (int t : ts) if (types[sl] == t) return sl; return -1; };

        rowAdd (MS_MACRO1, fixedT (pid::fltCut), 0.35f);
        rowAdd (MS_MACRO1, fixedT (pid::fltRes), 0.2f);
        rowAdd (MS_MACRO2, fixedT (pid::q8Drive), 0.45f);
        rowAdd (MS_MACRO2, fixedT (pid::fltDrive), 0.6f);

        int sp = findSlot ({ FX_REVERB, FX_DELAY, FX_REVERSE, FX_GRAINS, FX_CHORUS });
        if (sp < 0 && ! slotUsed[kSlots - 1]) { reverb (kSlots - 1, 0.75f, 0.5f, 0.15f, 0.f, 180.f); sp = kSlots - 1; }
        if (sp >= 0) rowAdd (MS_MACRO3, targetForSlot (sp, SF_MIX), 0.5f);

        int ch = findSlot ({ FX_STUTTER, FX_TAPESTOP, FX_DATAMOSH, FX_GRAINS, FX_FREQSHIFT });
        if (ch < 0 && ! slotUsed[kSlotsPerLane - 1]) { stutter (kSlotsPerLane - 1, 2, 0.f, 0.3f, 0.25f); ch = kSlotsPerLane - 1; }
        if (ch >= 0)
        {
            int field = SF_P1;
            if (types[ch] == FX_TAPESTOP) field = SF_P2;
            else if (types[ch] == FX_DATAMOSH) field = SF_P0;
            else if (types[ch] == FX_GRAINS) field = SF_P3;
            else if (types[ch] == FX_FREQSHIFT) field = SF_P3;
            rowAdd (MS_MACRO4, targetForSlot (ch, field), 0.5f);
        }
        return v;
    }
};

inline const std::vector<Preset>& factoryPresets()
{
    using namespace pid;
    static const std::vector<Preset> presets = []
    {
        std::vector<Preset> P;
        auto add = [&P] (const char* name, int cat, PB b)
        {
            PV v = b.finish();
            float og = 0.f;
            for (auto& kv : v) if (kv.first == pid::outGain) og = kv.second;
            v.push_back ({ pid::outGain, std::clamp (og + presetTrim (name), -24.f, 12.f) }); // loudness-matched
            P.push_back ({ name, cat, v });
        };

        // ======================= reusable 808 layers =======================
        const PV wideClip = { { q8Punch, 17 }, { q8PunchTime, 100 }, { q8Shape, 0 }, { q8Click, .1f }, { q8Phase, 60 },
                              { q8DrvMode, 0 }, { q8Drive, .6f }, { q8Asym, .35f }, { q8Tone, .55f }, { q8Knock, .3f }, { q8SubKeep, .1f },
                              { q8Mode, 2 }, { q8Glide, 60 }, { q8Sus, 1 } };
        const PV wobble = { { q8Punch, 15 }, { q8PunchTime, 110 }, { q8Shape, .3f }, { q8Phase, 40 }, { q8DrvMode, 3 }, { q8Drive, .5f },
                            { q8Asym, .2f }, { q8SubKeep, .5f }, { q8Cut, 6000 }, { q8Res, .35f }, { q8Fm, .25f }, { q8FmRatio, 1 }, { q8GrowlAmt, .3f },
                            { q8WobOn, 1 }, { q8WobDiv, 4 }, { q8WobShape, 0 }, { q8WobTarget, 5 }, { q8WobDepth, .75f } };
        const PV dungeon = { { q8Punch, 14 }, { q8PunchTime, 120 }, { q8Shape, .1f }, { q8DrvMode, 1 }, { q8Drive, .5f }, { q8Asym, .45f },
                             { q8Knock, .4f }, { q8SubKeep, .5f }, { q8Tone, .6f }, { q8Mode, 2 }, { q8Glide, 80 }, { subMono, 1 } };
        const PV rawDvd = { { q8Punch, 12 }, { q8PunchTime, 60 }, { q8Click, .35f }, { q8DrvMode, 0 }, { q8Drive, .5f }, { q8Knock, .6f },
                            { q8Decay, 700 }, { q8Sus, 0 }, { q8Rel, 150 }, { q8SubKeep, .35f }, { q8Mode, 1 }, { q8Glide, 30 }, { subMono, 1 } };
        const PV sleep = { { q8Punch, 17 }, { q8PunchTime, 110 }, { q8Phase, 120 }, { q8DrvMode, 1 }, { q8Drive, .5f }, { q8Asym, .3f },
                           { q8SubKeep, .3f }, { q8Glide, 200 }, { q8Rel, 300 } };
        const PV multiDim = { { q8Punch, 17 }, { q8PunchTime, 95 }, { q8Phase, 75 }, { q8DrvMode, 3 }, { q8Drive, .4f }, { q8SubKeep, .4f },
                              { q8Glide, 90 }, { q8Knock, .25f } };

        // ======================= reusable synth layers =======================
        const PV sonarPing = { { oscWave, 0 }, { pingAmt, 24 }, { pingDecay, 38 }, { atk, 0 }, { dec, 650 }, { sus, 0 }, { rel, 450 },
                               { fltCut, 6500 }, { fltRes, .3f } };
        const PV bubbleMachine = { { oscWave, .2f }, { pingAmt, 14 }, { pingDecay, 25 }, { bubRate, 9 }, { bubChance, .75f }, { bubRand, 12 },
                                   { bubGate, .85f }, { fltCut, 2200 }, { fltRes, .6f }, { fltEnv, .4f }, { fltDecay, 70 }, { atk, 2 }, { sus, 1 },
                                   { rel, 600 }, { voiceVol, .9f } };
        const PV wraithPad = { { oscWave, .66f }, { uniVoices, 4 }, { uniDetune, .35f }, { oscFold, .1f }, { fltCut, 1800 }, { fltRes, .25f },
                               { lfoShape, 5 }, { lfoRate, .3f }, { lfoCut, .3f }, { atk, 900 }, { dec, 2000 }, { sus, .9f }, { rel, 3000 }, { voiceVol, .5f } };
        const PV darkiePluck = { { oscWave, .33f }, { pingAmt, 7 }, { pingDecay, 25 }, { dec, 320 }, { sus, 0 }, { rel, 300 }, { fltCut, 2500 },
                                 { fltEnv, .45f }, { fltDecay, 160 }, { fltRes, .25f } };
        const PV eerieWhistle = { { oscWave, 0 }, { oscOct, 1 }, { noiseLvl, .04f }, { lfoPitch, .35f }, { lfoRate, 6 }, { atk, 40 }, { sus, 1 }, { rel, 600 },
                                  { polyMode, 2 }, { glide, 90 } };
        const PV toyKeys = { { oscWave, .33f }, { fmRatio, 7 }, { fmAmt, .2f }, { dec, 500 }, { sus, 0 }, { rel, 400 } };
        const PV organ = { { oscWave, 1 }, { obLvl, .6f }, { obWave, .66f }, { obOct, 1 }, { fltCut, 3000 }, { atk, 10 }, { sus, 1 }, { rel, 300 }, { voiceVol, .45f } };
        const PV grainPad = { { oscWave, .33f }, { uniVoices, 3 }, { uniDetune, .3f }, { atk, 400 }, { sus, 1 }, { rel, 2500 }, { voiceVol, .5f } };

        // =====================================================================
        add ("Init", CAT_SPLIT, PB());

        // =============================== 808 ===============================
        add ("Wide Clip 808", CAT_808, PB().only808().add (wideClip).clip (0, 3.f));
        add ("Wobble Wraith 808", CAT_808, PB().only808().add (wobble).s (q8Lvl, .9f).ott (0, .3f).reverb (9, .5f, .45f, 0, .15f, 200));
        add ("Yoi Talker 808", CAT_808, PB().only808().s (q8Punch, 12).s (q8PunchTime, 88).s (q8Shape, .2f).s (q8Phase, 30).s (q8DrvMode, 1).s (q8Drive, .6f)
             .s (q8Asym, .4f).s (q8SubKeep, .5f).s (q8Fm, .15f).s (q8FmRatio, 2).s (q8Vowel, .2f).s (q8GrowlRes, .7f)
             .s (q8WobOn, 1).s (q8WobDiv, 2).s (q8WobShape, 5).s (q8WobTarget, 1).s (q8WobDepth, .9f).ott (0, .25f));
        add ("Depth Charge 808", CAT_808, PB().only808().s (q8Punch, 36).s (q8PunchTime, 154).s (q8Click, .6f).s (q8Phase, 60).s (q8DrvMode, 4)
             .s (q8Drive, .5f).s (q8Fold, .3f).s (q8SubKeep, .45f).s (q8Tone, .8f).tapestop (0, 2, .3f, .25f, .1f).reverb (9, .4f, .5f, 0, .12f, 200));
        add ("Rattle Snake 808", CAT_808, PB().only808().add (wideClip).s (q8Phase, 70).s (q8WobOn, 1).s (q8WobDiv, 8).s (q8WobShape, 4).s (q8WobTarget, 4)
             .s (q8WobDepth, .08f).tapestop (0, 3, 0, .15f, 0).clip (1, 2.f));
        add ("Glide Bully 808", CAT_808, PB().only808().s (q8Punch, 19).s (q8PunchTime, 121).s (q8Shape, .25f).s (q8Phase, 45).s (q8DrvMode, 1).s (q8Drive, .75f)
             .s (q8Asym, .5f).s (q8Tone, .65f).s (q8SubKeep, .4f).s (q8Mode, 2).s (q8Glide, 160).s (subMono, 1).clip (0, 2.f));
        add ("Screamer 808", CAT_808, PB().only808().s (q8Punch, 24).s (q8PunchTime, 77).s (q8Phase, 50).s (q8DrvMode, 3).s (q8Drive, .7f).s (q8SubKeep, .55f)
             .s (q8Cut, 9000).s (q8Res, .3f).s (q8Fm, .5f).s (q8FmRatio, 3).s (q8GrowlAmt, .4f).s (q8Vowel, .8f)
             .s (q8WobOn, 1).s (q8WobDiv, 5).s (q8WobShape, 2).s (q8WobTarget, 2).s (q8WobDepth, .6f).s (q8Lvl, .9f).ott (0, .35f));
        add ("Dungeon Sub 808", CAT_808, PB().only808().add (dungeon).ott (0, .35f).clip (1, 2.f));
        add ("Cringe Stack 808", CAT_808, PB().only808().s (q8Punch, 19).s (q8PunchTime, 90).s (q8DrvMode, 0).s (q8Drive, .75f).s (q8Asym, .4f).s (q8Knock, .55f)
             .s (q8Tone, .65f).s (q8SubKeep, .25f).s (q8Mode, 2).s (q8Glide, 50).s (subMono, 1).dist (0, 5, .25f, .7f, 0, .5f).ott (1, .4f));
        add ("Itchy Wide 808", CAT_808, PB().only808().s (q8Punch, 17).s (q8PunchTime, 80).s (q8Phase, 90).s (q8DrvMode, 3).s (q8Drive, .45f).s (q8SubKeep, .3f)
             .s (q8Glide, 70).stereo (0, 130, 0, 0).clip (1, 3.f));
        add ("Sleep Deprived 808", CAT_808, PB().only808().add (sleep).tapestop (0, 1, 0, .2f, 0).reverb (9, .45f, .5f, 0, .1f, 250));
        add ("Roten Grit 808", CAT_808, PB().only808().s (q8Punch, 15).s (q8DrvMode, 4).s (q8Drive, .55f).s (q8Fold, .2f).s (q8Tone, .9f).s (q8SubKeep, .5f).s (subMono, 1)
             .blown (0, .45f, .4f, .7f, .3f, .6f).crush (1, 10, 16000, .1f, 0, .4f).ott (2, .4f));
        add ("Raw DVD 808", CAT_808, PB().only808().add (rawDvd).crush (0, 10, 22000, 0, 0, .35f).mosh (1, .08f, 30, .5f, .3f).clip (2, 2.f));
        add ("Multi-Dim 808", CAT_808, PB().only808().add (multiDim).tapestop (0, 2, 0, .3f, 0).grains (5, 80, 12, .3f, 0, .2f));
        add ("Shoelace Slide 808", CAT_808, PB().only808().s (q8Glide, 220).s (q8Mode, 2).s (q8Shape, .05f).s (q8DrvMode, 1).s (q8Drive, .4f).s (q8Asym, .3f)
             .s (q8Punch, 10).s (q8PunchTime, 150).s (q8SubKeep, .5f).s (subMono, 1).ott (0, .25f));
        add ("2-Step Dive 808", CAT_808, PB().only808().add (wideClip).s (q8Punch, 12).s (q8WobOn, 1).s (q8WobDiv, 1).s (q8WobShape, 2).s (q8WobTarget, 4)
             .s (q8WobDepth, .5f).ott (0, .3f));
        add ("Bitflip 808", CAT_808, PB().only808().add (wideClip).s (subMono, 1).mosh (0, .25f, 40, .3f, .45f, .7f).clip (1, 2.f));
        add ("Subharmonic Abyss 808", CAT_808, PB().only808().s (q8Oct, 0).s (q8Drive, .2f).s (q8Punch, 12).s (q8SubKeep, .4f).subh (0, .7f, 0, 90, .3f).clip (1, 2.f));
        add ("Phone Speaker 808", CAT_808, PB().only808().s (q8DrvMode, 1).s (q8Drive, .6f).s (q8Punch, 15).blown (0, .55f, .25f, .1f, .5f).eq (1, -6, 3, 0, 1200));
        add ("Ring Rust 808", CAT_808, PB().only808().s (q8DrvMode, 2).s (q8Drive, .4f).s (q8Punch, 17).s (q8SubKeep, .5f).ring (0, 35, .3f, .2f, 6000, .35f).ott (1, .3f));
        add ("Gated Chop 808", CAT_808, PB().only808().add (wideClip).gate (0, 1, 14, 1.f, .2f));
        add ("Reese 808", CAT_808, PB().only808().s (q8Shape, .6f).s (q8Phase, 30).s (q8Fm, .15f).s (q8FmRatio, .5f).s (q8Drive, .45f).s (q8SubKeep, .5f)
             .chorus (0, .3f, .5f, 2, .2f, .6f).filt (1, 1, 900, .2f, .2f));
        add ("Tape Warped 808", CAT_808, PB().only808().add (wideClip).s (q8Glide, 100).tape (0, .5f, .3f, .4f, .05f));
        add ("Formant Growl 808", CAT_808, PB().only808().s (q8DrvMode, 1).s (q8Drive, .55f).s (q8SubKeep, .5f).formant (0, .3f, .6f, 4, .7f, .7f).ott (1, .3f));
        add ("Clean Long 808", CAT_808, PB().only808().s (q8Punch, 12).s (q8PunchTime, 80).s (q8Drive, .15f).s (q8SubKeep, .7f).s (subMono, 1).s (q8Tone, .5f).s (q8Click, .05f));
        add ("Knock Punch 808", CAT_808, PB().only808().s (q8Decay, 500).s (q8Sus, 0).s (q8Punch, 20).s (q8PunchTime, 70).s (q8Click, .5f).s (q8Knock, .7f)
             .s (q8Drive, .6f).s (q8Mode, 1).clip (0, 3.f));
        add ("Spinback 808", CAT_808, PB().only808().add (wideClip).tapestop (0, 0, .2f, 0, .5f));
        add ("Barber Shift 808", CAT_808, PB().only808().s (q8DrvMode, 3).s (q8Drive, .5f).s (q8SubKeep, .5f).fshift (5, 8, .4f, 0, 0, .3f));

        // =============================== BASS ===============================
        add ("Reese Abyss", CAT_BASS, PB().synthOnly().s (oscWave, .66f).s (uniVoices, 4).s (uniDetune, .45f).s (oscOct, -1).s (fltType, 1).s (fltCut, 700).s (fltRes, .25f)
             .s (polyMode, 1).s (glide, 40).s (sus, 1).s (subLvl, .4f).dist (0, 5, .3f, .6f).ott (1, .4f));
        add ("Square Sub Bass", CAT_BASS, PB().synthOnly().s (oscWave, 1).s (oscOct, -1).s (fltCut, 380).s (fltRes, .1f).s (subLvl, .5f).s (polyMode, 1).s (glide, 70)
             .s (sus, 1).s (rel, 120).clip (0, 3.f));
        add ("FM Growl Bass", CAT_BASS, PB().synthOnly().s (oscWave, 0).s (oscOct, -1).s (fmRatio, 1).s (fmAmt, .55f).s (fltType, 1).s (fltCut, 1500).s (fltRes, .3f)
             .s (lfoSync, 1).s (lfoDiv, 3).s (lfoCut, .5f).s (polyMode, 1).s (sus, 1).dist (0, 2, .35f, .7f, 0, .6f).ott (1, .5f));
        add ("Acid Wraith", CAT_BASS, PB().synthOnly().s (oscWave, .66f).s (oscOct, -1).s (fltType, 6).s (fltCut, 400).s (fltRes, .8f).s (fltEnv, .6f).s (fltDecay, 180)
             .s (polyMode, 2).s (glide, 60).s (sus, 1).dist (0, 0, .4f, .8f).delay (5, 5, .35f, .5f, .2f, .2f));
        add ("Sync Saw Bass", CAT_BASS, PB().synthOnly().s (oscWave, .66f).s (oscALvl, .3f).s (obLvl, .8f).s (obWave, .66f).s (obOct, 1).s (obSemi, 5).s (obMode, 2)
             .s (oscOct, -1).s (fltCut, 2000).s (fltEnv, .4f).s (fltDecay, 300).s (polyMode, 1).s (sus, 1).ott (0, .4f));
        add ("Ring Mod Bass", CAT_BASS, PB().synthOnly().s (oscWave, .33f).s (obLvl, .7f).s (obMode, 1).s (obWave, 0).s (obSemi, 7).s (oscOct, -1).s (fltType, 1)
             .s (fltCut, 1200).s (polyMode, 1).s (sus, 1).clip (0, 3.f));
        add ("Fold Grinder Bass", CAT_BASS, PB().synthOnly().s (oscWave, 0).s (oscFold, .55f).s (oscOct, -1).s (fltCut, 3000).s (polyMode, 1).s (sus, 1)
             .lfoA (5, 0).mod (6, MS_LFOA, 1 + kSlotTargets + 3, .35f).dist (0, 1, .3f, .7f, 0, .5f).ott (1, .35f));
        add ("Bitcrushed Bass", CAT_BASS, PB().synthOnly().s (oscWave, 1).s (oscOct, -1).s (fltCut, 1800).s (subLvl, .3f).s (polyMode, 1).s (sus, 1)
             .crush (0, 6, 9000, .2f, 0, .8f).ott (1, .3f));
        add ("Wobble Synth Bass", CAT_BASS, PB().synthOnly().s (oscWave, .66f).s (uniVoices, 2).s (uniDetune, .2f).s (oscOct, -1).s (fltType, 1).s (fltCut, 3000)
             .s (fltRes, .45f).s (lfoSync, 1).s (lfoDiv, 3).s (lfoCut, .9f).s (polyMode, 1).s (sus, 1).dist (0, 0, .35f, .8f).ott (1, .5f));
        add ("Pluck Bass", CAT_BASS, PB().synthOnly().s (oscWave, .5f).s (oscOct, -1).s (dec, 420).s (sus, 0.15f).s (rel, 160).s (voiceVol, 1).s (subLvl, .4f).s (pingAmt, 12).s (pingDecay, 25)
             .s (fltCut, 900).s (fltEnv, .5f).s (fltDecay, 150).ott (0, .3f));

        // =============================== LEAD ===============================
        add ("Sonar Lead", CAT_LEAD, PB().synthOnly().s (oscWave, 0).s (pingAmt, 12).s (pingDecay, 30).s (lfoPitch, .25f).s (lfoRate, 5.5f).s (atk, 5).s (sus, .9f)
             .s (rel, 300).s (polyMode, 2).s (glide, 60).delay (5, 5, .4f, .55f, .2f, .25f).reverb (9, .7f, .45f, 0, .25f));
        add ("Eerie Whistle", CAT_LEAD, PB().synthOnly().add (eerieWhistle).reverb (9, .85f, .4f, .35f, .35f).delay (5, 6, .35f, .5f, .2f, .2f));
        add ("Toy Square Lead", CAT_LEAD, PB().synthOnly().s (oscWave, 1).s (oscOct, 1).s (fltCut, 4000).s (dec, 400).s (sus, .5f)
             .crush (0, 8, 14000, 0, 0, .7f).delay (5, 5, .4f, .5f, .15f, .25f).reverb (9, .5f, .45f, 0, .15f));
        add ("Rectified Ghost Lead", CAT_LEAD, PB().synthOnly().s (oscWave, .5f).s (uniVoices, 2).s (uniDetune, .15f).s (pingAmt, 5).s (pingDecay, 60).s (polyMode, 2)
             .s (glide, 120).s (lfoPitch, .3f).s (lfoRate, 5.5f).s (atk, 5).s (sus, .9f).s (rel, 350).s (fltCut, 4000).s (fltRes, .35f)
             .dist (0, 3, .55f, .6f, 0, .8f).delay (5, 5, .45f, .55f, .15f, .25f).reverb (9, .8f, .45f, .2f, .35f));
        add ("Detune Saw Lead", CAT_LEAD, PB().synthOnly().s (oscWave, .66f).s (uniVoices, 3).s (uniDetune, .25f).s (fltCut, 5000).s (fltRes, .2f).s (sus, .8f)
             .chorus (0, .4f, .35f, 2, .8f, .4f).delay (5, 3, .35f, .55f, .1f, .2f).reverb (9, .6f, .45f, 0, .2f));
        add ("Sync Scream Lead", CAT_LEAD, PB().synthOnly().s (oscALvl, .2f).s (obLvl, 1).s (obMode, 2).s (obOct, 1).s (obSemi, 7).s (fltCut, 2500).s (fltRes, .3f)
             .s (polyMode, 2).s (glide, 50).s (sus, 1).lfoA (4, 1).mod (6, MS_LFOA, 1 + kSlotTargets + 0, .3f).dist (0, 0, .4f, .7f).delay (5, 5, .4f, .5f, .2f, .25f));
        add ("Haunted Formant Lead", CAT_LEAD, PB().synthOnly().s (oscWave, .66f).s (lfoPitch, .15f).s (lfoRate, 5).s (sus, 1).s (polyMode, 2).s (glide, 70)
             .formant (0, .5f, .7f, 3, .6f).reverb (9, .8f, .45f, .1f, .3f));
        add ("Barberpole Lead", CAT_LEAD, PB().synthOnly().s (oscWave, .33f).s (sus, 1).s (polyMode, 2).fshift (0, 3, .55f, 0, 0, .5f).delay (5, 5, .4f, .5f, .2f, .2f)
             .reverb (9, .7f, .45f, 0, .25f));
        add ("Glide Mono Lead", CAT_LEAD, PB().synthOnly().s (oscWave, .5f).s (uniVoices, 2).s (uniDetune, .1f).s (polyMode, 2).s (glide, 140).s (fltCut, 3500).s (sus, 1)
             .dist (0, 5, .3f, .7f).delay (5, 5, .4f, .55f, .2f, .2f));
        add ("Corrupted Lead", CAT_LEAD, PB().synthOnly().s (oscWave, 1).s (sus, .9f).s (polyMode, 2).mosh (0, .3f, 25, .3f, .4f, .6f).delay (5, 5, .4f, .5f, .15f, .2f)
             .reverb (9, .6f, .45f, 0, .2f));
        add ("Phone Lead", CAT_LEAD, PB().synthOnly().s (oscWave, .66f).s (uniVoices, 2).s (uniDetune, .15f).s (sus, 1).blown (0, .4f, .2f, .05f, .3f).reverb (9, .5f, .45f, 0, .2f));
        add ("Vowel Chop Lead", CAT_LEAD, PB().synthOnly().s (oscWave, .66f).s (uniVoices, 2).s (sus, 1).formant (0, .7f, .6f, 0, 0).gate (1, 1, 7, .9f, .2f)
             .reverb (9, .6f, .45f, 0, .25f));
        add ("Pitch Ghost Lead", CAT_LEAD, PB().synthOnly().s (oscWave, 0).s (sus, 1).s (lfoPitch, .2f).s (lfoRate, 5).pitch (0, 12, .3f, 60, .35f).reverb (9, .8f, .45f, .3f, .3f));

        // =============================== PLUCK ===============================
        add ("Sonar Ping", CAT_PLUCK, PB().synthOnly().add (sonarPing).delay (5, 5, .55f, .5f, .2f, .38f).reverb (9, .75f, .45f, 0, .3f));
        add ("Crushed Blips", CAT_PLUCK, PB().synthOnly().s (oscWave, 1).s (oscOct, 1).s (pingAmt, -24).s (pingDecay, 20).s (atk, 0).s (dec, 120).s (sus, 0).s (rel, 120)
             .s (voiceVol, .55f).crush (0, 5, 4000, 0, 0, .9f).dist (1, 1, .4f, .8f).delay (5, 1, .5f, .55f, .15f, .3f).reverb (9, .4f, .45f, 0, .15f));
        add ("Darkie Pluck", CAT_PLUCK, PB().synthOnly().add (darkiePluck).delay (5, 5, .45f, .55f, .2f, .3f).reverb (9, .7f, .45f, 0, .3f));
        add ("Kalimba Wraith", CAT_PLUCK, PB().synthOnly().s (oscWave, 0).s (fmRatio, 3.5f).s (fmAmt, .3f).s (pingAmt, 5).s (pingDecay, 10).s (dec, 600).s (sus, 0)
             .tape (0, .2f, .1f, .3f, 0).reverb (9, .6f, .45f, 0, .25f));
        add ("Metal Pluck", CAT_PLUCK, PB().synthOnly().s (oscWave, .66f).s (dec, 200).s (sus, 0).reson (0, 51, .7f, .4f, 3).reverb (9, .6f, .45f, 0, .2f));
        add ("Water Drop", CAT_PLUCK, PB().synthOnly().s (oscWave, 0).s (pingAmt, 24).s (pingDecay, 20).s (dec, 250).s (sus, 0).s (fltType, 0).s (fltCut, 1400).s (fltRes, .35f)
             .s (fltEnv, .5f).s (fltDecay, 60).s (fltKey, 1).s (voiceVol, 1).delay (5, 1, .35f, .6f, .1f, .25f).reverb (9, .5f, .45f, 0, .25f));
        add ("Stutter Pluck", CAT_PLUCK, PB().synthOnly().s (oscWave, .5f).s (dec, 320).s (sus, 0).s (voiceVol, 1).stutter (0, 3, .45f, .3f, .2f).delay (5, 5, .35f, .5f, .1f, .2f));
        add ("Reverse Pluck", CAT_PLUCK, PB().synthOnly().s (oscWave, .33f).s (dec, 400).s (sus, 0).reverse (0, 1, .3f, .7f, .4f, .5f).reverb (9, .7f, .45f, 0, .25f));
        add ("Square Pluck", CAT_PLUCK, PB().synthOnly().s (oscWave, 1).s (fltType, 1).s (fltCut, 1200).s (fltEnv, .55f).s (fltDecay, 140).s (fltRes, .3f).s (dec, 250).s (sus, 0)
             .delay (5, 3, .4f, .5f, .1f, .25f));
        add ("Harp Shards", CAT_PLUCK, PB().synthOnly().s (oscWave, .66f).s (uniVoices, 2).s (uniDetune, .12f).s (dec, 500).s (sus, 0).s (fltCut, 6000).s (fltEnv, .2f)
             .reverb (9, .9f, .45f, .4f, .4f));
        add ("Bitcrush Pluck", CAT_PLUCK, PB().synthOnly().s (oscWave, .33f).s (dec, 300).s (sus, 0).crush (0, 6, 11000).delay (5, 5, .4f, .5f, .15f, .3f));
        add ("Comb Pluck", CAT_PLUCK, PB().synthOnly().s (oscWave, .66f).s (noiseLvl, .3f).s (fltType, 5).s (fltCut, 300).s (fltRes, .7f).s (fltEnv, .5f).s (fltDecay, 120)
             .s (dec, 300).s (sus, 0).s (fltKey, 1).reverb (9, .6f, .45f, 0, .2f));

        // =============================== KEYS ===============================
        add ("Glitch Bell", CAT_KEYS, PB().synthOnly().s (oscWave, 0).s (fmRatio, 3.5f).s (fmAmt, .45f).s (pingAmt, 12).s (pingDecay, 15).s (atk, 0).s (dec, 900).s (sus, 0)
             .s (rel, 600).crush (0, 9, 18000, 0, 0, .6f).stutter (1, 3, .45f, .3f, .3f).delay (5, 4, .4f, .55f, .15f, .25f).reverb (9, .6f, .45f, 0, .25f));
        add ("Corrupt Music Box", CAT_KEYS, PB().synthOnly().s (oscWave, 0).s (fmRatio, 4).s (fmAmt, .3f).s (oscOct, 1).s (dec, 700).s (sus, 0).s (rel, 500)
             .mosh (0, .12f, 60, .6f, .25f, .7f).tape (1, .35f, .2f, .35f, 0).reverb (9, .7f, .45f, 0, .3f));
        add ("Broken Toy Piano", CAT_KEYS, PB().synthOnly().add (toyKeys).crush (0, 9, 16000, 0, 0, .5f).tape (1, .25f, .3f, .4f, 0).reverb (9, .5f, .45f, 0, .15f));
        add ("Haunted Celeste", CAT_KEYS, PB().synthOnly().s (oscWave, 0).s (fmRatio, 3).s (fmAmt, .35f).s (oscOct, 1).s (dec, 1200).s (sus, 0).s (rel, 900)
             .delay (5, 7, .35f, .5f, .2f, .2f).reverb (9, .9f, .45f, .45f, .45f));
        add ("Detuned EP", CAT_KEYS, PB().synthOnly().s (oscWave, .2f).s (uniVoices, 2).s (uniDetune, .15f).s (obLvl, .3f).s (obWave, .33f).s (dec, 1500).s (sus, .3f).s (rel, 500)
             .chorus (0, .5f, .4f, 2, .8f, .5f).tape (1, .3f, .15f, .25f, 0).reverb (9, .5f, .45f, 0, .2f));
        add ("Dungeon Organ", CAT_KEYS, PB().synthOnly().add (organ).dist (0, 5, .2f, .7f, 0, .5f).chorus (1, .3f, .4f, 3, .8f, .45f).reverb (9, .75f, .45f, 0, .3f));
        add ("Glass Grain Bell", CAT_KEYS, PB().synthOnly().s (oscWave, 0).s (fmRatio, 3.5f).s (fmAmt, .3f).s (dec, 800).s (sus, 0).s (rel, 600)
             .grains (5, 120, 15, .4f, .1f, .45f).reverb (9, .8f, .45f, .2f, .3f));
        add ("Resonant Bell", CAT_KEYS, PB().synthOnly().s (oscWave, 0).s (pingAmt, 12).s (pingDecay, 10).s (dec, 900).s (sus, 0).reson (0, 60, .8f, .3f, 4, .45f)
             .reverb (9, .7f, .45f, 0, .25f));
        add ("Bitflip Keys", CAT_KEYS, PB().synthOnly().s (oscWave, .33f).s (uniVoices, 2).s (uniDetune, .12f).s (dec, 800).s (sus, .2f).mosh (0, .35f, 15, .2f, .5f, .5f)
             .chorus (1, .4f, .35f, 2, .7f, .4f));
        add ("Choir Keys", CAT_KEYS, PB().synthOnly().s (oscWave, .66f).s (uniVoices, 3).s (uniDetune, .2f).s (atk, 60).s (sus, .8f).s (rel, 800)
             .formant (0, .55f, .6f, 0, 0).chorus (1, .3f, .4f, 3, .8f, .4f).reverb (9, .8f, .45f, .15f, .35f));

        // =============================== PADS ===============================
        add ("Wraith Pad", CAT_PAD, PB().synthOnly().add (wraithPad).stutter (0, 2, .15f, .25f, .15f, .8f).delay (5, 7, .45f, .55f, .15f, .2f).reverb (9, .95f, .45f, .45f, .55f));
        add ("Underwater Radar", CAT_PAD, PB().synthOnly().s (oscWave, 0).s (oscFold, .35f).s (pingAmt, 7).s (pingDecay, 300).s (fltCut, 900).s (fltRes, .85f).s (fltEnv, .5f)
             .s (fltDecay, 400).s (lfoShape, 4).s (lfoSync, 1).s (lfoDiv, 4).s (lfoCut, .35f).s (atk, 1).s (dec, 1500).s (sus, .3f).s (rel, 1500).s (voiceVol, .45f)
             .delay (5, 6, .7f, .3f, .45f, .4f).reverb (9, .9f, .7f, 0, .45f));
        add ("Abyss Choir", CAT_PAD, PB().synthOnly().s (oscWave, .66f).s (uniVoices, 4).s (uniDetune, .3f).s (atk, 800).s (sus, .9f).s (rel, 3000).s (voiceVol, .5f)
             .formant (0, .4f, .6f, 1, .4f, .8f).reverb (9, .95f, .45f, .5f, .5f));
        add ("Frozen Grains", CAT_PAD, PB().synthOnly().add (grainPad).grains (0, 200, 20, .5f, .6f, .6f).reverb (9, .9f, .45f, .2f, .4f));
        add ("Tape Decay Pad", CAT_PAD, PB().synthOnly().s (oscWave, .66f).s (uniVoices, 4).s (uniDetune, .25f).s (fltCut, 2000).s (atk, 600).s (sus, 1).s (rel, 2500)
             .s (voiceVol, .5f).tape (0, .5f, .2f, .6f, .1f).reverb (9, .8f, .45f, 0, .35f));
        add ("Freq Shift Void", CAT_PAD, PB().synthOnly().s (oscWave, 0).s (uniVoices, 2).s (uniDetune, .2f).s (atk, 900).s (sus, 1).s (rel, 3000)
             .fshift (0, -12, .6f, .5f, 25, .5f).reverb (9, .95f, .45f, 0, .5f));
        add ("Phaser Fog", CAT_PAD, PB().synthOnly().s (oscWave, .66f).s (uniVoices, 3).s (uniDetune, .25f).s (atk, 700).s (sus, 1).s (rel, 2500).s (voiceVol, .5f)
             .phaser (0, .15f, .8f, .75f, 3).reverb (9, .85f, .45f, 0, .4f));
        add ("Reverse Swells", CAT_PAD, PB().synthOnly().s (oscWave, .33f).s (uniVoices, 2).s (atk, 300).s (dec, 800).s (sus, .6f).s (rel, 1500)
             .reverse (0, 2, .4f, .7f, .6f, .6f).reverb (9, .9f, .45f, 0, .4f));
        add ("Ring Fog", CAT_PAD, PB().synthOnly().s (oscWave, 0).s (uniVoices, 3).s (atk, 800).s (sus, 1).s (rel, 2500).ring (0, 180, .5f, .1f, 5000, .4f)
             .chorus (1, .2f, .5f, 3, .9f, .5f).reverb (9, .9f, .45f, 0, .4f));
        add ("Dark Sine Drone", CAT_PAD, PB().synthOnly().s (oscWave, 0).s (oscOct, -1).s (subLvl, .5f).s (noiseLvl, .02f).s (lfoShape, 5).s (lfoRate, .2f).s (lfoPitch, .2f)
             .s (atk, 1500).s (sus, 1).s (rel, 4000).tape (0, .3f, .1f, .3f, 0).reverb (9, .95f, .7f, 0, .4f));

        // =============================== ARP ===============================
        add ("Bubble Machine", CAT_ARP, PB().synthOnly().add (bubbleMachine).delay (5, 1, .35f, .55f, .15f, .25f).reverb (9, .5f, .45f, 0, .25f));
        add ("Haunted Arp Chaos", CAT_ARP, PB().synthOnly().s (oscWave, .33f).s (fmAmt, .2f).s (fmRatio, 7).s (pingAmt, 19).s (pingDecay, 18).s (bubRate, 12).s (bubChance, .55f)
             .s (bubRand, 24).s (bubGate, 1).s (atk, 1).s (sus, 1).s (rel, 400).s (voiceVol, .6f).dist (0, 2, .45f, .8f, 0, .5f).crush (1, 8, 12000, 0, 0, .5f)
             .stutter (2, 2, .5f, .4f, .3f).delay (5, 5, .5f, .55f, .15f, .3f).reverb (9, .7f, .45f, .3f, .35f));
        add ("Sonar Sequencer", CAT_ARP, PB().synthOnly().s (oscWave, 0).s (pingAmt, 19).s (pingDecay, 20).s (bubRate, 8).s (bubChance, .8f).s (bubRand, 7).s (bubGate, 1).s (sus, 1)
             .gate (0, 1, 2, .6f, .2f).delay (5, 5, .45f, .55f, .2f, .3f).reverb (9, .6f, .45f, 0, .25f));
        add ("Pentatonic Rain", CAT_ARP, PB().synthOnly().s (oscWave, .33f).s (bubRate, 6).s (bubChance, .6f).s (bubRand, 12).s (bubGate, .9f).s (pingAmt, 7).s (fltCut, 3000)
             .s (sus, 1).delay (5, 6, .5f, .5f, .2f, .3f).reverb (9, .8f, .45f, .3f, .35f));
        add ("Glitch Runner", CAT_ARP, PB().synthOnly().s (oscWave, 1).s (bubRate, 10).s (bubChance, .7f).s (bubRand, 12).s (bubGate, 1).s (sus, 1).s (voiceVol, .55f)
             .stutter (0, 2, .4f, .3f, .3f).tapestop (1, 2, .15f, .2f, 0).crush (2, 8, 14000, 0, 0, .4f));
        add ("Bitflip Bubbles", CAT_ARP, PB().synthOnly().add (bubbleMachine).mosh (0, .3f, 20, .3f, .4f, .6f).delay (5, 1, .35f, .55f, .15f, .25f));
        add ("Gated Saw Seq", CAT_ARP, PB().synthOnly().s (oscWave, .66f).s (uniVoices, 3).s (uniDetune, .2f).s (sus, 1).s (fltCut, 1500).s (voiceVol, .5f)
             .lfoA (2, 1).mod (6, MS_LFOA, 1 + kSlotTargets + 0, .4f).gate (0, 1, 0, 1.f, .15f).delay (5, 5, .4f, .5f, .1f, .2f).reverb (9, .6f, .45f, 0, .2f));
        add ("Random Radar", CAT_ARP, PB().synthOnly().s (oscWave, 0).s (oscFold, .3f).s (bubRate, 5).s (bubChance, .5f).s (bubRand, 24).s (lfoShape, 4).s (lfoSync, 1)
             .s (lfoDiv, 4).s (lfoCut, .4f).s (fltType, 2).s (fltCut, 1500).s (fltRes, .6f).s (sus, 1).delay (5, 3, .6f, .4f, .4f, .35f).reverb (9, .9f, .45f, 0, .35f));

        // =============================== FX ===============================
        add ("Depth Sounder", CAT_FX, PB().synthOnly().s (oscWave, 0).s (noiseLvl, .1f).s (pingAmt, -24).s (pingDecay, 400).s (dec, 2000).s (sus, 0)
             .delay (5, 7, .6f, .45f, .3f, .35f).reverb (9, .95f, .45f, 0, .5f));
        add ("Tape Stop Hit", CAT_FX, PB().synthOnly().s (oscWave, .66f).s (uniVoices, 3).s (uniDetune, .2f).s (sus, 1).tapestop (0, 0, 1.f, 0, 0).reverb (9, .5f, .45f, 0, .15f));
        add ("Datamosh Noise", CAT_FX, PB().synthOnly().s (noiseLvl, .8f).s (oscWave, .66f).s (fltCut, 6000).s (sus, 1).s (voiceVol, .45f)
             .mosh (0, .6f, 30, .4f, .6f).stutter (1, 3, .3f, .4f, .4f).reverb (9, .6f, .45f, 0, .2f));
        add ("Shimmer Riser", CAT_FX, PB().synthOnly().s (noiseLvl, .4f).s (oscWave, .66f).s (uniVoices, 4).s (uniDetune, .4f).s (fltCut, 300).s (atk, 3000).s (sus, 1)
             .s (voiceVol, .5f).lfoA (0, 2).mod (6, MS_LFOA, 1 + kSlotTargets + 0, .6f).reverb (9, .9f, .45f, .6f, .5f));
        add ("Laser Zap", CAT_FX, PB().synthOnly().s (oscWave, 1).s (pingAmt, -48).s (pingDecay, 80).s (dec, 150).s (sus, 0).delay (5, 1, .5f, .55f, .1f, .3f).reverb (9, .4f, .45f, 0, .15f));
        add ("Spinback Chaos", CAT_FX, PB().synthOnly().s (oscWave, .66f).s (uniVoices, 3).s (sus, 1).tapestop (0, 1, .2f, .1f, .9f).stutter (1, 2, .3f, .3f, .3f));
        add ("Metal Clank", CAT_FX, PB().synthOnly().s (noiseLvl, .6f).s (oscWave, 1).s (dec, 160).s (sus, 0).s (voiceVol, 1).reson (0, 36, .85f, .2f, 5, .9f).reverb (9, .6f, .45f, 0, .2f));
        add ("Radio Static", CAT_FX, PB().synthOnly().s (noiseLvl, .5f).s (oscWave, .33f).s (sus, 1).s (voiceVol, .5f).ring (0, 800, .6f, .5f, 8000, .6f)
             .blown (1, .4f, .3f, .05f, .4f).gate (2, 2, 15, .7f, .1f));

        // =============================== SPLITS ===============================
        add ("Sonar Over 808", CAT_SPLIT, PB().add (sonarPing).add (wideClip).s (splitNote, 48).s (q8Lvl, .6f).s (synthLvl, .75f).clip (0, 2.f)
             .delay (5, 5, .55f, .5f, .2f, .38f).reverb (9, .75f, .45f, 0, .3f));
        add ("Bubble Quake", CAT_SPLIT, PB().add (bubbleMachine).add (wobble).s (splitNote, 48).s (q8WobDiv, 7).s (q8WobTarget, 0).s (q8WobDepth, .5f).s (q8Lvl, .6f)
             .s (synthLvl, .6f).delay (5, 1, .35f, .55f, .15f, .25f).reverb (9, .5f, .45f, 0, .25f));
        add ("Haunted Drop", CAT_SPLIT, PB().add (wraithPad).s (splitNote, 48).s (synthLvl, .7f).s (q8Punch, 17).s (q8Phase, 60).s (q8DrvMode, 3).s (q8Drive, .6f)
             .s (q8SubKeep, .5f).s (q8Fm, .2f).s (q8GrowlAmt, .35f).s (q8WobOn, 1).s (q8WobDiv, 7).s (q8WobShape, 6).s (q8WobTarget, 5).s (q8WobDepth, .7f)
             .stutter (0, 2, .3f, .25f, .15f, .8f).tapestop (1, 2, .2f, .3f, 0).delay (5, 7, .45f, .55f, .15f, .2f).reverb (9, .95f, .45f, .45f, .55f));
        add ("Dungeon Split", CAT_SPLIT, PB().add (organ).add (dungeon).s (splitNote, 48).ott (0, .3f).chorus (5, .3f, .4f, 3, .8f, .45f).reverb (9, .75f, .45f, 0, .3f));
        add ("Darkie Split", CAT_SPLIT, PB().add (darkiePluck).add (wideClip).s (splitNote, 48).clip (0, 2.f).delay (5, 5, .45f, .55f, .2f, .3f).reverb (9, .7f, .45f, 0, .3f));
        add ("Raw DVD Split", CAT_SPLIT, PB().add (toyKeys).add (rawDvd).s (splitNote, 48).crush (0, 10, 20000, 0, 0, .35f).tape (5, .25f, .3f, .4f, 0)
             .reverb (9, .5f, .45f, 0, .15f));
        add ("Sleep Deprived Split", CAT_SPLIT, PB().add (eerieWhistle).add (sleep).s (splitNote, 48).tapestop (0, 1, 0, .2f, 0).delay (5, 6, .35f, .5f, .2f, .2f)
             .reverb (9, .85f, .4f, .35f, .35f));
        add ("Multi-Dim Split", CAT_SPLIT, PB().add (grainPad).add (multiDim).s (splitNote, 48).tapestop (0, 2, 0, .3f, 0).grains (5, 200, 20, .5f, .6f, .5f)
             .reverb (9, .9f, .45f, .2f, .4f));
        return P;
    }();
    return presets;
}

inline void applyPreset (Params& p, const Preset& pr)
{
    p.resetToDefaults();
    for (auto& kv : pr.values) p.v[kv.first] = kv.second;
}
} // namespace wc
