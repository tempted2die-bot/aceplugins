#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::String formatValue (float v, const juce::String& unit)
{
    auto whole = [] (float x) { return juce::String (juce::roundToInt (x)); }; // String (float, 0) means "full precision" in JUCE
    if (unit == "Hz")  return v < 1000.f ? (v < 10.f ? juce::String (v, 2) : whole (v)) + " Hz" : juce::String (v / 1000.f, 2) + " kHz";
    if (unit == "ms")  return v < 1000.f ? (v < 10.f ? juce::String (v, 1) : whole (v)) + " ms" : juce::String (v / 1000.f, 2) + " s";
    if (unit == "st")  return (v >= 0 ? "+" : "") + juce::String (v, 1) + " st";
    if (unit == "ct")  return (v >= 0 ? "+" : "") + whole (v) + " ct";
    if (unit == "dB")  return juce::String (v, 1) + " dB";
    if (unit == "x")   return juce::String (v, 2) + "x";
    if (unit == "deg") return juce::String (juce::roundToInt (v)) + " deg";
    if (unit == "bip") return (v >= 0 ? "+" : "") + juce::String (juce::roundToInt (v * 100.f)) + "%";
    if (unit == "note")
    {
        const int n = juce::roundToInt (v);
        if (n <= 0) return "OFF (all keys)";
        return juce::MidiMessage::getMidiNoteName (n, true, true, 5) + " in FL (" + juce::String (n) + ")";
    }
    return juce::String (juce::roundToInt (v * 100.f)) + "%";
}
} // namespace

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout WraithCoreProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> ps;
    const auto& defs = wc::paramDefs();
    for (int i = 0; i < wc::NUM_PARAMS; ++i)
    {
        const auto& d = defs[(size_t) i];
        const juce::ParameterID id { d.id, 1 };
        const juce::String unit = d.unit;
        juce::String name = d.name;
        if (i < wc::pid::BASE_COUNT)
        {
            const juce::String sid = d.id;
            const juce::String prefix = sid.startsWith ("q8") ? "808 " : sid.startsWith ("ob") ? "Osc B " : sid.startsWith ("flt") ? "Filter "
                                      : sid.startsWith ("bub") ? "Bubble " : sid.startsWith ("lfo") ? "Voice LFO " : "";
            if (prefix.isNotEmpty() && ! name.startsWithIgnoreCase (prefix.trim())) name = prefix + name;
        }
        switch (d.type)
        {
            case wc::PType::Float:
            {
                juce::NormalisableRange<float> range (d.min, d.max);
                if (d.centre > 0.f) range.setSkewForCentre (d.centre);
                if (unit == "note") range.interval = 1.f;
                std::function<juce::String (float, int)> fmt;
                if (unit == "fx")
                {
                    const int slot = (i - wc::SLOT_BASE) / wc::SF_COUNT, field = (i - wc::SLOT_BASE) % wc::SF_COUNT;
                    fmt = [this, slot, field] (float v, int) -> juce::String
                    {
                        const int t = raw[(size_t) wc::slotId (slot, wc::SF_TYPE)] ? juce::roundToInt (raw[(size_t) wc::slotId (slot, wc::SF_TYPE)]->load()) : 0;
                        return wc::fxText (t, field - wc::SF_P0, v);
                    };
                }
                else fmt = [unit] (float v, int) { return formatValue (v, unit); };
                ps.push_back (std::make_unique<juce::AudioParameterFloat> (id, name, range, d.def,
                    juce::AudioParameterFloatAttributes().withLabel (unit == "fx" ? juce::String() : unit).withStringFromValueFunction (fmt)));
                break;
            }
            case wc::PType::Int:
                ps.push_back (std::make_unique<juce::AudioParameterInt> (id, name, (int) d.min, (int) d.max, (int) d.def, juce::AudioParameterIntAttributes().withLabel (unit)));
                break;
            case wc::PType::Choice:
                ps.push_back (std::make_unique<juce::AudioParameterChoice> (id, name, juce::StringArray::fromTokens (juce::String (d.choices), "|", ""), (int) d.def));
                break;
            case wc::PType::Bool:
                ps.push_back (std::make_unique<juce::AudioParameterBool> (id, name, d.def > 0.5f));
                break;
        }
    }
    return { ps.begin(), ps.end() };
}

WraithCoreProcessor::WraithCoreProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (auto& s : scope) s.store (0.f);
    const auto& defs = wc::paramDefs();
    for (int i = 0; i < wc::NUM_PARAMS; ++i) raw[(size_t) i] = apvts.getRawParameterValue (defs[(size_t) i].id);
    events.reserve (1024);
    setCurrentProgram (0);
    startTimerHz (25);
}

WraithCoreProcessor::~WraithCoreProcessor() { stopTimer(); }

bool WraithCoreProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void WraithCoreProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    scratch.setSize (2, juce::jmax (samplesPerBlock, 64));
    syncModules();
}

void WraithCoreProcessor::syncModules()
{
    wc::Params snap;
    for (int i = 0; i < wc::NUM_PARAMS; ++i) snap.v[i] = raw[(size_t) i]->load (std::memory_order_relaxed);
    engine.updateModules (snap);
}

void WraithCoreProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n <= 0) return;

    for (int i = 0; i < wc::NUM_PARAMS; ++i) engine.p.v[i] = raw[(size_t) i]->load (std::memory_order_relaxed);

    double bpm = 140.0, ppq = 0.0; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            playing = pos->getIsPlaying();
        }

    events.clear();
    for (const auto meta : midi)
    {
        if (events.size() >= events.capacity()) break;
        const auto m = meta.getMessage();
        const int t = juce::jlimit (0, n - 1, meta.samplePosition);
        if (m.isNoteOn())                               events.push_back ({ t, wc::MidiEv::NoteOn, m.getNoteNumber(), m.getFloatVelocity() });
        else if (m.isNoteOff())                         events.push_back ({ t, wc::MidiEv::NoteOff, m.getNoteNumber(), 0.f });
        else if (m.isSustainPedalOn())                  events.push_back ({ t, wc::MidiEv::Sustain, 1, 0.f });
        else if (m.isSustainPedalOff())                 events.push_back ({ t, wc::MidiEv::Sustain, 0, 0.f });
        else if (m.isAllNotesOff() || m.isAllSoundOff()) events.push_back ({ t, wc::MidiEv::AllOff, 0, 0.f });
        else if (m.isPitchWheel())                      events.push_back ({ t, wc::MidiEv::Bend, 0, (float) (m.getPitchWheelValue() - 8192) / 8192.f });
        else if (m.isController() && m.getControllerNumber() == 1) events.push_back ({ t, wc::MidiEv::ModWheel, 0, (float) m.getControllerValue() / 127.f });
    }

    if (scratch.getNumSamples() < n) scratch.setSize (2, n, false, false, true);
    float* L = scratch.getWritePointer (0);
    float* R = scratch.getWritePointer (1);
    engine.beginBlock (bpm, playing, ppq);
    engine.process (L, R, n, events.data(), (int) events.size());

    if (buffer.getNumChannels() >= 2) { buffer.copyFrom (0, 0, L, n); buffer.copyFrom (1, 0, R, n); }
    else if (buffer.getNumChannels() == 1) { auto* o = buffer.getWritePointer (0); for (int i = 0; i < n; ++i) o[i] = 0.5f * (L[i] + R[i]); }

    float pl = 0.f, pr = 0.f;
    int w = scopeWrite.load (std::memory_order_relaxed);
    for (int i = 0; i < n; ++i)
    {
        pl = std::max (pl, std::abs (L[i])); pr = std::max (pr, std::abs (R[i]));
        scope[(size_t) w].store (0.5f * (L[i] + R[i]), std::memory_order_relaxed);
        if (++w >= kScopeSize) w = 0;
    }
    scopeWrite.store (w, std::memory_order_release);
    meterL.store (std::max (pl, meterL.load() * 0.85f));
    meterR.store (std::max (pr, meterR.load() * 0.85f));
}

//==============================================================================
void WraithCoreProcessor::setParam (int index, float realValue)
{
    if (auto* prm = apvts.getParameter (wc::paramDefs()[(size_t) index].id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (realValue));
}

void WraithCoreProcessor::applyValues (const wc::Params& p)
{
    for (int i = 0; i < wc::NUM_PARAMS; ++i) setParam (i, p.v[i]);
    syncModules();
    ++presetChangeCounter;
}

int WraithCoreProcessor::getNumPrograms() { return (int) wc::factoryPresets().size(); }

const juce::String WraithCoreProcessor::getProgramName (int index)
{
    const auto& ps = wc::factoryPresets();
    return juce::isPositiveAndBelow (index, (int) ps.size()) ? juce::String (ps[(size_t) index].name) : juce::String();
}

void WraithCoreProcessor::setCurrentProgram (int index)
{
    const auto& ps = wc::factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) ps.size())) return;
    currentProgram = index;
    presetName = ps[(size_t) index].name;
    presetCategory = wc::catName (ps[(size_t) index].cat);
    wc::Params p;
    wc::applyPreset (p, ps[(size_t) index]);
    applyValues (p);
}

juce::File WraithCoreProcessor::userPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("WRAITHCORE").getChildFile ("User Presets");
}

juce::Array<juce::File> WraithCoreProcessor::userPresets() const
{
    auto files = userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.wcpreset");
    files.sort();
    return files;
}

bool WraithCoreProcessor::saveUserPreset (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    auto dir = userPresetFolder();
    if (! dir.createDirectory()) return false;
    auto state = apvts.copyState();
    state.setProperty ("presetName", clean, nullptr);
    state.setProperty ("presetCategory", "USER", nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (! xml || ! xml->writeTo (dir.getChildFile (clean + ".wcpreset"))) return false;
    presetName = clean; presetCategory = "USER";
    ++presetChangeCounter;
    return true;
}

bool WraithCoreProcessor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (! xml || ! xml->hasTagName (apvts.state.getType())) return false;
    auto vt = juce::ValueTree::fromXml (*xml);
    wc::Params p;
    const auto& defs = wc::paramDefs();
    for (int i = 0; i < wc::NUM_PARAMS; ++i)
    {
        auto child = vt.getChildWithProperty ("id", juce::String (defs[(size_t) i].id));
        if (child.isValid()) p.v[i] = (float) child.getProperty ("value", defs[(size_t) i].def);
    }
    presetName = file.getFileNameWithoutExtension();
    presetCategory = "USER";
    applyValues (p);
    return true;
}

//==============================================================================
int WraithCoreProcessor::slotType (int slot) const { return juce::roundToInt (raw[(size_t) wc::slotId (slot, wc::SF_TYPE)]->load()); }

void WraithCoreProcessor::setSlotType (int slot, int type)
{
    const auto& info = wc::fxInfo (type);
    setParam (wc::slotId (slot, wc::SF_TYPE), (float) type);
    if (type != wc::FX_NONE)
    {
        setParam (wc::slotId (slot, wc::SF_ON), 1.f);
        setParam (wc::slotId (slot, wc::SF_MIX), info.defMix);
        for (int k = 0; k < 4; ++k) setParam (wc::slotId (slot, wc::SF_P0 + k), info.def[k]);
    }
    syncModules();
}

void WraithCoreProcessor::swapSlots (int a, int b)
{
    if (a == b || a < 0 || b < 0 || a >= wc::kSlots || b >= wc::kSlots) return;
    float va[wc::SF_COUNT], vb[wc::SF_COUNT];
    for (int f = 0; f < wc::SF_COUNT; ++f) { va[f] = getParam (wc::slotId (a, f)); vb[f] = getParam (wc::slotId (b, f)); }
    for (int f = 0; f < wc::SF_COUNT; ++f) { setParam (wc::slotId (a, f), vb[f]); setParam (wc::slotId (b, f), va[f]); }
    // keep modulation routings attached to the module that moved
    for (int r = 0; r < wc::kMtxRows; ++r)
    {
        const int t = juce::roundToInt (getParam (wc::mtxId (r, wc::MX_DST)));
        if (t >= 1 && t <= wc::kSlotTargets)
        {
            const int s = (t - 1) / 5, k = (t - 1) % 5;
            if (s == a) setParam (wc::mtxId (r, wc::MX_DST), (float) (1 + b * 5 + k));
            else if (s == b) setParam (wc::mtxId (r, wc::MX_DST), (float) (1 + a * 5 + k));
        }
    }
    syncModules();
}

void WraithCoreProcessor::mutate()
{
    using namespace wc::pid;
    juce::Random r;
    auto rr = [&r] (float a, float b) { return a + r.nextFloat() * (b - a); };
    auto chance = [&r] (float c) { return r.nextFloat() < c; };
    auto logr = [&r] (float a, float b) { return a * std::pow (b / a, r.nextFloat()); };
    auto pick = [&r] (std::initializer_list<float> l) { return *(l.begin() + r.nextInt ((int) l.size())); };

    wc::Params p;
    for (int i = 0; i < wc::NUM_PARAMS; ++i) p.v[i] = getParam (i); // keep layers, matrix, macros, master

    // synth
    p.v[oscWave] = chance (0.4f) ? 0.f : pick ({ 0.f, 0.33f, 0.66f, 1.f, rr (0.f, 1.f) });
    p.v[oscFold] = chance (0.35f) ? rr (0.05f, 0.5f) : 0.f;
    p.v[uniVoices] = chance (0.35f) ? (float) (2 + r.nextInt (3)) : 1.f;
    p.v[uniDetune] = rr (0.1f, 0.45f);
    p.v[obLvl] = chance (0.35f) ? rr (0.2f, 0.8f) : 0.f;
    p.v[obWave] = rr (0.f, 1.f); p.v[obMode] = (float) r.nextInt (3); p.v[obSemi] = pick ({ 0.f, 7.f, 12.f, -5.f, 3.f });
    p.v[fmAmt] = chance (0.45f) ? rr (0.1f, 0.6f) : 0.f;
    p.v[fmRatio] = pick ({ 0.5f, 1.f, 2.f, 3.f, 3.5f, 4.f, 7.f });
    p.v[pingAmt] = pick ({ 0.f, 7.f, 12.f, 19.f, 24.f, -24.f });
    p.v[pingDecay] = logr (10.f, 200.f);
    p.v[bubRate] = chance (0.3f) ? rr (3.f, 14.f) : 0.f;
    p.v[bubRand] = pick ({ 0.f, 7.f, 12.f, 24.f }); p.v[bubGate] = rr (0.3f, 1.f);
    p.v[fltType] = pick ({ 0.f, 0.f, 1.f, 1.f, 2.f, 5.f, 6.f });
    p.v[fltCut] = logr (400.f, 9000.f); p.v[fltRes] = rr (0.f, 0.75f);
    p.v[fltEnv] = rr (-0.2f, 0.6f); p.v[fltDecay] = logr (40.f, 800.f); p.v[fltDrive] = chance (0.3f) ? rr (0.1f, 0.6f) : 0.f;
    p.v[atk] = chance (0.25f) ? rr (200.f, 1500.f) : rr (0.f, 8.f);
    p.v[dec] = logr (120.f, 1500.f); p.v[sus] = chance (0.5f) ? 0.f : rr (0.3f, 1.f); p.v[rel] = logr (100.f, 2500.f);
    // 808
    p.v[q8Shape] = pick ({ 0.f, 0.f, 0.2f, 0.5f }); p.v[q8Punch] = pick ({ 12.f, 17.f, 17.f, 19.f, 24.f, 36.f });
    p.v[q8PunchTime] = logr (60.f, 200.f); p.v[q8Phase] = pick ({ 0.f, 30.f, 60.f, 90.f });
    p.v[q8DrvMode] = (float) r.nextInt (5); p.v[q8Drive] = rr (0.3f, 0.8f); p.v[q8Asym] = rr (0.f, 0.6f);
    p.v[q8Knock] = rr (0.f, 0.6f); p.v[q8SubKeep] = rr (0.1f, 0.6f); p.v[q8Fm] = chance (0.3f) ? rr (0.1f, 0.5f) : 0.f;
    p.v[q8GrowlAmt] = chance (0.3f) ? rr (0.2f, 0.6f) : 0.f; p.v[q8Vowel] = rr (0.f, 1.f);
    p.v[q8WobOn] = chance (0.4f) ? 1.f : 0.f; p.v[q8WobDiv] = (float) r.nextInt (12); p.v[q8WobShape] = (float) r.nextInt (7);
    p.v[q8WobTarget] = (float) r.nextInt (6); p.v[q8WobDepth] = p.v[q8WobTarget] == 4.f ? rr (0.03f, 0.15f) : rr (0.4f, 0.9f);
    // FX rack: 2-5 random modules, keep the SPACE/CHAOS macro slots usable
    static const int pool[] = { wc::FX_DISTORT, wc::FX_CRUSH, wc::FX_BLOWN, wc::FX_DATAMOSH, wc::FX_FILTER, wc::FX_FORMANT, wc::FX_RESONATOR,
                                wc::FX_OTT, wc::FX_GATE, wc::FX_CHORUS, wc::FX_PHASER, wc::FX_STUTTER, wc::FX_TAPESTOP, wc::FX_GRAINS,
                                wc::FX_FREQSHIFT, wc::FX_RINGMOD, wc::FX_TAPE, wc::FX_DELAY, wc::FX_REVERSE };
    for (int s = 0; s < wc::kSlots; ++s) p.v[wc::slotId (s, wc::SF_TYPE)] = 0.f;
    const int count = 2 + r.nextInt (4);
    for (int k = 0; k < count; ++k)
    {
        const int s = k < 3 ? k : 5 + (k - 3);
        const int t = pool[r.nextInt ((int) (sizeof (pool) / sizeof (pool[0])))];
        const auto& info = wc::fxInfo (t);
        p.v[wc::slotId (s, wc::SF_TYPE)] = (float) t;
        p.v[wc::slotId (s, wc::SF_ON)] = 1.f;
        p.v[wc::slotId (s, wc::SF_MIX)] = juce::jlimit (0.f, 1.f, info.defMix * rr (0.6f, 1.2f));
        for (int j = 0; j < 4; ++j) p.v[wc::slotId (s, wc::SF_P0 + j)] = juce::jlimit (0.f, 1.f, info.def[j] + rr (-0.3f, 0.3f));
    }
    p.v[wc::slotId (9, wc::SF_TYPE)] = (float) wc::FX_REVERB; p.v[wc::slotId (9, wc::SF_ON)] = 1.f;
    p.v[wc::slotId (9, wc::SF_MIX)] = rr (0.1f, 0.35f);
    for (int j = 0; j < 4; ++j) p.v[wc::slotId (9, wc::SF_P0 + j)] = wc::fxInfo (wc::FX_REVERB).def[j];
    p.v[wc::rackId (wc::R_ROUTING)] = 0.f;
    presetName = "~ mutated ~";
    applyValues (p);
}

//==============================================================================
void WraithCoreProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("presetCategory", presetCategory, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void WraithCoreProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentProgram = (int) vt.getProperty ("program", 0);
            presetName = vt.getProperty ("presetName", "Init").toString();
            presetCategory = vt.getProperty ("presetCategory", "").toString();
            apvts.replaceState (vt);
            syncModules();
            ++presetChangeCounter;
        }
}

juce::AudioProcessorEditor* WraithCoreProcessor::createEditor() { return new WraithCoreEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new WraithCoreProcessor(); }
