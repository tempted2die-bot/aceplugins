// WRAITHCORE - FX rack modules. Every module takes 4 normalised knobs (p[0..3]) decoded via FxCatalog.
#pragma once
#include "Core.h"
#include "Quake808.h" // FormantBank

namespace wc
{
struct FxCtx { float sr = 48000.f; double bpm = 140.0; bool playing = false; double ppq = 0.0; };

struct FxModule
{
    int type = FX_NONE;
    virtual ~FxModule() = default;
    virtual void prepare (float sr) = 0;
    virtual void process (float* L, float* R, int n, const float* p, const FxCtx& c) = 0;
};

inline double samplesPerBeat (const FxCtx& c) { return 60.0 / c.bpm * c.sr; }

// =========================== DRIVE ===========================
struct DistortFx : FxModule
{
    Oversampler2x os[2]; float lp[2] {}, dx[2] {}, dy[2] {}; float sr = 48000.f;
    void prepare (float s) override { sr = s; for (auto& o : os) o.reset(); }
    static float shape (float x, int mode)
    {
        switch (mode)
        {
            case 0:  return ftanh (x);
            case 1:  return clampf (x, -1.f, 1.f);
            case 2:  return std::sin (clampf (x, -40.f, 40.f) * kPi * 0.5f);
            case 3:  { const float y = ftanh (x); return 0.65f * std::fabs (y) + 0.35f * y; }
            case 4:  { float y = x + 0.25f; y = y > 0.f ? ftanh (y) : ftanh (y * 3.f) * 0.6f; return clampf (y * 1.4f, -1.f, 1.f); }
            default: return x >= 0.f ? ftanh (x * 0.9f) : ftanh (x * 1.5f) * 0.82f; // TUBE
        }
    }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        const int mode = fxStep (p[0], 6);
        const float g = dbToGain (p[1] * 42.f), bias = p[3] * 0.6f, b0 = shape (bias, mode);
        const float c = onePoleCoef (600.f * std::exp2 (p[2] * 5.06f), sr), makeup = dbToGain (-p[1] * 5.f);
        const float dcR = 1.f - kTwoPi * 8.f / sr;
        float* ch[2] = { L, R };
        for (int k = 0; k < 2; ++k)
            for (int i = 0; i < n; ++i)
            {
                const float y = os[k].process (ch[k][i], [&] (float u) { return shape (u * g + bias, mode) - b0; });
                lp[k] += (y - lp[k]) * c;
                const float o = lp[k] - dx[k] + dcR * dy[k]; dx[k] = lp[k]; dy[k] = o;
                ch[k][i] = o * makeup;
            }
    }
};

struct ClipperFx : FxModule
{
    Oversampler2x os[2];
    void prepare (float) override { for (auto& o : os) o.reset(); }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        const float drive = dbToGain (p[0] * 24.f), knee = p[1], ceil = dbToGain (fxLin (p[2], -12.f, 0.f)), out = dbToGain (fxLin (p[3], -12.f, 6.f));
        const float pre = drive / ceil;
        float* ch[2] = { L, R };
        for (int k = 0; k < 2; ++k)
            for (int i = 0; i < n; ++i)
                ch[k][i] = os[k].process (ch[k][i], [&] (float u) { const float x = u * pre; return lerp (ftanh (x), clampf (x, -1.f, 1.f), knee) * ceil; }) * out;
    }
};

struct CrushFx : FxModule
{
    float ph = 0.f, hL = 0.f, hR = 0.f, nextInc = 0.f; Rng rng { 0x51u };
    void prepare (float) override { ph = 0.f; }
    void process (float* L, float* R, int n, const float* p, const FxCtx& c) override
    {
        const float step = 2.f / std::exp2 (1.f + p[0] * 15.f);
        const float inc = fxLog (p[1], 200.f, 48000.f) / c.sr;
        const float jit = p[2], noise = p[3] * 0.06f;
        if (nextInc <= 0.f) nextInc = inc;
        for (int i = 0; i < n; ++i)
        {
            ph += nextInc;
            if (ph >= 1.f || inc >= 1.f)
            {
                ph -= std::floor (ph);
                hL = L[i] + rng.bi() * noise; hR = R[i] + rng.bi() * noise;
                nextInc = inc * (1.f + jit * rng.bi() * 0.9f);
            }
            L[i] = std::floor (hL / step + 0.5f) * step;
            R[i] = std::floor (hR / step + 0.5f) * step;
        }
    }
};

struct BlownFx : FxModule
{
    Oversampler2x os[2]; SVF bp[2]; OnePole lpC[2], hpC[2], env[2], buzzHp[2]; Rng rng { 0xB10Eu }; float sr = 48000.f;
    void prepare (float s) override { sr = s; for (auto& o : os) o.reset(); }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        const float g = dbToGain (p[0] * 30.f), rattle = p[1];
        const float fc = 3000.f * std::pow (250.f / 3000.f, p[2]);
        const float bg = std::tan (kPi * fc / sr), k = 2.f - 1.9f * p[3];
        const float cLp = onePoleCoef (std::min (fc * 3.5f, sr * 0.45f), sr), cHp = onePoleCoef (fc * 0.25f, sr);
        const float cEnv = onePoleCoef (40.f, sr), cBuzz = onePoleCoef (900.f, sr), makeup = dbToGain (-p[0] * 9.f);
        float* ch[2] = { L, R };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float x = ch[c][i];
                const float pre = x + bp[c].process (x, bg, k, 1) * k * (1.f + p[3] * 2.f);
                float y = os[c].process (pre * g, [] (float u) { const float v = u + 0.15f; return v > 0.f ? ftanh (v) : ftanh (v * 2.f) * 0.7f; });
                const float e = env[c].lp (std::fabs (x) * g, cEnv);
                const float buzz = buzzHp[c].hp (rng.bi() * std::max (0.f, e - 0.3f), cBuzz) * rattle * 1.2f;
                y = hpC[c].hp (lpC[c].lp (y + buzz, cLp), cHp);
                ch[c][i] = y * makeup * 1.6f;
            }
    }
};

struct SubharmFx : FxModule
{
    OnePole lp1, lp2, t1, t2, env; bool ff1 = false, ff2 = false; float last = 0.f; float sr = 48000.f;
    void prepare (float s) override { sr = s; }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        const float lvl = p[0] * 1.6f, toneC = onePoleCoef (fxLog (p[2], 40.f, 400.f), sr), drv = 1.f + p[3] * 7.f;
        const int oct = fxStep (p[1], 2);
        const float c150 = onePoleCoef (150.f, sr), cEnv = onePoleCoef (20.f, sr);
        for (int i = 0; i < n; ++i)
        {
            const float m = (L[i] + R[i]) * 0.5f;
            const float f = lp2.lp (lp1.lp (m, c150), c150);
            if (last <= 0.f && f > 0.f) { ff1 = ! ff1; if (ff1) ff2 = ! ff2; }
            last = f;
            const float e = env.lp (std::fabs (f) * 1.6f, cEnv);
            float s = ((oct == 0 ? ff1 : ff2) ? 1.f : -1.f) * e;
            s = t2.lp (t1.lp (s, toneC), toneC);
            s = ftanh (s * drv) / std::sqrt (drv);
            L[i] += s * lvl; R[i] += s * lvl;
        }
    }
};

struct DatamoshFx : FxModule
{
    enum Act { None, Flip, Hold, Shuffle, Decimate, Reverse };
    DelayLine bL, bR; Rng rng { 0xDA7Au };
    Act act = None; int count = 0, chunk = 1, pos = 0, holdCnt = 0; float wb = 0.f, src = 0.f, shL = 0.f, shR = 0.f, xf = 0.f;
    int mask = 0; float sr = 48000.f;
    void prepare (float s) override { sr = s; bL.prepare ((int) (s * 3.f)); bR.prepare ((int) (s * 3.f)); count = 0; }
    static float flip (float x, int m) { int q = (int) (clampf (x, -1.f, 1.f) * 32767.f); q = (q < 0) ? -((-q) ^ m) : (q ^ m); return (float) q / 32768.f; }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        const float corrupt = p[0], holdP = p[2];
        const int nb = 1 + (int) (p[3] * 11.f);
        chunk = std::max (16, (int) (fxLog (p[1], 2.f, 250.f) * 0.001f * sr));
        for (int i = 0; i < n; ++i)
        {
            bL.push (L[i]); bR.push (R[i]);
            const float wAbs = (float) bL.w - 1.f;
            if (--count <= 0)
            {
                count = chunk; pos = 0; wb = wAbs;
                if (rng.next() < corrupt)
                {
                    if (rng.next() < holdP) act = Hold;
                    else { static const Act a[] = { Flip, Shuffle, Decimate, Reverse }; act = a[(int) (rng.next() * 3.999f)]; }
                    mask = ((1 << nb) - 1) << (15 - nb);
                    mask &= (int) (rng.next() * 32767.f) | (1 << (14 - std::min (13, nb / 2)));
                    src = wAbs - (float) chunk - rng.next() * sr * 1.5f;
                }
                else act = None;
            }
            float yl = L[i], yr = R[i];
            switch (act)
            {
                case Flip:     yl = flip (yl, mask); yr = flip (yr, mask); break;
                case Hold:     yl = bL.at (wb - (float) chunk + (float) (pos % chunk)); yr = bR.at (wb - (float) chunk + (float) (pos % chunk)); break;
                case Shuffle:  yl = bL.at (src + (float) pos); yr = bR.at (src + (float) pos); break;
                case Decimate: if (holdCnt-- <= 0) { holdCnt = 6 + (int) (rng.next() * 26.f); shL = std::floor (yl * 8.f) / 8.f; shR = std::floor (yr * 8.f) / 8.f; } yl = shL; yr = shR; break;
                case Reverse:  yl = bL.at (wb - 1.f - (float) pos); yr = bR.at (wb - 1.f - (float) pos); break;
                default: break;
            }
            xf += ((act == None ? 0.f : 1.f) - xf) * 0.08f;
            L[i] += (yl - L[i]) * xf; R[i] += (yr - R[i]) * xf;
            ++pos;
        }
    }
};

// =========================== TONE ===========================
struct FilterFx : FxModule
{
    SVF s[4]; DelayLine cL, cR; float cut = -1.f, sr = 48000.f, dl = 0.f, dr = 0.f;
    void prepare (float r) override { sr = r; cL.prepare ((int) (r / 15.f) + 8); cR.prepare ((int) (r / 15.f) + 8); cut = -1.f; }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        const int type = fxStep (p[0], 7);
        const float target = fxLog (p[1], 20.f, 20000.f);
        if (cut < 0.f) cut = target;
        const float res = p[2], k = 2.f - 1.95f * res, drive = p[3], dg = 1.f + drive * 8.f, dc = 1.f / (1.f + drive * 2.f);
        const float damp = onePoleCoef (7000.f, sr);
        for (int i = 0; i < n; ++i)
        {
            cut += (target - cut) * 0.003f;
            float xl = L[i], xr = R[i];
            if (drive > 0.001f) { xl = ftanh (xl * dg) * dc * 1.6f; xr = ftanh (xr * dg) * dc * 1.6f; }
            if (type >= 5)
            {
                const float fb = (type == 5 ? 1.f : -1.f) * (0.4f + res * 0.57f);
                const float d = clampf (sr / cut, 2.f, (float) cL.size - 3.f);
                dl += (cL.read (d) - dl) * damp; dr += (cR.read (d) - dr) * damp;
                const float yl = xl + dl * fb, yr = xr + dr * fb;
                cL.push (ftanh (yl)); cR.push (ftanh (yr));
                L[i] = yl * 0.6f; R[i] = yr * 0.6f;
                continue;
            }
            const float g = std::tan (kPi * clampf (cut, 16.f, sr * 0.45f) / sr);
            switch (type)
            {
                case 0: L[i] = s[0].process (xl, g, k, 0); R[i] = s[1].process (xr, g, k, 0); break;
                case 1: L[i] = s[2].process (s[0].process (xl, g, k, 0), g, 1.4f, 0); R[i] = s[3].process (s[1].process (xr, g, k, 0), g, 1.4f, 0); break;
                case 2: L[i] = s[0].process (xl, g, k, 1) * std::pow (k, 0.75f); R[i] = s[1].process (xr, g, k, 1) * std::pow (k, 0.75f); break;
                case 3: L[i] = s[0].process (xl, g, k, 2); R[i] = s[1].process (xr, g, k, 2); break;
                default: L[i] = s[0].process (xl, g, k, 3); R[i] = s[1].process (xr, g, k, 3); break;
            }
        }
    }
};

struct FormantFx : FxModule
{
    FormantBank fb[2]; float ph = 0.f, sr = 48000.f;
    void prepare (float s) override { sr = s; fb[0].reset(); fb[1].reset(); }
    void process (float* L, float* R, int n, const float* p, const FxCtx& c) override
    {
        const int rateI = fxStep (p[2], 9);
        const double beats = fxDivBeats (6, rateI);
        const float depth = rateI == 0 ? 0.f : p[3];
        const double spb = samplesPerBeat (c);
        for (int i = 0; i < n; ++i)
        {
            if (rateI > 0)
            {
                if (c.playing) { const double t = (c.ppq + (double) i / spb) / beats; ph = (float) (t - std::floor (t)); }
                else { ph += (float) (1.0 / (beats * spb)); if (ph >= 1.f) ph -= 1.f; }
            }
            if ((i & 7) == 0)
            {
                const float u = 0.5f - 0.5f * std::cos (kTwoPi * ph);
                fb[0].setVowel (p[0] * (1.f - depth) + u * depth, p[1], sr);
                for (int j = 0; j < 3; ++j) fb[1].g[j] = fb[0].g[j];
                fb[1].k = fb[0].k;
            }
            L[i] = fb[0].process (L[i]); R[i] = fb[1].process (R[i]);
        }
    }
};

struct EqFx : FxModule
{
    Biquad lo[2], mid[2], hi[2]; float last[4] { -1, -1, -1, -1 }, sr = 48000.f;
    void prepare (float s) override { sr = s; last[0] = -1.f; }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        if (p[0] != last[0] || p[1] != last[1] || p[2] != last[2] || p[3] != last[3])
        {
            for (int c = 0; c < 2; ++c)
            {
                lo[c].lowShelf (150.f, fxLin (p[0], -15.f, 15.f), sr);
                mid[c].peak (fxLog (p[3], 150.f, 6800.f), fxLin (p[1], -15.f, 15.f), 0.9f, sr);
                hi[c].highShelf (6000.f, fxLin (p[2], -15.f, 15.f), sr);
            }
            for (int j = 0; j < 4; ++j) last[j] = p[j];
        }
        for (int i = 0; i < n; ++i)
        {
            L[i] = hi[0].process (mid[0].process (lo[0].process (L[i])));
            R[i] = hi[1].process (mid[1].process (lo[1].process (R[i])));
        }
    }
};

struct ResonatorFx : FxModule
{
    DelayLine d[2][3]; float lp[2][3] {}; float sr = 48000.f;
    void prepare (float s) override { sr = s; for (auto& a : d) for (auto& b : a) b.prepare ((int) (s / 25.f) + 8); }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        static const int chords[6][3] = { { 0, 0, 0 }, { 0, 12, 24 }, { 0, 7, 12 }, { 0, 3, 7 }, { 0, 4, 7 }, { 0, 1, 2 } };
        const int note = 24 + fxStep (p[0], 61), ch = fxStep (p[3], 6);
        const int nv = ch == 0 ? 1 : 3;
        const float fb = 0.85f + p[1] * 0.149f;
        const float dc = onePoleCoef (20000.f * std::pow (0.04f, p[2]), sr);
        float dl[3];
        for (int v = 0; v < 3; ++v) dl[v] = clampf (sr / mtof ((float) (note + chords[ch][v])), 2.f, (float) d[0][0].size - 3.f);
        const float norm = (1.f - fb) * 3.f / (float) nv;
        float* io[2] = { L, R };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float x = io[c][i];
                float out = 0.f;
                for (int v = 0; v < nv; ++v)
                {
                    lp[c][v] += (d[c][v].read (dl[v]) - lp[c][v]) * dc;
                    const float y = x + lp[c][v] * fb;
                    d[c][v].push (ftanh (y * 0.5f) * 2.f);
                    out += y;
                }
                io[c][i] = out * norm;
            }
    }
};

// =========================== DYNAMICS ===========================
struct OttFx : FxModule
{
    SVF xlp[2][2][2], xhp[2][2][2]; // [crossover][stage][ch]
    float env[3][2] {}, gain[3][2] { { 1, 1 }, { 1, 1 }, { 1, 1 } };
    float sr = 48000.f; int cnt = 0; float gTarget[3][2] { { 1, 1 }, { 1, 1 }, { 1, 1 } };
    void prepare (float s) override { sr = s; }
    float lr4lp (int xo, int ch, float x, float g) { return xlp[xo][1][ch].process (xlp[xo][0][ch].process (x, g, 1.41421f, 0), g, 1.41421f, 0); }
    float lr4hp (int xo, int ch, float x, float g) { return xhp[xo][1][ch].process (xhp[xo][0][ch].process (x, g, 1.41421f, 2), g, 1.41421f, 2); }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        const float depth = p[0], time = fxLog (p[1], 0.2f, 3.f), up = p[2], down = p[3];
        const float g1 = std::tan (kPi * 120.f / sr), g2 = std::tan (kPi * 2500.f / sr);
        const float cEnv = onePoleCoef (1000.f / (8.f * time), sr);
        const float att = onePoleCoef (1000.f / (3.f * time), sr), rel = onePoleCoef (1000.f / (90.f * time), sr);
        static const float thr[3] = { -26.f, -24.f, -30.f };
        const float makeup = dbToGain (3.f * depth);
        float* io[2] = { L, R };
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < 2; ++c)
            {
                const float x = io[c][i];
                float b[3];
                b[0] = lr4lp (0, c, x, g1);
                const float rest = lr4hp (0, c, x, g1);
                b[1] = lr4lp (1, c, rest, g2);
                b[2] = lr4hp (1, c, rest, g2);
                float y = 0.f;
                for (int k = 0; k < 3; ++k)
                {
                    env[k][c] += (b[k] * b[k] - env[k][c]) * cEnv;
                    if (cnt == 0)
                    {
                        const float ldb = 10.f * std::log10 (env[k][c] + 1e-12f);
                        float gdb = 0.f;
                        if (ldb > thr[k]) gdb -= (ldb - thr[k]) * 0.9f * down;
                        else if (ldb > -75.f) gdb += std::min (18.f, (thr[k] - ldb) * 0.67f * up) * clampf ((ldb + 75.f) / 15.f, 0.f, 1.f);
                        gTarget[k][c] = dbToGain (gdb * depth);
                    }
                    const float gt = gTarget[k][c];
                    gain[k][c] += (gt - gain[k][c]) * (gt < gain[k][c] ? att : rel);
                    y += b[k] * gain[k][c];
                }
                io[c][i] = y * makeup;
            }
            if (++cnt >= 8) cnt = 0;
        }
    }
};

struct PumpFx : FxModule
{
    float ph = 0.f, g = 1.f, sr = 48000.f;
    void prepare (float s) override { sr = s; }
    void process (float* L, float* R, int n, const float* p, const FxCtx& c) override
    {
        const double beats = fxDivBeats (2, fxStep (p[0], 5)), spb = samplesPerBeat (c);
        const float depth = p[1], curve = std::exp2 ((p[2] - 0.5f) * 3.f), hold = p[3] * 0.4f;
        const float sm = onePoleCoef (300.f, sr);
        for (int i = 0; i < n; ++i)
        {
            if (c.playing) { const double t = (c.ppq + (double) i / spb) / beats; ph = (float) (t - std::floor (t)); }
            else { ph += (float) (1.0 / (beats * spb)); if (ph >= 1.f) ph -= 1.f; }
            const float t = ph < hold ? 0.f : std::pow ((ph - hold) / (1.f - hold), curve);
            g += ((1.f - depth * (1.f - t)) - g) * sm;
            L[i] *= g; R[i] *= g;
        }
    }
};

struct GateFx : FxModule
{
    float g = 1.f, sr = 48000.f; double freePos = 0.0;
    void prepare (float s) override { sr = s; }
    void process (float* L, float* R, int n, const float* p, const FxCtx& c) override
    {
        const double beats = fxDivBeats (3, fxStep (p[0], 4)), spb = samplesPerBeat (c);
        const unsigned pat = gatePattern (fxStep (p[1], 16));
        const float depth = p[2], sm = onePoleCoef (1000.f / (0.5f + p[3] * 30.f), sr);
        for (int i = 0; i < n; ++i)
        {
            double pos;
            if (c.playing) pos = (c.ppq + (double) i / spb) / beats;
            else { freePos += 1.0 / (beats * spb); pos = freePos; }
            const int step = ((int) std::floor (pos)) & 15;
            const bool open = (pat >> (15 - step)) & 1u;
            const double frac = pos - std::floor (pos);
            const float target = (open && frac < 0.85) ? 1.f : 1.f - depth;
            g += (target - g) * sm;
            L[i] *= g; R[i] *= g;
        }
    }
};

// =========================== SPACE ===========================
struct DelayFx : FxModule
{
    DelayLine bL, bR; float lpL = 0, lpR = 0, time = -1.f, ph1 = 0.f, ph2 = 0.f, sr = 48000.f; OnePole hIn;
    void prepare (float s) override { sr = s; bL.prepare ((int) (s * 6.f)); bR.prepare ((int) (s * 6.f)); time = -1.f; }
    void process (float* L, float* R, int n, const float* p, const FxCtx& c) override
    {
        const float target = clampf ((float) (fxDivBeats (0, fxStep (p[0], 9)) * samplesPerBeat (c)), 16.f, (float) bL.size - 4096.f);
        if (time < 0.f) time = target;
        const float fb = p[1] * 1.02f;
        const float cT = onePoleCoef (400.f * std::exp2 (p[2] * 5.6f), sr);
        const float depth = p[3] * 0.006f * sr;
        const float i1 = kTwoPi * 0.63f / sr, i2 = kTwoPi * 2.9f / sr, hc = onePoleCoef (70.f, sr);
        for (int i = 0; i < n; ++i)
        {
            time += (target - time) * 0.0004f;
            ph1 += i1; if (ph1 > kTwoPi) ph1 -= kTwoPi;
            ph2 += i2; if (ph2 > kTwoPi) ph2 -= kTwoPi;
            const float mod = (std::sin (ph1) + 0.25f * std::sin (ph2)) * depth;
            const float yl = bL.read (time + mod + 2.f), yr = bR.read (time - mod + 2.f);
            lpL += (yl - lpL) * cT; lpR += (yr - lpR) * cT;
            bL.push (ftanh (hIn.hp ((L[i] + R[i]) * 0.5f, hc) + lpR * fb));
            bR.push (ftanh (lpL * fb));
            L[i] = yl; R[i] = yr; // wet only (rack mix blends)
        }
    }
};

struct PitchShifter
{
    std::vector<float> b; int size = 1, w = 0; float ph = 0.f;
    void prepare (int maxWin) { size = maxWin + 16; b.assign ((size_t) size, 0.f); w = 0; ph = 0.f; }
    float tap (float d) const
    {
        float idx = (float) w - d - 2.f; while (idx < 0.f) idx += (float) size;
        int i0 = (int) idx; const float fr = idx - (float) i0; if (i0 >= size) i0 -= size;
        const int i1 = (i0 + 1) % size;
        return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * fr;
    }
    float process (float x, float ratio, float win)
    {
        win = std::min (win, (float) size - 20.f);
        b[(size_t) w] = x;
        ph -= (ratio - 1.f) / win; ph -= std::floor (ph);
        float ph2 = ph + 0.5f; ph2 -= std::floor (ph2);
        const float y = tap (ph * win) * std::sin (kPi * ph) + tap (ph2 * win) * std::sin (kPi * ph2);
        if (++w >= size) w = 0;
        return y;
    }
};

struct ReverbFx : FxModule
{
    static constexpr int N = 8;
    std::vector<float> line[N]; int lsize = 1, w = 0;
    float lp[N] {};
    std::vector<float> ap[4]; int apLen[4] {}, apW[4] {};
    PitchShifter shifter;
    float shimmerFb = 0.f, sizeSm = -1.f, modPh = 0.f, sr = 48000.f, h1 = 0.f, h2 = 0.f;

    void prepare (float s) override
    {
        sr = s;
        lsize = (int) (sr * 0.25f) + 64;
        for (auto& l : line) l.assign ((size_t) lsize, 0.f);
        static const float apMs[4] = { 4.77f, 3.59f, 12.73f, 9.31f };
        for (int i = 0; i < 4; ++i) { apLen[i] = (int) (apMs[i] * 0.001f * sr); ap[i].assign ((size_t) apLen[i] + 1, 0.f); apW[i] = 0; }
        shifter.prepare ((int) (sr * 0.06f));
        w = 0; shimmerFb = 0.f; sizeSm = -1.f;
    }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        static const float baseMs[N] = { 31.7f, 37.3f, 41.9f, 47.1f, 53.6f, 59.3f, 67.1f, 73.9f };
        if (sizeSm < 0.f) sizeSm = p[0];
        sizeSm += (p[0] - sizeSm) * 0.05f;
        const float scale = 0.35f + sizeSm * 1.6f, t60 = 0.4f + sizeSm * sizeSm * 14.f;
        float len[N], g[N];
        for (int i = 0; i < N; ++i) { len[i] = std::min ((float) lsize - 40.f, baseMs[i] * scale * 0.001f * sr); g[i] = std::pow (10.f, -3.f * len[i] / (t60 * sr)); }
        const float c = onePoleCoef (18000.f * std::exp2 (-p[1] * 5.f), sr);
        const float shimmer = p[2], hc = onePoleCoef (fxLog (p[3], 20.f, 600.f), sr);
        const float modInc = kTwoPi * 0.31f / sr, modDepth = 0.00025f * sr, win = std::floor (sr * 0.06f);
        for (int i = 0; i < n; ++i)
        {
            float x = (L[i] + R[i]) * 0.5f;
            h1 += (x - h1) * hc; h2 += (h1 - h2) * hc;
            x = x - h2 + shimmerFb;
            for (int a = 0; a < 4; ++a)
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
            for (int k = 0; k < N; ++k) line[k][(size_t) w] = lp[k] * g[k] - h + ((k & 1) ? -x : x) * 0.5f;
            const float outL = (o[0] + o[2] + o[4] + o[6]) * 0.4f, outR = (o[1] + o[3] + o[5] + o[7]) * 0.4f;
            shimmerFb = shimmer > 0.001f ? ftanh (shifter.process ((outL + outR) * 0.5f, 2.f, win) * shimmer * 0.55f) : 0.f;
            L[i] = outL; R[i] = outR; // wet only
            if (++w >= lsize) w = 0;
        }
    }
};

struct ReverseFx : FxModule
{
    DelayLine bL, bR; int count = 0, chunk = 1, k = 0; float startPrev = 0.f, startCur = 0.f, outL = 0.f, outR = 0.f, sr = 48000.f;
    OnePole tl, tr; float apb[2][2][700] {}; int apw[2][2] {};
    void prepare (float s) override { sr = s; bL.prepare ((int) (s * 8.f)); bR.prepare ((int) (s * 8.f)); count = 0; }
    float allpass (int ch, int i, float x, int len)
    {
        float* b = apb[ch][i]; int& w = apw[ch][i];
        const float d = b[w]; const float y = -0.6f * x + d; b[w] = x + 0.6f * y; if (++w >= len) w = 0; return y;
    }
    void process (float* L, float* R, int n, const float* p, const FxCtx& c) override
    {
        const int len = (int) std::min ((double) bL.size - 64.0, fxDivBeats (5, fxStep (p[0], 5)) * samplesPerBeat (c));
        const float fb = p[1] * 0.85f, cT = onePoleCoef (500.f * std::exp2 (p[2] * 5.3f), sr), smear = p[3];
        const int a1 = std::min (699, (int) (0.0071f * sr)), a2 = std::min (699, (int) (0.0113f * sr));
        for (int i = 0; i < n; ++i)
        {
            if (--count <= 0) { count = len; chunk = len; k = 0; startPrev = startCur; startCur = (float) bL.w; }
            const float idx = startCur - 1.f - (float) k; // previous chunk, backwards
            const float fade = std::min (1.f, std::min ((float) k, (float) (chunk - k)) / (0.03f * (float) chunk + 1.f));
            float yl = bL.at (idx) * fade, yr = bR.at (idx) * fade;
            yl = tl.lp (yl, cT); yr = tr.lp (yr, cT);
            if (smear > 0.01f)
            {
                const float sl = allpass (0, 1, allpass (0, 0, yl, a1), a2), sr2 = allpass (1, 1, allpass (1, 0, yr, a1), a2);
                yl += (sl - yl) * smear; yr += (sr2 - yr) * smear;
            }
            bL.push (L[i] + outL * fb); bR.push (R[i] + outR * fb);
            outL = yl; outR = yr;
            L[i] = yl; R[i] = yr;
            ++k;
        }
    }
};

struct ChorusFx : FxModule
{
    DelayLine d; float ph = 0.f, sr = 48000.f;
    void prepare (float s) override { sr = s; d.prepare ((int) (s * 0.05f)); }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        const float rate = fxLog (p[0], 0.05f, 8.f), depth = p[1] * 0.008f * sr, base = 0.011f * sr;
        const int nv = 1 + fxStep (p[2], 4);
        const float width = p[3];
        float gl[4], gr[4];
        for (int v = 0; v < nv; ++v)
        {
            const float pan = nv == 1 ? 0.f : (((float) v / (float) (nv - 1)) * 2.f - 1.f) * width;
            gl[v] = std::cos ((pan + 1.f) * kPi * 0.25f) * 1.2f / std::sqrt ((float) nv);
            gr[v] = std::sin ((pan + 1.f) * kPi * 0.25f) * 1.2f / std::sqrt ((float) nv);
        }
        for (int i = 0; i < n; ++i)
        {
            d.push ((L[i] + R[i]) * 0.5f);
            ph += rate / sr; if (ph >= 1.f) ph -= 1.f;
            float yl = 0.f, yr = 0.f;
            for (int v = 0; v < nv; ++v)
            {
                const float s = d.read (base + depth * (0.5f + 0.5f * std::sin (kTwoPi * (ph + (float) v / (float) nv))) + (float) v * 0.0017f * sr);
                yl += s * gl[v]; yr += s * gr[v];
            }
            L[i] = yl; R[i] = yr;
        }
    }
};

struct PhaserFx : FxModule
{
    float z[2][12] {}, last[2] {}, ph = 0.f, sr = 48000.f;
    void prepare (float s) override { sr = s; }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        static const int st[5] = { 2, 4, 6, 8, 12 };
        const int ns = st[fxStep (p[3], 5)];
        const float rate = fxLog (p[0], 0.02f, 10.f), depth = p[1], fb = (p[2] * 2.f - 1.f) * 0.95f;
        float* io[2] = { L, R };
        for (int i = 0; i < n; ++i)
        {
            ph += rate / sr; if (ph >= 1.f) ph -= 1.f;
            for (int c = 0; c < 2; ++c)
            {
                const float l = 0.5f + 0.5f * std::sin (kTwoPi * (ph + (c ? 0.25f : 0.f)));
                const float fc = 250.f * std::exp2 (depth * 4.6f * l);
                const float t = std::tan (kPi * std::min (fc, sr * 0.45f) / sr), a = (t - 1.f) / (t + 1.f);
                float x = io[c][i] + last[c] * fb;
                for (int s = 0; s < ns; ++s) { const float y = a * x + z[c][s]; z[c][s] = x - a * y; x = y; }
                last[c] = ftanh (x);
                io[c][i] = (io[c][i] + x) * 0.5f;
            }
        }
    }
};

// =========================== GLITCH ===========================
struct StutterFx : FxModule
{
    DelayLine bL, bR; bool active = false; int segStart = 0, segLen = 1; float pos = 0.f, speed = 1.f, gain = 0.f;
    bool reverse = false; double freeCount = 0.0; long long lastSlice = -1; Rng rng { 0x5707u };
    void prepare (float s) override { bL.prepare ((int) (s * 6.f)); bR.prepare ((int) (s * 6.f)); active = false; gain = 0.f; lastSlice = -1; }
    void process (float* L, float* R, int n, const float* p, const FxCtx& c) override
    {
        const double divB = fxDivBeats (1, fxStep (p[0], 5)), spb = samplesPerBeat (c), sliceLen = std::max (64.0, divB * spb);
        for (int i = 0; i < n; ++i)
        {
            bL.push (L[i]); bR.push (R[i]);
            const int w = bL.w - 1;
            bool boundary = false;
            if (c.playing) { const long long s = (long long) std::floor ((c.ppq + (double) i / spb) / divB); if (s != lastSlice) { boundary = true; lastSlice = s; } }
            else { freeCount += 1.0; if (freeCount >= sliceLen) { freeCount -= sliceLen; boundary = true; } }
            if (boundary)
            {
                if (rng.next() < p[1])
                {
                    static const int divs[] = { 1, 2, 2, 4, 4, 8 };
                    segLen = std::max (64, (int) (sliceLen / divs[(int) (rng.next() * 5.999f)]));
                    segStart = w - segLen; reverse = rng.next() < p[2];
                    speed = rng.next() < p[3] ? (rng.next() < 0.5f ? 2.f : 0.5f) : 1.f;
                    pos = 0.f; active = true;
                }
                else active = false;
            }
            gain += ((active ? 1.f : 0.f) - gain) * 0.02f;
            if (gain > 1e-4f)
            {
                const float fade = std::min (96.f, segLen * 0.25f);
                const float rp = reverse ? (float) segLen - 1.f - pos : pos;
                const float win = std::min (1.f, std::min (pos / fade, ((float) segLen - pos) / fade));
                const float wl = bL.at ((float) segStart + rp) * win, wr = bR.at ((float) segStart + rp) * win;
                L[i] += (wl - L[i]) * gain; R[i] += (wr - R[i]) * gain;
                pos += speed; if (pos >= (float) segLen) pos -= (float) segLen;
            }
        }
    }
};

struct TapeStopFx : FxModule
{
    enum Mode { None, Stop, Gap, Spin };
    DelayLine bL, bR; Mode mode = None; float rpos = 0.f, speed = 1.f, dec = 0.f, gain = 0.f;
    double freeCount = 0.0; long long lastSlice = -1; Rng rng { 0x7A9Eu };
    void prepare (float s) override { bL.prepare ((int) (s * 8.f)); bR.prepare ((int) (s * 8.f)); mode = None; gain = 0.f; lastSlice = -1; }
    void process (float* L, float* R, int n, const float* p, const FxCtx& c) override
    {
        const double divB = fxDivBeats (4, fxStep (p[0], 4)), spb = samplesPerBeat (c), sliceLen = std::max (64.0, divB * spb);
        for (int i = 0; i < n; ++i)
        {
            bL.push (L[i]); bR.push (R[i]);
            bool boundary = false;
            if (c.playing) { const long long s = (long long) std::floor ((c.ppq + (double) i / spb) / divB); if (s != lastSlice) { boundary = true; lastSlice = s; } }
            else { freeCount += 1.0; if (freeCount >= sliceLen) { freeCount -= sliceLen; boundary = true; } }
            if (boundary)
            {
                const float r = rng.next();
                if (r < p[1] * 0.5f) { mode = Stop; rpos = (float) bL.w - 1.f; speed = 1.f; dec = 1.f / (float) sliceLen; }
                else if (rng.next() < p[3] * 0.4f) { mode = Spin; rpos = (float) bL.w - 1.f; speed = 0.f; dec = 4.f / (float) sliceLen; }
                else if (rng.next() < p[2] * 0.6f) mode = Gap;
                else mode = None;
            }
            gain += ((mode != None ? 1.f : 0.f) - gain) * 0.02f;
            if (gain > 1e-4f)
            {
                float wl = 0.f, wr = 0.f;
                if (mode == Stop)
                {
                    const float g = std::min (1.f, speed * 4.f);
                    wl = bL.at (rpos) * g; wr = bR.at (rpos) * g;
                    rpos += speed * speed; speed = std::max (0.f, speed - dec);
                }
                else if (mode == Spin) // DJ spinback: rewinds faster and faster, then fades
                {
                    const float g = std::max (0.f, 1.f - speed / 4.f);
                    wl = bL.at (rpos) * g; wr = bR.at (rpos) * g;
                    rpos -= speed; speed = std::min (4.f, speed + dec);
                }
                L[i] += (wl - L[i]) * gain; R[i] += (wr - R[i]) * gain;
            }
        }
    }
};

struct GrainsFx : FxModule
{
    struct Grain { bool on = false; float pos = 0.f, inc = 1.f, len = 1.f, t = 0.f, gl = 1.f, gr = 1.f; };
    DelayLine bL, bR; std::array<Grain, 24> grains {}; Rng rng { 0x6A1Eu };
    float spawn = 0.f, freezeTimer = 0.f, frozenW = 0.f, sr = 48000.f; bool frozen = false;
    void prepare (float s) override { sr = s; bL.prepare ((int) (s * 4.f)); bR.prepare ((int) (s * 4.f)); for (auto& g : grains) g.on = false; }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        const float size = fxLog (p[0], 10.f, 400.f) * 0.001f * sr, dens = fxLog (p[1], 1.f, 60.f), scatter = p[2], freezeP = p[3];
        const float overlap = std::max (1.f, dens * size / sr), norm = 1.4f / std::sqrt (overlap);
        for (int i = 0; i < n; ++i)
        {
            if ((freezeTimer -= 1.f) <= 0.f)
            {
                freezeTimer = sr * 0.25f;
                const bool f = rng.next() < freezeP;
                if (f && ! frozen) frozenW = (float) bL.w - 1.f;
                frozen = f;
            }
            if (! frozen) { bL.push (L[i]); bR.push (R[i]); }
            if ((spawn -= 1.f) <= 0.f)
            {
                spawn = sr / dens * (0.5f + rng.next());
                for (auto& g : grains)
                    if (! g.on)
                    {
                        g.on = true; g.t = 0.f;
                        g.len = std::max (32.f, size * (1.f + scatter * rng.bi() * 0.5f));
                        g.inc = (rng.next() < scatter * 0.3f ? (rng.next() < 0.5f ? 0.5f : 2.f) : 1.f) * semis (rng.bi() * scatter * 0.4f);
                        const float ref = frozen ? frozenW : (float) bL.w - 1.f;
                        g.pos = ref - g.len * std::max (1.f, g.inc) - 64.f - rng.next() * scatter * sr * 0.5f;
                        const float pan = rng.bi() * scatter;
                        g.gl = std::cos ((pan + 1.f) * kPi * 0.25f) * 1.41f; g.gr = std::sin ((pan + 1.f) * kPi * 0.25f) * 1.41f;
                        break;
                    }
            }
            float yl = 0.f, yr = 0.f;
            for (auto& g : grains)
            {
                if (! g.on) continue;
                const float win = 0.5f - 0.5f * std::cos (kTwoPi * g.t / g.len);
                yl += bL.at (g.pos) * win * g.gl; yr += bR.at (g.pos) * win * g.gr;
                g.pos += g.inc; if ((g.t += 1.f) >= g.len) g.on = false;
            }
            L[i] = yl * norm; R[i] = yr * norm;
        }
    }
};

struct FreqShiftFx : FxModule
{
    Hilbert h[2]; float ph = 0.f, lfo = 0.f, last[2] {}, sr = 48000.f;
    void prepare (float s) override { sr = s; }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        const float s = p[0] * 2.f - 1.f, base = (s < 0 ? -1.f : 1.f) * s * s * 1000.f;
        const float fb = p[1] * 0.9f, lr = fxLin (p[2], 0.f, 8.f), la = p[3] * 500.f;
        float* io[2] = { L, R };
        for (int i = 0; i < n; ++i)
        {
            lfo += lr / sr; if (lfo >= 1.f) lfo -= 1.f;
            const float shift = base + la * std::sin (kTwoPi * lfo);
            ph += shift / sr; ph -= std::floor (ph);
            const float cs = std::cos (kTwoPi * ph), sn = std::sin (kTwoPi * ph);
            for (int c = 0; c < 2; ++c)
            {
                float I, Q;
                h[c].process (io[c][i] + last[c] * fb, I, Q);
                const float y = c == 0 ? I * cs - Q * sn : I * cs + Q * sn; // opposite shift on the right = width
                last[c] = ftanh (y);
                io[c][i] = y;
            }
        }
    }
};

struct RingModFx : FxModule
{
    float ph = 0.f, drift = 0.f, driftT = 0.f, timer = 0.f, sr = 48000.f; OnePole tl, tr; Rng rng { 0x7176u };
    void prepare (float s) override { sr = s; }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        const float f = fxLog (p[0], 20.f, 5000.f), cT = onePoleCoef (fxLog (p[3], 300.f, 20000.f), sr), shape = p[2];
        const float dc = onePoleCoef (3.f, sr);
        for (int i = 0; i < n; ++i)
        {
            if ((timer -= 1.f) <= 0.f) { timer = sr * 0.1f; driftT = rng.bi(); }
            drift += (driftT - drift) * dc;
            ph += f * std::exp2 (drift * p[1] * 0.7f) / sr; ph -= std::floor (ph);
            float car = std::sin (kTwoPi * ph);
            car = lerp (car, ftanh (car * 6.f) * 1.004f, shape);
            L[i] = tl.lp (L[i] * car, cT); R[i] = tr.lp (R[i] * car, cT);
        }
    }
};

struct PitchFx : FxModule
{
    PitchShifter ps[2]; float last[2] {}, drift = 0.f, sr = 48000.f;
    void prepare (float s) override { sr = s; ps[0].prepare ((int) (s * 0.13f)); ps[1].prepare ((int) (s * 0.13f)); }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        const float st = (float) (fxStep (p[0], 49) - 24) + (p[1] * 2.f - 1.f) * 0.5f;
        const float ratio = semis (st), fb = p[2] * 0.8f, win = fxLin (p[3], 20.f, 120.f) * 0.001f * sr;
        float* io[2] = { L, R };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float y = ps[c].process (io[c][i] + last[c] * fb, ratio * (c ? 1.002f : 1.f), win * (c ? 0.93f : 1.f));
                last[c] = ftanh (y);
                io[c][i] = y;
            }
    }
};

// =========================== CHARACTER ===========================
struct TapeFx : FxModule
{
    DelayLine dL, dR; float ph1 = 0.f, ph2 = 0.f, rnd = 0.f, rndT = 0.f, rt = 0.f, dropG = 1.f, dropT = 0.f, dropTimer = 0.f, sr = 48000.f;
    OnePole lL, lR; Rng rng { 0x7A7Eu };
    void prepare (float s) override { sr = s; dL.prepare ((int) (s * 0.05f)); dR.prepare ((int) (s * 0.05f)); }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        const float wow = p[0], flut = p[1], age = p[2], drop = p[3];
        const float cAge = onePoleCoef (18000.f * std::pow (3000.f / 18000.f, age), sr), sat = 1.f + age * 2.5f;
        const float base = 0.008f * sr, rc = onePoleCoef (1.5f, sr), dc = onePoleCoef (60.f, sr);
        for (int i = 0; i < n; ++i)
        {
            ph1 += 0.55f / sr; if (ph1 >= 1.f) ph1 -= 1.f;
            ph2 += 9.3f / sr;  if (ph2 >= 1.f) ph2 -= 1.f;
            if ((rt -= 1.f) <= 0.f) { rt = sr * 0.3f; rndT = rng.bi(); }
            rnd += (rndT - rnd) * rc;
            const float mod = wow * (std::sin (kTwoPi * ph1) + 0.6f * rnd) * 0.004f * sr + flut * std::sin (kTwoPi * ph2) * 0.0004f * sr;
            dL.push (L[i]); dR.push (R[i]);
            float yl = dL.read (base + mod), yr = dR.read (base + mod * 0.9f);
            yl = lL.lp (ftanh (yl * sat) / std::sqrt (sat), cAge); yr = lR.lp (ftanh (yr * sat) / std::sqrt (sat), cAge);
            const float hiss = age * 0.006f;
            yl += rng.bi() * hiss; yr += rng.bi() * hiss;
            if ((dropTimer -= 1.f) <= 0.f)
            {
                dropTimer = sr * 0.05f;
                if (dropT < 1.f) dropT = 1.f; // recover
                if (rng.next() < drop * 0.12f) { dropT = 0.12f; dropTimer = sr * (0.03f + rng.next() * 0.12f); }
            }
            dropG += (dropT - dropG) * dc;
            L[i] = yl * dropG; R[i] = yr * dropG;
        }
    }
};

struct StereoFx : FxModule
{
    DelayLine hz; OnePole s1, s2; float sr = 48000.f;
    void prepare (float s) override { sr = s; hz.prepare ((int) (s * 0.03f)); }
    void process (float* L, float* R, int n, const float* p, const FxCtx&) override
    {
        const float wdt = p[0] * 2.f, haas = p[1] * 0.025f * sr, monoC = p[2] < 0.01f ? 0.f : onePoleCoef (fxLog (p[2], 20.f, 500.f), sr);
        const float pan = p[3] * 2.f - 1.f, gl = std::cos ((pan + 1.f) * kPi * 0.25f) * 1.4142f, gr = std::sin ((pan + 1.f) * kPi * 0.25f) * 1.4142f;
        for (int i = 0; i < n; ++i)
        {
            float l = L[i], r = R[i];
            hz.push (r);
            if (haas > 1.f) r = hz.read (haas);
            const float m = (l + r) * 0.5f;
            float s = (l - r) * 0.5f * wdt;
            if (monoC > 0.f) s = s - s2.lp (s1.lp (s, monoC), monoC);
            L[i] = (m + s) * gl; R[i] = (m - s) * gr;
        }
    }
};

//==============================================================================
inline std::unique_ptr<FxModule> createFx (int type, float sr)
{
    std::unique_ptr<FxModule> m;
    switch (type)
    {
        case FX_DISTORT:   m = std::make_unique<DistortFx>(); break;
        case FX_CLIPPER:   m = std::make_unique<ClipperFx>(); break;
        case FX_CRUSH:     m = std::make_unique<CrushFx>(); break;
        case FX_BLOWN:     m = std::make_unique<BlownFx>(); break;
        case FX_SUBHARM:   m = std::make_unique<SubharmFx>(); break;
        case FX_DATAMOSH:  m = std::make_unique<DatamoshFx>(); break;
        case FX_FILTER:    m = std::make_unique<FilterFx>(); break;
        case FX_FORMANT:   m = std::make_unique<FormantFx>(); break;
        case FX_EQ:        m = std::make_unique<EqFx>(); break;
        case FX_RESONATOR: m = std::make_unique<ResonatorFx>(); break;
        case FX_OTT:       m = std::make_unique<OttFx>(); break;
        case FX_PUMP:      m = std::make_unique<PumpFx>(); break;
        case FX_GATE:      m = std::make_unique<GateFx>(); break;
        case FX_DELAY:     m = std::make_unique<DelayFx>(); break;
        case FX_REVERB:    m = std::make_unique<ReverbFx>(); break;
        case FX_REVERSE:   m = std::make_unique<ReverseFx>(); break;
        case FX_CHORUS:    m = std::make_unique<ChorusFx>(); break;
        case FX_PHASER:    m = std::make_unique<PhaserFx>(); break;
        case FX_STUTTER:   m = std::make_unique<StutterFx>(); break;
        case FX_TAPESTOP:  m = std::make_unique<TapeStopFx>(); break;
        case FX_GRAINS:    m = std::make_unique<GrainsFx>(); break;
        case FX_FREQSHIFT: m = std::make_unique<FreqShiftFx>(); break;
        case FX_RINGMOD:   m = std::make_unique<RingModFx>(); break;
        case FX_PITCH:     m = std::make_unique<PitchFx>(); break;
        case FX_TAPE:      m = std::make_unique<TapeFx>(); break;
        case FX_STEREO:    m = std::make_unique<StereoFx>(); break;
        default: return nullptr;
    }
    m->type = type;
    m->prepare (sr);
    return m;
}
} // namespace wc
