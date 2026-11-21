// WRAITHCORE - FX rack page, mod page, preset browser
#pragma once
#include "Industrial.h"

namespace ui
{
//==============================================================================
class SlotCard : public juce::Component
{
public:
    SlotCard (WraithCoreProcessor&, int slot);
    void refresh();
    void paint (juce::Graphics&) override;
    void resized() override;
    std::function<void (int slot, int dir)> onMove;

private:
    WraithCoreProcessor& proc;
    int slot, shownType = -1;
    juce::ComboBox typeBox;
    juce::ToggleButton power;
    std::unique_ptr<juce::ButtonParameterAttachment> powerAtt;
    juce::TextButton left { "<" }, right { ">" };
    std::unique_ptr<ParamControl> knobs[4], mix;
};

class RackPage : public juce::Component, private juce::Timer
{
public:
    explicit RackPage (WraithCoreProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { for (auto* c : cards) c->refresh(); }
    WraithCoreProcessor& proc;
    juce::OwnedArray<SlotCard> cards;
    std::unique_ptr<ParamControl> routing, xover, rackMix, laneLvl[2], laneOn[2];
};

//==============================================================================
class MatrixRow : public juce::Component
{
public:
    MatrixRow (WraithCoreProcessor&, int row);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshNames();
    void refreshLive() { repaint (0, 0, 28, getHeight()); }

private:
    WraithCoreProcessor& proc;
    int row;
    juce::ComboBox src, dst;
    juce::Slider amt;
    std::unique_ptr<juce::ComboBoxParameterAttachment> srcAtt, dstAtt;
    std::unique_ptr<juce::SliderParameterAttachment> amtAtt;
};

class ModPage : public juce::Component, private juce::Timer
{
public:
    explicit ModPage (WraithCoreProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    WraithCoreProcessor& proc;
    std::unique_ptr<Panel> macros, lfoA, lfoB, env;
    juce::OwnedArray<MatrixRow> rows;
    juce::Rectangle<int> matrixArea;
    int lastTypes[wc::kSlots] {};
};

//==============================================================================
class BrowserOverlay : public juce::Component, public juce::ListBoxModel
{
public:
    explicit BrowserOverlay (WraithCoreProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void rebuild();
    void step (int dir);
    std::function<void()> onClose;

    int getNumRows() override { return (int) entries.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override { load (row); }

private:
    struct Entry { juce::String name, cat; int factory = -1; juce::File file; };
    void load (int row);
    int currentRow() const;
    WraithCoreProcessor& proc;
    std::vector<Entry> entries;
    int category = -1; // -1 all, 0..CAT_COUNT-1, 100 user
    juce::OwnedArray<juce::TextButton> catButtons;
    juce::TextEditor search;
    juce::ListBox list { "presets", this };
    juce::TextButton close { "CLOSE" };
};
} // namespace ui
