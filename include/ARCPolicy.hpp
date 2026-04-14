#pragma once
#include "ReplacementPolicy.hpp"
#include <list>
#include <unordered_map>
#include <algorithm>
#include <cmath>

// ─── ARC (Adaptive Replacement Cache) ────────────────────────────────────────
// T1: páginas vistas una vez (recientes)
// T2: páginas vistas más de una vez (frecuentes)
// B1: fantasmas de T1 (historial de evictions de recientes)
// B2: fantasmas de T2 (historial de evictions de frecuentes)
// p : tamaño objetivo de T1 (se adapta según ghost hits)
class ARCPolicy final : public ReplacementPolicy {
    using List = std::list<PageID>;
    using Map  = std::unordered_map<PageID, List::iterator>;

    size_t capacity_;
    int    p_ = 0;

    List T1_, T2_, B1_, B2_;
    Map  inT1_, inT2_, inB1_, inB2_;

    bool inList(const Map& m, PageID pid) const { return m.count(pid) > 0; }

    void remove(List& lst, Map& mp, PageID pid) {
        if (auto it = mp.find(pid); it != mp.end()) {
            lst.erase(it->second);
            mp.erase(it);
        }
    }

    void pushMRU(List& lst, Map& mp, PageID pid) {
        lst.push_front(pid);
        mp[pid] = lst.begin();
    }

    PageID popLRU(List& lst, Map& mp) {
        PageID pid = lst.back();
        lst.pop_back();
        mp.erase(pid);
        return pid;
    }

    size_t totalReal() const { return T1_.size() + T2_.size(); }

    // Elige qué página sacar para hacer espacio:
    // Si T1 > p → saca LRU de T1 hacia B1
    // Sino       → saca LRU de T2 hacia B2
    void replace(bool inB2hit) {
        bool t1TooLarge = T1_.size() > static_cast<size_t>(p_);
        bool t1ExactAndB2 = (T1_.size() == static_cast<size_t>(p_)) && inB2hit;

        if (!T1_.empty() && (t1TooLarge || t1ExactAndB2)) {
            PageID d = popLRU(T1_, inT1_);
            pushMRU(B1_, inB1_, d);
        } else if (!T2_.empty()) {
            PageID d = popLRU(T2_, inT2_);
            pushMRU(B2_, inB2_, d);
        } else if (!T1_.empty()) {
            // fallback: T2 vacío, sacar de T1
            PageID d = popLRU(T1_, inT1_);
            pushMRU(B1_, inB1_, d);
        }
    }

public:
    explicit ARCPolicy(size_t capacity) : capacity_(capacity) {}

    void onAccess(PageID pid) override {
        if (inList(inT1_, pid)) {
            // T1 hit → promover a T2 (ahora es frecuente)
            remove(T1_, inT1_, pid);
            pushMRU(T2_, inT2_, pid);
            // Aumentar p: esta página reciente fue usada de nuevo,
            // conviene dar más espacio a recientes
            p_ = static_cast<int>(std::min((size_t)(p_ + 1), capacity_));
            return;
        }
        if (inList(inT2_, pid)) {
            // T2 hit → mover al MRU de T2
            remove(T2_, inT2_, pid);
            pushMRU(T2_, inT2_, pid);
            return;
        }
    }

    void onLoad(PageID pid) override {
        // Si ya está en memoria, tratar como acceso
        if (inList(inT1_, pid) || inList(inT2_, pid)) {
            onAccess(pid);
            return;
        }

        bool b1Hit = inList(inB1_, pid);
        bool b2Hit = inList(inB2_, pid);

        if (b1Hit) {
            // Ghost hit en B1: esta página reciente fue eviccionada y volvió
            // → aumentar p (dar más espacio a recientes)
            int delta = (B1_.size() >= B2_.size()) ? 1
            : static_cast<int>(std::ceil(
                (double)B2_.size() / std::max((size_t)1, B1_.size())));
            p_ = static_cast<int>(std::min((size_t)(p_ + delta), capacity_));
            remove(B1_, inB1_, pid);
            if (totalReal() >= capacity_) replace(false);
            pushMRU(T2_, inT2_, pid);
            return;
        }

        if (b2Hit) {
            // Ghost hit en B2: esta página frecuente fue eviccionada y volvió
            // → reducir p (dar más espacio a frecuentes / T2)
            int delta = (B2_.size() >= B1_.size()) ? 1
            : static_cast<int>(std::ceil(
                (double)B1_.size() / std::max((size_t)1, B2_.size())));
            p_ = std::max(p_ - delta, 0);
            remove(B2_, inB2_, pid);
            if (totalReal() >= capacity_) replace(true);
            pushMRU(T2_, inT2_, pid);
            return;
        }

        // Página completamente nueva → entra a T1
        if (totalReal() >= capacity_) {
            size_t total = T1_.size() + B1_.size() + T2_.size() + B2_.size();
            if (T1_.size() < capacity_ && total < 2 * capacity_) {
                if (B1_.size() > 0) popLRU(B1_, inB1_);
            } else if (total >= 2 * capacity_ && !B2_.empty()) {
                popLRU(B2_, inB2_);
            }
            replace(false);
        }
        pushMRU(T1_, inT1_, pid);
    }

    void onEvict(PageID pid) override {
        remove(T1_, inT1_, pid);
        remove(T2_, inT2_, pid);
        remove(B1_, inB1_, pid);
        remove(B2_, inB2_, pid);
    }

    std::optional<PageID> victim() const override {
        if (!T1_.empty()) return T1_.back();
        if (!T2_.empty()) return T2_.back();
        return std::nullopt;
    }

    std::string name() const override { return "ARC"; }

    std::vector<PageID> order() const override {
        std::vector<PageID> out;
        for (auto p : T1_) out.push_back(p);
        for (auto p : T2_) out.push_back(p);
        return out;
    }

    size_t t1Size()  const { return T1_.size(); }
    size_t t2Size()  const { return T2_.size(); }
    size_t b1Size()  const { return B1_.size(); }
    size_t b2Size()  const { return B2_.size(); }
    int    pTarget() const { return p_; }
};
