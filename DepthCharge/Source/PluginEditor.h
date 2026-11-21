#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

//==============================================================================
class WraithLookAndFeel : public juce::LookAndFeel_V4
{
public:
    WraithLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (12.f, juce::Font::bold)); }
    juce::Font getPopupMenuFont() override { return juce::Font (juce::FontOptions (13.f)); }
};

//==============================================================================
// One knob / combo / toggle bound to a parameter, with a label underneath
class ParamControl : public juce::Component
{
public:
    ParamControl (juce::AudioProcessorValueTreeState&, int paramIndex, juce::Colour accent);
    void resized() override;
    void paint (juce::Graphics&) override;
    void parentHierarchyChanged() override;

private:
    const sw::ParamDef& def;
    juce::Colour accent;
    juce::Slider slider;
    juce::ComboBox combo;
    juce::ToggleButton toggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bAtt;
};

//==============================================================================
class Panel : public juce::Component
{
public:
    Panel (juce::AudioProcessorValueTreeState&, const juce::String& title, juce::Colour accent,
           std::initializer_list<int> params, int rows, int powerParam = -1);
    void paint (juce::Graphics&) override;
    void resized() override;
    int columns() const { return (int) std::ceil ((float) controls.size() / (float) rows); }

private:
    juce::String title;
    juce::Colour accent;
    int rows;
    juce::OwnedArray<ParamControl> controls;
    juce::ToggleButton power;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAtt;
};

//==============================================================================
class Scope : public juce::Component, private juce::Timer
{
public:
    explicit Scope (DepthChargeProcessor& p) : proc (p) { startTimerHz (30); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { repaint(); }
    DepthChargeProcessor& proc;
    std::array<float, 512> snap {};
};

//==============================================================================
// A page of panel rows (rows split the height by weight, panels split a row by their column count)
class Page : public juce::Component
{
public:
    struct Row { juce::OwnedArray<Panel> panels; float weight = 1.f; };
    Row& addRow (float weight) { auto* r = rows.add (new Row()); r->weight = weight; return *r; }
    void add (Row& r, Panel* p) { r.panels.add (p); addAndMakeVisible (p); }
    void resized() override;
private:
    juce::OwnedArray<Row> rows;
};

class TabButton : public juce::Button
{
public:
    TabButton (const juce::String& name, juce::Colour c) : juce::Button (name), accent (c) { setClickingTogglesState (false); }
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
private:
    juce::Colour accent;
};

//==============================================================================
class DepthChargeEditor : public juce::AudioProcessorEditor
{
public:
    explicit DepthChargeEditor (DepthChargeProcessor&);
    ~DepthChargeEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void showPage (int index);

private:

    DepthChargeProcessor& proc;
    WraithLookAndFeel lnf;
    juce::ComboBox presetBox;
    juce::TextButton mutateButton { "MUTATE" };
    Scope scope;
    TabButton tab808 { "808 QUAKE", juce::Colour (0xffff7a1a) }, tabSynth { "SONAR SYNTH", juce::Colour (0xff35f2ff) };
    Page page808, pageSynth, fxRow;
    juce::TooltipWindow tooltips { this, 700 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DepthChargeEditor)
};
