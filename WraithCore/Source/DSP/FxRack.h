// WRAITHCORE - Snap-Heap-style FX rack: 2 lanes x 5 slots, serial / parallel / low-high split routing.
// Modules are created off the audio thread and handed over lock-free.
#pragma once
#include "FxModules.h"

namespace wc
{
class FxRack
{
public:
    FxRack() { for (auto& s : slots) { s.pending.store (nullptr); s.retired.store (nullptr); s.activeType.store (0); } }
    ~FxRack()
    {
        for (auto& s : slots) { delete s.pending.exchange (nullptr); delete s.retired.exchange (nullptr); }
    }

    // not concurrent with process()
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        for (auto& s : slots)
        {
            if (s.mod) s.mod->prepare (sr);
            if (auto* pm = s.pending.load()) pm->prepare (sr);
            s.onSm = 0.f; s.mixSm = 1.f;
        }
        for (auto& f : xo) f.reset();
    }

    // message thread (or offline): build modules the slots ask for
    void updateModules (const Params& p)
    {
        for (int i = 0; i < kSlots; ++i)
        {
            auto& s = slots[(size_t) i];
            delete s.retired.exchange (nullptr);
            const int desired = p.n (slotId (i, SF_TYPE));
            if (desired <= FX_NONE || desired >= FX_COUNT) continue;
            if (s.activeType.load() == desired) continue;
            if (auto* pend = s.pending.load()) { if (pend->type == desired) continue; delete s.pending.exchange (nullptr); }
            s.pending.store (createFx (desired, sr).release());
        }
    }

    // audio thread, once per host block
    void adoptPending()
    {
        for (auto& s : slots)
        {
            if (s.pending.load() == nullptr || s.retired.load() != nullptr) continue;
            if (auto* m = s.pending.exchange (nullptr))
            {
                s.retired.store (s.mod.release());
                s.mod.reset (m);
                s.activeType.store (m->type);
                s.onSm = 0.f; // fade the new module in
            }
        }
    }

    int activeType (int slot) const { return slots[(size_t) slot].activeType.load(); }

    void process (float* L, float* R, int n, const float (*slotP)[4], const float* slotMix, const Params& p, const FxCtx& ctx)
    {
        n = std::min (n, kMaxSub);
        const int routing = p.n (rackId (R_ROUTING));
        const float lvlA = p.f (rackId (R_LANEA_LVL)), lvlB = p.f (rackId (R_LANEB_LVL)), rackMix = p.f (rackId (R_MIX));
        const bool onA = p.b (rackId (R_LANEA_ON)), onB = p.b (rackId (R_LANEB_ON));
        std::copy (L, L + n, dryL); std::copy (R, R + n, dryR);

        auto runLane = [&] (int lane, float* l, float* r) { for (int s = lane * kSlotsPerLane; s < (lane + 1) * kSlotsPerLane; ++s) processSlot (s, l, r, n, slotP[s], slotMix[s], p, ctx); };

        if (routing == 0) // SERIAL
        {
            if (onA) runLane (0, L, R);
            for (int i = 0; i < n; ++i) { L[i] *= lvlA; R[i] *= lvlA; }
            if (onB) runLane (1, L, R);
            for (int i = 0; i < n; ++i) { L[i] *= lvlB; R[i] *= lvlB; }
        }
        else
        {
            if (routing == 1) // PARALLEL: an OFF lane drops out of the sum
            {
                std::copy (L, L + n, aL); std::copy (R, R + n, aR); std::copy (L, L + n, bL); std::copy (R, R + n, bR);
            }
            else // SPLIT: lows -> A, highs -> B (LR4, sums flat)
            {
                xoSm += (p.f (rackId (R_XOVER)) - xoSm) * 0.2f;
                const float g = std::tan (kPi * clampf (xoSm, 20.f, sr * 0.4f) / sr), k = 1.41421f;
                for (int i = 0; i < n; ++i)
                {
                    aL[i] = xo[1].process (xo[0].process (L[i], g, k, 0), g, k, 0);
                    bL[i] = xo[3].process (xo[2].process (L[i], g, k, 2), g, k, 2);
                    aR[i] = xo[5].process (xo[4].process (R[i], g, k, 0), g, k, 0);
                    bR[i] = xo[7].process (xo[6].process (R[i], g, k, 2), g, k, 2);
                }
            }
            if (onA || routing == 2) { if (onA) runLane (0, aL, aR); }
            if (onB || routing == 2) { if (onB) runLane (1, bL, bR); }
            const float ga = (routing == 1 && ! onA) ? 0.f : lvlA, gb = (routing == 1 && ! onB) ? 0.f : lvlB;
            for (int i = 0; i < n; ++i) { L[i] = aL[i] * ga + bL[i] * gb; R[i] = aR[i] * ga + bR[i] * gb; }
        }

        for (int i = 0; i < n; ++i) { L[i] = dryL[i] + (L[i] - dryL[i]) * rackMix; R[i] = dryR[i] + (R[i] - dryR[i]) * rackMix; }
    }

    static constexpr int kMaxSub = 64;

private:
    struct Slot
    {
        std::unique_ptr<FxModule> mod;
        std::atomic<FxModule*> pending { nullptr };
        std::atomic<FxModule*> retired { nullptr };
        std::atomic<int> activeType { 0 };
        float onSm = 0.f, mixSm = 1.f;
    };
    std::array<Slot, kSlots> slots;
    float sr = 48000.f, xoSm = 180.f;
    SVF xo[8];
    float dryL[kMaxSub] {}, dryR[kMaxSub] {}, aL[kMaxSub] {}, aR[kMaxSub] {}, bL[kMaxSub] {}, bR[kMaxSub] {}, tL[kMaxSub] {}, tR[kMaxSub] {};

    void processSlot (int si, float* l, float* r, int n, const float* pp, float mix, const Params& p, const FxCtx& ctx)
    {
        auto& s = slots[(size_t) si];
        const int type = p.n (slotId (si, SF_TYPE));
        const bool ready = s.mod && s.mod->type == type && type != FX_NONE;
        const float onT = (ready && p.b (slotId (si, SF_ON))) ? 1.f : 0.f;
        if (! ready) { s.onSm = 0.f; return; }
        if (onT == 0.f && s.onSm < 1e-4f) { s.onSm = 0.f; return; }
        std::copy (l, l + n, tL); std::copy (r, r + n, tR);
        s.mod->process (tL, tR, n, pp, ctx);
        const float c = 0.02f;
        for (int i = 0; i < n; ++i)
        {
            s.onSm += (onT - s.onSm) * c;
            s.mixSm += (mix - s.mixSm) * c;
            const float g = s.onSm * s.mixSm;
            float wl = tL[i], wr = tR[i];
            if (! std::isfinite (wl)) wl = 0.f;
            if (! std::isfinite (wr)) wr = 0.f;
            l[i] += (wl - l[i]) * g;
            r[i] += (wr - r[i]) * g;
        }
    }
};
} // namespace wc
