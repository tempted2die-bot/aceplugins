// WRAITHCORE - industrial / glitch visual system: colours, fonts, LookAndFeel, base widgets
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../PluginProcessor.h"

namespace ui
{
namespace col
{
    const juce::Colour bg      { 0xff08090b };
    const juce::Colour plate   { 0xff15171b };
    const juce::Colour plateHi { 0xff1f2228 };
    const juce::Colour plateLo { 0xff0e1013 };
    const juce::Colour edge    { 0xff2c3138 };
    const juce::Colour groove  { 0xff050607 };
    const juce::Colour text    { 0xffd2d6db };
    const juce::Colour dim     { 0xff6d737c };
    const juce::Colour hazard  { 0xffffc21a };
    const juce::Colour orange  { 0xffff5a1f };
    const juce::Colour acid    { 0xffb6ff1c };
    const juce::Colour red     { 0xffff2b45 };
    const juce::Colour cyan    { 0xff27e1ff };
    const juce::Colour violet  { 0xffa66bff };
    const juce::Colour lcd     { 0xff060c07 };
    const juce::Colour lcdText { 0xffb6ff1c };
}

juce::Font display (float h);            // Anton (stencil-ish condensed)
juce::Font mono (float h);               // Anonymous Pro Bold
juce::Colour accentFor (int fxType);

void drawPlate (juce::Graphics&, juce::Rectangle<float>, juce::uint32 seed, bool screws = true, float corner = 4.f);
void drawScrew (juce::Graphics&, juce::Point<float> c, float r, float angle);
void drawHazard (juce::Graphics&, juce::Rectangle<float>, float alpha = 1.f, float stripe = 10.f);
void drawScanlines (juce::Graphics&, juce::Rectangle<float>, float alpha = 0.18f);
void drawLcd (juce::Graphics&, juce::Rectangle<float>);
void drawGlitchText (juce::Graphics&, const juce::String&, juce::Rectangle<int>, const juce::Font&, juce::Colour, juce::Justification, float glitch, juce::Random&);

//==============================================================================
class IndustrialLnF : public juce::LookAndFeel_V4
{
public:
    IndustrialLnF();
    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return mono (12.f); }
    juce::Font getPopupMenuFont() override { return mono (13.f); }
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawPopupMenuSectionHeader (juce::Graphics&, const juce::Rectangle<int>&, const juce::String&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton& b, int h) override
    {
        if (b.getProperties()["style"].toString() == "tab") return display (19.f);
        return display ((float) juce::jmin (16, h - 8));
    }
    juce::Font getSliderPopupFont (juce::Slider&) override { return mono (13.f); }
    void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbPos, int thumbSize, bool over, bool down) override;
};

//==============================================================================
// knob / combo / switch bound to one parameter, with label + live value readout
class ParamControl : public juce::Component
{
public:
    ParamControl (WraithCoreProcessor&, int paramIndex, juce::Colour accent, const juce::String& label = {});
    ~ParamControl() override;
    void resized() override;
    void paint (juce::Graphics&) override;
    void setLabel (const juce::String& s) { if (label != s) { label = s; repaint(); } }
    void setAccent (juce::Colour c);
    int paramIndex() const { return index; }
    bool showValue = true;

private:
    WraithCoreProcessor& proc;
    int index;
    const wc::ParamDef& def;
    juce::RangedAudioParameter* param = nullptr;
    juce::Colour accent;
    juce::String label;
    juce::Slider slider;
    juce::ComboBox combo;
    juce::ToggleButton toggle;
    std::unique_ptr<juce::SliderParameterAttachment> sAtt;
    std::unique_ptr<juce::ComboBoxParameterAttachment> cAtt;
    std::unique_ptr<juce::ButtonParameterAttachment> bAtt;
};

//==============================================================================
class Panel : public juce::Component
{
public:
    Panel (WraithCoreProcessor&, const juce::String& title, juce::Colour accent, std::initializer_list<int> params, int rows, int powerParam = -1);
    void paint (juce::Graphics&) override;
    void resized() override;
    int columns() const { return (int) std::ceil ((float) controls.size() / (float) rows); }
    juce::OwnedArray<ParamControl>& getControls() { return controls; }

private:
    juce::String title;
    juce::Colour accent;
    int rows;
    juce::OwnedArray<ParamControl> controls;
    juce::ToggleButton power;
    std::unique_ptr<juce::ButtonParameterAttachment> powerAtt;
    juce::uint32 seed;
};

// rows of panels; panels share a row by column count
class GridPage : public juce::Component
{
public:
    struct Row { juce::OwnedArray<Panel> panels; float weight = 1.f; };
    Row& addRow (float weight) { auto* r = rows.add (new Row()); r->weight = weight; return *r; }
    Panel* add (Row& r, Panel* p) { r.panels.add (p); addAndMakeVisible (p); return p; }
    void resized() override;
private:
    juce::OwnedArray<Row> rows;
};

//==============================================================================
class Scope : public juce::Component, private juce::Timer
{
public:
    explicit Scope (WraithCoreProcessor& p) : proc (p) { startTimerHz (30); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { repaint(); }
    WraithCoreProcessor& proc;
    std::array<float, 400> snap {};
};

class LedMeter : public juce::Component, private juce::Timer
{
public:
    explicit LedMeter (WraithCoreProcessor& p) : proc (p) { startTimerHz (30); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { repaint(); }
    WraithCoreProcessor& proc;
    float holdL = 0.f, holdR = 0.f;
};
} // namespace ui
