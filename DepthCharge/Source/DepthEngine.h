// DepthCharge - DSP engine (header-only, no JUCE dependency)
//
// Signal flow per voice:
//   [Morph Osc x unison] --PM--< [FM operator]
//        + Sub + Noise
//   pitch <- note + glide + PING envelope + BUBBLER scatter + LFO
//   -> TPT state-variable filter (LP/BP/HP/NOTCH, env + LFO)
//   -> Amp ADSR x Bubble gate
// 808 QUAKE layer per voice:
//   sine/tri/square core (+ L/R phase spread) --PM--< FM growl
//   pitch <- note + slide + PUNCH drop + wobble
//   -> fold -> DIRT (clip/tube/fold/fuzz/shred) -> tone -> GROWL formants -> wobble LP -> + clean sub
// Layers are key-split (808 below the split, synth above) then summed into the master FX chain:
//   DRIVE -> CRUSH -> GLITCH (stutter/gap/tape stop) -> SONAR DELAY -> ATMOS -> width / output
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

// Tempo-synced glitch: buffer-repeat stutters (fwd / reverse / warped), hard GAPS (dropouts) and TAPE STOPS
struct GlitchFx
{
    enum Mode { None, Stutter, Gap, Stop };
    std::vector<float> bL, bR;
    int size = 1, w = 0;
    Mode mode = None;
    int segStart = 0, segLen = 1;
    float pos = 0.f, speed = 1.f, gain = 0.f, stopPos = 0.f, stopSpeed = 1.f, stopDec = 0.f;
    bool reverse = false;
    double freeCount = 0.0;
    long long lastSlice = -1;
    Rng rng { 0xBADC0DEu };

    void prepare (float sr) { size = (int) (sr * 6.f) + 8; bL.assign ((size_t) size, 0.f); bR.assign ((size_t) size, 0.f); w = 0; mode = None; gain = 0.f; lastSlice = -1; freeCount = 0; }

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
        const double spb = 60.0 / bpm * sr;
        const double sliceLen = std::max (64.0, divB * spb);
        const float chance = p.f (pid::glChance), rev = p.f (pid::glRev), warp = p.f (pid::glPitch), mix = p.f (pid::glMix);
        const float gapP = p.f (pid::glGap), stopP = p.f (pid::glStop);

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
                const float r = rng.next();
                if (r < stopP * 0.5f)                     // tape stop (rarer by design)
                {
                    mode = Stop; stopPos = (float) w; stopSpeed = 1.f;
                    stopDec = 1.f / (float) sliceLen;
                }
                else if (rng.next() < gapP * 0.6f) mode = Gap;
                else if (rng.next() < chance)
                {
                    static const int divs[] = { 1, 2, 2, 4, 4, 8 };
                    segLen = std::max (64, (int) (sliceLen / divs[(int) (rng.next() * 5.999f)]));
                    segStart = w - segLen;
                    reverse = rng.next() < rev;
                    speed = rng.next() < warp ? (rng.next() < 0.5f ? 2.f : 0.5f) : 1.f;
                    pos = 0.f;
                    mode = Stutter;
                }
                else mode = None;
            }

            gain += ((mode != None ? 1.f : 0.f) - gain) * 0.02f;
            if (gain > 1e-4f)
            {
                float wl = 0.f, wr = 0.f;
                if (mode == Stutter)
                {
                    const float fade = std::min (96.f, segLen * 0.25f);
                    const float rp = reverse ? (float) segLen - 1.f - pos : pos;
                    const float win = std::min (1.f, std::min (pos / fade, ((float) segLen - pos) / fade));
                    wl = read (bL, (float) segStart + rp) * win;
                    wr = read (bR, (float) segStart + rp) * win;
                    pos += speed; if (pos >= (float) segLen) pos -= (float) segLen;
                }
                else if (mode == Stop)
                {
                    const float g = std::min (1.f, stopSpeed * 4.f); // fade out as it grinds to a halt
                    wl = read (bL, stopPos) * g;
                    wr = read (bR, stopPos) * g;
                    stopPos += stopSpeed * stopSpeed;            // quadratic slow-down sounds like a real deck
                    stopSpeed = std::max (0.f, stopSpeed - stopDec);
                }
                // Gap: wl = wr = 0 (hard dropout, de-clicked by the gain ramp)
                const float m = gain * mix;
                L[i] += (wl - L[i]) * m;
                R[i] += (wr - R[i]) * m;
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
    float hA1 = 0, hA2 = 0, hB1 = 0, hB2 = 0; // FX low-cut states (keeps 808 lows out of the echoes)
    static float hp (float x, float& s1, float& s2, float c) { s1 += (x - s1) * c; s2 += (s1 - s2) * c; return x - s2; }

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
        const float hc = onePoleCoef (p.f (pid::fxLowCut), sr);

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
                bL[(size_t) w] = ftanh (hp ((L[i] + R[i]) * 0.5f, hA1, hA2, hc) + lpR * fb);
                bR[(size_t) w] = ftanh (lpL * fb);
            }
            else
            {
                bL[(size_t) w] = ftanh (hp (L[i], hA1, hA2, hc) + lpL * fb);
                bR[(size_t) w] = ftanh (hp (R[i], hB1, hB2, hc) + lpR * fb);
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
    float shimmerFb = 0.f, sizeSm = -1.f, modPh = 0.f, sr = 48000.f, h1 = 0.f, h2 = 0.f;

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
        const float hc = onePoleCoef (p.f (pid::fxLowCut), sr);

        for (int i = 0; i < n; ++i)
        {
            float x = (L[i] + R[i]) * 0.5f;
            h1 += (x - h1) * hc; h2 += (h1 - h2) * hc;
            x = x - h2 + shimmerFb;
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
// 808 QUAKE layer
//==============================================================================
inline double wobDivBeats (int i) { static const double v[] = { 4, 2, 1, 2.0 / 3, 0.5, 1.0 / 3, 0.25, 1.0 / 6, 0.125, 3, 1.5, 0.75 }; return v[std::clamp (i, 0, 11)]; }

// 3-band vowel formant filter (U-O-A-E-I sweep = "yoy / wow" growl)
struct FormantBank
{
    SVF f[3];
    float g[3] { 0.1f, 0.2f, 0.4f }, k = 0.2f;
    void reset() { for (auto& x : f) x.reset(); }
    void setVowel (float v, float q01, float sr)
    {
        static const float F[5][3] = { { 300, 870, 2240 }, { 570, 840, 2410 }, { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 } };
        v = clampf (v, 0.f, 1.f) * 4.f;
        const int i0 = std::min (3, (int) v); const float fr = v - (float) i0;
        for (int j = 0; j < 3; ++j)
        {
            const float hz = F[i0][j] + (F[i0 + 1][j] - F[i0][j]) * fr;
            g[j] = std::tan (kPi * std::min (hz, sr * 0.45f) / sr);
        }
        k = 1.f / (2.f + q01 * 14.f);
    }
    float process (float x)
    {
        static const float amp[3] = { 1.f, 0.7f, 0.35f };
        float y = 0.f;
        for (int j = 0; j < 3; ++j) y += f[j].process (x, g[j], k, 1) * k * amp[j];
        return y * (2.2f + 0.08f / k);
    }
};

inline float dirt808 (float y, int mode)
{
    switch (mode)
    {
        case 0:  { const float h = clampf (y, -1.f, 1.f); return h * 0.8f + ftanh (y) * 0.2f; }                         // CLIP
        case 1:  return y > 0.f ? ftanh (y) : clampf (ftanh (y * 0.5f) * 1.3f, -1.f, 1.f);                                // TUBE (even harmonics)
        case 2:  return std::sin (clampf (y, -6.f, 6.f) * kPi * 0.5f);                                                    // FOLD (notched peaks)
        case 3:  return clampf (0.75f * ftanh (std::fabs (y) * 1.5f) + 0.5f * ftanh (y) - 0.35f, -1.f, 1.f);             // FUZZ (octave-up)
        default: { float z = clampf (std::sin (clampf (y * 1.3f, -5.f, 5.f) * kPi * 0.5f) * 1.4f, -1.f, 1.f);              // SHRED
                   return std::floor (z * 12.f + 0.5f) / 12.f; }
    }
}

inline float shape808 (float t, float shape) // sine -> triangle -> rounded square
{
    const float s = std::sin (kTwoPi * t);
    if (shape <= 0.001f) return s;
    float u = t + 0.75f; u -= std::floor (u);
    const float tri = 4.f * std::fabs (u - 0.5f) - 1.f;
    if (shape <= 0.5f) return s + (tri - s) * (shape * 2.f);
    const float sq = ftanh (s * 4.f) * 1.004f;
    return tri + (sq - tri) * ((shape - 0.5f) * 2.f);
}

class Quake808
{
public:
    int note = -1;
    float vel = 1.f;
    uint64_t age = 0;
    bool gateHeld = false, pendingRelease = false;

    explicit Quake808 (uint32_t seed = 7) : rng (seed) {}
    bool active() const { return amp.active(); }

    void start (int n, float velocity, float glideFrom, bool glideOn)
    {
        const bool was = active();
        note = n; vel = velocity; gateHeld = true; pendingRelease = false;
        target = (float) n;
        if (! was || ! glideOn) cur = (glideOn && glideFrom >= 0.f) ? glideFrom : (float) n;
        punch = 1.f; click = 1.f; clickPh = 0.f;
        if (! was)
        {
            ph = 0.f; mph = 0.f;
            lpL.reset(); lpR.reset(); fbL.reset(); fbR.reset(); kbL.reset(); kbR.reset();
            toneL = toneR = dxL = dyL = dxR = dyR = 0.f;
        }
        amp.noteOn();
    }
    void slideTo (int n) { note = n; target = (float) n; } // legato 808 slide: no retrigger
    void release() { gateHeld = false; amp.noteOff(); }
    void kill() { amp.kill(); note = -1; }

    void render (float* L, float* R, int n, const Params& p, const float* wob, float sr, float bend)
    {
        if (! active()) return;
        amp.set (1.f, p.f (pid::q8Decay), p.f (pid::q8Sus), p.f (pid::q8Rel), sr);

        const bool wobOn   = p.b (pid::q8WobOn);
        const int  target_ = p.n (pid::q8WobTarget);
        const float depth  = wobOn ? p.f (pid::q8WobDepth) : 0.f;
        const bool tFilter = target_ == 0 || target_ == 5, tGrowl = target_ == 1 || target_ == 5,
                   tFm = target_ == 2 || target_ == 5, tFold = target_ == 3, tPitch = target_ == 4;

        const float pitchOfs  = (float) p.sn (pid::q8Oct) * 12.f + bend;
        const float punchAmt  = p.f (pid::q8Punch);
        const float punchInc  = 1.f / std::max (1.f, p.f (pid::q8PunchTime) * 0.001f * sr);
        const float dcR       = 1.f - kTwoPi * 4.f / sr; // DC blocker at ~4 Hz (an 808 lives at 30 Hz)
        const float clickCoef = decayCoef (6.f, sr);
        const float clickAmt  = p.f (pid::q8Click);
        const float glideMs   = p.f (pid::q8Glide);
        const float glideCoef = glideMs > 0.5f ? std::exp (-1.f / (glideMs * 0.001f * sr * 0.25f)) : 0.f;
        const float shape     = p.f (pid::q8Shape);
        const float phaseOfs  = p.f (pid::q8Phase) / 360.f;
        const float fmBase    = p.f (pid::q8Fm), fmRatio = p.f (pid::q8FmRatio);
        const float foldBase  = p.f (pid::q8Fold);
        const int   mode      = p.n (pid::q8DrvMode);
        const float drvG      = dbToGain (p.f (pid::q8Drive) * 36.f);
        const float bias      = p.f (pid::q8Asym) * 0.6f;
        const float toneC     = onePoleCoef (500.f * std::exp2 (p.f (pid::q8Tone) * 5.3f), sr);
        const float subKeep   = p.f (pid::q8SubKeep);
        const float knock     = p.f (pid::q8Knock) * 3.f; // up to ~+12 dB around the 3rd harmonic
        const float cutBase   = p.f (pid::q8Cut);
        const float k         = 2.f - 1.9f * p.f (pid::q8Res);
        const float vowelBase = p.f (pid::q8Vowel), formQ = p.f (pid::q8GrowlRes);
        const float growlAmt  = std::max (p.f (pid::q8GrowlAmt), tGrowl ? depth : 0.f);
        const float level     = 0.85f * (0.75f + 0.25f * vel);
        const float nyq       = sr * 0.45f;

        for (int i = 0; i < n; ++i)
        {
            const float w = wob[i]; // unipolar 0..1

            cur = target + (cur - target) * glideCoef;
            // punch: curved drop that settles in exactly Punch Time (env = ramp^1.25, fitted to a real 808)
            float pitch = cur + pitchOfs + punchAmt * punch * std::sqrt (std::sqrt (punch));
            if (tPitch) pitch += depth * 12.f * (w * 2.f - 1.f);
            punch = std::max (0.f, punch - punchInc);
            const float f0 = clampf (mtof (pitch), 5.f, nyq);
            const float dt = f0 / sr;

            // FM growl
            const float fmA = clampf (fmBase + (tFm ? depth * w : 0.f), 0.f, 1.f);
            float mod = 0.f;
            if (fmA > 0.0005f)
            {
                mph += dt * fmRatio; mph -= std::floor (mph);
                mod = std::sin (kTwoPi * mph) * fmA * fmA * 6.f / kTwoPi;
            }
            float tL = ph + mod;            tL -= std::floor (tL);
            float tR = ph + phaseOfs + mod; tR -= std::floor (tR);
            float sL = shape808 (tL, shape), sR = shape808 (tR, shape);

            const float fold = clampf (foldBase + (tFold ? depth * w : 0.f), 0.f, 1.f);
            if (fold > 0.001f)
            {
                const float fm = std::min (1.f, fold * 4.f), fg = (1.f + fold * 6.f) * kPi * 0.5f;
                sL += fm * (std::sin (sL * fg) - sL);
                sR += fm * (std::sin (sR * fg) - sR);
            }

            // DIRT + DC block + tone
            float yL = dirt808 ((sL + bias) * drvG, mode), yR = dirt808 ((sR + bias) * drvG, mode);
            const float oL = yL - dxL + dcR * dyL; dxL = yL; dyL = oL;
            const float oR = yR - dxR + dcR * dyR; dxR = yR; dyR = oR;
            toneL += (oL - toneL) * toneC; toneR += (oR - toneR) * toneC;
            yL = toneL; yR = toneR;

            // KNOCK: pitch-tracking resonant boost on harmonics 2-4 (the 'mixed 808' punch)
            if (knock > 0.001f)
            {
                if ((i & 7) == 0) knockG = std::tan (kPi * std::min (f0 * 3.f, nyq) / sr);
                yL += kbL.process (yL, knockG, 1.2f, 1) * 1.2f * knock;
                yR += kbR.process (yR, knockG, 1.2f, 1) * 1.2f * knock;
            }

            // GROWL formants
            if (growlAmt > 0.001f)
            {
                if ((i & 7) == 0)
                {
                    const float v = tGrowl ? vowelBase * (1.f - depth) + w * depth : vowelBase;
                    fbL.setVowel (v, formQ, sr); fbR.g[0] = fbL.g[0]; fbR.g[1] = fbL.g[1]; fbR.g[2] = fbL.g[2]; fbR.k = fbL.k;
                }
                yL += (fbL.process (yL) - yL) * growlAmt;
                yR += (fbR.process (yR) - yR) * growlAmt;
            }

            // FILTER (wobble closes it from the set cutoff)
            float cut = cutBase;
            if (tFilter && depth > 0.f) cut *= std::exp2 (depth * 7.f * (w - 1.f));
            cut = clampf (cut, 20.f, nyq);
            const float g = std::tan (kPi * cut / sr);
            yL = lpL.process (yL, g, k, 0);
            yR = lpR.process (yR, g, k, 0);

            // clean mono sub underneath + transient click
            const float sub = std::sin (kTwoPi * ph) * subKeep;
            float c = 0.f;
            if (click > 0.0005f && clickAmt > 0.f)
            {
                c = (rng.bi() * 0.5f + std::sin (kTwoPi * clickPh)) * click * clickAmt * 0.8f;
                clickPh += 1700.f / sr; if (clickPh >= 1.f) clickPh -= 1.f;
                click *= clickCoef;
            }

            const float a = amp.next() * level;
            L[i] += softLimit ((yL * 0.8f + sub) * a + c * a);
            R[i] += softLimit ((yR * 0.8f + sub) * a + c * a);

            ph += dt; if (ph >= 1.f) ph -= 1.f;
            if (! amp.active()) { note = -1; break; }
        }
    }

private:
    Rng rng;
    ADSR amp;
    float cur = 36.f, target = 36.f, punch = 0.f, click = 0.f, clickPh = 0.f, ph = 0.f, mph = 0.f;
    SVF lpL, lpR, kbL, kbR;
    float knockG = 0.01f;
    FormantBank fbL, fbR;
    float toneL = 0, toneR = 0, dxL = 0, dyL = 0, dxR = 0, dyR = 0;
};

//==============================================================================
class Engine
{
public:
    Params p;
    static constexpr int kVoices = 12;
    static constexpr int k808Voices = 4;

    Engine()
    {
        for (int i = 0; i < kVoices; ++i) voices.emplace_back (0x1234567u + 7919u * (uint32_t) i);
        for (int i = 0; i < k808Voices; ++i) quakes.emplace_back (0xABCDEFu + 104729u * (uint32_t) i);
        held.reserve (128); held808.reserve (128);
    }

    void prepare (double sampleRate, int /*maxBlock*/)
    {
        sr = (float) sampleRate;
        glitch.prepare (sr); delay.prepare (sr); atmos.prepare (sr);
        drive = {}; crush = {};
        sideLp1 = sideLp2 = 0.f;
        allOff();
    }

    void setPitchBend (float normalised) { bendNorm = normalised; } // -1..1

    void beginBlock (double bpmIn, bool playingIn, double ppqIn)
    {
        bpm = (bpmIn > 20.0 && bpmIn < 999.0) ? bpmIn : 140.0;
        playing = playingIn; ppq = ppqIn;
        if (playing && p.b (pid::lfoSync))
        {
            const double ph = ppq / lfoDivBeats (p.n (pid::lfoDiv));
            const float np = (float) (ph - std::floor (ph));
            if (np < lfoPh && p.n (pid::lfoShape) >= 4) newRandomTarget();
            lfoPh = np;
        }
        if (playing && ! p.b (pid::q8WobRetrig)) // free wobble locks to the bar grid
        {
            const double ph = ppq / wobDivBeats (p.n (pid::q8WobDiv));
            wobPh = (float) (ph - std::floor (ph));
        }
    }

    // ---------------- MIDI ----------------
    void noteOn (int note, float velocity)
    {
        const int split = (int) (p.f (pid::splitNote) + 0.5f);
        const bool to808 = p.b (pid::q8On) && (split == 0 || note < split);
        const bool toSynth = p.f (pid::synthLvl) > 0.0001f && (split == 0 || note >= split);
        if (to808) noteOn808 (note, velocity);
        if (toSynth) noteOnSynth (note, velocity);
    }

    void noteOff (int note) { noteOffSynth (note); noteOff808 (note); }

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

    // ADDs both layers into L/R
    void renderVoices (float* L, float* R, int n)
    {
        const float bend = bendNorm * (float) p.n (pid::bendRange);
        const float synthLvl = p.f (pid::synthLvl), q8Lvl = p.f (pid::q8Lvl);
        const bool subMono = p.b (pid::subMono);
        const float sideC = onePoleCoef (140.f, sr);
        while (n > 0)
        {
            const int c = std::min (n, kChunk);
            computeLfo (c);
            computeWobble (c);
            std::fill (sL.begin(), sL.begin() + c, 0.f); std::fill (sR.begin(), sR.begin() + c, 0.f);
            std::fill (qL.begin(), qL.begin() + c, 0.f); std::fill (qR.begin(), qR.begin() + c, 0.f);
            for (auto& v : voices) v.render (sL.data(), sR.data(), c, p, lfoBuf.data(), sr, bend);
            for (auto& q : quakes) q.render (qL.data(), qR.data(), c, p, wobBuf.data(), sr, bend);
            for (int i = 0; i < c; ++i)
            {
                float a = qL[(size_t) i], b = qR[(size_t) i];
                if (subMono) // high-pass the side channel: lows stay mono, the width stays up top
                {
                    const float m = (a + b) * 0.5f, s = (a - b) * 0.5f;
                    sideLp1 += (s - sideLp1) * sideC; sideLp2 += (sideLp1 - sideLp2) * sideC;
                    const float sh = s - sideLp2;
                    a = m + sh; b = m - sh;
                }
                L[i] += sL[(size_t) i] * synthLvl + a * q8Lvl;
                R[i] += sR[(size_t) i] * synthLvl + b * q8Lvl;
            }
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
    std::vector<Quake808> quakes;
    std::vector<int> held, held808;
    DriveFx drive; CrushFx crush; GlitchFx glitch; SonarDelayFx delay; AtmosFx atmos;
    std::array<float, kChunk> lfoBuf {}, wobBuf {}, sL {}, sR {}, qL {}, qR {};
    float sr = 48000.f, bendNorm = 0.f, lastNote = -1.f, last808 = -1.f;
    double bpm = 140.0, ppq = 0.0; bool playing = false;
    bool sustainDown = false;
    uint64_t counter = 0;
    float lfoPh = 0.f, shVal = 0.f, shPrev = 0.f, shTarget = 0.f;
    float wobPh = 0.f, wobVal = 0.f, wobRand = 0.f;
    float sideLp1 = 0.f, sideLp2 = 0.f;
    Rng lfoRng { 0xC0FFEEu }, wobRng { 0xDEADBEEFu };

    // ---------------- synth layer ----------------
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
                if (v.active() && v.note == note && v.gateHeld)
                {
                    if (sustainDown) v.pendingRelease = true; else v.release();
                }
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

    // ---------------- 808 layer ----------------
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
            if (q.active() && q.gateHeld && mode == 2) { q.slideTo (note); retrigWob = false; } // legato slide
            else q.start (note, velocity, last808, glideOn && q.active());
            q.age = ++counter;
        }
        if (retrigWob && p.b (pid::q8WobRetrig)) { wobPh = 0.f; }
        last808 = (float) note;
    }

    void noteOff808 (int note)
    {
        const int mode = p.n (pid::q8Mode);
        if (mode == 0)
        {
            for (auto& q : quakes)
                if (q.active() && q.note == note && q.gateHeld)
                {
                    if (sustainDown) q.pendingRelease = true; else q.release();
                }
        }
        else
        {
            const bool wasHeld = std::find (held808.begin(), held808.end(), note) != held808.end();
            held808.erase (std::remove (held808.begin(), held808.end(), note), held808.end());
            auto& q = quakes[0];
            if (wasHeld && q.note == note)
            {
                if (! held808.empty()) q.slideTo (held808.back()); // slide back to the still-held note
                else if (sustainDown) q.pendingRelease = true;
                else q.release();
            }
        }
    }

    // ---------------- modulators ----------------
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

    // dubstep wobble LFO: always tempo-synced, unipolar 0..1, slew-limited so squares don't click
    void computeWobble (int n)
    {
        const int shape = p.n (pid::q8WobShape);
        const float inc = (float) (bpm / 60.0 / wobDivBeats (p.n (pid::q8WobDiv))) / sr;
        const float slew = onePoleCoef (300.f, sr);
        for (int i = 0; i < n; ++i)
        {
            wobPh += inc;
            if (wobPh >= 1.f) { wobPh -= 1.f; wobRand = wobRng.next(); }
            float y;
            switch (shape)
            {
                case 0:  y = 0.5f - 0.5f * std::cos (kTwoPi * wobPh); break;     // SINE (starts closed)
                case 1:  y = 1.f - 2.f * std::fabs (wobPh - 0.5f); break;        // TRI
                case 2:  y = 1.f - wobPh; break;                                 // SAW DN
                case 3:  y = wobPh; break;                                       // SAW UP
                case 4:  y = wobPh < 0.5f ? 1.f : 0.f; break;                    // SQUARE
                case 5:  { const float s = std::sin (kPi * wobPh); y = s * s * s * s; break; } // YOI
                default: y = wobRand; break;                                     // RANDOM
            }
            wobVal += (y - wobVal) * slew;
            wobBuf[(size_t) i] = wobVal;
        }
    }
};
} // namespace sw
