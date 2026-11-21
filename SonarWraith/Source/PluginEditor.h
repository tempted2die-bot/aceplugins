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
    explicit Scope (SonarWraithProcessor& p) : proc (p) { startTimerHz (30); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { repaint(); }
    SonarWraithProcessor& proc;
    std::array<float, 512> snap {};
};

//==============================================================================
class SonarWraithEditor : public juce::AudioProcessorEditor
{
public:
    explicit SonarWraithEditor (SonarWraithProcessor&);
    ~SonarWraithEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SonarWraithProcessor& proc;
    WraithLookAndFeel lnf;
    juce::ComboBox presetBox;
    juce::TextButton mutateButton { "MUTATE" };
    Scope scope;
    juce::OwnedArray<Panel> row1, row2, row3;
    juce::TooltipWindow tooltips { this, 700 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonarWraithEditor)
};
