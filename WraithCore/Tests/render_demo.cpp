// Offline renderer: plays every factory preset through the engine, checks levels, writes WAVs.
// g++ -O2 -std=c++17 -ISource/DSP Tests/render_demo.cpp -o render_demo
#include "Engine.h"
#include "Presets.h"
#include <cstdio>
#include <string>
#include <xmmintrin.h>
#include <algorithm>
#include <functional>
using namespace wc;

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb"); if (! f) return;
    const uint32_t n = (uint32_t) L.size(), bytes = n * 4;
    auto u32 = [f] (uint32_t v) { std::fwrite (&v, 4, 1, f); }; auto u16 = [f] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + bytes); std::fwrite ("WAVEfmt ", 1, 8, f); u32 (16); u16 (1); u16 (2); u32 ((uint32_t) sr); u32 ((uint32_t) sr * 4); u16 (4); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 (bytes);
    for (uint32_t i = 0; i < n; ++i) { int16_t a = (int16_t) (clampf (L[i], -1, 1) * 32767), b = (int16_t) (clampf (R[i], -1, 1) * 32767); std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f); }
    std::fclose (f);
}

struct Ev { double beat; int note; bool on; };

int main (int argc, char** argv)
{
    _mm_setcsr (_mm_getcsr() | 0x8040);
    const int sr = 48000, block = 512; const double bpm = 150.0, spb = 60.0 / bpm * sr;
    std::vector<Ev> bass, upper;
    auto add = [] (std::vector<Ev>& v, double b, int n, double len) { v.push_back ({ b, n, true }); v.push_back ({ b + len, n, false }); };
    add (bass, 0.0, 34, 1.8); add (bass, 2.0, 34, 2.0); add (bass, 3.0, 41, 0.6); add (bass, 4.0, 37, 1.4); add (bass, 5.5, 32, 2.4); add (bass, 8.0, 29, 7.5);
    const int arp[] = { 58, 61, 65, 68, 70, 68, 65, 61, 56, 61, 65, 68, 73, 68, 65, 61 };
    for (int s = 0; s < 16; ++s) add (upper, s * 0.5, arp[s], 0.3);
    for (int n : { 58, 61, 65 }) add (upper, 8.0, n, 7.5);
    const double totalBeats = 22.0;
    const bool writeAll = argc > 1 && std::string (argv[1]) == "--wav";

    std::vector<float> demoL, demoR;
    std::vector<std::pair<std::string, float>> trims;
    const bool calibrate = argc > 1 && std::string (argv[1]) == "--calibrate";
    int fails = 0, idx = 0;
    for (auto& pr : factoryPresets())
    {
        Engine e; applyPreset (e.p, pr); e.prepare (sr); e.updateModules();
        const bool only808 = e.p.f (pid::synthLvl) < 0.001f;
        const bool synthOnly = ! e.p.b (pid::q8On);
        std::vector<Ev> evs;
        if (! synthOnly || pr.cat == CAT_BASS) for (auto& x : bass) evs.push_back ({ x.beat, x.note + (pr.cat == CAT_BASS ? 12 : 0), x.on });
        if (! only808 && pr.cat != CAT_BASS) for (auto& x : upper) evs.push_back (x);
        std::stable_sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) { return a.beat < b.beat; });
        const int total = (int) (totalBeats * spb);
        std::vector<float> L ((size_t) total), R ((size_t) total);
        size_t ei = 0;
        for (int pos = 0; pos < total; pos += block)
        {
            const int n = std::min (block, total - pos);
            e.updateModules();
            e.beginBlock (bpm, true, pos / spb);
            std::vector<MidiEv> mev;
            while (ei < evs.size() && evs[ei].beat * spb < pos + n)
            {
                const int t = std::max (0, (int) (evs[ei].beat * spb) - pos);
                mev.push_back ({ t, evs[ei].on ? MidiEv::NoteOn : MidiEv::NoteOff, evs[ei].note, 0.85f });
                ++ei;
            }
            e.process (&L[(size_t) pos], &R[(size_t) pos], n, mev.data(), (int) mev.size());
        }
        float peak = 0; double rms = 0; int bad = 0;
        for (int i = 0; i < total; ++i)
        {
            if (! std::isfinite (L[(size_t) i]) || ! std::isfinite (R[(size_t) i])) ++bad;
            peak = std::max (peak, std::max (std::fabs (L[(size_t) i]), std::fabs (R[(size_t) i])));
            rms += 0.5 * (L[(size_t) i] * L[(size_t) i] + R[(size_t) i] * R[(size_t) i]);
        }
        rms = 20.0 * std::log10 (std::sqrt (rms / total) + 1e-9);
        // gated loudness: mean power of the loudest half of 100 ms windows above -60 dB
        std::vector<double> wins;
        const int W = sr / 10;
        for (int w0 = 0; w0 + W <= total; w0 += W)
        {
            double acc = 0; for (int i = w0; i < w0 + W; ++i) acc += 0.5 * (L[(size_t) i] * L[(size_t) i] + R[(size_t) i] * R[(size_t) i]);
            acc /= W; if (acc > 1e-6) wins.push_back (acc);
        }
        std::sort (wins.begin(), wins.end(), std::greater<double>());
        double g = 0; const size_t half = std::max<size_t> (1, wins.size() / 2);
        for (size_t i = 0; i < half && i < wins.size(); ++i) g += wins[i];
        const double loud = wins.empty() ? -99.0 : 10.0 * std::log10 (g / (double) half);
        static const double target[CAT_COUNT] = { -10.0, -9.0, -10.0, -12.0, -15.0, -13.0, -14.0, -14.0, -13.0 };
        const double trimNeeded = target[pr.cat] - loud;
        trims.push_back ({ pr.name, (float) (presetTrim (pr.name) + trimNeeded) });
        const bool ok = bad == 0 && peak > 0.02f && peak < 1.51f;
        fails += ok ? 0 : 1;
        std::printf ("%3d %-13s %-24s peak %.2f  loud %6.1f dB  (trim %+5.1f) %s\n", idx, catName (pr.cat), pr.name.c_str(), peak, loud, trimNeeded, ok ? "" : "  <-- CHECK");
        std::string fn = "Tests/out/" + std::to_string (idx) + "_" + pr.name + ".wav";
        for (auto& c : fn) if (c == ' ' || c == '+') c = '_';
        if (writeAll) writeWav (fn, L, R, sr);
        // demo reel: 2 bars of every preset
        const int seg = (int) (8.0 * spb);
        const int off = (only808 || pr.cat == CAT_BASS) ? 0 : 0;
        demoL.insert (demoL.end(), L.begin() + off, L.begin() + off + seg);
        demoR.insert (demoR.end(), R.begin() + off, R.begin() + off + seg);
        ++idx;
    }
    writeWav ("Tests/out/WRAITHCORE_preset_reel.wav", demoL, demoR, sr);
    if (calibrate)
    {
        FILE* f = std::fopen ("Source/DSP/PresetTrims.h", "w");
        std::fprintf (f, "// generated by Tests/render_demo --calibrate : per-preset output trims (dB) for matched loudness\n#pragma once\n#include <string>\n#include <map>\nnamespace wc\n{\ninline float presetTrim (const std::string& name)\n{\n    static const std::map<std::string, float> t = {\n");
        for (auto& tr : trims) std::fprintf (f, "        { \"%s\", %.1ff },\n", tr.first.c_str(), std::clamp (tr.second, -18.f, 18.f));
        std::fprintf (f, "    };\n    auto it = t.find (name);\n    return it == t.end() ? 0.f : it->second;\n}\n} // namespace wc\n");
        std::fclose (f);
    }
    std::printf ("%d presets, %d flagged\n", idx, fails);
    return fails;
}
