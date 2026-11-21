// WRAITHCORE - DSP building blocks (no JUCE dependency)
#pragma once
#include "ParamDefs.h"
#include <cmath>
#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include <memory>
#include <atomic>

namespace wc
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
// extra building blocks
//==============================================================================
inline float lerp (float a, float b, float t) { return a + (b - a) * t; }
inline float ceilingClip (float x) // transparent below -2 dBFS, never exceeds 0 dBFS
{
    const float a = std::fabs (x);
    if (a <= 0.8f) return x;
    const float y = 0.8f + 0.2f * ftanh ((a - 0.8f) / 0.2f);
    return x > 0.f ? y : -y;
}

struct OnePole // simple LP / HP helper
{
    float s = 0.f;
    float lp (float x, float c) { s += (x - s) * c; return s; }
    float hp (float x, float c) { s += (x - s) * c; return x - s; }
    void reset() { s = 0.f; }
};

// fractional delay line (linear interpolation)
struct DelayLine
{
    std::vector<float> b; int size = 1, w = 0;
    void prepare (int samples) { size = std::max (4, samples); b.assign ((size_t) size, 0.f); w = 0; }
    void clear() { std::fill (b.begin(), b.end(), 0.f); }
    void push (float x) { b[(size_t) w] = x; if (++w >= size) w = 0; }
    float read (float d) const // d samples behind the most recent write
    {
        d = clampf (d, 1.f, (float) size - 2.f);
        float idx = (float) w - d; if (idx < 0.f) idx += (float) size;
        int i0 = (int) idx; const float fr = idx - (float) i0; if (i0 >= size) i0 -= size;
        const int i1 = (i0 + 1 == size) ? 0 : i0 + 1;
        return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * fr;
    }
    float at (float absIdx) const // absolute (wrapping) index read
    {
        int i0 = (int) std::floor (absIdx); const float fr = absIdx - (float) i0;
        i0 %= size; if (i0 < 0) i0 += size;
        const int i1 = (i0 + 1 == size) ? 0 : i0 + 1;
        return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * fr;
    }
};

// RBJ biquad
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0.f; }
    float process (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void set (float c0, float c1, float c2, float d0, float d1, float d2) { b0 = c0 / d0; b1 = c1 / d0; b2 = c2 / d0; a1 = d1 / d0; a2 = d2 / d0; }
    void lowShelf (float f, float db, float sr)
    {
        const float A = std::pow (10.f, db / 40.f), w = kTwoPi * f / sr, cs = std::cos (w), sn = std::sin (w), al = sn / 2.f * std::sqrt (2.f), sa = 2.f * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * cs + sa), 2 * A * ((A - 1) - (A + 1) * cs), A * ((A + 1) - (A - 1) * cs - sa),
             (A + 1) + (A - 1) * cs + sa, -2 * ((A - 1) + (A + 1) * cs), (A + 1) + (A - 1) * cs - sa);
    }
    void highShelf (float f, float db, float sr)
    {
        const float A = std::pow (10.f, db / 40.f), w = kTwoPi * f / sr, cs = std::cos (w), sn = std::sin (w), al = sn / 2.f * std::sqrt (2.f), sa = 2.f * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * cs + sa), -2 * A * ((A - 1) + (A + 1) * cs), A * ((A + 1) + (A - 1) * cs - sa),
             (A + 1) - (A - 1) * cs + sa, 2 * ((A - 1) - (A + 1) * cs), (A + 1) - (A - 1) * cs - sa);
    }
    void peak (float f, float db, float q, float sr)
    {
        const float A = std::pow (10.f, db / 40.f), w = kTwoPi * f / sr, cs = std::cos (w), al = std::sin (w) / (2.f * q);
        set (1 + al * A, -2 * cs, 1 - al * A, 1 + al / A, -2 * cs, 1 - al / A);
    }
};

// 2x oversampler: 31-tap windowed-sinc halfband up/down, per channel
struct Oversampler2x
{
    static constexpr int N = 31;
    float h[N] {};
    float up[N] {}, dn[N] {};
    int upW = 0, dnW = 0;
    Oversampler2x()
    {
        const int M = (N - 1) / 2;
        float sum = 0.f;
        for (int i = 0; i < N; ++i)
        {
            const float n = (float) (i - M);
            const float sinc = n == 0.f ? 0.5f : std::sin (kPi * 0.5f * n) / (kPi * n);
            const float win = 0.42f - 0.5f * std::cos (kTwoPi * (float) i / (float) (N - 1)) + 0.08f * std::cos (2.f * kTwoPi * (float) i / (float) (N - 1));
            h[i] = sinc * win; sum += h[i];
        }
        for (auto& c : h) c /= sum;
    }
    void reset() { std::fill (std::begin (up), std::end (up), 0.f); std::fill (std::begin (dn), std::end (dn), 0.f); }
    float conv (const float* buf, int w) const
    {
        float acc = 0.f; int idx = w;
        for (int i = 0; i < N; ++i) { if (--idx < 0) idx = N - 1; acc += h[i] * buf[idx]; }
        return acc;
    }
    template <typename F> float process (float x, F&& fn)
    {
        up[upW] = x * 2.f; if (++upW >= N) upW = 0;
        const float u0 = conv (up, upW);
        up[upW] = 0.f; if (++upW >= N) upW = 0;
        const float u1 = conv (up, upW);
        dn[dnW] = fn (u0); if (++dnW >= N) dnW = 0;
        dn[dnW] = fn (u1); if (++dnW >= N) dnW = 0;
        return conv (dn, dnW);
    }
};

// Hilbert transformer (Olli Niemitalo's 8th-order allpass pair, ~90 deg from 20 Hz to Nyquist)
struct Hilbert
{
    float x1[4][2] {}, y1[4][2] {}, x2[4][2] {}, y2[4][2] {}; // [section][history]
    float delayed = 0.f;
    static float section (float in, float c, float* xh, float* yh)
    {
        const float out = c * (in + yh[1]) - xh[1];
        xh[1] = xh[0]; xh[0] = in; yh[1] = yh[0]; yh[0] = out;
        return out;
    }
    void process (float x, float& I, float& Q)
    {
        static const float c1[4] = { 0.6923878f * 0.6923878f, 0.9360654322959f * 0.9360654322959f, 0.9882295226860f * 0.9882295226860f, 0.9987488452737f * 0.9987488452737f };
        static const float c2[4] = { 0.4021921162426f * 0.4021921162426f, 0.8561710882420f * 0.8561710882420f, 0.9722909545651f * 0.9722909545651f, 0.9952884791278f * 0.9952884791278f };
        float a = x, b = x;
        for (int s = 0; s < 4; ++s) a = section (a, c1[s], x1[s], y1[s]);
        for (int s = 0; s < 4; ++s) b = section (b, c2[s], x2[s], y2[s]);
        I = delayed; delayed = a; // path 1 delayed one sample
        Q = b;
    }
};

inline double divBeats13 (int i) // "4 BAR|2 BAR|1/1|1/2|1/4|1/8|1/16|1/32|1/4T|1/8T|1/16T|1/4D|1/8D"
{
    static const double v[] = { 16, 8, 4, 2, 1, 0.5, 0.25, 0.125, 2.0 / 3, 1.0 / 3, 1.0 / 6, 1.5, 0.75 };
    return v[std::clamp (i, 0, 12)];
}
} // namespace wc
