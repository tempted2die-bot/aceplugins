// Offline test harness: renders every factory preset through the engine to WAV
// and checks for NaN/Inf, silence and runaway levels.
// Build: g++ -O2 -std=c++17 -I../Source render_demo.cpp -o render_demo
#include "SonarEngine.h"
#include "Presets.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <xmmintrin.h>

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 4;
    auto u32 = [f] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [f] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + dataBytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    u32 (16); u16 (1); u16 (2); u32 ((uint32_t) sr); u32 ((uint32_t) sr * 4); u16 (4); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 (dataBytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t a = (int16_t) (sw::clampf (L[i], -1.f, 1.f) * 32767.f);
        int16_t b = (int16_t) (sw::clampf (R[i], -1.f, 1.f) * 32767.f);
        std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f);
    }
    std::fclose (f);
}

struct Ev { double beat; int note; bool on; };

int main()
{
    _mm_setcsr (_mm_getcsr() | 0x8040); // FTZ/DAZ like JUCE's ScopedNoDenormals
    const int sr = 48000, block = 512;
    const double bpm = 140.0, spb = 60.0 / bpm * sr;

    // 2 bars of a dark minor arp (16ths gated), then a 2-bar held chord, then tail
    std::vector<Ev> evs;
    const int arp[] = { 60, 63, 67, 70, 72, 70, 67, 63, 58, 63, 67, 70, 75, 70, 67, 63 };
    for (int s = 0; s < 16; ++s)
    {
        const double b = s * 0.5; // 8th notes over 2 bars
        evs.push_back ({ b, arp[s], true });
        evs.push_back ({ b + 0.3, arp[s], false });
    }
    for (int nn : { 48, 60, 63, 67 }) { evs.push_back ({ 8.0, nn, true }); evs.push_back ({ 15.5, nn, false }); }
    const double totalBeats = 16.0 + 6.0;

    std::vector<float> allL, allR;
    bool ok = true;
    for (auto& pr : sw::factoryPresets())
    {
        sw::Engine eng;
        sw::applyPreset (eng.p, pr);
        eng.prepare (sr, block);
        const int total = (int) (totalBeats * spb);
        std::vector<float> L ((size_t) total, 0.f), R ((size_t) total, 0.f);
        size_t ei = 0;
        for (int pos = 0; pos < total; pos += block)
        {
            const int n = std::min (block, total - pos);
            eng.beginBlock (bpm, true, pos / spb);
            int done = 0;
            while (ei < evs.size() && evs[ei].beat * spb < pos + n)
            {
                const int t = std::max (0, (int) (evs[ei].beat * spb) - pos);
                if (t > done) { eng.renderVoices (&L[(size_t) (pos + done)], &R[(size_t) (pos + done)], t - done); done = t; }
                if (evs[ei].on) eng.noteOn (evs[ei].note, 0.85f); else eng.noteOff (evs[ei].note);
                ++ei;
            }
            if (done < n) eng.renderVoices (&L[(size_t) (pos + done)], &R[(size_t) (pos + done)], n - done);
            eng.processFx (&L[(size_t) pos], &R[(size_t) pos], n);
        }
        float peak = 0.f; double rms = 0.0; int bad = 0;
        for (int i = 0; i < total; ++i)
        {
            if (! std::isfinite (L[(size_t) i]) || ! std::isfinite (R[(size_t) i])) ++bad;
            peak = std::max (peak, std::max (std::fabs (L[(size_t) i]), std::fabs (R[(size_t) i])));
            rms += L[(size_t) i] * L[(size_t) i];
        }
        rms = std::sqrt (rms / total);
        const bool pass = bad == 0 && peak > 0.01f && peak < 1.6f;
        ok &= pass;
        std::printf ("%-22s peak %.3f  rms %.4f  nan %d  %s\n", pr.name, peak, rms, bad, pass ? "OK" : "FAIL");

        std::string fn = std::string ("out/") + pr.name + ".wav";
        for (auto& c : fn) if (c == ' ') c = '_';
        writeWav (fn, L, R, sr);
        allL.insert (allL.end(), L.begin(), L.end()); allR.insert (allR.end(), R.begin(), R.end());
        allL.insert (allL.end(), (size_t) (sr / 2), 0.f); allR.insert (allR.end(), (size_t) (sr / 2), 0.f);
    }
    writeWav ("out/ALL_PRESETS_DEMO.wav", allL, allR, sr);
    return ok ? 0 : 1;
}
