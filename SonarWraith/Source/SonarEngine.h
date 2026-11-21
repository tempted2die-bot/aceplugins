// SonarWraith - DSP engine (header-only, no JUCE dependency)
//
// Signal flow per voice:
//   [Morph Osc x unison] --PM--< [FM operator]
//        + Sub + Noise
//   pitch <- note + glide + PING envelope + BUBBLER scatter + LFO
//   -> TPT state-variable filter (LP/BP/HP/NOTCH, env + LFO)
//   -> Amp ADSR x Bubble gate
// Master FX chain:
//   DRIVE -> CRUSH -> GLITCH -> SONAR DELAY -> ATMOS (shimmer FDN reverb) -> width / output
#pragma once
#include "ParamDefs.h"
#include <cmath>
#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>

namespace sw
{
constexpr float kPi    = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

inline float clampf (float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
inline float semis  (float s) { return std::exp2 (s * (1.f / 12.f)); }
inline float dbToGain (float db) { return std::pow (10.f, db * 0.05f); }
inline float mtof   (float n) { return 440.f * std::exp2 ((n - 69.f) * (1.f / 12.f)); }
inline float ftanh  (float x) { x = clampf (x, -3.f, 3.f); const float x2 = x * x; return x * (27.f + x2) / (27.f + 9.f * x2); }
inline float softLimit (float x) // linear below 1, smooth above
{
    const float a = std::fabs (x);
    if (a <= 1.f) return x;
    const float y = 1.f + ftanh (a - 1.f) * 0.5f;
    return x > 0 ? y : -y;
}
// coefficient for an exponential segment that falls 60 dB in `ms`
inline float decayCoef (float ms, float sr) { return std::exp (-6.9078f / std::max (1.f, ms * 0.001f * sr)); }
inline float onePoleCoef (float hz, float sr) { return 1.f - std::exp (-kTwoPi * hz / sr); }

struct Rng
{
    uint32_t s = 0x9E3779B9u;
    explicit Rng (uint32_t seed = 0x9E3779B9u) : s (seed ? seed : 1u) {}
    float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xFFFFFF) / 16777216.f; } // [0,1)
    float bi() { return next() * 2.f - 1.f; }
};

// tempo divisions (in quarter-note beats)
inline double lfoDivBeats (int i) { static const double v[] = { 4, 2, 1, 0.5, 0.25, 0.125, 2.0 / 3, 1.0 / 3, 1.0 / 6, 0.75 }; return v[std::clamp (i, 0, 9)]; }
inline double glDivBeats  (int i) { static const double v[] = { 1, 0.5, 0.25, 0.125, 0.0625 }; return v[std::clamp (i, 0, 4)]; }
inline double dlyDivBeats (int i) { static const double v[] = { 0.125, 0.25, 1.0 / 6, 0.5, 1.0 / 3, 0.75, 1.0, 1.5, 2.0 }; return v[std::clamp (i, 0, 8)]; }

// quantise a random semitone offset to minor pentatonic (dark + always musical)
inline float quantPenta (float r)
{
    static const float deg[] = { 0.f, 3.f, 5.f, 7.f, 10.f, 12.f };
    const float oct = std::floor (r / 12.f);
    const float rem = r - oct * 12.f;
    float best = 0.f, bestD = 100.f;
    for (float d : deg) { const float dd = std::fabs (rem - d); if (dd < bestD) { bestD = dd; best = d; } }
    return oct * 12.f + best;
}

inline float polyBlep (float t, float dt)
{
    if (t < dt)         { t /= dt;               return t + t - t * t - 1.f; }
    if (t > 1.f - dt)   { t = (t - 1.f) / dt;    return t * t + t + t + 1.f; }
    return 0.f;
}

// morphing waveform: 0 = sine, 1/3 = triangle, 2/3 = saw, 1 = square
inline float morphShape (float t, float dt, float morph)
{
    auto shape = [t, dt] (int s) -> float
    {
        switch (s)
        {
            case 0:  return std::sin (kTwoPi * t);
            case 1:  { float u = t + 0.75f; u -= std::floor (u); return 4.f * std::fabs (u - 0.5f) - 1.f; }
            case 2:  return 2.f * t - 1.f - polyBlep (t, dt);
            default: { float u = t + 0.5f; u -= std::floor (u);
                       return (t < 0.5f ? 1.f : -1.f) + polyBlep (t, dt) - polyBlep (u, dt); }
        }
    };
    const float m = clampf (morph, 0.f, 1.f) * 3.f;
    const int seg = std::min (2, (int) m);
    const float fr = m - (float) seg;
    if (fr < 1e-4f) return shape (seg);
    if (fr > 0.9999f) return shape (seg + 1);
    return shape (seg) * (1.f - fr) + shape (seg + 1) * fr;
}

//==============================================================================
struct ADSR
{
    enum Stage { Idle, Attack, Decay, Sustain, Release } st = Idle;
    float v = 0.f, aInc = 1.f, dCoef = 0.f, rCoef = 0.f, sus = 1.f;

    void set (float aMs, float dMs, float s, float rMs, float sr)
    {
        aInc  = 1.f / std::max (0.0004f * sr, aMs * 0.001f * sr); // >=0.4 ms: punchy but click-free
        dCoef = decayCoef (dMs, sr);
        rCoef = decayCoef (rMs, sr);
        sus   = s;
    }
    void noteOn()  { st = Attack; }
    void noteOff() { if (st != Idle) st = Release; }
    void kill()    { st = Idle; v = 0.f; }
    bool active() const { return st != Idle; }
    float next()
    {
        switch (st)
        {
            case Attack:  v += aInc; if (v >= 1.f) { v = 1.f; st = Decay; } break;
            case Decay:   v = sus + (v - sus) * dCoef; if (v - sus < 1e-4f) { v = sus; st = Sustain; } break;
            case Sustain: v = sus; if (sus < 1e-5f) { v = 0.f; st = Idle; } break;
            case Release: v *= rCoef; if (v < 1e-5f) { v = 0.f; st = Idle; } break;
            case Idle:    break;
        }
        return v;
    }
};

struct ADEnv // fast attack, exponential decay (filter env)
{
    float v = 0.f, aInc = 0.02f, dCoef = 0.999f; bool attacking = false;
    void trigger() { attacking = true; }
    float next()
    {
        if (attacking) { v += aInc; if (v >= 1.f) { v = 1.f; attacking = false; } }
        else v *= dCoef;
        return v;
    }
};

// Zavalishin TPT state variable filter
struct SVF
{
    float ic1 = 0.f, ic2 = 0.f;
    void reset() { ic1 = ic2 = 0.f; }
    float process (float x, float g, float k, int type)
    {
        const float a1 = 1.f / (1.f + g * (g + k));
        const float a2 = g * a1, a3 = g * a2;
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.f * v1 - ic1;
        ic2 = 2.f * v2 - ic2;
        switch (type)
        {
            case 0:  return v2;                       // LP
            case 1:  return v1;                       // BP (normalised by caller)
            case 2:  return x - k * v1 - v2;          // HP
            default: return x - k * v1;               // NOTCH
        }
    }
};

//==============================================================================
class Voice
{
public:
    int note = -1;
    float vel = 1.f;
    uint64_t age = 0;
    bool gateHeld = false;      // key physically down (or held by sustain)
    bool pendingRelease = false; // released while sustain pedal down

    explicit Voice (uint32_t seed = 1) : rng (seed) {}

    bool active() const { return amp.active(); }

    void start (int n, float velocity, float glideFrom, bool retriggerAmp, bool glideOn)
    {
        const bool wasActive = active();
        note = n; vel = velocity; gateHeld = true; pendingRelease = false;
        target = (float) n;
        if (! wasActive || ! glideOn) cur = glideOn && glideFrom >= 0.f ? glideFrom : (float) n;

        ping = 1.f;
        bubEnv = 1.f;
        bubOffset = 0.f;
        bubCount = 0.f;
        fenv.trigger();

        if (! wasActive)
        {
            svfL.reset(); svfR.reset();
            cutSm = -1.f;
            for (size_t u = 0; u < units.size(); ++u) { units[u].ph = (u == 0) ? 0.0f : rng.next(); units[u].mph = 0.f; }
            subPh = 0.f;
        }
        if (retriggerAmp || ! wasActive) amp.noteOn();
    }

    void glideTo (int n) { note = n; target = (float) n; ping = 1.f; fenv.trigger(); bubEnv = 1.f; }
    void release() { gateHeld = false; amp.noteOff(); }
    void kill() { amp.kill(); note = -1; }

    void render (float* L, float* R, int n, const Params& p, const float* lfo, float sr, float bend)
    {
        if (! active()) return;

        amp.set (p.f (pid::atk), p.f (pid::dec), p.f (pid::sus), p.f (pid::rel), sr);
        fenv.aInc  = 1.f / std::max (1.f, 0.001f * sr);
        fenv.dCoef = decayCoef (p.f (pid::fltDecay), sr);

        const int nu = std::clamp (p.n (pid::uniVoices), 1, 4);
        float det[4], gl[4], gr[4];
        const float norm = std::sqrt (2.f) / std::sqrt ((float) nu);
        for (int u = 0; u < nu; ++u)
        {
            const float pos = nu == 1 ? 0.f : ((float) u / (float) (nu - 1)) * 2.f - 1.f;
            det[u] = semis (pos * p.f (pid::uniDetune) * 0.5f);
            const float pan = pos * 0.85f;
            gl[u] = std::cos ((pan + 1.f) * kPi * 0.25f) * norm;
            gr[u] = std::sin ((pan + 1.f) * kPi * 0.25f) * norm;
        }

        const float pingCoef  = decayCoef (p.f (pid::pingDecay), sr);
        const float bubRate   = p.f (pid::bubRate);
        const float interval  = bubRate > 0.05f ? sr / bubRate : 0.f;
        const float bubCoef   = decayCoef (interval > 0.f ? std::max (15.f, 900.f / bubRate) : std::max (30.f, p.f (pid::pingDecay) * 3.f), sr);
        const float glideMs   = p.f (pid::glide);
        const float glideCoef = glideMs > 0.5f ? std::exp (-1.f / (glideMs * 0.001f * sr * 0.25f)) : 0.f;
        const float fmIndex   = p.f (pid::fmAmt) * p.f (pid::fmAmt) * 10.f / kTwoPi;
        const float fmRatio   = p.f (pid::fmRatio);
        const float fold      = p.f (pid::oscFold);
        const float foldMix   = std::min (1.f, fold * 4.f);
        const float foldGain  = (1.f + fold * 7.f) * kPi * 0.5f;
        const float wave      = p.f (pid::oscWave);
        const float k         = 2.f - 1.96f * p.f (pid::fltRes);
        const int   ftype     = p.n (pid::fltType);
        const float bpNorm    = ftype == 1 ? std::pow (k, 0.75f) : 1.f;
        const float pitchOfs  = (float) p.sn (pid::oscOct) * 12.f + p.f (pid::oscFine) * 0.01f + bend;
        const float pingAmt   = p.f (pid::pingAmt);
        const float lfoPitch  = p.f (pid::lfoPitch);
        const float lfoCut    = p.f (pid::lfoCut);
        const float fltEnv    = p.f (pid::fltEnv);
        const float baseCut   = p.f (pid::fltCut);
        const float subLvl    = p.f (pid::subLvl);
        const float noiseLvl  = p.f (pid::noiseLvl) * 0.5f;
        const float gateAmt   = p.f (pid::bubGate);
        const float bubChance = p.f (pid::bubChance);
        const float bubRand   = p.f (pid::bubRand);
        const float level     = p.f (pid::voiceVol) * (0.35f + 0.65f * vel);
        const float nyq       = sr * 0.45f;
        const float gateC     = onePoleCoef (350.f, sr);
        if (cutSm < 0.f) cutSm = baseCut;

        for (int i = 0; i < n; ++i)
        {
            // ---- BUBBLER: random re-triggers of the ping while the note is held
            if (interval > 0.f)
            {
                bubCount -= 1.f;
                if (bubCount <= 0.f)
                {
                    bubCount += interval * (0.7f + 0.6f * rng.next());
                    if (rng.next() < bubChance)
                    {
                        ping = 1.f; bubEnv = 1.f; fenv.trigger();
                        bubOffset = bubRand > 0.01f ? quantPenta (rng.bi() * bubRand) : 0.f;
                    }
                }
            }

            cur = target + (cur - target) * glideCoef;
            const float pitch = cur + pitchOfs + pingAmt * ping + bubOffset + lfo[i] * lfoPitch;
            ping   *= pingCoef;
            bubEnv *= bubCoef;

            const float f0 = std::min (mtof (pitch), nyq);
            float sl = 0.f, sr_ = 0.f;
            for (int u = 0; u < nu; ++u)
            {
                auto& U = units[(size_t) u];
                const float dt = f0 * det[u] / sr;
                U.mph += dt * fmRatio; U.mph -= std::floor (U.mph);
                float t = U.ph;
                if (fmIndex > 0.f) { t += std::sin (kTwoPi * U.mph) * fmIndex; t -= std::floor (t); }
                float s = morphShape (t, dt, wave);
                if (foldMix > 0.f) s += foldMix * (std::sin (s * foldGain) - s);
                U.ph += dt; if (U.ph >= 1.f) U.ph -= 1.f;
                sl += s * gl[u]; sr_ += s * gr[u];
            }
            if (subLvl > 0.f)
            {
                subPh += f0 * 0.5f / sr; if (subPh >= 1.f) subPh -= 1.f;
                const float sub = std::sin (kTwoPi * subPh) * subLvl;
                sl += sub; sr_ += sub;
            }
            if (noiseLvl > 0.f) { const float nz = rng.bi() * noiseLvl; sl += nz; sr_ += nz; }

            // ---- filter
            const float fe = fenv.next();
            cutSm += (baseCut - cutSm) * 0.002f;
            float cut = cutSm * std::exp2 (fltEnv * fe * 7.f + lfo[i] * lfoCut * 4.f);
            cut = clampf (cut, 16.f, nyq);
            const float g = std::tan (kPi * cut / sr);
            const float yl = svfL.process (sl, g, k, ftype);
            const float yr = svfR.process (sr_, g, k, ftype);

            gateSm += (bubEnv - gateSm) * gateC; // de-click the bubble gate
            const float a = amp.next() * level * bpNorm * (1.f - gateAmt + gateAmt * gateSm);
            L[i] += softLimit (yl * a);
            R[i] += softLimit (yr * a);
            if (! amp.active()) { note = -1; break; }
        }
    }

private:
    struct Unit { float ph = 0.f, mph = 0.f; };
    std::array<Unit, 4> units {};
    Rng rng;
    ADSR amp;
    ADEnv fenv;
    SVF svfL, svfR;
    float cur = 60.f, target = 60.f;
    float ping = 0.f, bubEnv = 0.f, bubOffset = 0.f, bubCount = 0.f;
    float subPh = 0.f, cutSm = -1.f, gateSm = 0.f;
};

//==============================================================================
// FX
//==============================================================================
struct DriveFx
{
    float lpL = 0, lpR = 0, dxL = 0, dyL = 0, dxR = 0, dyR = 0;
    static float shape (float x, int mode, float g)
    {
        switch (mode)
        {
            case 0: return ftanh (x * g);
            case 1: return clampf (x * g, -1.f, 1.f);
            case 2: return std::sin (clampf (x * g, -40.f, 40.f) * kPi * 0.5f);           // wavefold
            case 3: { const float y = ftanh (x * g); return 0.65f * std::fabs (y) + 0.35f * y; } // octave-up rectifier
            default:                                                                       // BLOWN speaker: biased + asymmetric + gated grit
            {
                float y = x * g + 0.25f;
                y = y > 0.f ? ftanh (y) : ftanh (y * 3.f) * 0.6f;
                return clampf (y * 1.4f, -1.f, 1.f);
            }
        }
    }
    void process (float* L, float* R, int n, const Params& p, float sr)
    {
        const int mode = p.n (pid::drvMode);
        const float amt = p.f (pid::drvAmt);
        const float g = dbToGain (amt * 42.f);
        const float makeup = dbToGain (-amt * 4.f);
        const float c = onePoleCoef (600.f * std::exp2 (p.f (pid::drvTone) * 5.06f), sr);
        const float mix = p.f (pid::drvMix);
        for (int i = 0; i < n; ++i)
        {
            float yl = shape (L[i], mode, g), yr = shape (R[i], mode, g);
            lpL += (yl - lpL) * c; lpR += (yr - lpR) * c;
            // DC blocker (needed for rectify / blown)
            const float ol = lpL - dxL + 0.995f * dyL; dxL = lpL; dyL = ol;
            const float orr = lpR - dxR + 0.995f * dyR; dxR = lpR; dyR = orr;
            L[i] += (ol * makeup - L[i]) * mix;
            R[i] += (orr * makeup - R[i]) * mix;
        }
    }
};

struct CrushFx
{
    float ph = 0.f, hL = 0.f, hR = 0.f;
    void process (float* L, float* R, int n, const Params& p, float sr)
    {
        const float step = 2.f / std::exp2 (p.f (pid::crsBits));
        const float inc = p.f (pid::crsRate) / sr;
        const float mix = p.f (pid::crsMix);
        for (int i = 0; i < n; ++i)
        {
            ph += inc;
            if (ph >= 1.f || inc >= 1.f) { ph -= std::floor (ph); hL = L[i]; hR = R[i]; }
            const float ql = std::floor (hL / step + 0.5f) * step;
            const float qr = std::floor (hR / step + 0.5f) * step;
            L[i] += (ql - L[i]) * mix;
            R[i] += (qr - R[i]) * mix;
        }
    }
};

// Tempo-synced buffer-repeat stutter with reverse + tape-speed warps
struct GlitchFx
{
    std::vector<float> bL, bR;
    int size = 1, w = 0;
    bool active = false;
    int segStart = 0, segLen = 1;
    float pos = 0.f, speed = 1.f, gain = 0.f;
    bool reverse = false;
    double freeCount = 0.0;
    long long lastSlice = -1;
    Rng rng { 0xBADC0DEu };

    void prepare (float sr) { size = (int) (sr * 4.f) + 8; bL.assign ((size_t) size, 0.f); bR.assign ((size_t) size, 0.f); w = 0; active = false; gain = 0.f; lastSlice = -1; freeCount = 0; }

    float read (const std::vector<float>& b, float idx) const
    {
        int i0 = (int) std::floor (idx); const float fr = idx - (float) i0;
        i0 %= size; if (i0 < 0) i0 += size;
        const int i1 = (i0 + 1) % size;
        return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * fr;
    }

    void process (float* L, float* R, int n, const Params& p, float sr, double bpm, bool playing, double ppq)
    {
        const double divB = glDivBeats (p.n (pid::glDiv));
        const double spb = 60.0 / bpm * sr; // samples per beat
        const double sliceLen = std::max (64.0, divB * spb);
        const float chance = p.f (pid::glChance), rev = p.f (pid::glRev), warp = p.f (pid::glPitch), mix = p.f (pid::glMix);

        for (int i = 0; i < n; ++i)
        {
            bL[(size_t) w] = L[i]; bR[(size_t) w] = R[i];

            bool boundary = false;
            if (playing)
            {
                const long long s = (long long) std::floor ((ppq + (double) i / spb) / divB);
                if (s != lastSlice) { boundary = true; lastSlice = s; }
            }
            else
            {
                freeCount += 1.0;
                if (freeCount >= sliceLen) { freeCount -= sliceLen; boundary = true; }
            }

            if (boundary)
            {
                if (rng.next() < chance)
                {
                    static const int divs[] = { 1, 2, 2, 4, 4, 8 };
                    segLen = std::max (64, (int) (sliceLen / divs[(int) (rng.next() * 5.999f)]));
                    segStart = w - segLen; // most recent audio
                    reverse = rng.next() < rev;
                    speed = rng.next() < warp ? (rng.next() < 0.5f ? 2.f : 0.5f) : 1.f;
                    pos = 0.f;
                    active = true;
                }
                else active = false;
            }

            gain += ((active ? 1.f : 0.f) - gain) * 0.02f;
            if (gain > 1e-4f)
            {
                const float fade = std::min (96.f, segLen * 0.25f);
                const float rp = reverse ? (float) segLen - 1.f - pos : pos;
                const float win = std::min (1.f, std::min (pos / fade, ((float) segLen - pos) / fade));
                const float wl = read (bL, (float) segStart + rp) * win;
                const float wr = read (bR, (float) segStart + rp) * win;
                const float m = gain * mix;
                L[i] += (wl - L[i]) * m;
                R[i] += (wr - R[i]) * m;
                pos += speed; if (pos >= (float) segLen) pos -= (float) segLen;
            }
            if (++w >= size) w = 0;
        }
    }
};

struct SonarDelayFx
{
    std::vector<float> bL, bR;
    int size = 1, w = 0;
    float lpL = 0, lpR = 0, time = -1.f, ph1 = 0.f, ph2 = 0.f;

    void prepare (float sr) { size = (int) (sr * 6.f) + 8; bL.assign ((size_t) size, 0.f); bR.assign ((size_t) size, 0.f); w = 0; time = -1.f; lpL = lpR = 0; }

    float read (const std::vector<float>& b, float d) const
    {
        float idx = (float) w - std::max (1.f, d); while (idx < 0.f) idx += (float) size;
        int i0 = (int) idx; const float fr = idx - (float) i0;
        if (i0 >= size) i0 -= size;
        const int i1 = (i0 + 1) % size;
        return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * fr;
    }

    void process (float* L, float* R, int n, const Params& p, float sr, double bpm)
    {
        const float target = clampf ((float) (dlyDivBeats (p.n (pid::dlyDiv)) * 60.0 / bpm * sr), 16.f, (float) size - 4096.f);
        if (time < 0.f) time = target;
        const float fb = p.f (pid::dlyFb) * 1.02f;
        const float c = onePoleCoef (400.f * std::exp2 (p.f (pid::dlyTone) * 5.6f), sr);
        const float depth = p.f (pid::dlyWarble) * 0.006f * sr;
        const float mix = p.f (pid::dlyMix);
        const bool pp = p.b (pid::dlyPP);
        const float i1 = kTwoPi * 0.63f / sr, i2 = kTwoPi * 2.9f / sr;

        for (int i = 0; i < n; ++i)
        {
            time += (target - time) * 0.0004f;
            ph1 += i1; if (ph1 > kTwoPi) ph1 -= kTwoPi;
            ph2 += i2; if (ph2 > kTwoPi) ph2 -= kTwoPi;
            const float mod = (std::sin (ph1) + 0.25f * std::sin (ph2)) * depth;
            const float yl = read (bL, time + mod + 2.f);
            const float yr = read (bR, time - mod + 2.f);
            lpL += (yl - lpL) * c; lpR += (yr - lpR) * c;
            if (pp)
            {
                bL[(size_t) w] = ftanh ((L[i] + R[i]) * 0.5f + lpR * fb);
                bR[(size_t) w] = ftanh (lpL * fb);
            }
            else
            {
                bL[(size_t) w] = ftanh (L[i] + lpL * fb);
                bR[(size_t) w] = ftanh (R[i] + lpR * fb);
            }
            L[i] += yl * mix;
            R[i] += yr * mix;
            if (++w >= size) w = 0;
        }
    }
};

// Granular octave-up shifter used inside the reverb loop (shimmer)
struct PitchShifter
{
    std::vector<float> b; int size = 1, w = 0; float ph = 0.f, win = 2048.f;
    void prepare (float sr) { win = std::floor (sr * 0.06f); size = (int) win + 16; b.assign ((size_t) size, 0.f); w = 0; ph = 0.f; }
    float tap (float d) const
    {
        float idx = (float) w - d - 2.f; while (idx < 0.f) idx += (float) size;
        int i0 = (int) idx; const float fr = idx - (float) i0; if (i0 >= size) i0 -= size;
        const int i1 = (i0 + 1) % size;
        return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * fr;
    }
    float process (float x, float ratio)
    {
        b[(size_t) w] = x;
        ph -= (ratio - 1.f) / win; ph -= std::floor (ph);
        float ph2 = ph + 0.5f; ph2 -= std::floor (ph2);
        const float y = tap (ph * win) * std::sin (kPi * ph) + tap (ph2 * win) * std::sin (kPi * ph2);
        if (++w >= size) w = 0;
        return y;
    }
};

// 8-line feedback delay network reverb with input diffusion, modulation and shimmer
struct AtmosFx
{
    static constexpr int N = 8;
    std::vector<float> line[N]; int lsize = 1, w = 0;
    float lp[N] {};
    std::vector<float> ap[4]; int apLen[4] {}, apW[4] {};
    PitchShifter shifter;
    float shimmerFb = 0.f, sizeSm = -1.f, modPh = 0.f, sr = 48000.f;

    void prepare (float sampleRate)
    {
        sr = sampleRate;
        lsize = (int) (sr * 0.25f) + 64;
        for (auto& l : line) l.assign ((size_t) lsize, 0.f);
        static const float apMs[4] = { 4.77f, 3.59f, 12.73f, 9.31f };
        for (int i = 0; i < 4; ++i) { apLen[i] = (int) (apMs[i] * 0.001f * sr); ap[i].assign ((size_t) apLen[i] + 1, 0.f); apW[i] = 0; }
        for (auto& v : lp) v = 0.f;
        shifter.prepare (sr);
        w = 0; shimmerFb = 0.f; sizeSm = -1.f;
    }

    void process (float* L, float* R, int n, const Params& p)
    {
        static const float baseMs[N] = { 31.7f, 37.3f, 41.9f, 47.1f, 53.6f, 59.3f, 67.1f, 73.9f };
        const float size = p.f (pid::revSize);
        if (sizeSm < 0.f) sizeSm = size;
        sizeSm += (size - sizeSm) * 0.05f; // per-block smoothing

        const float scale = 0.35f + sizeSm * 1.6f;
        const float t60 = 0.4f + sizeSm * sizeSm * 14.f;
        float len[N], g[N];
        for (int i = 0; i < N; ++i)
        {
            len[i] = std::min ((float) lsize - 40.f, baseMs[i] * scale * 0.001f * sr);
            g[i] = std::pow (10.f, -3.f * len[i] / (t60 * sr));
        }
        const float c = onePoleCoef (18000.f * std::exp2 (-p.f (pid::revDamp) * 5.f), sr);
        const float shimmer = p.f (pid::revShimmer);
        const float mix = p.f (pid::revMix);
        const float modInc = kTwoPi * 0.31f / sr;
        const float modDepth = 0.00025f * sr;

        for (int i = 0; i < n; ++i)
        {
            float x = (L[i] + R[i]) * 0.5f + shimmerFb;
            for (int a = 0; a < 4; ++a) // Schroeder allpass diffusion
            {
                auto& b = ap[a];
                const float d = b[(size_t) apW[a]];
                const float y = -0.65f * x + d;
                b[(size_t) apW[a]] = x + 0.65f * y;
                if (++apW[a] >= apLen[a]) apW[a] = 0;
                x = y;
            }

            modPh += modInc; if (modPh > kTwoPi) modPh -= kTwoPi;
            float o[N], sum = 0.f;
            for (int k = 0; k < N; ++k)
            {
                float d = len[k];
                if (k == 1) d += std::sin (modPh) * modDepth;
                if (k == 5) d += std::sin (modPh + 2.1f) * modDepth;
                float idx = (float) w - d; while (idx < 0.f) idx += (float) lsize;
                int i0 = (int) idx; const float fr = idx - (float) i0; if (i0 >= lsize) i0 -= lsize;
                const int i1 = (i0 + 1) % lsize;
                o[k] = line[k][(size_t) i0] + (line[k][(size_t) i1] - line[k][(size_t) i0]) * fr;
                lp[k] += (o[k] - lp[k]) * c;
                sum += lp[k] * g[k];
            }
            const float h = sum * (2.f / N);
            for (int k = 0; k < N; ++k)
                line[k][(size_t) w] = lp[k] * g[k] - h + ((k & 1) ? -x : x) * 0.5f;

            const float outL = (o[0] + o[2] + o[4] + o[6]) * 0.4f;
            const float outR = (o[1] + o[3] + o[5] + o[7]) * 0.4f;
            shimmerFb = shimmer > 0.001f ? ftanh (shifter.process ((outL + outR) * 0.5f, 2.f) * shimmer * 0.55f) : 0.f;

            L[i] += (outL - L[i]) * mix * 0.85f + outL * mix * 0.15f;
            R[i] += (outR - R[i]) * mix * 0.85f + outR * mix * 0.15f;
            if (++w >= lsize) w = 0;
        }
    }
};

//==============================================================================
class Engine
{
public:
    Params p;
    static constexpr int kVoices = 12;

    Engine()
    {
        for (int i = 0; i < kVoices; ++i) voices.emplace_back (0x1234567u + 7919u * (uint32_t) i);
        held.reserve (128);
    }

    void prepare (double sampleRate, int /*maxBlock*/)
    {
        sr = (float) sampleRate;
        glitch.prepare (sr); delay.prepare (sr); atmos.prepare (sr);
        drive = {}; crush = {};
        allOff();
    }

    void setPitchBend (float semitones) { bend = semitones; }

    void beginBlock (double bpmIn, bool playingIn, double ppqIn)
    {
        bpm = (bpmIn > 20.0 && bpmIn < 999.0) ? bpmIn : 140.0;
        playing = playingIn; ppq = ppqIn;
        if (playing && p.b (pid::lfoSync))
        {
            double ph = ppq / lfoDivBeats (p.n (pid::lfoDiv));
            const float np = (float) (ph - std::floor (ph));
            if (np < lfoPh && p.n (pid::lfoShape) >= 4) newRandomTarget(); // keep S&H stepping on wrap
            lfoPh = np;
        }
    }

    void noteOn (int note, float velocity)
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

    void noteOff (int note)
    {
        const int mode = p.n (pid::polyMode);
        if (mode == 0)
        {
            for (auto& v : voices)
                if (v.active() && v.note == note && v.gateHeld)
                {
                    if (sustainDown) v.pendingRelease = true; else v.release();
                }
        }
        else
        {
            held.erase (std::remove (held.begin(), held.end(), note), held.end());
            auto& v = voices[0];
            if (v.note == note)
            {
                if (! held.empty()) v.glideTo (held.back());
                else if (sustainDown) v.pendingRelease = true;
                else v.release();
            }
        }
    }

    void sustain (bool down)
    {
        sustainDown = down;
        if (! down)
            for (auto& v : voices) if (v.pendingRelease) { v.pendingRelease = false; v.release(); }
    }

    void allOff() { for (auto& v : voices) v.kill(); held.clear(); sustainDown = false; }

    // ADDs voices into L/R
    void renderVoices (float* L, float* R, int n)
    {
        while (n > 0)
        {
            const int c = std::min (n, kChunk);
            computeLfo (c);
            for (auto& v : voices) v.render (L, R, c, p, lfoBuf.data(), sr, bend);
            L += c; R += c; n -= c;
        }
    }

    void processFx (float* L, float* R, int n)
    {
        if (p.b (pid::drvOn))  drive.process (L, R, n, p, sr);
        if (p.b (pid::crsOn))  crush.process (L, R, n, p, sr);
        if (p.b (pid::glOn))   glitch.process (L, R, n, p, sr, bpm, playing, ppq);
        if (p.b (pid::dlyOn))  delay.process (L, R, n, p, sr, bpm);
        if (p.b (pid::revOn))  atmos.process (L, R, n, p);

        const float wdt = p.f (pid::width);
        const float out = dbToGain (p.f (pid::outGain));
        for (int i = 0; i < n; ++i)
        {
            const float m = (L[i] + R[i]) * 0.5f, s = (L[i] - R[i]) * 0.5f * wdt;
            float l = (m + s) * out, r = (m - s) * out;
            if (! std::isfinite (l)) l = 0.f;
            if (! std::isfinite (r)) r = 0.f;
            L[i] = softLimit (l);
            R[i] = softLimit (r);
        }
    }

private:
    static constexpr int kChunk = 256;
    std::vector<Voice> voices;
    std::vector<int> held;
    DriveFx drive; CrushFx crush; GlitchFx glitch; SonarDelayFx delay; AtmosFx atmos;
    std::array<float, kChunk> lfoBuf {};
    float sr = 48000.f, bend = 0.f, lastNote = -1.f;
    double bpm = 140.0, ppq = 0.0; bool playing = false;
    bool sustainDown = false;
    uint64_t counter = 0;
    float lfoPh = 0.f, shVal = 0.f, shPrev = 0.f, shTarget = 0.f;
    Rng lfoRng { 0xC0FFEEu };

    void newRandomTarget() { shPrev = shTarget; shTarget = lfoRng.bi(); shVal = shTarget; }

    void computeLfo (int n)
    {
        const int shape = p.n (pid::lfoShape);
        const float rate = p.b (pid::lfoSync) ? (float) (bpm / 60.0 / lfoDivBeats (p.n (pid::lfoDiv))) : p.f (pid::lfoRate);
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
};
} // namespace sw
