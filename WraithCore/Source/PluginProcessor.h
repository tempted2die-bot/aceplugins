#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/Engine.h"
#include "DSP/Presets.h"
#include <array>
#include <atomic>

class WraithCoreProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    WraithCoreProcessor();
    ~WraithCoreProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---------- presets ----------
    juce::String getPresetName() const { return presetName; }
    juce::String getPresetCategory() const { return presetCategory; }
    static juce::File userPresetFolder();
    juce::Array<juce::File> userPresets() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& file);
    std::atomic<int> presetChangeCounter { 0 };

    // ---------- FX rack editing (message thread) ----------
    void setSlotType (int slot, int type);
    void swapSlots (int a, int b);
    int slotType (int slot) const;
    int activeFxType (int slot) const { return engine.activeFxType (slot); }

    void mutate();
    void setParam (int index, float realValue);
    float getParam (int index) const { return raw[(size_t) index]->load(); }

    // ---------- meters / scope ----------
    static constexpr int kScopeSize = 2048;
    std::array<std::atomic<float>, kScopeSize> scope;
    std::atomic<int> scopeWrite { 0 };
    std::atomic<float> meterL { 0.f }, meterR { 0.f };
    float modSourceValue (int s) const { return engine.srcMonitor[(size_t) s].load (std::memory_order_relaxed); }

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override { syncModules(); }
    void syncModules();
    void applyValues (const wc::Params& p);

    wc::Engine engine;
    std::array<std::atomic<float>*, wc::NUM_PARAMS> raw {};
    juce::AudioBuffer<float> scratch;
    std::vector<wc::MidiEv> events;
    int currentProgram = 0;
    juce::String presetName { "Init" }, presetCategory { "808 + SYNTH" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WraithCoreProcessor)
};
