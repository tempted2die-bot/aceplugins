#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "SonarEngine.h"
#include <array>
#include <atomic>

class SonarWraithProcessor : public juce::AudioProcessor
{
public:
    SonarWraithProcessor();
    ~SonarWraithProcessor() override = default;

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
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // randomise the patch inside "dark / glitchy / bubbly" musical ranges
    void mutate();

    juce::AudioProcessorValueTreeState apvts;

    // lock-free oscilloscope feed for the editor
    static constexpr int kScopeSize = 2048;
    std::array<std::atomic<float>, kScopeSize> scope;
    std::atomic<int> scopeWrite { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (int index, float realValue);

    sw::Engine engine;
    std::array<std::atomic<float>*, sw::pid::COUNT> raw {};
    juce::AudioBuffer<float> scratch;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonarWraithProcessor)
};
