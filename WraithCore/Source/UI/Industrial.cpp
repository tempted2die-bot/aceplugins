#include "Industrial.h"
#include "BinaryData.h"

namespace ui
{
static juce::Typeface::Ptr antonTf()
{
    static juce::Typeface::Ptr tf = juce::Typeface::createSystemTypefaceFor (BinaryData::AntonRegular_ttf, BinaryData::AntonRegular_ttfSize);
    return tf;
}
static juce::Typeface::Ptr monoTf()
{
    static juce::Typeface::Ptr tf = juce::Typeface::createSystemTypefaceFor (BinaryData::AnonymousProBold_ttf, BinaryData::AnonymousProBold_ttfSize);
    return tf;
}
juce::Font display (float h) { return juce::Font (juce::FontOptions().withTypeface (antonTf()).withHeight (h)); }
juce::Font mono (float h)    { return juce::Font (juce::FontOptions().withTypeface (monoTf()).withHeight (h)); }

juce::Colour accentFor (int t)
{
    const juce::String g = wc::fxInfo (t).group;
    if (g == "DRIVE") return col::orange;
    if (g == "GLITCH") return col::acid;
    if (g == "TONE") return col::violet;
    if (g == "DYNAMICS") return col::hazard;
    if (g == "SPACE") return col::cyan;
    if (g == "CHARACTER") return col::red;
    return col::dim;
}

void drawScrew (juce::Graphics& g, juce::Point<float> c, float r, float angle)
{
    juce::ColourGradient gr (juce::Colour (0xff4a5058), c.x - r * 0.5f, c.y - r * 0.5f, juce::Colour (0xff15171a), c.x + r, c.y + r, true);
    g.setGradientFill (gr);
    g.fillEllipse (c.x - r, c.y - r, r * 2.f, r * 2.f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawEllipse (c.x - r, c.y - r, r * 2.f, r * 2.f, 0.8f);
    juce::Path slot;
    slot.addRectangle (-r * 0.75f, -0.6f, r * 1.5f, 1.2f);
    g.setColour (juce::Colour (0xff0a0b0d));
    g.fillPath (slot, juce::AffineTransform::rotation (angle).translated (c));
}

void drawPlate (juce::Graphics& g, juce::Rectangle<float> r, juce::uint32 seed, bool screws, float corner)
{
    juce::ColourGradient grad (col::plateHi, r.getX(), r.getY(), col::plateLo, r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, corner);
    // brushed metal streaks
    juce::Random rnd ((juce::int64) seed);
    for (int i = 0; i < (int) (r.getHeight() / 2.5f); ++i)
    {
        const float y = r.getY() + rnd.nextFloat() * r.getHeight();
        g.setColour (juce::Colours::white.withAlpha (0.012f + rnd.nextFloat() * 0.018f));
        const float x0 = r.getX() + rnd.nextFloat() * r.getWidth() * 0.5f;
        g.fillRect (x0, y, rnd.nextFloat() * r.getWidth() * 0.6f, 1.f);
    }
    // scratches
    g.setColour (juce::Colours::white.withAlpha (0.025f));
    for (int i = 0; i < 3; ++i)
    {
        const float x = r.getX() + rnd.nextFloat() * r.getWidth(), y = r.getY() + rnd.nextFloat() * r.getHeight();
        g.drawLine (x, y, x + 15.f + rnd.nextFloat() * 40.f, y + rnd.nextFloat() * 12.f - 6.f, 0.7f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawLine (r.getX() + corner, r.getY() + 0.5f, r.getRight() - corner, r.getY() + 0.5f, 1.f);
    g.setColour (juce::Colours::black.withAlpha (0.9f));
    g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.f);
    if (screws && r.getWidth() > 60.f && r.getHeight() > 40.f)
    {
        const float s = 3.6f, m = 7.f;
        drawScrew (g, { r.getX() + m, r.getY() + m }, s, rnd.nextFloat() * 3.f);
        drawScrew (g, { r.getRight() - m, r.getY() + m }, s, rnd.nextFloat() * 3.f);
        drawScrew (g, { r.getX() + m, r.getBottom() - m }, s, rnd.nextFloat() * 3.f);
        drawScrew (g, { r.getRight() - m, r.getBottom() - m }, s, rnd.nextFloat() * 3.f);
    }
}

void drawHazard (juce::Graphics& g, juce::Rectangle<float> r, float alpha, float stripe)
{
    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (r.toNearestInt());
    g.setColour (juce::Colour (0xff111111).withAlpha (alpha));
    g.fillRect (r);
    g.setColour (col::hazard.withAlpha (alpha));
    for (float x = r.getX() - r.getHeight(); x < r.getRight() + r.getHeight(); x += stripe * 2.f)
    {
        juce::Path p;
        p.startNewSubPath (x, r.getBottom());
        p.lineTo (x + stripe, r.getBottom());
        p.lineTo (x + stripe + r.getHeight(), r.getY());
        p.lineTo (x + r.getHeight(), r.getY());
        p.closeSubPath();
        g.fillPath (p);
    }
}

void drawScanlines (juce::Graphics& g, juce::Rectangle<float> r, float alpha)
{
    g.setColour (juce::Colours::black.withAlpha (alpha));
    for (float y = r.getY(); y < r.getBottom(); y += 3.f) g.fillRect (r.getX(), y, r.getWidth(), 1.f);
}

void drawLcd (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (col::lcd);
    g.fillRoundedRectangle (r, 3.f);
    juce::ColourGradient glow (col::acid.withAlpha (0.06f), r.getCentreX(), r.getCentreY(), juce::Colours::transparentBlack, r.getX(), r.getY(), true);
    g.setGradientFill (glow);
    g.fillRoundedRectangle (r, 3.f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r, 3.f, 1.5f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawLine (r.getX() + 3.f, r.getBottom() + 0.5f, r.getRight() - 3.f, r.getBottom() + 0.5f, 1.f);
}

void drawGlitchText (juce::Graphics& g, const juce::String& s, juce::Rectangle<int> r, const juce::Font& f, juce::Colour c, juce::Justification j, float glitch, juce::Random& rnd)
{
    g.setFont (f);
    const int off = 1 + (int) (glitch * 4.f);
    g.setColour (col::red.withAlpha (0.55f)); g.drawText (s, r.translated (-off, 0), j);
    g.setColour (col::cyan.withAlpha (0.55f)); g.drawText (s, r.translated (off, 0), j);
    g.setColour (c); g.drawText (s, r, j);
    if (glitch > 0.01f) // displaced slices
    {
        const int slices = 2 + (int) (glitch * 5.f);
        for (int i = 0; i < slices; ++i)
        {
            const int y = r.getY() + rnd.nextInt (juce::jmax (1, r.getHeight())), h = 2 + rnd.nextInt (6), dx = rnd.nextInt (21) - 10;
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (r.getX() - 20, y, r.getWidth() + 40, h);
            g.setColour (col::bg); g.fillRect (r.getX() - 20, y, r.getWidth() + 40, h);
            g.setColour (rnd.nextBool() ? col::acid : c); g.drawText (s, r.translated (dx, 0), j);
        }
    }
}

//==============================================================================
IndustrialLnF::IndustrialLnF()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0d0f12));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::hazard.withAlpha (0.85f));
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    setColour (juce::PopupMenu::headerTextColourId, col::hazard);
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xff070809));
    setColour (juce::BubbleComponent::outlineColourId, col::hazard);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff070809));
    setColour (juce::TooltipWindow::textColourId, col::text);
    setColour (juce::TooltipWindow::outlineColourId, col::hazard);
    setColour (juce::ComboBox::backgroundColourId, col::lcd);
    setColour (juce::ComboBox::textColourId, col::lcdText);
    setColour (juce::ComboBox::arrowColourId, col::hazard);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::black);
    setColour (juce::TextButton::textColourOffId, col::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    setColour (juce::TextEditor::textColourId, col::lcdText);
    setColour (juce::TextEditor::highlightColourId, col::hazard.withAlpha (0.4f));
    setColour (juce::CaretComponent::caretColourId, col::hazard);
    setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ScrollBar::thumbColourId, col::hazard);
    setColour (juce::Label::textColourId, col::text);
    setColour (juce::AlertWindow::backgroundColourId, col::plate);
    setColour (juce::AlertWindow::textColourId, col::text);
    setColour (juce::AlertWindow::outlineColourId, col::hazard);
}

juce::Typeface::Ptr IndustrialLnF::getTypefaceForFont (const juce::Font& f)
{
    if (f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName() || f.getTypefaceName() == "<Sans-Serif>")
        return monoTf();
    return LookAndFeel_V4::getTypefaceForFont (f);
}

void IndustrialLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);
    const float r = (float) juce::jmin (w, h) * 0.5f - 4.f;
    const float cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f;
    const float angle = a0 + pos * (a1 - a0);
    if (r < 6.f) return;

    // tick ring
    g.setColour (col::dim.withAlpha (0.45f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 10.f - juce::MathConstants<float>::halfPi;
        const float r1 = r + 2.5f, r2 = r + (i % 5 == 0 ? 0.f : 1.5f);
        g.drawLine (cx + std::cos (a) * r2, cy + std::sin (a) * r2, cx + std::cos (a) * r1, cy + std::sin (a) * r1, 1.f);
    }
    // groove + value arc
    const float ar = r - 1.f;
    juce::Path track; track.addCentredArc (cx, cy, ar, ar, 0.f, a0, a1, true);
    g.setColour (col::groove);
    g.strokePath (track, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    float from = a0;
    if (s.getMinimum() < 0.0 && s.getMaximum() > 0.0) from = a0 + (float) s.valueToProportionOfLength (0.0) * (a1 - a0);
    if (std::abs (angle - from) > 0.001f)
    {
        juce::Path v; v.addCentredArc (cx, cy, ar, ar, 0.f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        g.setColour (accent.withAlpha (0.25f));
        g.strokePath (v, juce::PathStrokeType (8.f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        g.setColour (accent);
        g.strokePath (v, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    }
    // knurled body
    const float kr = r * 0.72f;
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (cx - kr - 1.f, cy - kr + 1.5f, kr * 2.f + 2.f, kr * 2.f + 2.f);
    juce::ColourGradient body (juce::Colour (0xff3a3f46), cx - kr, cy - kr, juce::Colour (0xff111316), cx + kr, cy + kr, false);
    g.setGradientFill (body);
    g.fillEllipse (cx - kr, cy - kr, kr * 2.f, kr * 2.f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    for (int i = 0; i < 28; ++i)
    {
        const float a = angle + juce::MathConstants<float>::twoPi * (float) i / 28.f;
        g.drawLine (cx + std::cos (a) * kr * 0.84f, cy + std::sin (a) * kr * 0.84f, cx + std::cos (a) * kr, cy + std::sin (a) * kr, 1.f);
    }
    const float cr = kr * 0.68f;
    juce::ColourGradient cap (juce::Colour (0xff4a5058), cx, cy - cr, juce::Colour (0xff1a1d21), cx, cy + cr, false);
    g.setGradientFill (cap);
    g.fillEllipse (cx - cr, cy - cr, cr * 2.f, cr * 2.f);
    g.setColour (juce::Colours::white.withAlpha (s.isMouseOverOrDragging() ? 0.25f : 0.1f));
    g.drawEllipse (cx - cr, cy - cr, cr * 2.f, cr * 2.f, 1.f);
    // indicator
    juce::Path ptr; ptr.addRoundedRectangle (-1.5f, -kr + 2.f, 3.f, kr * 0.55f, 1.f);
    g.setColour (accent.withAlpha (0.35f));
    g.fillPath (ptr, juce::AffineTransform::scale (2.2f, 1.f).rotated (angle).translated (cx, cy));
    g.setColour (accent);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (cx, cy));
}

void IndustrialLnF::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider& s)
{
    const auto accent = s.findColour (juce::Slider::trackColourId);
    auto r = juce::Rectangle<float> ((float) x, (float) y + (float) h * 0.5f - 5.f, (float) w, 10.f);
    g.setColour (col::groove); g.fillRoundedRectangle (r, 2.f);
    g.setColour (juce::Colours::black); g.drawRoundedRectangle (r, 2.f, 1.f);
    const float zero = s.getMinimum() < 0.0 ? r.getX() + r.getWidth() * (float) s.valueToProportionOfLength (0.0) : r.getX();
    const float a = juce::jmin (zero, pos), b = juce::jmax (zero, pos);
    g.setColour (accent.withAlpha (0.25f)); g.fillRect (a, r.getY() - 2.f, b - a, r.getHeight() + 4.f);
    g.setColour (accent); g.fillRect (a, r.getY() + 2.f, b - a, r.getHeight() - 4.f);
    g.setColour (col::dim); g.fillRect (zero - 0.5f, r.getY() - 3.f, 1.f, r.getHeight() + 6.f);
    g.setColour (col::text); g.fillRect (pos - 1.5f, r.getY() - 3.f, 3.f, r.getHeight() + 6.f);
}

void IndustrialLnF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const auto accent = b.findColour (juce::ToggleButton::tickColourId);
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    if (b.getButtonText().isEmpty()) // rocker / power switch
    {
        g.setColour (col::groove); g.fillRoundedRectangle (r, 3.f);
        g.setColour (juce::Colours::black); g.drawRoundedRectangle (r, 3.f, 1.f);
        auto knob = r.reduced (2.f).withWidth (r.getWidth() * 0.5f - 2.f);
        if (on) knob.setX (r.getCentreX());
        juce::ColourGradient gr (juce::Colour (0xff4a5058), knob.getX(), knob.getY(), juce::Colour (0xff1c1f23), knob.getX(), knob.getBottom(), false);
        g.setGradientFill (gr); g.fillRoundedRectangle (knob, 2.f);
        const auto led = juce::Rectangle<float> (4.f, 4.f).withCentre (knob.getCentre());
        if (on) { g.setColour (accent.withAlpha (0.4f)); g.fillEllipse (led.expanded (3.f)); }
        g.setColour (on ? accent : (highlighted ? col::dim : juce::Colour (0xff2a2e33)));
        g.fillEllipse (led);
        return;
    }
    g.setColour (on ? accent.withAlpha (0.14f) : col::groove);
    g.fillRoundedRectangle (r, 3.f);
    g.setColour (on ? accent : (highlighted ? col::dim : juce::Colour (0xff2a2e33)));
    g.drawRoundedRectangle (r, 3.f, 1.f);
    const float d = 6.f;
    const juce::Rectangle<float> led (r.getX() + 7.f, r.getCentreY() - d * 0.5f, d, d);
    if (on) { g.setColour (accent.withAlpha (0.45f)); g.fillEllipse (led.expanded (3.f)); }
    g.setColour (on ? accent : juce::Colour (0xff33383e));
    g.fillEllipse (led);
    g.setColour (on ? accent : col::dim);
    g.setFont (mono (11.f));
    g.drawFittedText (b.getButtonText(), r.withTrimmedLeft (16.f).withTrimmedRight (3.f).toNearestInt(), juce::Justification::centred, 1, 0.6f);
}

void IndustrialLnF::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
    drawLcd (g, r);
    drawScanlines (g, r.reduced (2.f), 0.25f);
    const float ax = (float) w - 10.f, ay = (float) h * 0.5f;
    juce::Path arrow;
    arrow.addTriangle (ax - 3.5f, ay - 2.f, ax + 3.5f, ay - 2.f, ax, ay + 2.5f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

void IndustrialLnF::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (3, 1, box.getWidth() - 16, box.getHeight() - 2);
    label.setFont (mono (juce::jmin (13.f, (float) box.getHeight() - 8.f)));
}

void IndustrialLnF::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (juce::Colour (0xff0d0f12));
    drawScanlines (g, { 0.f, 0.f, (float) w, (float) h }, 0.12f);
    g.setColour (col::hazard.withAlpha (0.6f));
    g.drawRect (0, 0, w, h, 1);
}

void IndustrialLnF::drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& name)
{
    auto r = area.reduced (6, 2);
    drawHazard (g, r.removeFromLeft (10).toFloat().reduced (0.f, 3.f), 0.9f, 3.f);
    g.setColour (col::hazard);
    g.setFont (display (13.f));
    g.drawText (name, r.withTrimmedLeft (6), juce::Justification::centredLeft);
}

void IndustrialLnF::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.f);
    const juce::String style = b.getProperties().getWithDefault ("style", "plate").toString();
    if (style == "hazard")
    {
        drawHazard (g, r, highlighted ? 1.f : 0.85f, 7.f);
        auto inner = r.reduced (4.f);
        g.setColour (down ? col::hazard : juce::Colour (0xff0b0c0e));
        g.fillRect (inner);
        g.setColour (juce::Colours::black); g.drawRect (r, 1.f);
        return;
    }
    if (style == "tab")
    {
        const bool on = b.getToggleState();
        const auto accent = b.findColour (juce::TextButton::buttonOnColourId);
        juce::Path p;
        p.startNewSubPath (r.getX(), r.getBottom());
        p.lineTo (r.getX() + 10.f, r.getY());
        p.lineTo (r.getRight() - 10.f, r.getY());
        p.lineTo (r.getRight(), r.getBottom());
        p.closeSubPath();
        juce::ColourGradient gr (on ? col::plateHi.brighter (0.15f) : col::plate, 0.f, r.getY(), on ? col::plate : col::plateLo, 0.f, r.getBottom(), false);
        g.setGradientFill (gr); g.fillPath (p);
        g.setColour (on ? accent : (highlighted ? col::dim : juce::Colours::black)); g.strokePath (p, juce::PathStrokeType (1.f));
        if (on) { g.setColour (accent); g.fillRect (r.getX() + 14.f, r.getBottom() - 3.f, r.getWidth() - 28.f, 3.f); }
        return;
    }
    juce::ColourGradient gr (down ? col::plateLo : col::plateHi.brighter (highlighted ? 0.1f : 0.f), 0.f, r.getY(), down ? col::plateHi : col::plateLo, 0.f, r.getBottom(), false);
    g.setGradientFill (gr); g.fillRoundedRectangle (r, 3.f);
    if (b.getToggleState())
    {
        g.setColour (col::hazard.withAlpha (0.16f)); g.fillRoundedRectangle (r, 3.f);
        g.setColour (col::hazard); g.fillRect (r.getX() + 1.f, r.getY() + 4.f, 3.f, r.getHeight() - 8.f);
    }
    g.setColour (highlighted || b.getToggleState() ? col::hazard.withAlpha (0.8f) : juce::Colours::black);
    g.drawRoundedRectangle (r, 3.f, 1.f);
}

void IndustrialLnF::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool highlighted, bool down)
{
    const juce::String style = b.getProperties().getWithDefault ("style", "plate").toString();
    juce::Colour c = col::text;
    if (style == "hazard") c = down ? juce::Colours::black : col::hazard;
    else if (style == "tab") c = b.getToggleState() ? b.findColour (juce::TextButton::buttonOnColourId) : (highlighted ? col::text : col::dim);
    else if (highlighted || b.getToggleState()) c = col::hazard;
    g.setColour (c);
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (6, 2), juce::Justification::centred, 1, 0.7f);
}

void IndustrialLnF::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor&)
{
    drawLcd (g, { 0.f, 0.f, (float) w, (float) h });
}

void IndustrialLnF::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& te)
{
    g.setColour (te.hasKeyboardFocus (true) ? col::hazard : juce::Colours::black);
    g.drawRoundedRectangle (0.5f, 0.5f, (float) w - 1.f, (float) h - 1.f, 3.f, 1.f);
}

void IndustrialLnF::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbPos, int thumbSize, bool over, bool)
{
    g.setColour (col::groove); g.fillRect (x, y, w, h);
    g.setColour (col::hazard.withAlpha (over ? 0.9f : 0.6f));
    if (vertical) g.fillRect (x + 2, thumbPos, w - 4, thumbSize);
    else g.fillRect (thumbPos, y + 2, thumbSize, h - 4);
}

//==============================================================================
ParamControl::ParamControl (WraithCoreProcessor& p, int idx, juce::Colour acc, const juce::String& lbl)
    : proc (p), index (idx), def (wc::paramDefs()[(size_t) idx]), accent (acc), label (lbl.isEmpty() ? juce::String (def.name).toUpperCase() : lbl)
{
    param = proc.apvts.getParameter (def.id);
    switch (def.type)
    {
        case wc::PType::Float:
        case wc::PType::Int:
            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
            slider.setMouseDragSensitivity (220);
            slider.setVelocityBasedMode (false);
            sAtt = std::make_unique<juce::SliderParameterAttachment> (*param, slider);
            slider.onValueChange = [this] { repaint(); };
            addAndMakeVisible (slider);
            break;
        case wc::PType::Choice:
            combo.addItemList (juce::StringArray::fromTokens (juce::String (def.choices), "|", ""), 1);
            combo.setColour (juce::ComboBox::textColourId, accent);
            combo.setColour (juce::ComboBox::arrowColourId, accent);
            combo.setJustificationType (juce::Justification::centred);
            cAtt = std::make_unique<juce::ComboBoxParameterAttachment> (*param, combo);
            addAndMakeVisible (combo);
            break;
        case wc::PType::Bool:
            toggle.setButtonText (label);
            toggle.setColour (juce::ToggleButton::tickColourId, accent);
            bAtt = std::make_unique<juce::ButtonParameterAttachment> (*param, toggle);
            addAndMakeVisible (toggle);
            break;
    }
}

ParamControl::~ParamControl() = default;

void ParamControl::setAccent (juce::Colour c)
{
    accent = c;
    slider.setColour (juce::Slider::rotarySliderFillColourId, c);
    combo.setColour (juce::ComboBox::textColourId, c);
    combo.setColour (juce::ComboBox::arrowColourId, c);
    toggle.setColour (juce::ToggleButton::tickColourId, c);
    repaint();
}

void ParamControl::resized()
{
    auto b = getLocalBounds();
    if (def.type == wc::PType::Bool) { toggle.setBounds (b.withSizeKeepingCentre (juce::jmin (b.getWidth() - 4, 100), 24)); return; }
    b.removeFromBottom (showValue ? 24 : 13);
    if (def.type == wc::PType::Choice) { combo.setBounds (b.withSizeKeepingCentre (juce::jmin (b.getWidth() - 6, 104), 24)); return; }
    const int s = juce::jmin (b.getWidth(), b.getHeight());
    slider.setBounds (b.withSizeKeepingCentre (s, s));
}

void ParamControl::paint (juce::Graphics& g)
{
    if (def.type == wc::PType::Bool) return;
    auto b = getLocalBounds();
    auto txt = b.removeFromBottom (showValue ? 24 : 13);
    g.setColour (col::dim);
    g.setFont (display (12.f));
    g.drawFittedText (label, txt.removeFromTop (12), juce::Justification::centred, 1, 0.7f);
    if (showValue && def.type != wc::PType::Choice && param != nullptr)
    {
        auto vr = txt.reduced (juce::jmax (2, txt.getWidth() / 2 - 40), 0).toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (vr, 2.f);
        g.setColour (slider.isMouseOverOrDragging() ? accent : accent.withAlpha (0.75f));
        g.setFont (mono (11.f));
        g.drawFittedText (param->getCurrentValueAsText(), vr.toNearestInt(), juce::Justification::centred, 1, 0.6f);
    }
}

//==============================================================================
Panel::Panel (WraithCoreProcessor& p, const juce::String& t, juce::Colour acc, std::initializer_list<int> params, int r, int powerParam)
    : title (t), accent (acc), rows (juce::jmax (1, r)), seed ((juce::uint32) t.hashCode())
{
    for (int idx : params) addAndMakeVisible (controls.add (new ParamControl (p, idx, accent)));
    if (powerParam >= 0)
    {
        power.setColour (juce::ToggleButton::tickColourId, accent);
        powerAtt = std::make_unique<juce::ButtonParameterAttachment> (*p.apvts.getParameter (wc::paramDefs()[(size_t) powerParam].id), power);
        power.onStateChange = [this]
        {
            const float a = power.getToggleState() ? 1.f : 0.35f;
            for (auto* c : controls) if (std::abs (c->getAlpha() - a) > 0.01f) c->setAlpha (a);
        };
        power.onStateChange();
        addAndMakeVisible (power);
    }
}

void Panel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (3.f);
    drawPlate (g, b, seed);
    auto header = b.reduced (16.f, 0.f).removeFromTop (26.f).withTrimmedTop (5.f);
    g.setColour (accent);
    g.fillRect (header.getX(), header.getY() + 3.f, 4.f, header.getHeight() - 6.f);
    g.setColour (col::text);
    g.setFont (display (15.f));
    g.drawText (title, header.withTrimmedLeft (10.f).toNearestInt(), juce::Justification::centredLeft);
    const float tw = display (15.f).getStringWidthFloat (title) + 22.f;
    const float stripeEnd = header.getRight() - (powerAtt ? 50.f : 0.f);
    if (stripeEnd - (header.getX() + tw) > 30.f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        for (float x = header.getX() + tw; x < stripeEnd - 4.f; x += 5.f)
            g.drawLine (x, header.getBottom() - 4.f, x + 4.f, header.getY() + 4.f, 1.f);
    }
}

void Panel::resized()
{
    auto b = getLocalBounds().reduced (14, 8);
    auto header = b.removeFromTop (24);
    if (powerAtt) power.setBounds (header.removeFromRight (40).reduced (0, 3));
    b.removeFromTop (4);
    if (controls.isEmpty()) return;
    const int cols = columns(), cw = b.getWidth() / cols, ch = b.getHeight() / rows;
    for (int i = 0; i < controls.size(); ++i)
        controls[i]->setBounds (b.getX() + (i % cols) * cw, b.getY() + (i / cols) * ch, cw, ch);
}

void GridPage::resized()
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

//==============================================================================
void Scope::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    drawLcd (g, b);
    g.setColour (col::acid.withAlpha (0.07f));
    for (int i = 1; i < 10; ++i) g.fillRect (b.getX() + b.getWidth() * (float) i / 10.f, b.getY() + 3.f, 1.f, b.getHeight() - 6.f);
    for (int i = 1; i < 4; ++i) g.fillRect (b.getX() + 3.f, b.getY() + b.getHeight() * (float) i / 4.f, b.getWidth() - 6.f, 1.f);

    constexpr int N = WraithCoreProcessor::kScopeSize;
    const int w = proc.scopeWrite.load (std::memory_order_acquire);
    const int start = (w - 1200 + N) % N;
    int trig = start;
    for (int k = 0; k < 700; ++k)
    {
        const int i0 = (start + k) % N, i1 = (i0 + 1) % N;
        if (proc.scope[(size_t) i0].load() <= 0.f && proc.scope[(size_t) i1].load() > 0.f) { trig = i0; break; }
    }
    for (int k = 0; k < (int) snap.size(); ++k) snap[(size_t) k] = proc.scope[(size_t) ((trig + k) % N)].load();
    juce::Path p;
    const float h = b.getHeight() * 0.42f;
    for (int k = 0; k < (int) snap.size(); ++k)
    {
        const float x = b.getX() + 3.f + (b.getWidth() - 6.f) * (float) k / (float) (snap.size() - 1);
        const float y = b.getCentreY() - juce::jlimit (-1.f, 1.f, snap[(size_t) k]) * h;
        if (k == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (col::acid.withAlpha (0.2f)); g.strokePath (p, juce::PathStrokeType (4.f));
    g.setColour (col::acid); g.strokePath (p, juce::PathStrokeType (1.2f));
    drawScanlines (g, b.reduced (2.f), 0.22f);
    g.setColour (col::acid.withAlpha (0.6f));
    g.setFont (mono (10.f));
    g.drawText ("SIG//OUT", b.reduced (7.f, 4.f).toNearestInt(), juce::Justification::topLeft);
}

void LedMeter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (col::groove); g.fillRoundedRectangle (b, 2.f);
    holdL = std::max (proc.meterL.load(), holdL * 0.92f);
    holdR = std::max (proc.meterR.load(), holdR * 0.92f);
    const int segs = 18;
    auto drawBar = [&] (juce::Rectangle<float> r, float level)
    {
        const float db = juce::Decibels::gainToDecibels (level, -60.f);
        const int lit = juce::jlimit (0, segs, (int) std::round ((db + 48.f) / 48.f * (float) segs));
        const float sh = r.getHeight() / (float) segs;
        for (int i = 0; i < segs; ++i)
        {
            const auto seg = juce::Rectangle<float> (r.getX(), r.getBottom() - (float) (i + 1) * sh + 1.f, r.getWidth(), sh - 2.f);
            const auto c = i >= segs - 2 ? col::red : (i >= segs - 5 ? col::hazard : col::acid);
            g.setColour (i < lit ? c : c.withAlpha (0.08f));
            g.fillRect (seg);
        }
    };
    auto inner = b.reduced (3.f);
    drawBar (inner.removeFromLeft (inner.getWidth() * 0.5f - 1.f), holdL);
    inner.removeFromLeft (2.f);
    drawBar (inner, holdR);
}
} // namespace ui
