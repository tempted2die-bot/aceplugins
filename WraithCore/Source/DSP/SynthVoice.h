// WRAITHCORE - sonar synth voice
//   OSC A (morph sine>tri>saw>square, fold, unison) + OSC B (ADD / RING / SYNC) --PM--< FM operator
//   pitch <- note + glide + PING envelope + BUBBLER scatter + voice LFO
//   -> drive -> filter (LP12 / LP24 / BAND / HIGH / NOTCH / COMB / ACID, env + LFO + key track) -> amp x bubble gate
#pragma once
#include "Core.h"

namespace wc
{
class Voice
{
public:
    int note = -1;
    float vel = 1.f;
    uint64_t age = 0;
    bool gateHeld = false;
    bool pendingRelease = false;

    explicit Voice (uint32_t seed = 1) : rng (seed) {}

    void prepare (float sr)
    {
        const int sz = (int) (sr / 15.f) + 8;
        combL.prepare (sz); combR.prepare (sz);
    }

    bool active() const { return amp.active(); }

    void start (int n, float velocity, float glideFrom, bool retriggerAmp, bool glideOn)
    {
        const bool wasActive = active();
        note = n; vel = velocity; gateHeld = true; pendingRelease = false;
        target = (float) n;
        if (! wasActive || ! glideOn) cur = glideOn && glideFrom >= 0.f ? glideFrom : (float) n;

        ping = 1.f; bubEnv = 1.f; bubOffset = 0.f; bubCount = 0.f;
        fenv.trigger();

        if (! wasActive)
        {
            for (auto& s : svf) s.reset();
            combL.clear(); combR.clear(); combLp[0] = combLp[1] = 0.f;
            cutSm = -1.f;
            for (size_t u = 0; u < units.size(); ++u)
            {
                units[u].ph = (u == 0) ? 0.0f : rng.next();
                units[u].bph = (u == 0) ? 0.0f : rng.next();
                units[u].mph = 0.f;
            }
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
        const float aLvl      = p.f (pid::oscALvl);
        const float bLvl      = p.f (pid::obLvl);
        const float bWave     = p.f (pid::obWave);
        const int   bMode     = p.n (pid::obMode);
        const float bRatio    = semis ((float) p.sn (pid::obOct) * 12.f + (float) p.sn (pid::obSemi) + p.f (pid::obFine) * 0.01f);
        const float res       = p.f (pid::fltRes);
        const float k         = 2.f - 1.96f * res;
        const int   ftype     = p.n (pid::fltType);
        const float bpNorm    = std::pow (k, 0.75f);
        const float drive     = p.f (pid::fltDrive);
        const float drvG      = 1.f + drive * 8.f;
        const float drvComp   = 1.f / (1.f + drive * 2.f);
        const float keyTrack  = p.f (pid::fltKey);
        const float combFb    = 0.45f + res * 0.53f;
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
        const float combDamp  = onePoleCoef (6000.f, sr);
        if (cutSm < 0.f) cutSm = baseCut;

        for (int i = 0; i < n; ++i)
        {
            // ---- BUBBLER
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
                float mod = 0.f;
                if (fmIndex > 0.f)
                {
                    U.mph += dt * fmRatio; U.mph -= std::floor (U.mph);
                    mod = std::sin (kTwoPi * U.mph) * fmIndex;
                }
                float t = U.ph + mod; t -= std::floor (t);
                const float sA = morphShape (t, dt, wave);
                float s = sA * aLvl;
                if (bLvl > 0.0005f)
                {
                    const float dtB = std::min (dt * bRatio, 0.45f);
                    float tb = U.bph + mod; tb -= std::floor (tb);
                    const float sB = morphShape (tb, dtB, bWave);
                    s += (bMode == 1 ? sA * sB * 1.5f : sB) * bLvl;
                    U.bph += dtB; if (U.bph >= 1.f) U.bph -= std::floor (U.bph);
                }
                if (foldMix > 0.f) s += foldMix * (std::sin (s * foldGain) - s);
                U.ph += dt;
                if (U.ph >= 1.f) { U.ph -= 1.f; if (bMode == 2) U.bph = U.ph * bRatio - std::floor (U.ph * bRatio); } // hard sync
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
            float cut = cutSm * std::exp2 (fltEnv * fe * 7.f + lfo[i] * lfoCut * 4.f + keyTrack * (cur - 60.f) / 12.f);
            cut = clampf (cut, 16.f, nyq);
            if (drive > 0.001f) { sl = ftanh (sl * drvG) * drvComp * 1.6f; sr_ = ftanh (sr_ * drvG) * drvComp * 1.6f; }
            float yl, yr;
            if (ftype == 5) // COMB
            {
                const float d = clampf (sr / cut, 2.f, (float) combL.size - 3.f);
                combLp[0] += (combL.read (d) - combLp[0]) * combDamp;
                combLp[1] += (combR.read (d) - combLp[1]) * combDamp;
                yl = sl + combLp[0] * combFb; yr = sr_ + combLp[1] * combFb;
                combL.push (ftanh (yl)); combR.push (ftanh (yr));
                yl *= 0.5f; yr *= 0.5f;
            }
            else
            {
                const float g = std::tan (kPi * cut / sr);
                switch (ftype)
                {
                    case 0:  yl = svf[0].process (sl, g, k, 0); yr = svf[1].process (sr_, g, k, 0); break;
                    case 1:  yl = svf[2].process (svf[0].process (sl, g, k, 0), g, 1.4f, 0);
                             yr = svf[3].process (svf[1].process (sr_, g, k, 0), g, 1.4f, 0); break;
                    case 2:  yl = svf[0].process (sl, g, k, 1) * bpNorm; yr = svf[1].process (sr_, g, k, 1) * bpNorm; break;
                    case 3:  yl = svf[0].process (sl, g, k, 2); yr = svf[1].process (sr_, g, k, 2); break;
                    case 4:  yl = svf[0].process (sl, g, k, 3); yr = svf[1].process (sr_, g, k, 3); break;
                    default: // ACID: driven 4-pole with resonant first stage
                        yl = svf[2].process (ftanh (svf[0].process (ftanh (sl * 1.5f), g, k, 0) * 1.4f), g, 1.2f, 0);
                        yr = svf[3].process (ftanh (svf[1].process (ftanh (sr_ * 1.5f), g, k, 0) * 1.4f), g, 1.2f, 0);
                        break;
                }
            }

            gateSm += (bubEnv - gateSm) * gateC;
            const float a = amp.next() * level * (1.f - gateAmt + gateAmt * gateSm);
            L[i] += softLimit (yl * a);
            R[i] += softLimit (yr * a);
            if (! amp.active()) { note = -1; break; }
        }
    }

private:
    struct Unit { float ph = 0.f, bph = 0.f, mph = 0.f; };
    std::array<Unit, 4> units {};
    Rng rng;
    ADSR amp;
    ADEnv fenv;
    SVF svf[4];
    DelayLine combL, combR;
    float combLp[2] {};
    float cur = 60.f, target = 60.f;
    float ping = 0.f, bubEnv = 0.f, bubOffset = 0.f, bubCount = 0.f;
    float subPh = 0.f, cutSm = -1.f, gateSm = 0.f;
};
} // namespace wc
