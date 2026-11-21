#include "Pages.h"

namespace ui
{
//==============================================================================
SlotCard::SlotCard (WraithCoreProcessor& p, int s) : proc (p), slot (s)
{
    typeBox.addItem ("EMPTY", 1);
    const char* groups[] = { "DRIVE", "TONE", "DYNAMICS", "SPACE", "GLITCH", "CHARACTER" };
    for (auto* grp : groups)
    {
        typeBox.addSectionHeading (grp);
        for (int t = 1; t < wc::FX_COUNT; ++t)
            if (juce::String (wc::fxInfo (t).group) == grp) typeBox.addItem (wc::fxInfo (t).name, t + 1);
    }
    typeBox.setTooltip ("Pick a module for this slot");
    typeBox.onChange = [this]
    {
        const int t = typeBox.getSelectedId() - 1;
        if (t >= 0 && t != proc.slotType (slot)) proc.setSlotType (slot, t);
        refresh();
    };
    addAndMakeVisible (typeBox);

    power.setColour (juce::ToggleButton::tickColourId, col::acid);
    powerAtt = std::make_unique<juce::ButtonParameterAttachment> (*proc.apvts.getParameter (wc::paramDefs()[(size_t) wc::slotId (slot, wc::SF_ON)].id), power);
    addAndMakeVisible (power);

    for (auto* b : { &left, &right })
    {
        b->getProperties().set ("style", "plate");
        addAndMakeVisible (b);
    }
    left.setTooltip ("Move module left");
    right.setTooltip ("Move module right");
    left.onClick = [this] { if (onMove) onMove (slot, -1); };
    right.onClick = [this] { if (onMove) onMove (slot, +1); };

    for (int k = 0; k < 4; ++k)
    {
        knobs[k] = std::make_unique<ParamControl> (proc, wc::slotId (slot, wc::SF_P0 + k), col::acid, "P" + juce::String (k + 1));
        addAndMakeVisible (*knobs[k]);
    }
    mix = std::make_unique<ParamControl> (proc, wc::slotId (slot, wc::SF_MIX), col::text, "MIX");
    addAndMakeVisible (*mix);
    refresh();
}

void SlotCard::refresh()
{
    const int t = proc.slotType (slot);
    if (t == shownType) return;
    shownType = t;
    typeBox.setSelectedId (t + 1, juce::dontSendNotification);
    const auto& info = wc::fxInfo (t);
    const auto acc = accentFor (t);
    for (int k = 0; k < 4; ++k)
    {
        knobs[k]->setLabel (info.p[k]);
        knobs[k]->setAccent (acc);
        knobs[k]->setVisible (t != wc::FX_NONE && juce::String (info.p[k]).isNotEmpty());
    }
    mix->setVisible (t != wc::FX_NONE);
    power.setVisible (t != wc::FX_NONE);
    power.setColour (juce::ToggleButton::tickColourId, acc);
    typeBox.setColour (juce::ComboBox::textColourId, t == wc::FX_NONE ? col::dim : acc);
    repaint();
}

void SlotCard::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (3.f);
    const int t = shownType;
    if (t <= wc::FX_NONE)
    {
        g.setColour (col::plateLo.withAlpha (0.7f));
        g.fillRoundedRectangle (b, 4.f);
        juce::Path dash; dash.addRoundedRectangle (b.reduced (2.f), 4.f);
        juce::Path dashed; const float d[] = { 6.f, 5.f };
        juce::PathStrokeType (1.2f).createDashedStroke (dashed, dash, d, 2);
        g.setColour (col::dim.withAlpha (0.5f)); g.fillPath (dashed);
        g.setColour (col::dim);
        const auto c = b.getCentre().translated (0.f, -12.f);
        g.fillRect (c.x - 12.f, c.y - 1.5f, 24.f, 3.f);
        g.fillRect (c.x - 1.5f, c.y - 12.f, 3.f, 24.f);
        g.setFont (mono (11.f));
        g.drawText ("SLOT " + juce::String (wc::slotLabel (slot)) + " // PICK A MODULE", b.withTrimmedTop (b.getHeight() * 0.5f + 10.f).withHeight (20.f).toNearestInt(), juce::Justification::centred);
        return;
    }
    const auto acc = accentFor (t);
    drawPlate (g, b, (juce::uint32) (slot * 7919 + t * 104729), true, 4.f);
    // header LED strip
    auto hdr = b.reduced (12.f, 0.f).removeFromTop (34.f);
    g.setColour (acc.withAlpha (0.9f));
    g.fillRect (hdr.getX(), hdr.getBottom() - 2.f, hdr.getWidth(), 1.f);
    // blurb at the bottom
    g.setColour (col::dim);
    g.setFont (mono (10.f));
    g.drawFittedText (juce::String (wc::fxInfo (t).blurb).toUpperCase(), b.reduced (12.f, 6.f).removeFromBottom (14.f).toNearestInt(), juce::Justification::centredLeft, 1, 0.6f);
    // slot id badge
    auto badge = juce::Rectangle<float> (b.getX() + 12.f, b.getY() + 8.f, 26.f, 18.f);
    g.setColour (acc); g.fillRoundedRectangle (badge, 2.f);
    g.setColour (juce::Colours::black);
    g.setFont (display (14.f));
    g.drawText (wc::slotLabel (slot), badge.toNearestInt(), juce::Justification::centred);
}

void SlotCard::resized()
{
    auto b = getLocalBounds().reduced (13, 7);
    auto hdr = b.removeFromTop (24);
    hdr.removeFromLeft (32);
    right.setBounds (hdr.removeFromRight (20));
    left.setBounds (hdr.removeFromRight (20));
    hdr.removeFromRight (4);
    power.setBounds (hdr.removeFromRight (34).reduced (0, 2));
    hdr.removeFromRight (4);
    typeBox.setBounds (hdr);
    b.removeFromTop (6);
    b.removeFromBottom (16);
    auto right = b.removeFromRight (juce::jmax (60, b.getWidth() / 3));
    mix->setBounds (right.withSizeKeepingCentre (right.getWidth(), juce::jmin (right.getHeight(), right.getWidth() + 28)));
    const int cw = b.getWidth() / 2, ch = b.getHeight() / 2;
    for (int k = 0; k < 4; ++k) knobs[k]->setBounds (b.getX() + (k % 2) * cw, b.getY() + (k / 2) * ch, cw, ch);
}

//==============================================================================
RackPage::RackPage (WraithCoreProcessor& p) : proc (p)
{
    for (int s = 0; s < wc::kSlots; ++s)
    {
        auto* c = cards.add (new SlotCard (proc, s));
        c->onMove = [this] (int sl, int dir)
        {
            const int lane = sl / wc::kSlotsPerLane, pos = sl % wc::kSlotsPerLane + dir;
            int target = sl + dir;
            if (pos < 0 || pos >= wc::kSlotsPerLane) target = (1 - lane) * wc::kSlotsPerLane + (dir > 0 ? 0 : wc::kSlotsPerLane - 1); // hop to the other lane
            proc.swapSlots (sl, target);
            timerCallback();
        };
        addAndMakeVisible (c);
    }
    routing = std::make_unique<ParamControl> (proc, wc::rackId (wc::R_ROUTING), col::hazard, "ROUTING");
    xover = std::make_unique<ParamControl> (proc, wc::rackId (wc::R_XOVER), col::hazard, "SPLIT FREQ");
    rackMix = std::make_unique<ParamControl> (proc, wc::rackId (wc::R_MIX), col::text, "RACK MIX");
    for (int l = 0; l < 2; ++l)
    {
        laneLvl[l] = std::make_unique<ParamControl> (proc, wc::rackId (l == 0 ? wc::R_LANEA_LVL : wc::R_LANEB_LVL), col::text, "LEVEL");
        laneOn[l] = std::make_unique<ParamControl> (proc, wc::rackId (l == 0 ? wc::R_LANEA_ON : wc::R_LANEB_ON), col::acid, "ACTIVE");
        addAndMakeVisible (*laneLvl[l]); addAndMakeVisible (*laneOn[l]);
    }
    for (auto* c : { routing.get(), xover.get(), rackMix.get() }) addAndMakeVisible (c);
    startTimerHz (10);
}

void RackPage::paint (juce::Graphics& g)
{
    auto b = getLocalBounds();
    auto top = b.removeFromTop (70).toFloat().reduced (3.f);
    drawPlate (g, top, 0xFACEu, true);
    g.setColour (col::hazard);
    g.setFont (display (22.f));
    g.drawText ("FX RACK", top.reduced (18.f, 0.f).toNearestInt(), juce::Justification::centredLeft);
    g.setColour (col::dim);
    g.setFont (mono (11.f));
    g.drawFittedText ("2 LANES x 5 SLOTS  //  < > MOVES A MODULE (ROUTINGS FOLLOW IT)  //  PARALLEL: AN INACTIVE LANE DROPS OUT  //  SPLIT: LOWS > A, HIGHS > B",
                      top.reduced (150.f, 0.f).withTrimmedRight (330.f).toNearestInt(), juce::Justification::centred, 2, 0.7f);
    b.removeFromTop (4);
    for (int l = 0; l < 2; ++l)
    {
        auto lane = b.removeFromTop (b.getHeight() / (2 - l)).toFloat();
        auto head = lane.removeFromLeft (92.f).reduced (3.f);
        drawPlate (g, head, (juce::uint32) (0xA0 + l), true);
        drawHazard (g, head.reduced (8.f).removeFromTop (8.f).withY (head.getY() + 10.f), 0.9f, 4.f);
        g.setColour (col::text);
        g.setFont (display (44.f));
        g.drawText (l == 0 ? "A" : "B", head.withTrimmedTop (18.f).removeFromTop (54.f).toNearestInt(), juce::Justification::centred);
        g.setColour (col::dim);
        g.setFont (mono (10.f));
        g.drawText ("LANE", head.withTrimmedTop (68.f).removeFromTop (14.f).toNearestInt(), juce::Justification::centred);
        // signal-flow arrows between cards
        const float cw = (lane.getWidth()) / (float) wc::kSlotsPerLane;
        g.setColour (col::hazard.withAlpha (0.55f));
        for (int s = 1; s < wc::kSlotsPerLane; ++s)
        {
            const float x = lane.getX() + cw * (float) s, y = lane.getCentreY();
            juce::Path a; a.addTriangle (x - 3.f, y - 5.f, x - 3.f, y + 5.f, x + 3.f, y);
            g.fillPath (a);
        }
    }
}

void RackPage::resized()
{
    auto b = getLocalBounds();
    auto top = b.removeFromTop (70).reduced (14, 6);
    rackMix->setBounds (top.removeFromRight (90));
    xover->setBounds (top.removeFromRight (90));
    routing->setBounds (top.removeFromRight (150).withTrimmedTop (6));
    b.removeFromTop (4);
    for (int l = 0; l < 2; ++l)
    {
        auto lane = b.removeFromTop (b.getHeight() / (2 - l));
        auto head = lane.removeFromLeft (92).reduced (10, 8);
        head.removeFromTop (88);
        laneOn[l]->setBounds (head.removeFromTop (30));
        laneLvl[l]->setBounds (head.withSizeKeepingCentre (head.getWidth(), juce::jmin (head.getHeight(), 96)));
        const int cw = lane.getWidth() / wc::kSlotsPerLane;
        for (int s = 0; s < wc::kSlotsPerLane; ++s)
            cards[l * wc::kSlotsPerLane + s]->setBounds (lane.getX() + s * cw + 4, lane.getY(), cw - 8, lane.getHeight());
    }
}

//==============================================================================
static juce::String targetName (WraithCoreProcessor& proc, int t)
{
    if (t <= 0) return "---";
    if (t <= wc::kSlotTargets)
    {
        const int s = (t - 1) / 5, k = (t - 1) % 5;
        const int type = proc.slotType (s);
        const juce::String slot = wc::slotLabel (s);
        if (type == wc::FX_NONE) return slot + (k < 4 ? " P" + juce::String (k + 1) : juce::String (" MIX")) + " (empty)";
        return slot + " " + wc::fxInfo (type).name + " " + (k < 4 ? juce::String (wc::fxInfo (type).p[k]) : juce::String ("MIX"));
    }
    const int f = t - 1 - wc::kSlotTargets;
    return f < (int) wc::fixedTargets().size() ? juce::String (wc::fixedTargets()[(size_t) f].second) : juce::String ("?");
}

MatrixRow::MatrixRow (WraithCoreProcessor& p, int r) : proc (p), row (r)
{
    for (int s = 0; s < wc::MS_COUNT; ++s) src.addItem (wc::modSourceName (s), s + 1);
    for (int t = 0; t < wc::numTargets(); ++t)
    {
        if (t == 1) dst.addSectionHeading ("FX RACK");
        if (t == 1 + wc::kSlotTargets) dst.addSectionHeading ("SYNTH / 808 / RACK");
        dst.addItem (targetName (proc, t), t + 1);
    }
    const auto& defs = wc::paramDefs();
    srcAtt = std::make_unique<juce::ComboBoxParameterAttachment> (*proc.apvts.getParameter (defs[(size_t) wc::mtxId (row, wc::MX_SRC)].id), src);
    dstAtt = std::make_unique<juce::ComboBoxParameterAttachment> (*proc.apvts.getParameter (defs[(size_t) wc::mtxId (row, wc::MX_DST)].id), dst);
    amt.setSliderStyle (juce::Slider::LinearHorizontal);
    amt.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    amt.setColour (juce::Slider::trackColourId, col::hazard);
    amtAtt = std::make_unique<juce::SliderParameterAttachment> (*proc.apvts.getParameter (defs[(size_t) wc::mtxId (row, wc::MX_AMT)].id), amt);
    amt.onValueChange = [this] { repaint(); };
    amt.setDoubleClickReturnValue (true, 0.0);
    src.setColour (juce::ComboBox::textColourId, col::cyan);
    dst.setColour (juce::ComboBox::textColourId, col::hazard);
    for (auto* c : { (juce::Component*) &src, (juce::Component*) &dst, (juce::Component*) &amt }) addAndMakeVisible (c);
}

void MatrixRow::refreshNames()
{
    for (int t = 1; t <= wc::kSlotTargets; ++t) dst.changeItemText (t + 1, targetName (proc, t));
}

void MatrixRow::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour ((row & 1) ? juce::Colours::white.withAlpha (0.02f) : juce::Colours::transparentBlack);
    g.fillRect (b);
    const int s = src.getSelectedId() - 1;
    const float v = s > 0 ? proc.modSourceValue (s) * (float) amt.getValue() : 0.f;
    auto led = juce::Rectangle<float> (8.f, b.getCentreY() - 6.f, 12.f, 12.f);
    g.setColour (col::groove); g.fillEllipse (led);
    if (std::abs (v) > 0.01f) { g.setColour (col::acid.withAlpha (juce::jlimit (0.15f, 1.f, std::abs (v)))); g.fillEllipse (led.reduced (2.f)); }
    g.setColour (col::dim);
    g.setFont (display (14.f));
    g.drawText (juce::String (row + 1), 26, 0, 20, getHeight(), juce::Justification::centred);
    g.setColour (col::hazard.withAlpha (0.7f));
    const float ax = (float) dst.getX() - 14.f;
    juce::Path a; a.addTriangle (ax, b.getCentreY() - 5.f, ax, b.getCentreY() + 5.f, ax + 8.f, b.getCentreY());
    g.fillPath (a);
    g.setColour (col::text);
    g.setFont (mono (12.f));
    const double av = amt.getValue();
    g.drawText ((av >= 0 ? "+" : "") + juce::String (juce::roundToInt (av * 100.0)) + "%", getWidth() - 56, 0, 52, getHeight(), juce::Justification::centredRight);
}

void MatrixRow::resized()
{
    auto b = getLocalBounds().reduced (0, 6);
    b.removeFromLeft (50);
    src.setBounds (b.removeFromLeft (170));
    b.removeFromLeft (24);
    dst.setBounds (b.removeFromLeft (250));
    b.removeFromRight (60);
    b.removeFromLeft (12);
    amt.setBounds (b);
}

ModPage::ModPage (WraithCoreProcessor& p) : proc (p)
{
    macros = std::make_unique<Panel> (proc, "MACROS", col::hazard, std::initializer_list<int> { wc::modId (wc::M_MACRO1), wc::modId (wc::M_MACRO2), wc::modId (wc::M_MACRO3), wc::modId (wc::M_MACRO4) }, 1);
    lfoA = std::make_unique<Panel> (proc, "LFO A", col::cyan, std::initializer_list<int> { wc::modId (wc::M_LFOA_SHAPE), wc::modId (wc::M_LFOA_SYNC), wc::modId (wc::M_LFOA_DIV), wc::modId (wc::M_LFOA_RATE) }, 1);
    lfoB = std::make_unique<Panel> (proc, "LFO B", col::violet, std::initializer_list<int> { wc::modId (wc::M_LFOB_SHAPE), wc::modId (wc::M_LFOB_SYNC), wc::modId (wc::M_LFOB_DIV), wc::modId (wc::M_LFOB_RATE) }, 1);
    env = std::make_unique<Panel> (proc, "ENV FOLLOW", col::acid, std::initializer_list<int> { wc::modId (wc::M_ENV_ATK), wc::modId (wc::M_ENV_REL), wc::modId (wc::M_ENV_GAIN) }, 1);
    const char* mn[] = { "1 BRIGHT", "2 DIRT", "3 SPACE", "4 CHAOS" };
    for (int i = 0; i < 4; ++i) macros->getControls()[i]->setLabel (mn[i]);
    for (auto* c : { macros.get(), lfoA.get(), lfoB.get(), env.get() }) addAndMakeVisible (c);
    for (int r = 0; r < wc::kMtxRows; ++r) addAndMakeVisible (rows.add (new MatrixRow (proc, r)));
    for (auto& t : lastTypes) t = -1;
    startTimerHz (20);
}

void ModPage::timerCallback()
{
    bool changed = false;
    for (int s = 0; s < wc::kSlots; ++s) { const int t = proc.slotType (s); if (t != lastTypes[s]) { lastTypes[s] = t; changed = true; } }
    for (auto* r : rows) { if (changed) r->refreshNames(); r->refreshLive(); }
    repaint (getLocalBounds().removeFromLeft (getWidth() / 2 - 300));
}

void ModPage::paint (juce::Graphics& g)
{
    auto m = matrixArea.toFloat();
    drawPlate (g, m, 0xBEEFu, true);
    auto hdr = m.reduced (18.f, 0.f).removeFromTop (34.f);
    g.setColour (col::hazard);
    g.fillRect (hdr.getX(), hdr.getY() + 11.f, 4.f, 14.f);
    g.setColour (col::text);
    g.setFont (display (18.f));
    g.drawText ("MOD MATRIX", hdr.withTrimmedLeft (10.f).toNearestInt(), juce::Justification::centredLeft);
    g.setColour (col::dim);
    g.setFont (mono (10.f));
    g.drawText ("SOURCE                        TARGET                                AMOUNT", hdr.withTrimmedLeft (160.f).toNearestInt(), juce::Justification::centredLeft);
    // live source meters under the LFO / env panels
    auto bar = [&] (juce::Rectangle<int> r, float v, bool bip, juce::Colour c)
    {
        auto rr = r.toFloat();
        g.setColour (col::groove); g.fillRoundedRectangle (rr, 2.f);
        const float mid = bip ? rr.getCentreX() : rr.getX();
        const float x = bip ? rr.getCentreX() + v * rr.getWidth() * 0.5f : rr.getX() + v * rr.getWidth();
        g.setColour (c); g.fillRect (juce::jmin (mid, x), rr.getY() + 2.f, std::abs (x - mid), rr.getHeight() - 4.f);
    };
    if (lfoA) bar (lfoA->getBounds().removeFromBottom (14).reduced (20, 3), proc.modSourceValue (wc::MS_LFOA), true, col::cyan);
    if (lfoB) bar (lfoB->getBounds().removeFromBottom (14).reduced (20, 3), proc.modSourceValue (wc::MS_LFOB), true, col::violet);
    if (env) bar (env->getBounds().removeFromBottom (14).reduced (20, 3), proc.modSourceValue (wc::MS_ENV), false, col::acid);
}

void ModPage::resized()
{
    auto b = getLocalBounds();
    auto left = b.removeFromLeft (500);
    const int h = left.getHeight();
    macros->setBounds (left.removeFromTop (h * 30 / 100));
    lfoA->setBounds (left.removeFromTop (h * 24 / 100));
    lfoB->setBounds (left.removeFromTop (h * 24 / 100));
    env->setBounds (left);
    b.removeFromLeft (4);
    matrixArea = b.reduced (3);
    auto inner = matrixArea.reduced (16, 0);
    inner.removeFromTop (40);
    inner.removeFromBottom (14);
    const int rh = inner.getHeight() / wc::kMtxRows;
    for (auto* r : rows) r->setBounds (inner.removeFromTop (rh));
}

//==============================================================================
BrowserOverlay::BrowserOverlay (WraithCoreProcessor& p) : proc (p)
{
    auto addCat = [this] (const juce::String& name, int id)
    {
        auto* b = catButtons.add (new juce::TextButton (name));
        b->getProperties().set ("style", "plate");
        b->setClickingTogglesState (false);
        b->onClick = [this, id] { category = id; rebuild(); };
        b->getProperties().set ("cat", id);
        addAndMakeVisible (b);
    };
    addCat ("ALL", -1);
    for (int c = 0; c < wc::CAT_COUNT; ++c) addCat (wc::catName (c), c);
    addCat ("USER", 100);
    search.setTextToShowWhenEmpty ("SEARCH...", col::dim);
    search.setFont (mono (14.f));
    search.onTextChange = [this] { rebuild(); };
    addAndMakeVisible (search);
    list.setRowHeight (28);
    list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (list);
    close.getProperties().set ("style", "hazard");
    close.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (close);
    rebuild();
}

void BrowserOverlay::rebuild()
{
    entries.clear();
    const auto q = search.getText().trim().toLowerCase();
    const auto& ps = wc::factoryPresets();
    if (category != 100)
        for (int i = 0; i < (int) ps.size(); ++i)
        {
            if (category >= 0 && ps[(size_t) i].cat != category) continue;
            const juce::String n = ps[(size_t) i].name;
            if (q.isNotEmpty() && ! n.toLowerCase().contains (q) && ! juce::String (wc::catName (ps[(size_t) i].cat)).toLowerCase().contains (q)) continue;
            entries.push_back ({ n, wc::catName (ps[(size_t) i].cat), i, {} });
        }
    if (category == 100 || category == -1)
        for (auto& f : proc.userPresets())
        {
            const auto n = f.getFileNameWithoutExtension();
            if (q.isNotEmpty() && ! n.toLowerCase().contains (q)) continue;
            entries.push_back ({ n, "USER", -1, f });
        }
    for (auto* b : catButtons) b->setToggleState ((int) b->getProperties()["cat"] == category, juce::dontSendNotification);
    list.updateContent();
    list.repaint();
    repaint();
}

int BrowserOverlay::currentRow() const
{
    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].name == proc.getPresetName()) return (int) i;
    return -1;
}

void BrowserOverlay::load (int row)
{
    if (row < 0 || row >= (int) entries.size()) return;
    const auto& e = entries[(size_t) row];
    if (e.factory >= 0) proc.setCurrentProgram (e.factory);
    else proc.loadUserPreset (e.file);
    list.repaint();
}

void BrowserOverlay::step (int dir)
{
    if (entries.empty()) return;
    const int n = (int) entries.size();
    int r = currentRow();
    r = r < 0 ? 0 : ((r + dir) % n + n) % n;
    load (r);
    list.scrollToEnsureRowIsOnscreen (r);
}

void BrowserOverlay::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool)
{
    if (row < 0 || row >= (int) entries.size()) return;
    const auto& e = entries[(size_t) row];
    const bool cur = e.name == proc.getPresetName();
    auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (2.f, 1.f);
    g.setColour (cur ? col::hazard.withAlpha (0.18f) : ((row & 1) ? juce::Colours::white.withAlpha (0.025f) : juce::Colours::transparentBlack));
    g.fillRect (r);
    if (cur) { g.setColour (col::hazard); g.fillRect (r.getX(), r.getY(), 3.f, r.getHeight()); }
    g.setColour (cur ? col::hazard : col::text);
    g.setFont (mono (14.f));
    g.drawText (e.name, r.withTrimmedLeft (12.f).toNearestInt(), juce::Justification::centredLeft);
    g.setColour (col::dim);
    g.setFont (mono (11.f));
    g.drawText (e.cat, r.withTrimmedRight (10.f).toNearestInt(), juce::Justification::centredRight);
}

void BrowserOverlay::paint (juce::Graphics& g)
{
    g.fillAll (col::bg.withAlpha (0.96f));
    auto b = getLocalBounds().toFloat().reduced (6.f);
    drawPlate (g, b, 0xB0B0u, true);
    auto hdr = b.reduced (20.f, 0.f).removeFromTop (44.f);
    drawHazard (g, hdr.removeFromLeft (40.f).reduced (0.f, 14.f), 1.f, 5.f);
    g.setColour (col::text);
    g.setFont (display (24.f));
    g.drawText ("PRESET BROWSER", hdr.withTrimmedLeft (12.f).toNearestInt(), juce::Justification::centredLeft);
    g.setColour (col::dim);
    g.setFont (mono (11.f));
    g.drawText (juce::String ((int) entries.size()) + " PRESETS  //  CLICK TO LOAD  //  < > IN THE HEADER STEPS THROUGH THIS LIST",
                hdr.withTrimmedLeft (230.f).toNearestInt(), juce::Justification::centredLeft);
    auto listBg = list.getBounds().toFloat().expanded (4.f);
    drawLcd (g, listBg);
}

void BrowserOverlay::resized()
{
    auto b = getLocalBounds().reduced (26, 16);
    auto hdr = b.removeFromTop (34);
    close.setBounds (hdr.removeFromRight (110).reduced (0, 2));
    hdr.removeFromRight (10);
    search.setBounds (hdr.removeFromRight (260).reduced (0, 3));
    b.removeFromTop (12);
    auto cats = b.removeFromLeft (200);
    for (auto* c : catButtons) { c->setBounds (cats.removeFromTop (38).reduced (0, 3)); }
    b.removeFromLeft (16);
    list.setBounds (b.reduced (4));
}
} // namespace ui
