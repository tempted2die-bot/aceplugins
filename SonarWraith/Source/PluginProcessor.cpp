#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

namespace
{
juce::String sectionFor (const juce::String& id)
{
    if (id.startsWith ("drv")) return "Drive";
    if (id.startsWith ("crs")) return "Crush";
    if (id.startsWith ("gl") && ! id.startsWith ("glide")) return "Glitch";
    if (id.startsWith ("dly")) return "Delay";
    if (id.startsWith ("rev")) return "Atmos";
    if (id.startsWith ("bub")) return "Bubble";
    if (id.startsWith ("lfo")) return "LFO";
    if (id.startsWith ("flt")) return "Filter";
    if (id.startsWith ("uni")) return "Unison";
    if (id == "atk" || id == "dec" || id == "sus" || id == "rel") return "Amp";
    return {};
}

juce::String hostName (const sw::ParamDef& d)
{
    const juce::String sec = sectionFor (d.id), nm = d.name;
    if (sec.isEmpty() || nm.startsWithIgnoreCase (sec)) return nm;
    return sec + " " + nm;
}

juce::String formatValue (float v, const juce::String& unit)
{
    if (unit == "Hz")  return v < 1000.f ? juce::String (v, v < 10.f ? 2 : 0) + " Hz" : juce::String (v / 1000.f, 2) + " kHz";
    if (unit == "ms")  return v < 1000.f ? juce::String (v, v < 10.f ? 1 : 0) + " ms" : juce::String (v / 1000.f, 2) + " s";
    if (unit == "st")  return (v >= 0 ? "+" : "") + juce::String (v, 1) + " st";
    if (unit == "ct")  return (v >= 0 ? "+" : "") + juce::String (v, 0) + " ct";
    if (unit == "dB")  return juce::String (v, 1) + " dB";
    if (unit == "x")   return juce::String (v, 2) + "x";
    if (unit == "bit") return juce::String (v, 1) + " bit";
    return juce::String (juce::roundToInt (v * 100.f)) + "%";
}
} // namespace

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout SonarWraithProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> ps;
    for (const auto& d : sw::paramDefs())
    {
        const juce::ParameterID id { d.id, 1 };
        const juce::String name = hostName (d);
        const juce::String unit = d.unit;
        switch (d.type)
        {
            case sw::PType::Float:
            {
                juce::NormalisableRange<float> range (d.min, d.max);
                if (d.centre > 0.f) range.setSkewForCentre (d.centre);
                ps.push_back (std::make_unique<juce::AudioParameterFloat> (id, name, range, d.def,
                    juce::AudioParameterFloatAttributes()
                        .withLabel (unit)
                        .withStringFromValueFunction ([unit] (float v, int) { return formatValue (v, unit); })));
                break;
            }
            case sw::PType::Int:
                ps.push_back (std::make_unique<juce::AudioParameterInt> (id, name, (int) d.min, (int) d.max, (int) d.def,
                    juce::AudioParameterIntAttributes().withLabel (unit)));
                break;
            case sw::PType::Choice:
                ps.push_back (std::make_unique<juce::AudioParameterChoice> (id, name,
                    juce::StringArray::fromTokens (d.choices, "|", ""), (int) d.def));
                break;
            case sw::PType::Bool:
                ps.push_back (std::make_unique<juce::AudioParameterBool> (id, name, d.def > 0.5f));
                break;
        }
    }
    return { ps.begin(), ps.end() };
}

SonarWraithProcessor::SonarWraithProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (auto& s : scope) s.store (0.f);
    const auto& defs = sw::paramDefs();
    for (int i = 0; i < sw::pid::COUNT; ++i)
        raw[(size_t) i] = apvts.getRawParameterValue (defs[(size_t) i].id);
}

bool SonarWraithProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void SonarWraithProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    scratch.setSize (2, juce::jmax (samplesPerBlock, 32));
}

void SonarWraithProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n <= 0) return;

    for (int i = 0; i < sw::pid::COUNT; ++i)
        engine.p.v[i] = raw[(size_t) i]->load (std::memory_order_relaxed);

    double bpm = 140.0, ppq = 0.0;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            playing = pos->getIsPlaying();
        }
    engine.beginBlock (bpm, playing, ppq);

    if (scratch.getNumSamples() < n) scratch.setSize (2, n, false, false, true);
    scratch.clear (0, n);
    float* L = scratch.getWritePointer (0);
    float* R = scratch.getWritePointer (1);

    int pos = 0;
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        const int t = juce::jlimit (0, n, metadata.samplePosition);
        if (t > pos) { engine.renderVoices (L + pos, R + pos, t - pos); pos = t; }

        if (msg.isNoteOn())                                 engine.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff())                           engine.noteOff (msg.getNoteNumber());
        else if (msg.isSustainPedalOn())                    engine.sustain (true);
        else if (msg.isSustainPedalOff())                   engine.sustain (false);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) engine.allOff();
        else if (msg.isPitchWheel())                        engine.setPitchBend ((float) (msg.getPitchWheelValue() - 8192) / 8192.f * 2.f);
    }
    if (pos < n) engine.renderVoices (L + pos, R + pos, n - pos);

    engine.processFx (L, R, n);

    if (buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom (0, 0, L, n);
        buffer.copyFrom (1, 0, R, n);
    }
    else if (buffer.getNumChannels() == 1)
    {
        auto* out = buffer.getWritePointer (0);
        for (int i = 0; i < n; ++i) out[i] = 0.5f * (L[i] + R[i]);
    }

    int w = scopeWrite.load (std::memory_order_relaxed);
    for (int i = 0; i < n; ++i)
    {
        scope[(size_t) w].store (0.5f * (L[i] + R[i]), std::memory_order_relaxed);
        if (++w >= kScopeSize) w = 0;
    }
    scopeWrite.store (w, std::memory_order_release);
}

//==============================================================================
void SonarWraithProcessor::setParam (int index, float realValue)
{
    if (auto* prm = apvts.getParameter (sw::paramDefs()[(size_t) index].id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (realValue));
}

int SonarWraithProcessor::getNumPrograms() { return (int) sw::factoryPresets().size(); }

const juce::String SonarWraithProcessor::getProgramName (int index)
{
    const auto& ps = sw::factoryPresets();
    return juce::isPositiveAndBelow (index, (int) ps.size()) ? juce::String (ps[(size_t) index].name) : juce::String();
}

void SonarWraithProcessor::setCurrentProgram (int index)
{
    const auto& ps = sw::factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) ps.size())) return;
    currentProgram = index;
    sw::Params p;
    sw::applyPreset (p, ps[(size_t) index]);
    for (int i = 0; i < sw::pid::COUNT; ++i) setParam (i, p.v[i]);
}

void SonarWraithProcessor::mutate()
{
    using namespace sw::pid;
    juce::Random r;
    auto rr = [&r] (float a, float b) { return a + r.nextFloat() * (b - a); };
    auto chance = [&r] (float c) { return r.nextFloat() < c; };
    auto logr = [&r] (float a, float b) { return a * std::pow (b / a, r.nextFloat()); };
    auto pick = [&r] (std::initializer_list<float> l) { return *(l.begin() + r.nextInt ((int) l.size())); };

    sw::Params p; // start from defaults so nothing stale survives
    p.v[voiceVol] = raw[(size_t) voiceVol]->load();
    p.v[outGain]  = raw[(size_t) outGain]->load();

    // oscillator
    p.v[oscWave]   = chance (0.4f) ? 0.f : pick ({ 0.f, 0.33f, 0.66f, 1.f, rr (0.f, 1.f) });
    p.v[oscFold]   = chance (0.35f) ? rr (0.05f, 0.5f) : 0.f;
    p.v[oscOct]    = pick ({ -1.f, 0.f, 0.f, 1.f });
    p.v[uniVoices] = chance (0.3f) ? (float) (2 + r.nextInt (3)) : 1.f;
    p.v[uniDetune] = rr (0.1f, 0.5f);
    p.v[subLvl]    = chance (0.25f) ? rr (0.2f, 0.6f) : 0.f;
    p.v[noiseLvl]  = chance (0.2f) ? rr (0.02f, 0.2f) : 0.f;
    // fm + ping
    p.v[fmAmt]     = chance (0.5f) ? rr (0.1f, 0.6f) : 0.f;
    p.v[fmRatio]   = pick ({ 0.5f, 1.f, 1.5f, 2.f, 3.f, 3.5f, 4.f, 5.f, 7.f, 11.f });
    p.v[pingAmt]   = pick ({ -24.f, -12.f, 7.f, 12.f, 19.f, 24.f, 36.f });
    p.v[pingDecay] = logr (10.f, 250.f);
    // bubbler
    if (chance (0.5f))
    {
        p.v[bubRate]   = rr (3.f, 16.f);
        p.v[bubChance] = rr (0.4f, 0.9f);
        p.v[bubRand]   = pick ({ 0.f, 5.f, 7.f, 12.f, 24.f });
        p.v[bubGate]   = rr (0.3f, 1.f);
    }
    // filter
    p.v[fltType]  = pick ({ 0.f, 0.f, 0.f, 1.f, 2.f, 3.f });
    p.v[fltCut]   = logr (300.f, 12000.f);
    p.v[fltRes]   = rr (0.f, 0.85f);
    p.v[fltEnv]   = rr (-0.3f, 0.7f);
    p.v[fltDecay] = logr (30.f, 800.f);
    // amp
    p.v[atk] = chance (0.25f) ? rr (200.f, 1500.f) : rr (0.f, 8.f);
    p.v[dec] = logr (80.f, 1500.f);
    p.v[sus] = chance (0.5f) ? 0.f : rr (0.3f, 1.f);
    p.v[rel] = logr (100.f, 2500.f);
    // lfo
    p.v[lfoShape] = (float) r.nextInt (6);
    p.v[lfoSync]  = chance (0.6f) ? 1.f : 0.f;
    p.v[lfoDiv]   = (float) r.nextInt (10);
    p.v[lfoRate]  = logr (0.1f, 12.f);
    p.v[lfoPitch] = chance (0.25f) ? rr (0.1f, 12.f) : 0.f;
    p.v[lfoCut]   = rr (0.f, 0.4f);
    // fx
    p.v[drvOn] = chance (0.5f) ? 1.f : 0.f; p.v[drvMode] = (float) r.nextInt (5); p.v[drvAmt] = rr (0.15f, 0.8f);
    p.v[drvTone] = rr (0.3f, 1.f); p.v[drvMix] = rr (0.4f, 1.f);
    p.v[crsOn] = chance (0.4f) ? 1.f : 0.f; p.v[crsBits] = rr (4.f, 12.f); p.v[crsRate] = logr (2000.f, 24000.f); p.v[crsMix] = rr (0.3f, 1.f);
    p.v[glOn] = chance (0.5f) ? 1.f : 0.f; p.v[glDiv] = (float) (1 + r.nextInt (4)); p.v[glChance] = rr (0.1f, 0.6f);
    p.v[glRev] = rr (0.f, 0.6f); p.v[glPitch] = rr (0.f, 0.5f); p.v[glMix] = rr (0.6f, 1.f);
    p.v[dlyOn] = chance (0.75f) ? 1.f : 0.f; p.v[dlyDiv] = (float) r.nextInt (9); p.v[dlyFb] = rr (0.2f, 0.75f);
    p.v[dlyTone] = rr (0.2f, 0.9f); p.v[dlyWarble] = rr (0.f, 0.5f); p.v[dlyMix] = rr (0.15f, 0.45f); p.v[dlyPP] = chance (0.6f) ? 1.f : 0.f;
    p.v[revOn] = chance (0.8f) ? 1.f : 0.f; p.v[revSize] = rr (0.4f, 0.98f); p.v[revDamp] = rr (0.2f, 0.8f);
    p.v[revShimmer] = chance (0.35f) ? rr (0.1f, 0.6f) : 0.f; p.v[revMix] = rr (0.15f, 0.55f);

    for (int i = 0; i < COUNT; ++i) setParam (i, p.v[i]);
}

//==============================================================================
void SonarWraithProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SonarWraithProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentProgram = (int) vt.getProperty ("program", 0);
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* SonarWraithProcessor::createEditor() { return new SonarWraithEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SonarWraithProcessor(); }
