// Headless plugin test: presets, rack editing, user presets, state, mutate + GUI snapshots of every page.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <iostream>

static float runBlocks (WraithCoreProcessor& proc, int blocks, int noteBase = 36)
{
    juce::AudioBuffer<float> buf (2, 512);
    float peak = 0.f;
    for (int b = 0; b < blocks; ++b)
    {
        juce::MidiBuffer midi;
        if (b % 24 == 0)  { midi.addEvent (juce::MidiMessage::noteOn (1, noteBase, (juce::uint8) 100), 17);
                            midi.addEvent (juce::MidiMessage::noteOn (1, 62, (juce::uint8) 90), 300);
                            midi.addEvent (juce::MidiMessage::noteOn (1, 65, (juce::uint8) 90), 310); }
        if (b % 24 == 14) { midi.addEvent (juce::MidiMessage::noteOff (1, noteBase), 5);
                            midi.addEvent (juce::MidiMessage::noteOff (1, 62), 5);
                            midi.addEvent (juce::MidiMessage::noteOff (1, 65), 5); }
        proc.processBlock (buf, midi);
        peak = juce::jmax (peak, buf.getMagnitude (0, 512));
    }
    return peak;
}

static void snap (juce::Component& c, const juce::File& f)
{
    auto img = c.createComponentSnapshot (c.getLocalBounds(), true, 1.0f);
    f.deleteFile();
    juce::FileOutputStream out (f);
    juce::PNGImageFormat().writeImageToStream (img, out);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    WraithCoreProcessor proc;
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);
    int failures = 0;
    auto check = [&] (bool ok, const juce::String& what) { std::cout << (ok ? "OK   " : "FAIL ") << what << "\n"; if (! ok) ++failures; };

    for (int i = 0; i < proc.getNumPrograms(); ++i)
    {
        proc.setCurrentProgram (i);
        runBlocks (proc, 2);                // module hand-over happens here
        const float pk = runBlocks (proc, 60);
        if (! (pk > 0.005f && pk <= 1.001f && std::isfinite (pk))) { std::cout << "FAIL preset " << proc.getProgramName (i) << " peak " << pk << "\n"; ++failures; }
    }
    check (true, juce::String (proc.getNumPrograms()) + " factory presets rendered");

    // rack editing
    proc.setCurrentProgram (0);
    proc.setSlotType (0, wc::FX_DATAMOSH);
    proc.setSlotType (6, wc::FX_FREQSHIFT);
    proc.setParam (wc::mtxId (7, wc::MX_SRC), wc::MS_LFOA);
    proc.setParam (wc::mtxId (7, wc::MX_DST), (float) wc::targetForSlot (0, wc::SF_P0));
    proc.setParam (wc::mtxId (7, wc::MX_AMT), 0.5f);
    runBlocks (proc, 4);
    check (proc.activeFxType (0) == wc::FX_DATAMOSH && proc.activeFxType (6) == wc::FX_FREQSHIFT, "modules created + handed to audio thread");
    proc.swapSlots (0, 3);
    runBlocks (proc, 4);
    check (proc.slotType (3) == wc::FX_DATAMOSH && proc.slotType (0) == wc::FX_NONE, "slot move");
    check (juce::roundToInt (proc.getParam (wc::mtxId (7, wc::MX_DST))) == wc::targetForSlot (3, wc::SF_P0), "mod routing follows moved module");
    const float pk = runBlocks (proc, 80);
    check (pk > 0.005f && pk <= 1.001f, "audio after rack edits (peak " + juce::String (pk, 3) + ")");

    // user preset round trip
    check (proc.saveUserPreset ("zz_test_patch"), "save user preset");
    proc.setCurrentProgram (5);
    auto f = WraithCoreProcessor::userPresetFolder().getChildFile ("zz_test_patch.wcpreset");
    check (proc.loadUserPreset (f) && proc.slotType (3) == wc::FX_DATAMOSH, "load user preset");
    f.deleteFile();

    // host state round trip
    proc.setCurrentProgram (12);
    juce::MemoryBlock mb; proc.getStateInformation (mb);
    const float before = proc.getParam (wc::slotId (0, wc::SF_TYPE));
    proc.setCurrentProgram (0);
    proc.setStateInformation (mb.getData(), (int) mb.getSize());
    check (std::abs (proc.getParam (wc::slotId (0, wc::SF_TYPE)) - before) < 0.01f && proc.getPresetName() == proc.getProgramName (12), "host state round trip");

    for (int m = 0; m < 30; ++m)
    {
        proc.mutate(); runBlocks (proc, 2);
        const float p2 = runBlocks (proc, 30);
        if (! std::isfinite (p2) || p2 > 1.001f) { ++failures; std::cout << "FAIL mutate " << m << "\n"; }
    }
    check (true, "mutate x30");

    // GUI snapshots
    const juce::File dir (argc > 1 ? argv[1] : "/tmp");
    proc.setCurrentProgram (argc > 2 ? juce::String (argv[2]).getIntValue() : 102);
    runBlocks (proc, 40);
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        auto* wce = dynamic_cast<WraithCoreEditor*> (ed.get());
        auto& v = wce->getView();
        const char* names[] = { "synth", "808", "rack", "mod" };
        for (int pg = 0; pg < 4; ++pg) { v.showPage (pg); snap (*ed, dir.getChildFile (juce::String ("wc_") + names[pg] + ".png")); }
        v.showPage (2); v.showBrowser (true);
        snap (*ed, dir.getChildFile ("wc_browser.png"));
        std::cout << "GUI snapshots written (" << ed->getWidth() << "x" << ed->getHeight() << ")\n";
    }
    std::cout << (failures == 0 ? "ALL PASSED" : "FAILURES: " + std::to_string (failures)) << "\n";
    return failures;
}
