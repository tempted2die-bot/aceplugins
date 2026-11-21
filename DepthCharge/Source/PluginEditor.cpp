#include "PluginEditor.h"

namespace pal
{
const juce::Colour bg      { 0xff04070d };
const juce::Colour panel   { 0xff0c1019 };
const juce::Colour track   { 0xff2a2236 };
const juce::Colour text    { 0xffd8d0e8 };
const juce::Colour dim     { 0xff7d7290 };
const juce::Colour cyan    { 0xff35f2ff };
const juce::Colour toxic   { 0xffa6ff3a };
const juce::Colour magenta { 0xffff2e7e };
const juce::Colour violet  { 0xffb46bff };
const juce::Colour amber   { 0xffffb340 };
const juce::Colour hazard  { 0xffff7a1a };
const juce::Colour blood   { 0xffff3344 };
}

static juce::Font uiFont (float h, bool bold = true)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

//==============================================================================
WraithLookAndFeel::WraithLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0e0b14));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, pal::magenta.withAlpha (0.35f));
    setColour (juce::PopupMenu::textColourId, pal::text);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xff0e0b14));
    setColour (juce::BubbleComponent::outlineColourId, pal::cyan.withAlpha (0.6f));
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff0e0b14));
    setColour (juce::TooltipWindow::textColourId, pal::text);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0e0b14));
    setColour (juce::ComboBox::textColourId, pal::text);
    setColour (juce::ComboBox::arrowColourId, pal::cyan);
    setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
}

void WraithLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float startAngle, float endAngle, juce::Slider& s)
{
    const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);
    const float r  = (float) juce::jmin (w, h) * 0.5f - 5.f;
    const float cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    juce::Path trackPath;
    trackPath.addCentredArc (cx, cy, r, r, 0.f, startAngle, endAngle, true);
    g.setColour (pal::track);
    g.strokePath (trackPath, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    float from = startAngle;
    if (s.getMinimum() < 0.0 && s.getMaximum() > 0.0)
        from = startAngle + (float) s.valueToProportionOfLength (0.0) * (endAngle - startAngle);
    if (std::abs (angle - from) > 0.001f)
    {
        juce::Path val;
        val.addCentredArc (cx, cy, r, r, 0.f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        g.setColour (accent.withAlpha (0.22f));
        g.strokePath (val, juce::PathStrokeType (9.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (val, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float kr = r * 0.68f;
    juce::ColourGradient grad (juce::Colour (0xff2d2440), cx, cy - kr, juce::Colour (0xff0b0910), cx, cy + kr, false);
    g.setGradientFill (grad);
    g.fillEllipse (cx - kr, cy - kr, kr * 2.f, kr * 2.f);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawEllipse (cx - kr, cy - kr, kr * 2.f, kr * 2.f, 1.2f);
    g.setColour (accent.withAlpha (s.isMouseOverOrDragging() ? 0.5f : 0.18f));
    g.drawEllipse (cx - kr + 2.f, cy - kr + 2.f, kr * 2.f - 4.f, kr * 2.f - 4.f, 1.f);

    juce::Path ptr;
    ptr.addRoundedRectangle (-1.6f, -kr + 3.f, 3.2f, kr * 0.5f, 1.6f);
    g.setColour (accent);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (cx, cy));
}

void WraithLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const auto accent = b.findColour (juce::ToggleButton::tickColourId);
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat().reduced (2.f);
    const float rad = r.getHeight() * 0.5f;

    if (b.getButtonText().isEmpty()) // panel power: slide switch
    {
        g.setColour (on ? accent.withAlpha (0.28f) : juce::Colour (0xff1a1522));
        g.fillRoundedRectangle (r, rad);
        g.setColour (on ? accent : juce::Colour (0xff4a3f5a));
        g.drawRoundedRectangle (r, rad, 1.2f);
        const float d = r.getHeight() - 6.f;
        const juce::Rectangle<float> knob (on ? r.getRight() - 3.f - d : r.getX() + 3.f, r.getY() + 3.f, d, d);
        if (on) { g.setColour (accent.withAlpha (0.4f)); g.fillEllipse (knob.expanded (3.f)); }
        g.setColour (on ? accent : (highlighted ? pal::dim : juce::Colour (0xff3a3046)));
        g.fillEllipse (knob);
        return;
    }

    g.setColour (on ? accent.withAlpha (0.16f) : juce::Colour (0xff1a1522));
    g.fillRoundedRectangle (r, rad);
    g.setColour (on ? accent : (highlighted ? pal::dim : juce::Colour (0xff4a3f5a)));
    g.drawRoundedRectangle (r, rad, 1.2f);

    const float d = r.getHeight() * 0.42f;
    const juce::Rectangle<float> led (r.getX() + rad - d * 0.5f, r.getCentreY() - d * 0.5f, d, d);
    if (on) { g.setColour (accent.withAlpha (0.35f)); g.fillEllipse (led.expanded (3.f)); }
    g.setColour (on ? accent : juce::Colour (0xff3a3046));
    g.fillEllipse (led);

    if (b.getButtonText().isNotEmpty())
    {
        g.setColour (on ? accent : pal::dim);
        g.setFont (uiFont (10.5f));
        g.drawFittedText (b.getButtonText(), r.withTrimmedLeft (rad * 1.7f).withTrimmedRight (4.f).toNearestInt(), juce::Justification::centred, 1, 0.6f);
    }
}

void WraithLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (1.f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 4.f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 4.f, 1.f);

    const float ax = (float) w - 13.f, ay = (float) h * 0.5f;
    juce::Path arrow;
    arrow.addTriangle (ax - 4.f, ay - 2.f, ax + 4.f, ay - 2.f, ax, ay + 3.f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

void WraithLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (2.f);
    const auto c = pal::magenta;
    if (highlighted || down) { g.setColour (c.withAlpha (0.25f)); g.fillRoundedRectangle (r.expanded (2.f), 8.f); }
    juce::ColourGradient grad (c.brighter (0.2f), r.getX(), r.getY(), c.darker (0.6f), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (down ? juce::Colours::white : c.brighter (0.5f));
    g.drawRoundedRectangle (r, 6.f, 1.2f);
}

//==============================================================================
ParamControl::ParamControl (juce::AudioProcessorValueTreeState& s, int idx, juce::Colour acc)
    : def (sw::paramDefs()[(size_t) idx]), accent (acc)
{
    switch (def.type)
    {
        case sw::PType::Float:
        case sw::PType::Int:
            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
            slider.setPopupDisplayEnabled (true, true, nullptr);
            sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, def.id, slider);
            addAndMakeVisible (slider);
            break;
        case sw::PType::Choice:
            combo.addItemList (juce::StringArray::fromTokens (def.choices, "|", ""), 1);
            combo.setColour (juce::ComboBox::outlineColourId, accent.withAlpha (0.5f));
            combo.setColour (juce::ComboBox::textColourId, accent);
            combo.setColour (juce::ComboBox::arrowColourId, accent);
            combo.setJustificationType (juce::Justification::centred);
            cAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, def.id, combo);
            addAndMakeVisible (combo);
            break;
        case sw::PType::Bool:
            toggle.setButtonText (juce::String (def.name).toUpperCase());
            toggle.setColour (juce::ToggleButton::tickColourId, accent);
            bAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, def.id, toggle);
            addAndMakeVisible (toggle);
            break;
    }
}

void ParamControl::parentHierarchyChanged()
{
    // show the value bubble inside the plugin window rather than as a desktop window
    if (slider.isVisible())
        if (auto* top = findParentComponentOfClass<juce::AudioProcessorEditor>())
            slider.setPopupDisplayEnabled (true, true, top);
}

void ParamControl::resized()
{
    auto b = getLocalBounds();
    if (def.type == sw::PType::Bool)
    {
        toggle.setBounds (b.withSizeKeepingCentre (juce::jmin (b.getWidth() - 4, 96), 24));
        return;
    }
    b.removeFromBottom (15);
    if (def.type == sw::PType::Choice)
        combo.setBounds (b.withSizeKeepingCentre (juce::jmin (b.getWidth() - 6, 92), 24));
    else
        slider.setBounds (b.withSizeKeepingCentre (juce::jmin (b.getWidth(), b.getHeight()), juce::jmin (b.getWidth(), b.getHeight())));
}

void ParamControl::paint (juce::Graphics& g)
{
    if (def.type == sw::PType::Bool) return;
    g.setColour (pal::dim);
    g.setFont (uiFont (10.5f));
    g.drawText (juce::String (def.name).toUpperCase(), getLocalBounds().removeFromBottom (15), juce::Justification::centred);
}

//==============================================================================
Panel::Panel (juce::AudioProcessorValueTreeState& s, const juce::String& t, juce::Colour acc,
              std::initializer_list<int> params, int r, int powerParam)
    : title (t), accent (acc), rows (juce::jmax (1, r))
{
    for (int idx : params)
        addAndMakeVisible (controls.add (new ParamControl (s, idx, accent)));

    if (powerParam >= 0)
    {
        power.setColour (juce::ToggleButton::tickColourId, accent);
        powerAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, sw::paramDefs()[(size_t) powerParam].id, power);
        power.onStateChange = [this]
        {
            const float a = power.getToggleState() ? 1.f : 0.38f;
            for (auto* c : controls) if (std::abs (c->getAlpha() - a) > 0.01f) c->setAlpha (a);
        };
        power.onStateChange();
        addAndMakeVisible (power);
    }
}

void Panel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (3.f);
    g.setColour (pal::panel);
    g.fillRoundedRectangle (b, 8.f);
    g.setColour (accent.withAlpha (0.28f));
    g.drawRoundedRectangle (b, 8.f, 1.f);

    auto header = b.reduced (10.f, 0.f).removeFromTop (26.f);
    g.setColour (accent);
    g.setFont (uiFont (12.5f));
    g.drawText (title, header.toNearestInt(), juce::Justification::centredLeft);
    g.setColour (accent.withAlpha (0.15f));
    g.fillRect (header.getX(), header.getBottom(), header.getWidth(), 1.f);
}

void Panel::resized()
{
    auto b = getLocalBounds().reduced (9, 6);
    auto header = b.removeFromTop (22);
    if (powerAtt) power.setBounds (header.removeFromRight (42).reduced (0, 1));
    b.removeFromTop (4);
    if (controls.isEmpty()) return;
    const int cols = columns();
    const int cw = b.getWidth() / cols, ch = b.getHeight() / rows;
    for (int i = 0; i < controls.size(); ++i)
        controls[i]->setBounds (b.getX() + (i % cols) * cw, b.getY() + (i / cols) * ch, cw, ch);
}

//==============================================================================
void Scope::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0a0810));
    g.fillRoundedRectangle (b, 6.f);
    g.setColour (pal::toxic.withAlpha (0.25f));
    g.drawRoundedRectangle (b.reduced (0.5f), 6.f, 1.f);
    g.setColour (pal::toxic.withAlpha (0.06f));
    for (int i = 1; i < 8; ++i) g.fillRect (b.getX() + b.getWidth() * (float) i / 8.f, b.getY() + 3.f, 1.f, b.getHeight() - 6.f);
    g.fillRect (b.getX() + 3.f, b.getCentreY(), b.getWidth() - 6.f, 1.f);

    constexpr int N = DepthChargeProcessor::kScopeSize;
    const int w = proc.scopeWrite.load (std::memory_order_acquire);
    const int start = (w - 1100 + N) % N;
    int trig = start;
    for (int k = 0; k < 560; ++k)
    {
        const int i0 = (start + k) % N, i1 = (i0 + 1) % N;
        if (proc.scope[(size_t) i0].load (std::memory_order_relaxed) <= 0.f && proc.scope[(size_t) i1].load (std::memory_order_relaxed) > 0.f) { trig = i0; break; }
    }
    for (int k = 0; k < (int) snap.size(); ++k)
        snap[(size_t) k] = proc.scope[(size_t) ((trig + k) % N)].load (std::memory_order_relaxed);

    juce::Path p;
    const float h = b.getHeight() * 0.44f;
    for (int k = 0; k < (int) snap.size(); ++k)
    {
        const float x = b.getX() + 4.f + (b.getWidth() - 8.f) * (float) k / (float) (snap.size() - 1);
        const float y = b.getCentreY() - juce::jlimit (-1.f, 1.f, snap[(size_t) k]) * h;
        if (k == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (pal::toxic.withAlpha (0.18f));
    g.strokePath (p, juce::PathStrokeType (4.f));
    g.setColour (pal::toxic);
    g.strokePath (p, juce::PathStrokeType (1.3f));

    g.setColour (pal::toxic.withAlpha (0.5f));
    g.setFont (uiFont (9.f));
    g.drawText ("DEPTH", b.reduced (8.f, 4.f).toNearestInt(), juce::Justification::topLeft);
}

//==============================================================================
void Page::resized()
{
    float total = 0.f;
    for (auto* r : rows) total += r->weight;
    auto area = getLocalBounds();
    const int h = area.getHeight();
    for (int ri = 0; ri < rows.size(); ++ri)
    {
        auto* r = rows[ri];
        auto rowArea = (ri == rows.size() - 1) ? area : area.removeFromTop ((int) ((float) h * r->weight / total));
        int cols = 0;
        for (auto* p : r->panels) cols += p->columns();
        int x = rowArea.getX();
        for (int i = 0; i < r->panels.size(); ++i)
        {
            const int w = (i == r->panels.size() - 1) ? rowArea.getRight() - x : rowArea.getWidth() * r->panels[i]->columns() / juce::jmax (1, cols);
            r->panels[i]->setBounds (x, rowArea.getY(), w, rowArea.getHeight());
            x += w;
        }
    }
}

void TabButton::paintButton (juce::Graphics& g, bool highlighted, bool)
{
    auto r = getLocalBounds().toFloat().reduced (3.f, 2.f);
    const bool on = getToggleState();
    if (on)
    {
        g.setColour (accent.withAlpha (0.18f)); g.fillRoundedRectangle (r, 6.f);
        g.setColour (accent);                   g.drawRoundedRectangle (r, 6.f, 1.4f);
        g.fillRect (r.getX() + 12.f, r.getBottom() - 2.f, r.getWidth() - 24.f, 2.f);
    }
    else
    {
        g.setColour (juce::Colour (0xff0e131d)); g.fillRoundedRectangle (r, 6.f);
        g.setColour (highlighted ? accent.withAlpha (0.6f) : juce::Colour (0xff2a3446)); g.drawRoundedRectangle (r, 6.f, 1.f);
    }
    g.setColour (on ? accent : (highlighted ? pal::text : pal::dim));
    g.setFont (uiFont (13.f));
    g.drawText (getName(), r.toNearestInt(), juce::Justification::centred);
}

//==============================================================================
DepthChargeEditor::DepthChargeEditor (DepthChargeProcessor& p)
    : AudioProcessorEditor (&p), proc (p), scope (p)
{
    setLookAndFeel (&lnf);
    auto& s = p.apvts;
    using namespace sw::pid;

    // ---------------- 808 QUAKE page ----------------
    {
        auto& a = page808.addRow (0.62f);
        page808.add (a, new Panel (s, "808 CORE", pal::hazard, { q8Oct, q8Shape, q8Punch, q8PunchTime, q8Click, q8Phase, q8SubKeep }, 2, q8On));
        page808.add (a, new Panel (s, "808 DIRT", pal::blood, { q8DrvMode, q8Drive, q8Fold, q8Asym, q8Tone, q8Knock }, 2));
        page808.add (a, new Panel (s, "FILTER + FM", pal::hazard, { q8Cut, q8Res, q8Fm, q8FmRatio }, 2));
        page808.add (a, new Panel (s, "GROWL", pal::toxic, { q8Vowel, q8GrowlRes, q8GrowlAmt }, 2));
        auto& b = page808.addRow (0.38f);
        page808.add (b, new Panel (s, "808 AMP + SLIDE", pal::hazard, { q8Decay, q8Sus, q8Rel, q8Mode, q8Glide }, 1));
        page808.add (b, new Panel (s, "WOBBLE", pal::toxic, { q8WobDiv, q8WobShape, q8WobTarget, q8WobDepth, q8WobRetrig }, 1, q8WobOn));
        page808.add (b, new Panel (s, "LAYERS", pal::text, { q8Lvl, synthLvl, splitNote, subMono }, 1));
    }
    // ---------------- SONAR SYNTH page ----------------
    {
        auto& a = pageSynth.addRow (0.62f);
        pageSynth.add (a, new Panel (s, "OSCILLATOR", pal::cyan, { oscWave, oscFold, oscOct, oscFine, uniVoices, uniDetune, subLvl, noiseLvl }, 2));
        pageSynth.add (a, new Panel (s, "FM + PING", pal::cyan, { fmRatio, fmAmt, pingAmt, pingDecay }, 2));
        pageSynth.add (a, new Panel (s, "BUBBLER", pal::toxic, { bubRate, bubChance, bubRand, bubGate }, 2));
        pageSynth.add (a, new Panel (s, "FILTER", pal::violet, { fltType, fltCut, fltRes, fltEnv, fltDecay }, 2));
        auto& b = pageSynth.addRow (0.38f);
        pageSynth.add (b, new Panel (s, "AMP", pal::violet, { atk, dec, sus, rel }, 1));
        pageSynth.add (b, new Panel (s, "LFO", pal::amber, { lfoShape, lfoSync, lfoRate, lfoDiv, lfoPitch, lfoCut }, 1));
        pageSynth.add (b, new Panel (s, "VOICE", pal::cyan, { polyMode, glide, voiceVol }, 1));
    }
    // ---------------- FX (always visible) ----------------
    {
        auto& r = fxRow.addRow (1.f);
        fxRow.add (r, new Panel (s, "DRIVE", pal::magenta, { drvMode, drvAmt, drvTone, drvMix }, 2, drvOn));
        fxRow.add (r, new Panel (s, "CRUSH", pal::magenta, { crsBits, crsRate, crsMix }, 2, crsOn));
        fxRow.add (r, new Panel (s, "GLITCH", pal::toxic, { glDiv, glChance, glRev, glPitch, glGap, glStop, glMix }, 2, glOn));
        fxRow.add (r, new Panel (s, "SONAR DELAY", pal::cyan, { dlyDiv, dlyPP, dlyFb, dlyTone, dlyWarble, dlyMix }, 2, dlyOn));
        fxRow.add (r, new Panel (s, "ATMOS", pal::violet, { revSize, revDamp, revShimmer, revMix }, 2, revOn));
        fxRow.add (r, new Panel (s, "MASTER", pal::text, { width, outGain, fxLowCut, bendRange }, 2));
    }
    addChildComponent (page808);
    addChildComponent (pageSynth);
    addAndMakeVisible (fxRow);

    tab808.onClick   = [this] { showPage (0); };
    tabSynth.onClick = [this] { showPage (1); };
    addAndMakeVisible (tab808);
    addAndMakeVisible (tabSynth);
    showPage (0);

    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.setColour (juce::ComboBox::outlineColourId, pal::hazard.withAlpha (0.6f));
    presetBox.setColour (juce::ComboBox::textColourId, pal::hazard);
    presetBox.setColour (juce::ComboBox::arrowColourId, pal::hazard);
    presetBox.setTextWhenNothingSelected ("~ preset ~");
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0) proc.setCurrentProgram (id - 1);
    };
    addAndMakeVisible (presetBox);

    mutateButton.setTooltip ("Randomise the synth, the 808 and the FX inside dark / glitchy / dubstep ranges");
    mutateButton.onClick = [this]
    {
        proc.mutate();
        presetBox.setText ("~ mutated ~", juce::dontSendNotification);
    };
    addAndMakeVisible (mutateButton);
    addAndMakeVisible (scope);

    setSize (1200, 800);
}

DepthChargeEditor::~DepthChargeEditor() { setLookAndFeel (nullptr); }

void DepthChargeEditor::showPage (int index)
{
    page808.setVisible (index == 0);
    pageSynth.setVisible (index == 1);
    tab808.setToggleState (index == 0, juce::dontSendNotification);
    tabSynth.setToggleState (index == 1, juce::dontSendNotification);
}

void DepthChargeEditor::paint (juce::Graphics& g)
{
    g.fillAll (pal::bg);

    // depth-sounder rings + grid
    g.setColour (pal::hazard.withAlpha (0.025f));
    for (int x = 0; x < getWidth(); x += 24) g.fillRect (x, 0, 1, getHeight());
    for (int y = 0; y < getHeight(); y += 24) g.fillRect (0, y, getWidth(), 1);
    g.setColour (pal::cyan.withAlpha (0.04f));
    for (int r = 80; r < 1400; r += 90)
        g.drawEllipse ((float) getWidth() * 0.5f - (float) r, -(float) r * 0.55f, (float) r * 2.f, (float) r * 1.1f, 1.f);
    juce::ColourGradient glow (pal::hazard.withAlpha (0.08f), (float) getWidth() * 0.5f, 0.f,
                               juce::Colours::transparentBlack, (float) getWidth() * 0.5f, (float) getHeight() * 0.6f, true);
    g.setGradientFill (glow);
    g.fillRect (getLocalBounds());

    const juce::Rectangle<int> title (22, 12, 360, 40);
    g.setFont (uiFont (34.f));
    g.setColour (pal::hazard.withAlpha (0.2f));
    for (auto d : { juce::Point<int> (-2, 0), juce::Point<int> (2, 0), juce::Point<int> (0, 2), juce::Point<int> (0, -2) })
        g.drawText ("DEPTHCHARGE", title + d, juce::Justification::centredLeft);
    g.setColour (pal::cyan.withAlpha (0.5f));
    g.drawText ("DEPTHCHARGE", title + juce::Point<int> (-2, 1), juce::Justification::centredLeft); // chromatic glitch offset
    g.setColour (juce::Colour (0xfffff3e8));
    g.drawText ("DEPTHCHARGE", title, juce::Justification::centredLeft);

    g.setColour (pal::dim);
    g.setFont (uiFont (10.5f, false));
    g.drawText ("808 quake  /  glitch  /  atmos  /  sonar synth   v1.0", 24, 52, 380, 14, juce::Justification::centredLeft);
}

void DepthChargeEditor::resized()
{
    auto b = getLocalBounds().reduced (10);
    auto header = b.removeFromTop (64);
    header.removeFromLeft (370);
    scope.setBounds (header.removeFromRight (320).reduced (0, 6));
    header.removeFromRight (14);
    mutateButton.setBounds (header.removeFromRight (110).withSizeKeepingCentre (110, 34));
    header.removeFromRight (10);
    presetBox.setBounds (header.removeFromRight (240).withSizeKeepingCentre (240, 30));
    b.removeFromTop (4);

    auto tabs = b.removeFromTop (32);
    tab808.setBounds (tabs.removeFromLeft (170));
    tabs.removeFromLeft (6);
    tabSynth.setBounds (tabs.removeFromLeft (170));
    b.removeFromTop (2);

    fxRow.setBounds (b.removeFromBottom (238));
    page808.setBounds (b);
    pageSynth.setBounds (b);
}
