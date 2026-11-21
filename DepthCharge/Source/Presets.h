// DepthCharge - factory presets (values in real parameter units; unspecified params = default)
#pragma once
#include "ParamDefs.h"
#include <vector>
#include <utility>
#include <initializer_list>

namespace sw
{
using PV = std::vector<std::pair<int, float>>;

struct Preset
{
    const char* name;
    PV values;
};

inline PV cat (std::initializer_list<PV> parts)
{
    PV out;
    for (auto& p : parts) out.insert (out.end(), p.begin(), p.end());
    return out;
}

inline const std::vector<Preset>& factoryPresets()
{
    using namespace pid;

    // ---------- reusable synth layers ----------
    static const PV sonarPing = {
        { oscWave, 0.f }, { pingAmt, 24.f }, { pingDecay, 38.f },
        { atk, 0.f }, { dec, 650.f }, { sus, 0.f }, { rel, 450.f },
        { fltCut, 6500.f }, { fltRes, 0.3f },
        { dlyOn, 1 }, { dlyDiv, 5 }, { dlyFb, 0.55f }, { dlyMix, 0.38f }, { dlyWarble, 0.2f }, { dlyTone, 0.5f },
        { revOn, 1 }, { revSize, 0.75f }, { revMix, 0.3f } };
    static const PV bubbleMachine = {
        { oscWave, 0.2f }, { pingAmt, 14.f }, { pingDecay, 25.f },
        { bubRate, 9.f }, { bubChance, 0.75f }, { bubRand, 12.f }, { bubGate, 0.85f },
        { fltCut, 2200.f }, { fltRes, 0.6f }, { fltEnv, 0.4f }, { fltDecay, 70.f },
        { atk, 2.f }, { sus, 1.f }, { rel, 600.f }, { voiceVol, 0.9f },
        { dlyOn, 1 }, { dlyDiv, 1 }, { dlyFb, 0.35f }, { dlyMix, 0.25f },
        { revOn, 1 }, { revSize, 0.5f }, { revMix, 0.25f } };
    static const PV wraithPad = {
        { oscWave, 0.66f }, { uniVoices, 4 }, { uniDetune, 0.35f }, { oscFold, 0.1f },
        { fltCut, 1800.f }, { fltRes, 0.25f }, { lfoShape, 5 }, { lfoRate, 0.3f }, { lfoCut, 0.3f },
        { atk, 900.f }, { dec, 2000.f }, { sus, 0.9f }, { rel, 3000.f }, { voiceVol, 0.5f },
        { glOn, 1 }, { glDiv, 2 }, { glChance, 0.15f }, { glMix, 0.8f },
        { dlyOn, 1 }, { dlyDiv, 7 }, { dlyMix, 0.2f },
        { revOn, 1 }, { revSize, 0.95f }, { revShimmer, 0.45f }, { revMix, 0.55f } };
    static const PV glitchBell = {
        { oscWave, 0.f }, { fmRatio, 3.5f }, { fmAmt, 0.45f }, { pingAmt, 12.f }, { pingDecay, 15.f },
        { atk, 0.f }, { dec, 900.f }, { sus, 0.f }, { rel, 600.f },
        { crsOn, 1 }, { crsBits, 9.f }, { crsRate, 18000.f }, { crsMix, 0.6f },
        { glOn, 1 }, { glDiv, 3 }, { glChance, 0.45f }, { glRev, 0.3f }, { glPitch, 0.3f },
        { dlyOn, 1 }, { dlyDiv, 4 }, { dlyFb, 0.4f }, { dlyMix, 0.25f },
        { revOn, 1 }, { revSize, 0.6f }, { revMix, 0.25f } };
    static const PV underwaterRadar = {
        { oscWave, 0.f }, { oscFold, 0.35f }, { pingAmt, 7.f }, { pingDecay, 300.f },
        { fltCut, 900.f }, { fltRes, 0.85f }, { fltEnv, 0.5f }, { fltDecay, 400.f },
        { lfoShape, 4 }, { lfoSync, 1 }, { lfoDiv, 4 }, { lfoCut, 0.35f },
        { atk, 1.f }, { dec, 1500.f }, { sus, 0.3f }, { rel, 1500.f }, { voiceVol, 0.45f },
        { dlyOn, 1 }, { dlyDiv, 6 }, { dlyFb, 0.7f }, { dlyWarble, 0.45f }, { dlyTone, 0.3f }, { dlyMix, 0.4f },
        { revOn, 1 }, { revSize, 0.9f }, { revDamp, 0.7f }, { revMix, 0.45f } };
    static const PV hauntedArp = {
        { oscWave, 0.33f }, { fmAmt, 0.2f }, { fmRatio, 7.f }, { pingAmt, 19.f }, { pingDecay, 18.f },
        { bubRate, 12.f }, { bubChance, 0.55f }, { bubRand, 24.f }, { bubGate, 1.f },
        { atk, 1.f }, { sus, 1.f }, { rel, 400.f }, { voiceVol, 0.6f },
        { drvOn, 1 }, { drvMode, 2 }, { drvAmt, 0.45f }, { drvMix, 0.5f },
        { crsOn, 1 }, { crsBits, 8.f }, { crsRate, 12000.f }, { crsMix, 0.5f },
        { glOn, 1 }, { glDiv, 2 }, { glChance, 0.5f }, { glRev, 0.4f }, { glPitch, 0.3f },
        { dlyOn, 1 }, { dlyDiv, 5 }, { dlyFb, 0.5f }, { dlyMix, 0.3f },
        { revOn, 1 }, { revSize, 0.7f }, { revShimmer, 0.3f }, { revMix, 0.35f } };

    // ---------- reusable 808 layers ----------
    static const PV wideClip808 = {   // fitted to the reference: +17 st drop over 100 ms, clipped, ~60 deg wide
        { q8Punch, 17.f }, { q8PunchTime, 100.f }, { q8Shape, 0.f }, { q8Click, 0.1f }, { q8Phase, 60.f },
        { q8DrvMode, 0 }, { q8Drive, 0.6f }, { q8Fold, 0.f }, { q8Asym, 0.35f }, { q8Tone, 0.55f },
        { q8Knock, 0.3f }, { q8SubKeep, 0.1f },
        { q8Mode, 2 }, { q8Glide, 60.f }, { q8Sus, 1.f } };
    static const PV wobble808 = {
        { q8Punch, 15.f }, { q8PunchTime, 110.f }, { q8Shape, 0.3f }, { q8Phase, 40.f },
        { q8DrvMode, 3 }, { q8Drive, 0.5f }, { q8Asym, 0.2f }, { q8SubKeep, 0.5f },
        { q8Cut, 6000.f }, { q8Res, 0.35f }, { q8Fm, 0.25f }, { q8FmRatio, 1.f }, { q8GrowlAmt, 0.3f },
        { q8WobOn, 1 }, { q8WobDiv, 4 }, { q8WobShape, 0 }, { q8WobTarget, 5 }, { q8WobDepth, 0.75f } };

    static const std::vector<Preset> presets = {
        { "Init", {} },

        // ===== 808 FIRST =====
        { "Wide Clip 808", cat ({ wideClip808, { { synthLvl, 0.f }, { splitNote, 0.f } } }) },

        { "Wobble Wraith 808", cat ({ wobble808, { { synthLvl, 0.f }, { splitNote, 0.f },
            { revOn, 1 }, { revSize, 0.5f }, { revMix, 0.15f }, { fxLowCut, 200.f }, { q8Lvl, 0.9f }, { outGain, 3.f } } }) },

        { "Yoi Talker 808", { { synthLvl, 0.f }, { splitNote, 0.f },
            { q8Punch, 12.f }, { q8PunchTime, 88.f }, { q8Shape, 0.2f }, { q8Phase, 30.f },
            { q8DrvMode, 1 }, { q8Drive, 0.6f }, { q8Asym, 0.4f }, { q8SubKeep, 0.5f },
            { q8Fm, 0.15f }, { q8FmRatio, 2.f }, { q8Vowel, 0.2f }, { q8GrowlRes, 0.7f },
            { q8WobOn, 1 }, { q8WobDiv, 2 }, { q8WobShape, 5 }, { q8WobTarget, 1 }, { q8WobDepth, 0.9f } } },

        { "Depth Charge", { { synthLvl, 0.f }, { splitNote, 0.f },
            { q8Punch, 36.f }, { q8PunchTime, 154.f }, { q8Click, 0.6f }, { q8Phase, 60.f },
            { q8DrvMode, 4 }, { q8Drive, 0.5f }, { q8Fold, 0.3f }, { q8SubKeep, 0.45f }, { q8Tone, 0.8f },
            { glOn, 1 }, { glDiv, 1 }, { glChance, 0.15f }, { glGap, 0.25f }, { glStop, 0.3f }, { glMix, 1.f },
            { revOn, 1 }, { revSize, 0.4f }, { revMix, 0.12f } } },

        { "Rattle Snake 808", cat ({ wideClip808, { { synthLvl, 0.f }, { splitNote, 0.f }, { q8Phase, 70.f },
            { q8WobOn, 1 }, { q8WobDiv, 8 }, { q8WobShape, 4 }, { q8WobTarget, 4 }, { q8WobDepth, 0.08f },
            { glOn, 1 }, { glDiv, 2 }, { glChance, 0.1f }, { glGap, 0.15f } } }) },

        { "Glide Bully 808", { { synthLvl, 0.f }, { splitNote, 0.f },
            { q8Punch, 19.f }, { q8PunchTime, 121.f }, { q8Shape, 0.25f }, { q8Phase, 45.f },
            { q8DrvMode, 1 }, { q8Drive, 0.75f }, { q8Asym, 0.5f }, { q8Tone, 0.65f }, { q8SubKeep, 0.4f },
            { q8Mode, 2 }, { q8Glide, 160.f }, { subMono, 1 } } },

        { "Screamer 808", { { synthLvl, 0.f }, { splitNote, 0.f },
            { q8Punch, 24.f }, { q8PunchTime, 77.f }, { q8Phase, 50.f },
            { q8DrvMode, 3 }, { q8Drive, 0.7f }, { q8SubKeep, 0.55f }, { q8Cut, 9000.f }, { q8Res, 0.3f },
            { q8Fm, 0.5f }, { q8FmRatio, 3.f }, { q8GrowlAmt, 0.4f }, { q8Vowel, 0.8f },
            { q8WobOn, 1 }, { q8WobDiv, 5 }, { q8WobShape, 2 }, { q8WobTarget, 2 }, { q8WobDepth, 0.6f },
            { q8Lvl, 0.9f }, { outGain, 3.f } } },

        // ===== 808 + SYNTH SPLITS (808 below C4 in FL / MIDI 48, synth above) =====
        { "Sonar Over 808", cat ({ sonarPing, wideClip808, { { splitNote, 48.f }, { q8Lvl, 0.6f }, { synthLvl, 0.75f } } }) },

        { "Bubble Quake", cat ({ bubbleMachine, wobble808, { { splitNote, 48.f }, { q8WobDiv, 7 }, { q8WobTarget, 0 },
            { q8WobDepth, 0.5f }, { q8Lvl, 0.6f }, { synthLvl, 0.6f } } }) },

        { "Haunted Drop", cat ({ wraithPad, { { splitNote, 48.f }, { synthLvl, 0.7f },
            { q8Punch, 17.f }, { q8Phase, 60.f }, { q8DrvMode, 3 }, { q8Drive, 0.6f }, { q8SubKeep, 0.5f },
            { q8Fm, 0.2f }, { q8GrowlAmt, 0.35f },
            { q8WobOn, 1 }, { q8WobDiv, 7 }, { q8WobShape, 6 }, { q8WobTarget, 5 }, { q8WobDepth, 0.7f },
            { glChance, 0.3f }, { glGap, 0.3f }, { glStop, 0.2f } } }) },

        // ===== SYNTH ONLY (808 off) =====
        { "Sonar Ping (synth)",       cat ({ sonarPing,       { { q8On, 0 }, { splitNote, 0.f } } }) },
        { "Bubble Machine (synth)",   cat ({ bubbleMachine,   { { q8On, 0 }, { splitNote, 0.f } } }) },
        { "Wraith Pad (synth)",       cat ({ wraithPad,       { { q8On, 0 }, { splitNote, 0.f } } }) },
        { "Glitch Bell (synth)",      cat ({ glitchBell,      { { q8On, 0 }, { splitNote, 0.f } } }) },
        { "Underwater Radar (synth)", cat ({ underwaterRadar, { { q8On, 0 }, { splitNote, 0.f } } }) },
        { "Haunted Arp Chaos (synth)",cat ({ hauntedArp,      { { q8On, 0 }, { splitNote, 0.f } } }) },
    };
    return presets;
}

inline void applyPreset (Params& p, const Preset& pr)
{
    p.resetToDefaults();
    for (auto& kv : pr.values) p.v[kv.first] = kv.second;
}
} // namespace sw
