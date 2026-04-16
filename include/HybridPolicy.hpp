#pragma once
#include "ReplacementPolicy.hpp"
#include <deque>
#include <vector>
#include <optional>
#include <algorithm>
#include <string>

// ─── HybridPolicy (CAR + Optimistic Concurrency + 3-Ref Clock) ───────────────
class HybridPolicy final : public ReplacementPolicy {
public:
    struct Entry { PageID pid; bool R; };

private:
    std::deque<Entry> T1_; // Recientes
    std::deque<Entry> T2_; // Frecuentes
    std::deque<PageID> B1_; // Historial de recientes eviccionadas
    std::deque<PageID> B2_; // Historial de frecuentes eviccionadas

    size_t capacity_;
    size_t p_ = 0; // Parámetro adaptativo
    size_t refs_ = 0;
    size_t clockCycles_ = 0;

    static constexpr size_t REFS_PER_CYCLE = 3;

    // El ciclo de reloj exigido: apaga los bits R cada 3 referencias
    void runCycle() {
        if (T1_.empty() && T2_.empty()) return;
        ++clockCycles_;
        for (auto& e : T1_) e.R = false;
        for (auto& e : T2_) e.R = false;
    }

    void countRef() {
        ++refs_;
        if (refs_ % REFS_PER_CYCLE == 0) runCycle();
    }

public:
    explicit HybridPolicy(size_t capacity) : capacity_(capacity) {}

    void onAccess(PageID pid) override {
        // Lock-free "Hit": Solo busca la página y enciende su bit R
        auto it1 = std::find_if(T1_.begin(), T1_.end(), [pid](const Entry& e){ return e.pid == pid; });
        if (it1 != T1_.end()) {
            it1->R = true;
            countRef();
            return;
        }
        auto it2 = std::find_if(T2_.begin(), T2_.end(), [pid](const Entry& e){ return e.pid == pid; });
        if (it2 != T2_.end()) {
            it2->R = true;
            countRef();
            return;
        }
    }

    void onLoad(PageID pid) override {
        auto inB1 = std::find(B1_.begin(), B1_.end(), pid);
        auto inB2 = std::find(B2_.begin(), B2_.end(), pid);

        // Ajuste adaptativo inteligente si la página estaba en el historial fantasma
        if (inB1 != B1_.end()) {
            p_ = std::min(capacity_, p_ + std::max<size_t>(1, B2_.size() / B1_.size()));
            B1_.erase(inB1);
            T2_.push_back({pid, true});
        } else if (inB2 != B2_.end()) {
            p_ = (p_ > std::max<size_t>(1, B1_.size() / B2_.size())) ? p_ - std::max<size_t>(1, B1_.size() / B2_.size()) : 0;
            B2_.erase(inB2);
            T2_.push_back({pid, true});
        } else {
            T1_.push_back({pid, true});
        }
        countRef();
    }

    void onEvict(PageID pid) override {
        auto it1 = std::find_if(T1_.begin(), T1_.end(), [pid](const Entry& e){ return e.pid == pid; });
        if (it1 != T1_.end()) {
            T1_.erase(it1);
            B1_.push_back(pid);
            if (B1_.size() > capacity_) B1_.pop_front();
            return;
        }
        auto it2 = std::find_if(T2_.begin(), T2_.end(), [pid](const Entry& e){ return e.pid == pid; });
        if (it2 != T2_.end()) {
            T2_.erase(it2);
            B2_.push_back(pid);
            if (B2_.size() > capacity_) B2_.pop_front();
        }
    }

    std::optional<PageID> victim() const override {
        if (T1_.empty() && T2_.empty()) return std::nullopt;

        // Si T1 es más grande que el target, evicciona de T1 (busca R=0)
        if (!T1_.empty() && (T1_.size() > p_ || (T1_.size() == p_ && T2_.empty()))) {
            for (size_t i = 0; i < T1_.size(); ++i) {
                if (!T1_[i].R) return T1_[i].pid; // Encuentra la primera R=0
            }
            return T1_.front().pid; // Respaldo si todas tienen R=1
        } else if (!T2_.empty()) {
            for (size_t i = 0; i < T2_.size(); ++i) {
                if (!T2_[i].R) return T2_[i].pid;
            }
            return T2_.front().pid;
        }
        return T1_.empty() ? T2_.front().pid : T1_.front().pid;
    }

    std::string name() const override { return "Hybrid-CAR"; }

    std::vector<PageID> order() const override {
        std::vector<PageID> out;
        for (const auto& e : T1_) out.push_back(e.pid);
        for (const auto& e : T2_) out.push_back(e.pid);
        return out;
    }

    std::string getPageInfo(PageID pid) override {
        for (const auto& e : T1_) if (e.pid == pid) return "T1 (R=" + std::to_string(e.R) + ")";
        for (const auto& e : T2_) if (e.pid == pid) return "T2 (R=" + std::to_string(e.R) + ")";
        for (auto id : B1_) if (id == pid) return "Swap/B1 (Fantasma)";
        for (auto id : B2_) if (id == pid) return "Swap/B2 (Fantasma)";
        return "En Swap (Disco)";
    }

    // Funciones de introspección para la GUI
    size_t t1Size() const { return T1_.size(); }
    size_t t2Size() const { return T2_.size(); }
    size_t b1Size() const { return B1_.size(); }
    size_t b2Size() const { return B2_.size(); }
    size_t pTarget() const { return p_; }
    size_t cycles() const { return clockCycles_; }
    size_t refs() const { return refs_; }
    size_t refsUntilNextCycle() const {
        size_t rem = REFS_PER_CYCLE - (refs_ % REFS_PER_CYCLE);
        return rem == REFS_PER_CYCLE ? 0 : rem;
    }
    bool getRbit(PageID pid) const {
        for(const auto& e: T1_) if(e.pid == pid) return e.R;
        for(const auto& e: T2_) if(e.pid == pid) return e.R;
        return false;
    }
};
