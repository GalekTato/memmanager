#pragma once
#include "ReplacementPolicy.hpp"
#include <deque>
#include <unordered_map>
#include <optional>

// ─── ClockPolicy (Second Chance) ─────────────────────────────────────────────
//
// Regla: cada 3 referencias globales → 1 ciclo de reloj automático.
//
// Un ciclo completo recorre TODO el anillo UNA vez:
//   - Páginas con R=1: se limpia R→0 (segunda oportunidad)
//   - Primera página con R=0 que encuentre: queda como víctima (hand_ apunta ahí)
//
class ClockPolicy final : public ReplacementPolicy {
public:
    struct Entry { PageID pid; bool R; };

private:
    std::deque<Entry>                  ring_;
    std::unordered_map<PageID, size_t> idx_;
    size_t hand_        = 0;
    size_t refs_        = 0;
    size_t clockCycles_ = 0;

    static constexpr size_t REFS_PER_CYCLE = 3;

    void rebuildIdx() {
        idx_.clear();
        for (size_t i = 0; i < ring_.size(); ++i)
            idx_[ring_[i].pid] = i;
    }

    // Un ciclo de reloj: recorre el anillo completo una vez.
    // Limpia R=1 en todas las páginas que encuentra.
    // Al terminar, hand_ apunta a la primera página con R=0 original,
    // o avanza una posición si todas tuvieron segunda oportunidad.
    void runCycle() {
        if (ring_.empty()) return;
        ++clockCycles_;

        size_t n = ring_.size();
        std::optional<size_t> first_r0;

        for (size_t i = 0; i < n; ++i) {
            size_t pos = (hand_ + i) % n;
            if (ring_[pos].R) {
                ring_[pos].R = false;   // segunda oportunidad
            } else if (!first_r0) {
                first_r0 = pos;         // guarda la primera con R=0 original
            }
        }

        if (first_r0) {
            hand_ = *first_r0;
        } else {
            // Si TODAS las páginas tenían R=1 y fueron limpiadas,
            // forzamos al puntero a avanzar una posición.
            // Así evitamos que se estanque eternamente en P1.
            hand_ = (hand_ + 1) % n;
        }
    }

    void countRef() {
        ++refs_;
        if (refs_ % REFS_PER_CYCLE == 0) runCycle();
    }

public:
    void onAccess(PageID pid) override {
        if (auto it = idx_.find(pid); it != idx_.end())
            ring_[it->second].R = true;
        countRef();
    }

    void onLoad(PageID pid) override {
        ring_.push_back({pid, true});
        idx_[pid] = ring_.size() - 1;
        if (hand_ >= ring_.size()) hand_ = 0;
        countRef();
    }

    void onEvict(PageID pid) override {
        auto it = idx_.find(pid);
        if (it == idx_.end()) return;
        ring_.erase(ring_.begin() + static_cast<long>(it->second));
        if (!ring_.empty() && hand_ >= ring_.size()) hand_ = 0;
        rebuildIdx();
    }

    std::optional<PageID> victim() const override {
        if (ring_.empty()) return std::nullopt;
        // Busca la primera con R=0 desde hand_
        size_t n = ring_.size();
        for (size_t i = 0; i < n; ++i) {
            size_t pos = (hand_ + i) % n;
            if (!ring_[pos].R) return ring_[pos].pid;
        }
        // Todas tienen R=1: devuelve la apuntada por hand_
        return ring_[hand_ % n].pid;
    }

    std::string name() const override { return "Clock"; }

    std::vector<PageID> order() const override {
        std::vector<PageID> out;
        for (auto& e : ring_) out.push_back(e.pid);
        return out;
    }

    const std::deque<Entry>& ring()   const { return ring_; }
    size_t hand()                     const { return hand_; }
    size_t refs()                     const { return refs_; }
    size_t cycles()                   const { return clockCycles_; }
    size_t refsUntilNextCycle()       const {
        size_t rem = REFS_PER_CYCLE - (refs_ % REFS_PER_CYCLE);
        return rem == REFS_PER_CYCLE ? 0 : rem;
    }
    bool getRbit(PageID pid) const {
        auto it = idx_.find(pid);
        return it != idx_.end() && ring_[it->second].R;
    }
};
