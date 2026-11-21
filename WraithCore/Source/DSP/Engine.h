// WRAITHCORE - engine: sonar synth layer + 808 QUAKE layer (key split) -> modulation -> FX rack -> master
#pragma once
#include "SynthVoice.h"
#include "Quake808.h"
#include "FxRack.h"

namespace wc
{
struct MidiEv
{
    enum Type : uint8_t { NoteOn, NoteOff, Sustain, AllOff, Bend, ModWheel };
    int pos; Type type; int a; float b;
};

struct ModLfo
{
    float ph = 0.f, prev = 0.f, next = 0.f; Rng rng;
    explicit ModLfo (uint32_t seed) : rng (seed) {}
    void advance (float inc) { ph += inc; if (ph >= 1.f) { ph -= std::floor (ph); prev = next; next = rng.bi(); } }
    void lockTo (float phase) { if (phase < ph - 0.5f) { prev = next; next = rng.bi(); } ph = phase; }
    float value (int shape) const
    {
        switch (shape)
        {
            case 0:  return std::sin (kTwoPi * ph);
            case 1:  return 1.f - 4.f * std::fabs (ph - 0.5f);
            case 2:  return 2.f * ph - 1.f;
            case 3:  return 1.f - 2.f * ph;
            case 4:  return ph < 0.5f ? 1.f : -1.f;
            case 5:  return next;
            default: { const float t = ph * ph * (3.f - 2.f * ph); return prev + (next - prev) * t; }
        }
    }
};

class Engine
{
public:
    Params p;   // base values (from the host)
    Params pm;  // modulated working copy
    static constexpr int kVoices = 12, k808Voices = 4, kSub = 32;
    std::array<std::atomic<float>, MS_COUNT> srcMonitor {}; // for the UI

    Engine()
    {
        for (int i = 0; i < kVoices; ++i) voices.emplace_back (0x1234567u + 7919u * (uint32_t) i);
        for (int i = 0; i < k808Voices; ++i) quakes.emplace_back (0xABCDEFu + 104729u * (uint32_t) i);
        held.reserve (128); held808.reserve (128);
        for (auto& m : srcMonitor) m.store (0.f);
    }

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        for (auto& v : voices) v.prepare (sr);
        rack.prepare (sr);
        sideLp1 = sideLp2 = 0.f; envVal = 0.f;
        allOff();
    }

    void updateModules() { rack.updateModules (p); }                               // offline use
    void updateModules (const Params& snapshot) { rack.updateModules (snapshot); }  // message thread
    int activeFxType (int slot) const { return rack.activeType (slot); }

    void beginBlock (double bpmIn, bool playingIn, double ppqIn)
    {
        bpm = (bpmIn > 20.0 && bpmIn < 999.0) ? bpmIn : 140.0;
        playing = playingIn; ppq = ppqIn;
        rack.adoptPending();
    }

    // renders one host block (L/R are overwritten)
    void process (float* L, float* R, int n, const MidiEv* evs, int nev)
    {
        std::fill (L, L + n, 0.f); std::fill (R, R + n, 0.f);
        const double spb = 60.0 / bpm * sr;
        int ei = 0, pos = 0;
        while (pos < n)
        {
            while (ei < nev && evs[ei].pos <= pos) handle (evs[ei++]);
            int end = std::min (n, pos + kSub);
            if (ei < nev && evs[ei].pos < end) end = std::max (pos + 1, evs[ei].pos);
            const int m = end - pos;
            const double ppqNow = ppq + (double) pos / spb;

            computeModulation (m, ppqNow);
            renderVoices (L + pos, R + pos, m);
            followEnvelope (L + pos, R + pos, m);
            FxCtx ctx { sr, bpm, playing, ppqNow };
            rack.process (L + pos, R + pos, m, slotP, slotMix, pm, ctx);
            master (L + pos, R + pos, m);
            pos = end;
        }
        while (ei < nev) handle (evs[ei++]);
    }

private:
    std::vector<Voice> voices;
    std::vector<Quake808> quakes;
    std::vector<int> held, held808;
    FxRack rack;
    float slotP[kSlots][4] {};
    float slotMix[kSlots] {};
    std::array<float, kSub> lfoBuf {}, wobBuf {}, sL {}, sR {}, qL {}, qR {};
    float sr = 48000.f, bendNorm = 0.f, modWheel = 0.f, lastVel = 0.8f, noteRand = 0.f, lastNote = -1.f, last808 = -1.f;
    double bpm = 140.0, ppq = 0.0; bool playing = false;
    bool sustainDown = false;
    uint64_t counter = 0;
    float lfoPh = 0.f, shVal = 0.f, shPrev = 0.f, shTarget = 0.f;
    float wobPh = 0.f, wobVal = 0.f, wobRand = 0.f;
    float sideLp1 = 0.f, sideLp2 = 0.f, envVal = 0.f;
    ModLfo lfoA { 0xA11Au }, lfoB { 0xB22Bu };
    Rng lfoRng { 0xC0FFEEu }, wobRng { 0xDEADBEEFu }, noteRng { 0x5EEDu };

    // ---------------- MIDI ----------------
    void handle (const MidiEv& e)
    {
        switch (e.type)
        {
            case MidiEv::NoteOn:   noteOn (e.a, e.b); break;
            case MidiEv::NoteOff:  noteOffSynth (e.a); noteOff808 (e.a); break;
            case MidiEv::Sustain:  sustain (e.a != 0); break;
            case MidiEv::AllOff:   allOff(); break;
            case MidiEv::Bend:     bendNorm = e.b; break;
            case MidiEv::ModWheel: modWheel = e.b; break;
        }
    }

    void noteOn (int note, float velocity)
    {
        lastVel = velocity; noteRand = noteRng.next();
        const int split = (int) (p.f (pid::splitNote) + 0.5f);
        if (p.b (pid::q8On) && (split == 0 || note < split)) noteOn808 (note, velocity);
        if (p.f (pid::synthLvl) > 0.0001f && (split == 0 || note >= split)) noteOnSynth (note, velocity);
    }

    void sustain (bool down)
    {
        sustainDown = down;
        if (! down)
        {
            for (auto& v : voices) if (v.pendingRelease) { v.pendingRelease = false; v.release(); }
            for (auto& q : quakes) if (q.pendingRelease) { q.pendingRelease = false; q.release(); }
        }
    }

    void allOff()
    {
        for (auto& v : voices) v.kill();
        for (auto& q : quakes) q.kill();
        held.clear(); held808.clear(); sustainDown = false;
    }

    void noteOnSynth (int note, float velocity)
    {
        const int mode = p.n (pid::polyMode);
        const bool glideOn = p.f (pid::glide) > 0.5f;
        if (mode == 0)
        {
            Voice* v = nullptr;
            for (auto& x : voices) if (x.active() && x.note == note) { v = &x; break; }
            if (! v) for (auto& x : voices) if (! x.active()) { v = &x; break; }
            if (! v) { v = &voices[0]; for (auto& x : voices) if (x.age < v->age) v = &x; }
            v->age = ++counter;
            v->start (note, velocity, lastNote, true, glideOn);
        }
        else
        {
            held.erase (std::remove (held.begin(), held.end(), note), held.end());
            if (held.size() < held.capacity()) held.push_back (note);
            auto& v = voices[0];
            if (v.active() && mode == 2) v.glideTo (note);
            else v.start (note, velocity, lastNote, true, glideOn || v.active());
            v.age = ++counter;
        }
        lastNote = (float) note;
    }

    void noteOffSynth (int note)
    {
        const int mode = p.n (pid::polyMode);
        if (mode == 0)
        {
            for (auto& v : voices)
                if (v.active() && v.note == note && v.gateHeld) { if (sustainDown) v.pendingRelease = true; else v.release(); }
        }
        else
        {
            const bool wasHeld = std::find (held.begin(), held.end(), note) != held.end();
            held.erase (std::remove (held.begin(), held.end(), note), held.end());
            auto& v = voices[0];
            if (wasHeld && v.note == note)
            {
                if (! held.empty()) v.glideTo (held.back());
                else if (sustainDown) v.pendingRelease = true;
                else v.release();
            }
        }
    }

    void noteOn808 (int note, float velocity)
    {
        const int mode = p.n (pid::q8Mode);
        const bool glideOn = p.f (pid::q8Glide) > 0.5f;
        bool retrigWob = true;
        if (mode == 0)
        {
            Quake808* q = nullptr;
            for (auto& x : quakes) if (x.active() && x.note == note) { q = &x; break; }
            if (! q) for (auto& x : quakes) if (! x.active()) { q = &x; break; }
            if (! q) { q = &quakes[0]; for (auto& x : quakes) if (x.age < q->age) q = &x; }
            q->age = ++counter;
            q->start (note, velocity, last808, false);
        }
        else
        {
            held808.erase (std::remove (held808.begin(), held808.end(), note), held808.end());
            if (held808.size() < held808.capacity()) held808.push_back (note);
            auto& q = quakes[0];
            if (q.active() && q.gateHeld && mode == 2) { q.slideTo (note); retrigWob = false; }
            else q.start (note, velocity, last808, glideOn && q.active());
            q.age = ++counter;
        }
        if (retrigWob && p.b (pid::q8WobRetrig)) wobPh = 0.f;
        last808 = (float) note;
    }

    void noteOff808 (int note)
    {
        const int mode = p.n (pid::q8Mode);
        if (mode == 0)
        {
            for (auto& q : quakes)
                if (q.active() && q.note == note && q.gateHeld) { if (sustainDown) q.pendingRelease = true; else q.release(); }
        }
        else
        {
            const bool wasHeld = std::find (held808.begin(), held808.end(), note) != held808.end();
            held808.erase (std::remove (held808.begin(), held808.end(), note), held808.end());
            auto& q = quakes[0];
            if (wasHeld && q.note == note)
            {
                if (! held808.empty()) q.slideTo (held808.back());
                else if (sustainDown) q.pendingRelease = true;
                else q.release();
            }
        }
    }

    // ---------------- modulation ----------------
    void computeModulation (int n, double ppqNow)
    {
        // sources
        float src[MS_COUNT] {};
        for (int m = 0; m < 4; ++m) src[MS_MACRO1 + m] = p.f (modId (M_MACRO1 + m));
        auto runLfo = [&] (ModLfo& l, int rateF, int syncF, int divF, int shapeF) -> float
        {
            if (p.b (modId (syncF)))
            {
                const double beats = divBeats13 (p.n (modId (divF)));
                if (playing) { const double t = ppqNow / beats; l.lockTo ((float) (t - std::floor (t))); }
                else l.advance ((float) (bpm / 60.0 / beats) * (float) n / sr);
            }
            else l.advance (p.f (modId (rateF)) * (float) n / sr);
            return l.value (p.n (modId (shapeF)));
        };
        src[MS_LFOA] = runLfo (lfoA, M_LFOA_RATE, M_LFOA_SYNC, M_LFOA_DIV, M_LFOA_SHAPE);
        src[MS_LFOB] = runLfo (lfoB, M_LFOB_RATE, M_LFOB_SYNC, M_LFOB_DIV, M_LFOB_SHAPE);
        src[MS_ENV] = clampf (envVal * dbToGain (p.f (modId (M_ENV_GAIN))), 0.f, 1.f);
        src[MS_MODWHEEL] = modWheel;
        src[MS_VELOCITY] = lastVel;
        src[MS_RANDOM] = noteRand;
        for (int s = 0; s < MS_COUNT; ++s) srcMonitor[(size_t) s].store (src[s], std::memory_order_relaxed);

        // matrix
        std::copy (std::begin (p.v), std::end (p.v), std::begin (pm.v));
        float slotOff[kSlots][5] {};
        const auto& defs = paramDefs();
        for (int r = 0; r < kMtxRows; ++r)
        {
            const int s = p.n (mtxId (r, MX_SRC)), t = p.n (mtxId (r, MX_DST));
            const float amt = p.f (mtxId (r, MX_AMT));
            if (s <= 0 || t <= 0 || amt == 0.f) continue;
            const float off = src[s] * amt;
            if (t <= kSlotTargets) { slotOff[(t - 1) / 5][(t - 1) % 5] += off; continue; }
            const int idx = targetParam (t);
            if (idx < 0) continue;
            const auto& d = defs[(size_t) idx];
            pm.v[idx] = fromNorm (d, toNorm (d, pm.v[idx]) + off);
        }
        for (int s = 0; s < kSlots; ++s)
        {
            for (int k = 0; k < 4; ++k) slotP[s][k] = clampf (p.v[slotId (s, SF_P0 + k)] + slotOff[s][k], 0.f, 1.f);
            slotMix[s] = clampf (p.v[slotId (s, SF_MIX)] + slotOff[s][4], 0.f, 1.f);
        }
    }

    void followEnvelope (const float* L, const float* R, int n)
    {
        const float a = onePoleCoef (1000.f / std::max (0.1f, p.f (modId (M_ENV_ATK))), sr);
        const float r = onePoleCoef (1000.f / std::max (1.f, p.f (modId (M_ENV_REL))), sr);
        for (int i = 0; i < n; ++i)
        {
            const float x = std::max (std::fabs (L[i]), std::fabs (R[i]));
            envVal += (x - envVal) * (x > envVal ? a : r);
        }
    }

    void renderVoices (float* L, float* R, int n)
    {
        const float bend = bendNorm * (float) pm.n (pid::bendRange);
        const float synthLvl = pm.f (pid::synthLvl), q8Lvl = pm.f (pid::q8Lvl);
        const bool subMono = pm.b (pid::subMono);
        const float sideC = onePoleCoef (140.f, sr);
        computeLfo (n);
        computeWobble (n);
        std::fill (sL.begin(), sL.begin() + n, 0.f); std::fill (sR.begin(), sR.begin() + n, 0.f);
        std::fill (qL.begin(), qL.begin() + n, 0.f); std::fill (qR.begin(), qR.begin() + n, 0.f);
        for (auto& v : voices) v.render (sL.data(), sR.data(), n, pm, lfoBuf.data(), sr, bend);
        for (auto& q : quakes) q.render (qL.data(), qR.data(), n, pm, wobBuf.data(), sr, bend);
        for (int i = 0; i < n; ++i)
        {
            float a = qL[(size_t) i], b = qR[(size_t) i];
            if (subMono)
            {
                const float m = (a + b) * 0.5f, s = (a - b) * 0.5f;
                sideLp1 += (s - sideLp1) * sideC; sideLp2 += (sideLp1 - sideLp2) * sideC;
                const float sh = s - sideLp2;
                a = m + sh; b = m - sh;
            }
            L[i] += sL[(size_t) i] * synthLvl + a * q8Lvl;
            R[i] += sR[(size_t) i] * synthLvl + b * q8Lvl;
        }
    }

    void master (float* L, float* R, int n)
    {
        const float wdt = pm.f (pid::width), out = dbToGain (pm.f (pid::outGain));
        const bool lim = pm.b (pid::limiter);
        for (int i = 0; i < n; ++i)
        {
            const float m = (L[i] + R[i]) * 0.5f, s = (L[i] - R[i]) * 0.5f * wdt;
            float l = (m + s) * out, r = (m - s) * out;
            if (! std::isfinite (l)) l = 0.f;
            if (! std::isfinite (r)) r = 0.f;
            if (lim) { l = ceilingClip (l); r = ceilingClip (r); }
            else { l = clampf (l, -4.f, 4.f); r = clampf (r, -4.f, 4.f); }
            L[i] = l; R[i] = r;
        }
    }

    void newRandomTarget() { shPrev = shTarget; shTarget = lfoRng.bi(); shVal = shTarget; }

    void computeLfo (int n)
    {
        const int shape = pm.n (pid::lfoShape);
        if (playing && pm.b (pid::lfoSync))
        {
            const double t = ppq / lfoDivBeats (pm.n (pid::lfoDiv)); // block-start lock keeps it on the grid
            const float np = (float) (t - std::floor (t));
            if (std::fabs (np - lfoPh) > 0.25f && std::fabs (np - lfoPh) < 0.75f) lfoPh = np;
        }
        const float rate = pm.b (pid::lfoSync) ? (float) (bpm / 60.0 / lfoDivBeats (pm.n (pid::lfoDiv))) : pm.f (pid::lfoRate);
        const float inc = rate / sr;
        for (int i = 0; i < n; ++i)
        {
            lfoPh += inc;
            if (lfoPh >= 1.f) { lfoPh -= 1.f; newRandomTarget(); }
            float y;
            switch (shape)
            {
                case 0:  y = std::sin (kTwoPi * lfoPh); break;
                case 1:  y = 1.f - 4.f * std::fabs (lfoPh - 0.5f); break;
                case 2:  y = 1.f - 2.f * lfoPh; break;
                case 3:  y = lfoPh < 0.5f ? 1.f : -1.f; break;
                case 4:  y = shVal; break;
                default: { const float t = 0.5f - 0.5f * std::cos (kPi * lfoPh); y = shPrev + (shTarget - shPrev) * t; break; }
            }
            lfoBuf[(size_t) i] = y;
        }
    }

    void computeWobble (int n)
    {
        const int shape = pm.n (pid::q8WobShape);
        const double beats = wobDivBeats (pm.n (pid::q8WobDiv));
        if (playing && ! pm.b (pid::q8WobRetrig))
        {
            const double t = ppq / beats;
            const float np = (float) (t - std::floor (t));
            if (std::fabs (np - wobPh) > 0.25f && std::fabs (np - wobPh) < 0.75f) wobPh = np;
        }
        const float inc = (float) (bpm / 60.0 / beats) / sr;
        const float slew = onePoleCoef (300.f, sr);
        for (int i = 0; i < n; ++i)
        {
            wobPh += inc;
            if (wobPh >= 1.f) { wobPh -= 1.f; wobRand = wobRng.next(); }
            float y;
            switch (shape)
            {
                case 0:  y = 0.5f - 0.5f * std::cos (kTwoPi * wobPh); break;
                case 1:  y = 1.f - 2.f * std::fabs (wobPh - 0.5f); break;
                case 2:  y = 1.f - wobPh; break;
                case 3:  y = wobPh; break;
                case 4:  y = wobPh < 0.5f ? 1.f : 0.f; break;
                case 5:  { const float s = std::sin (kPi * wobPh); y = s * s * s * s; break; }
                default: y = wobRand; break;
            }
            wobVal += (y - wobVal) * slew;
            wobBuf[(size_t) i] = wobVal;
        }
    }
};
} // namespace wc
