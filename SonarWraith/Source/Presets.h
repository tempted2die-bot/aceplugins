// SonarWraith - factory presets (values are in real parameter units; unspecified params = default)
#pragma once
#include "ParamDefs.h"
#include <vector>
#include <utility>

namespace sw
{
struct Preset
{
    const char* name;
    std::vector<std::pair<int, float>> values;
};

inline const std::vector<Preset>& factoryPresets()
{
    using namespace pid;
    static const std::vector<Preset> presets = {
        { "Init", {} },

        { "Sonar Ping", {
            { oscWave, 0.f }, { pingAmt, 24.f }, { pingDecay, 38.f },
            { atk, 0.f }, { dec, 650.f }, { sus, 0.f }, { rel, 450.f },
            { fltCut, 6500.f }, { fltRes, 0.3f },
            { dlyOn, 1 }, { dlyDiv, 5 }, { dlyFb, 0.55f }, { dlyMix, 0.38f }, { dlyWarble, 0.2f }, { dlyTone, 0.5f },
            { revOn, 1 }, { revSize, 0.75f }, { revMix, 0.3f } } },

        { "Bubble Machine", {
            { oscWave, 0.2f }, { pingAmt, 14.f }, { pingDecay, 25.f },
            { bubRate, 9.f }, { bubChance, 0.75f }, { bubRand, 12.f }, { bubGate, 0.85f },
            { fltCut, 2200.f }, { fltRes, 0.6f }, { fltEnv, 0.4f }, { fltDecay, 70.f },
            { atk, 2.f }, { sus, 1.f }, { rel, 600.f }, { voiceVol, 0.9f },
            { dlyOn, 1 }, { dlyDiv, 1 }, { dlyFb, 0.35f }, { dlyMix, 0.25f },
            { revOn, 1 }, { revSize, 0.5f }, { revMix, 0.25f } } },

        { "Wraith Pad", {
            { oscWave, 0.66f }, { uniVoices, 4 }, { uniDetune, 0.35f }, { oscFold, 0.1f },
            { fltCut, 1800.f }, { fltRes, 0.25f }, { lfoShape, 5 }, { lfoRate, 0.3f }, { lfoCut, 0.3f },
            { atk, 900.f }, { dec, 2000.f }, { sus, 0.9f }, { rel, 3000.f }, { voiceVol, 0.5f },
            { glOn, 1 }, { glDiv, 2 }, { glChance, 0.15f }, { glMix, 0.8f },
            { dlyOn, 1 }, { dlyDiv, 7 }, { dlyMix, 0.2f },
            { revOn, 1 }, { revSize, 0.95f }, { revShimmer, 0.45f }, { revMix, 0.55f } } },

        { "Glitch Bell", {
            { oscWave, 0.f }, { fmRatio, 3.5f }, { fmAmt, 0.45f }, { pingAmt, 12.f }, { pingDecay, 15.f },
            { atk, 0.f }, { dec, 900.f }, { sus, 0.f }, { rel, 600.f },
            { crsOn, 1 }, { crsBits, 9.f }, { crsRate, 18000.f }, { crsMix, 0.6f },
            { glOn, 1 }, { glDiv, 3 }, { glChance, 0.45f }, { glRev, 0.3f }, { glPitch, 0.3f },
            { dlyOn, 1 }, { dlyDiv, 4 }, { dlyFb, 0.4f }, { dlyMix, 0.25f },
            { revOn, 1 }, { revSize, 0.6f }, { revMix, 0.25f } } },

        { "Blown Sonar Bass", {
            { oscWave, 0.f }, { oscOct, -1 }, { subLvl, 0.6f }, { pingAmt, 36.f }, { pingDecay, 30.f },
            { polyMode, 1 }, { glide, 80.f },
            { atk, 1.f }, { dec, 800.f }, { sus, 0.85f }, { rel, 200.f },
            { fltCut, 1200.f }, { fltRes, 0.15f },
            { drvOn, 1 }, { drvMode, 4 }, { drvAmt, 0.7f }, { drvTone, 0.5f }, { outGain, -4.f } } },

        { "Crushed Blips", {
            { oscWave, 1.f }, { oscOct, 1 }, { pingAmt, -24.f }, { pingDecay, 20.f },
            { atk, 0.f }, { dec, 120.f }, { sus, 0.f }, { rel, 120.f }, { voiceVol, 0.55f },
            { crsOn, 1 }, { crsBits, 5.f }, { crsRate, 4000.f }, { crsMix, 0.9f },
            { drvOn, 1 }, { drvMode, 1 }, { drvAmt, 0.4f },
            { dlyOn, 1 }, { dlyDiv, 1 }, { dlyFb, 0.5f }, { dlyMix, 0.3f },
            { revOn, 1 }, { revSize, 0.4f }, { revMix, 0.15f }, { outGain, -2.f } } },

        { "Underwater Radar", {
            { oscWave, 0.f }, { oscFold, 0.35f }, { pingAmt, 7.f }, { pingDecay, 300.f },
            { fltCut, 900.f }, { fltRes, 0.85f }, { fltEnv, 0.5f }, { fltDecay, 400.f },
            { lfoShape, 4 }, { lfoSync, 1 }, { lfoDiv, 4 }, { lfoCut, 0.35f },
            { atk, 1.f }, { dec, 1500.f }, { sus, 0.3f }, { rel, 1500.f }, { voiceVol, 0.45f },
            { dlyOn, 1 }, { dlyDiv, 6 }, { dlyFb, 0.7f }, { dlyWarble, 0.45f }, { dlyTone, 0.3f }, { dlyMix, 0.4f },
            { revOn, 1 }, { revSize, 0.9f }, { revDamp, 0.7f }, { revMix, 0.45f } } },

        { "Haunted Arp Chaos", {
            { oscWave, 0.33f }, { fmAmt, 0.2f }, { fmRatio, 7.f }, { pingAmt, 19.f }, { pingDecay, 18.f },
            { bubRate, 12.f }, { bubChance, 0.55f }, { bubRand, 24.f }, { bubGate, 1.f },
            { atk, 1.f }, { sus, 1.f }, { rel, 400.f }, { voiceVol, 0.6f },
            { drvOn, 1 }, { drvMode, 2 }, { drvAmt, 0.45f }, { drvMix, 0.5f },
            { crsOn, 1 }, { crsBits, 8.f }, { crsRate, 12000.f }, { crsMix, 0.5f },
            { glOn, 1 }, { glDiv, 2 }, { glChance, 0.5f }, { glRev, 0.4f }, { glPitch, 0.3f },
            { dlyOn, 1 }, { dlyDiv, 5 }, { dlyFb, 0.5f }, { dlyMix, 0.3f },
            { revOn, 1 }, { revSize, 0.7f }, { revShimmer, 0.3f }, { revMix, 0.35f } } },

        { "Rectified Ghost Lead", {
            { oscWave, 0.5f }, { uniVoices, 2 }, { uniDetune, 0.15f }, { pingAmt, 5.f }, { pingDecay, 60.f },
            { polyMode, 2 }, { glide, 120.f }, { lfoPitch, 0.3f }, { lfoRate, 5.5f },
            { atk, 5.f }, { sus, 0.9f }, { rel, 350.f }, { fltCut, 4000.f }, { fltRes, 0.35f },
            { drvOn, 1 }, { drvMode, 3 }, { drvAmt, 0.55f }, { drvTone, 0.6f }, { drvMix, 0.8f },
            { dlyOn, 1 }, { dlyDiv, 5 }, { dlyMix, 0.25f },
            { revOn, 1 }, { revSize, 0.8f }, { revShimmer, 0.2f }, { revMix, 0.35f } } },
    };
    return presets;
}

inline void applyPreset (Params& p, const Preset& pr)
{
    p.resetToDefaults();
    for (auto& kv : pr.values) p.v[kv.first] = kv.second;
}
} // namespace sw
