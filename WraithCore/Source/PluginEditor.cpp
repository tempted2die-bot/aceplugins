#include "PluginEditor.h"

using namespace ui;

MainView::MainView (WraithCoreProcessor& p)
    : proc (p), rackPage (p), modPage (p), browser (p), scope (p), meter (p)
{
    using namespace wc::pid;
    // ---------------- SYNTH page ----------------
    {
        auto& a = synthPage.addRow (0.58f);
        synthPage.add (a, new Panel (proc, "OSC A", col::cyan, { oscWave, oscFold, oscOct, oscFine, oscALvl, uniVoices, uniDetune, subLvl, noiseLvl }, 2));
        synthPage.add (a, new Panel (proc, "OSC B", col::cyan, { obLvl, obWave, obMode, obOct, obSemi, obFine }, 2));
        synthPage.add (a, new Panel (proc, "FM + PING", col::acid, { fmRatio, fmAmt, pingAmt, pingDecay }, 2));
        synthPage.add (a, new Panel (proc, "FILTER", col::violet, { fltType, fltCut, fltRes, fltDrive, fltEnv, fltDecay, fltKey }, 2));
        auto& b = synthPage.addRow (0.42f);
        synthPage.add (b, new Panel (proc, "BUBBLER", col::acid, { bubRate, bubChance, bubRand, bubGate }, 1));
        synthPage.add (b, new Panel (proc, "AMP", col::violet, { atk, dec, sus, rel }, 1));
        synthPage.add (b, new Panel (proc, "VOICE LFO", col::hazard, { lfoShape, lfoSync, lfoRate, lfoDiv, lfoPitch, lfoCut }, 1));
        synthPage.add (b, new Panel (proc, "VOICE", col::cyan, { polyMode, glide, voiceVol, bendRange }, 1));
    }
    // ---------------- 808 page ----------------
    {
        auto& a = quakePage.addRow (0.6f);
        quakePage.add (a, new Panel (proc, "808 CORE", col::orange, { q8Oct, q8Shape, q8Punch, q8PunchTime, q8Click, q8Phase, q8SubKeep }, 2, q8On));
        quakePage.add (a, new Panel (proc, "808 DIRT", col::red, { q8DrvMode, q8Drive, q8Fold, q8Asym, q8Tone, q8Knock }, 2));
        quakePage.add (a, new Panel (proc, "FILTER + FM", col::orange, { q8Cut, q8Res, q8Fm, q8FmRatio }, 2));
        quakePage.add (a, new Panel (proc, "GROWL", col::acid, { q8Vowel, q8GrowlRes, q8GrowlAmt }, 2));
        auto& b = quakePage.addRow (0.4f);
        quakePage.add (b, new Panel (proc, "808 AMP + SLIDE", col::orange, { q8Decay, q8Sus, q8Rel, q8Mode, q8Glide }, 1));
        quakePage.add (b, new Panel (proc, "WOBBLE", col::acid, { q8WobDiv, q8WobShape, q8WobTarget, q8WobDepth, q8WobRetrig }, 1, q8WobOn));
        quakePage.add (b, new Panel (proc, "LAYERS", col::text, { q8Lvl, splitNote, subMono }, 1));
    }
    addChildComponent (synthPage);
    addChildComponent (quakePage);
    addChildComponent (rackPage);
    addChildComponent (modPage);
    addChildComponent (browser);
    browser.onClose = [this] { showBrowser (false); };

    const char* tabNames[] = { "SYNTH", "808 QUAKE", "FX RACK", "MOD MATRIX" };
    const juce::Colour tabCols[] = { col::cyan, col::orange, col::acid, col::hazard };
    for (int i = 0; i < 4; ++i)
    {
        tabs[i].setButtonText (tabNames[i]);
        tabs[i].getProperties().set ("style", "tab");
        tabs[i].setColour (juce::TextButton::buttonOnColourId, tabCols[i]);
        tabs[i].onClick = [this, i] { showPage (i); };
        addAndMakeVisible (tabs[i]);
    }

    for (auto* b : { &prevBtn, &nextBtn, &browseBtn, &saveBtn }) { b->getProperties().set ("style", "plate"); addAndMakeVisible (b); }
    prevBtn.onClick = [this] { browser.step (-1); };
    nextBtn.onClick = [this] { browser.step (+1); };
    browseBtn.onClick = [this] { showBrowser (! browser.isVisible()); };
    saveBtn.onClick = [this] { savePreset(); };
    mutateBtn.getProperties().set ("style", "hazard");
    mutateBtn.setTooltip ("Randomise synth, 808 and FX rack inside dark / glitchy / dubstep ranges");
    mutateBtn.onClick = [this] { proc.mutate(); };
    addAndMakeVisible (mutateBtn);
    addAndMakeVisible (scope);
    addAndMakeVisible (meter);

    const juce::Colour mc[] = { col::hazard, col::orange, col::cyan, col::acid };
    const char* mn[] = { "1 BRIGHT", "2 DIRT", "3 SPACE", "4 CHAOS" };
    for (int i = 0; i < 4; ++i)
    {
        macroKnobs[i] = std::make_unique<ParamControl> (proc, wc::modId (wc::M_MACRO1 + i), mc[i], mn[i]);
        addAndMakeVisible (*macroKnobs[i]);
    }
    q8Level = std::make_unique<ParamControl> (proc, q8Lvl, col::orange, "808 LVL");
    synthLevel = std::make_unique<ParamControl> (proc, synthLvl, col::cyan, "SYNTH LVL");
    split = std::make_unique<ParamControl> (proc, splitNote, col::hazard, "SPLIT");
    width = std::make_unique<ParamControl> (proc, wc::pid::width, col::text, "WIDTH");
    output = std::make_unique<ParamControl> (proc, outGain, col::text, "OUTPUT");
    limiter = std::make_unique<ParamControl> (proc, wc::pid::limiter, col::red, "CLIP 0dB");
    for (auto* c : { q8Level.get(), synthLevel.get(), split.get(), width.get(), output.get(), limiter.get() }) addAndMakeVisible (c);

    scaleBox.addItemList ({ "70%", "85%", "100%", "115%", "130%" }, 1);
    scaleBox.setSelectedId (3, juce::dontSendNotification);
    scaleBox.setTooltip ("UI size");
    scaleBox.onChange = [this]
    {
        static const float s[] = { 0.7f, 0.85f, 1.f, 1.15f, 1.3f };
        if (onScale) onScale (s[juce::jlimit (0, 4, scaleBox.getSelectedId() - 1)]);
    };
    addAndMakeVisible (scaleBox);

    showPage (0);
    setSize (W, H);
    startTimerHz (15);
}

MainView::~MainView() { stopTimer(); }

void MainView::showPage (int index)
{
    page = index;
    synthPage.setVisible (index == 0);
    quakePage.setVisible (index == 1);
    rackPage.setVisible (index == 2);
    modPage.setVisible (index == 3);
    for (int i = 0; i < 4; ++i) tabs[i].setToggleState (i == index, juce::dontSendNotification);
    showBrowser (false);
}

void MainView::showBrowser (bool show)
{
    if (show) browser.rebuild();
    browser.setVisible (show);
    browseBtn.setToggleState (show, juce::dontSendNotification);
    if (show) browser.toFront (false);
}

void MainView::savePreset()
{
    auto* aw = new juce::AlertWindow ("SAVE PRESET", "Name it. It lands in Documents/WRAITHCORE/User Presets.", juce::MessageBoxIconType::NoIcon);
    aw->setLookAndFeel (&getLookAndFeel());
    aw->addTextEditor ("name", proc.getPresetName() == "~ mutated ~" ? juce::String ("My Patch") : proc.getPresetName());
    aw->addButton ("SAVE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw] (int r)
    {
        if (r == 1) { proc.saveUserPreset (aw->getTextEditorContents ("name")); browser.rebuild(); }
        aw->setLookAndFeel (nullptr);
    }), true);
}

void MainView::timerCallback()
{
    if (glitchFrames > 0) { --glitchFrames; glitch = rnd.nextFloat(); }
    else { glitch = 0.f; if (rnd.nextFloat() < 0.035f) glitchFrames = 2 + rnd.nextInt (4); }
    const int pc = proc.presetChangeCounter.load();
    if (pc != lastPresetCounter) { lastPresetCounter = pc; glitchFrames = std::max (glitchFrames, 3); repaint (presetArea); }
    repaint (logoArea);
    if (glitchFrames > 0) repaint (presetArea);
}

void MainView::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);
    // background texture: grid + vignette
    g.setColour (juce::Colours::white.withAlpha (0.015f));
    for (int x = 0; x < W; x += 32) g.fillRect (x, 0, 1, H);
    for (int y = 0; y < H; y += 32) g.fillRect (0, y, W, 1);

    // ---------- header ----------
    drawHazard (g, { 0.f, 0.f, (float) W, 9.f }, 1.f, 9.f);
    auto header = juce::Rectangle<float> (4.f, 12.f, (float) W - 8.f, 84.f);
    drawPlate (g, header, 0xC0DEu, true);
    const int ww = juce::roundToInt (display (52.f).getStringWidthFloat ("WRAITH"));
    drawGlitchText (g, "WRAITH", logoArea.withWidth (ww + 12), display (52.f), col::text, juce::Justification::centredLeft, glitch, rnd);
    drawGlitchText (g, "CORE", logoArea.withTrimmedLeft (ww + 4), display (52.f), col::hazard, juce::Justification::centredLeft, glitch, rnd);
    g.setColour (col::dim);
    g.setFont (mono (11.f));
    g.drawText ("SONAR SYNTH // 808 QUAKE // FX RACK   v1.0   ACE CORP", logoArea.getX() + 2, logoArea.getBottom() - 4, 380, 14, juce::Justification::centredLeft);

    // preset LCD
    drawLcd (g, presetArea.toFloat());
    {
        auto r = presetArea.reduced (12, 6);
        g.setColour (col::acid.withAlpha (0.55f));
        g.setFont (mono (11.f));
        g.drawText ("PRESET // " + proc.getPresetCategory(), r.removeFromTop (14), juce::Justification::centredLeft);
        drawGlitchText (g, proc.getPresetName().toUpperCase(), r, mono (22.f), col::lcdText, juce::Justification::centredLeft, glitch * 0.6f, rnd);
        drawScanlines (g, presetArea.toFloat().reduced (2.f), 0.22f);
    }

    // ---------- footer ----------
    auto footer = juce::Rectangle<float> (4.f, (float) H - 128.f, (float) W - 8.f, 124.f);
    drawPlate (g, footer, 0xF00Du, true);
    g.setColour (col::hazard);
    g.setFont (display (16.f));
    g.drawText ("MACROS", (int) footer.getX() + 22, (int) footer.getY() + 6, 120, 20, juce::Justification::centredLeft);
    g.drawText ("LAYERS", 640, (int) footer.getY() + 6, 120, 20, juce::Justification::centredLeft);
    g.drawText ("MASTER", 925, (int) footer.getY() + 6, 120, 20, juce::Justification::centredLeft);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRect (628.f, footer.getY() + 12.f, 2.f, footer.getHeight() - 24.f);
    g.fillRect (912.f, footer.getY() + 12.f, 2.f, footer.getHeight() - 24.f);
    drawHazard (g, { footer.getRight() - 150.f, footer.getBottom() - 22.f, 130.f, 8.f }, 0.7f, 5.f);
    g.setColour (col::dim);
    g.setFont (mono (10.f));
    g.drawText ("UI SIZE", (int) footer.getRight() - 150, (int) footer.getY() + 8, 60, 16, juce::Justification::centredLeft);
}

void MainView::resized()
{
    // header
    logoArea = { 28, 22, 380, 60 };
    presetArea = { 420, 24, 330, 60 };
    prevBtn.setBounds (756, 24, 32, 28);
    nextBtn.setBounds (790, 24, 30, 28);
    browseBtn.setBounds (756, 56, 64, 28);
    saveBtn.setBounds (826, 24, 66, 60);
    mutateBtn.setBounds (898, 24, 110, 60);
    scope.setBounds (1016, 22, 214, 64);
    meter.setBounds (1238, 22, 26, 64);

    // tabs
    for (int i = 0; i < 4; ++i) tabs[i].setBounds (14 + i * 168, 100, 164, 34);

    // pages
    const juce::Rectangle<int> pageArea (6, 138, W - 12, H - 138 - 132);
    synthPage.setBounds (pageArea);
    quakePage.setBounds (pageArea);
    rackPage.setBounds (pageArea);
    modPage.setBounds (pageArea);
    browser.setBounds (pageArea);

    // footer
    const int fy = H - 124;
    for (int i = 0; i < 4; ++i) macroKnobs[i]->setBounds (30 + i * 148, fy + 18, 140, 100);
    q8Level->setBounds (648, fy + 22, 86, 96);
    synthLevel->setBounds (734, fy + 22, 86, 96);
    split->setBounds (820, fy + 22, 86, 96);
    width->setBounds (930, fy + 22, 80, 96);
    output->setBounds (1010, fy + 22, 80, 96);
    limiter->setBounds (1094, fy + 50, 74, 30);
    scaleBox.setBounds (W - 92, fy + 6, 72, 22);
}

//==============================================================================
WraithCoreEditor::WraithCoreEditor (WraithCoreProcessor& p) : AudioProcessorEditor (&p), proc (p), view (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (view);
    view.onScale = [this] (float s) { applyScale (s); proc.apvts.state.setProperty ("uiScale", s, nullptr); };
    const float s = (float) proc.apvts.state.getProperty ("uiScale", 1.0f);
    view.setScaleChoice (s);
    applyScale (s);
}

WraithCoreEditor::~WraithCoreEditor() { setLookAndFeel (nullptr); }

void WraithCoreEditor::applyScale (float s)
{
    s = juce::jlimit (0.5f, 2.f, s);
    view.setTransform (juce::AffineTransform::scale (s));
    setSize (juce::roundToInt (MainView::W * s), juce::roundToInt (MainView::H * s));
}

void WraithCoreEditor::resized() { view.setBounds (0, 0, MainView::W, MainView::H); }
