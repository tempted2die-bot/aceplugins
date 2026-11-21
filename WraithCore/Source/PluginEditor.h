#pragma once
#include "PluginProcessor.h"
#include "UI/Industrial.h"
#include "UI/Pages.h"

class MainView : public juce::Component, private juce::Timer
{
public:
    static constexpr int W = 1280, H = 860;
    explicit MainView (WraithCoreProcessor&);
    ~MainView() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void showPage (int index);
    void showBrowser (bool show);
    std::function<void (float)> onScale;
    void setScaleChoice (float s)
    {
        static const float v[] = { 0.7f, 0.85f, 1.f, 1.15f, 1.3f };
        int best = 2; for (int i = 0; i < 5; ++i) if (std::abs (v[i] - s) < std::abs (v[best] - s)) best = i;
        scaleBox.setSelectedId (best + 1, juce::dontSendNotification);
    }

private:
    void timerCallback() override;
    void savePreset();

    WraithCoreProcessor& proc;
    juce::TextButton tabs[4];
    ui::GridPage synthPage, quakePage;
    ui::RackPage rackPage;
    ui::ModPage modPage;
    ui::BrowserOverlay browser;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, browseBtn { "BROWSE" }, saveBtn { "SAVE" }, mutateBtn { "MUTATE" };
    ui::Scope scope;
    ui::LedMeter meter;
    std::unique_ptr<ui::ParamControl> macroKnobs[4], q8Level, synthLevel, split, width, output, limiter;
    juce::ComboBox scaleBox;
    juce::Rectangle<int> presetArea, logoArea;
    juce::Random rnd;
    float glitch = 0.f;
    int glitchFrames = 0, lastPresetCounter = -1, page = 0;
    juce::TooltipWindow tooltips { this, 700 };
};

class WraithCoreEditor : public juce::AudioProcessorEditor
{
public:
    explicit WraithCoreEditor (WraithCoreProcessor&);
    ~WraithCoreEditor() override;
    void resized() override;
    MainView& getView() { return view; }

private:
    void applyScale (float s);
    WraithCoreProcessor& proc;
    ui::IndustrialLnF lnf;
    MainView view;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WraithCoreEditor)
};
