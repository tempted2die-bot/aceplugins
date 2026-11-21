// Headless plugin test: presets, audio, state round-trip, mutate, and a GUI snapshot.
// Built only when configuring with -DSW_BUILD_TESTS=ON
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <iostream>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    DepthChargeProcessor proc;
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);
    int failures = 0;

    auto run = [&proc] (int blocks)
    {
        juce::AudioBuffer<float> buf (2, 512);
        float peak = 0.f;
        for (int b = 0; b < blocks; ++b)
        {
            juce::MidiBuffer midi;
            if (b % 20 == 0)  { midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 17);
                                midi.addEvent (juce::MidiMessage::noteOn (1, 67, (juce::uint8) 90), 300); }
            if (b % 20 == 12) { midi.addEvent (juce::MidiMessage::noteOff (1, 60), 5);
                                midi.addEvent (juce::MidiMessage::noteOff (1, 67), 5); }
            proc.processBlock (buf, midi);
            peak = juce::jmax (peak, buf.getMagnitude (0, 512));
        }
        return peak;
    };

    for (int i = 0; i < proc.getNumPrograms(); ++i)
    {
        proc.setCurrentProgram (i);
        const float pk = run (120);
        const bool ok = pk > 0.01f && pk < 1.6f && std::isfinite (pk);
        failures += ok ? 0 : 1;
        std::cout << (ok ? "OK   " : "FAIL ") << proc.getProgramName (i) << "  peak " << pk << "\n";
    }

    for (int m = 0; m < 25; ++m)
    {
        proc.mutate();
        const float pk = run (40);
        if (! std::isfinite (pk) || pk > 1.6f) { ++failures; std::cout << "FAIL mutate " << m << " peak " << pk << "\n"; }
    }
    std::cout << "mutate x25 done\n";

    proc.setCurrentProgram (1);
    juce::MemoryBlock mb;
    proc.getStateInformation (mb);
    const float before = proc.apvts.getRawParameterValue ("q8Phase")->load();
    proc.setCurrentProgram (0);
    proc.setStateInformation (mb.getData(), (int) mb.getSize());
    const float after = proc.apvts.getRawParameterValue ("q8Phase")->load();
    const bool stateOk = std::abs (before - after) < 1e-4f && before > 0.f;
    failures += stateOk ? 0 : 1;
    std::cout << (stateOk ? "OK   " : "FAIL ") << "state round-trip (" << before << " -> " << after << ")\n";

    proc.setCurrentProgram (argc > 2 ? juce::String (argv[2]).getIntValue() : 8);
    run (60);
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f (argc > 1 ? argv[1] : "/tmp/dc_gui.png");
        f.deleteFile();
        { juce::FileOutputStream out (f); juce::PNGImageFormat().writeImageToStream (img, out); }
        std::cout << "GUI snapshot " << img.getWidth() << "x" << img.getHeight() << " -> " << f.getFullPathName() << "\n";
        if (auto* dc = dynamic_cast<DepthChargeEditor*> (ed.get()))
        {
            dc->showPage (1);
            auto img2 = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            juce::File f2 (f.getSiblingFile (f.getFileNameWithoutExtension() + "_synth.png"));
            f2.deleteFile();
            juce::FileOutputStream out2 (f2); juce::PNGImageFormat().writeImageToStream (img2, out2);
        }
    }
    std::cout << (failures == 0 ? "ALL PASSED" : "FAILURES") << "\n";
    return failures;
}
