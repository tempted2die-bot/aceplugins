// WRAITHCORE - 808 QUAKE voice
#pragma once
#include "Core.h"

namespace wc
{
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

} // namespace wc
