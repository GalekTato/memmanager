#pragma once
#include "ReplacementPolicy.hpp"
#include <list>
#include <unordered_map>

// ─── LRU ─────────────────────────────────────────────────────────────────────
// Keeps a doubly-linked list; most-recently-used at front, LRU victim at back.
class LRUPolicy final : public ReplacementPolicy {
    std::list<PageID>                             lru_list_;
    std::unordered_map<PageID,
        std::list<PageID>::iterator>              pos_;

    void moveToFront(PageID pid) {
        if (auto it = pos_.find(pid); it != pos_.end()) {
            lru_list_.erase(it->second);
        }
        lru_list_.push_front(pid);
        pos_[pid] = lru_list_.begin();
    }

public:
    void onAccess(PageID pid) override { moveToFront(pid); }
    void onLoad  (PageID pid) override { moveToFront(pid); }

    void onEvict(PageID pid) override {
        if (auto it = pos_.find(pid); it != pos_.end()) {
            lru_list_.erase(it->second);
            pos_.erase(it);
        }
    }

    std::optional<PageID> victim() const override {
        if (lru_list_.empty()) return std::nullopt;
        return lru_list_.back();
    }

    std::string name() const override { return "LRU"; }

    std::vector<PageID> order() const override {
        return { lru_list_.begin(), lru_list_.end() };
    }
};
