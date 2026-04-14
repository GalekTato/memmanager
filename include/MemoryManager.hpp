#pragma once
#include "Page.hpp"
#include "ReplacementPolicy.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <functional>
#include <atomic>
#include <mutex>

// ─── MemoryManager ────────────────────────────────────────────────────────────
// Buffer-pool style manager inspired by VMCache:
//  - Fixed pool of frames (physical memory)
//  - Pluggable replacement policy (LRU / Clock / ARC)
//  - Tracks hits, misses, evictions, dirty writes
//
// VMCache influence: version-latched frames (optimistic concurrency),
// eviction driven by policy, not simple FIFO.
class MemoryManager {
public:
    // ── Statistics ────────────────────────────────────────────────────────────
    struct Stats {
        std::atomic<uint64_t> hits     {0};
        std::atomic<uint64_t> misses   {0};
        std::atomic<uint64_t> evictions{0};
        std::atomic<uint64_t> dirtyW   {0};   // dirty-page write-backs

        double hitRate() const {
            auto total = hits + misses;
            return total ? (double)hits / total * 100.0 : 0.0;
        }

        void reset() { hits=0; misses=0; evictions=0; dirtyW=0; }
    };

private:
    size_t                                         capacity_;
    std::vector<Frame>                             frames_;
    std::unordered_map<PageID, FrameID>            pageTable_;   // page→frame
    std::unique_ptr<ReplacementPolicy>             policy_;
    Stats                                          stats_;
    mutable std::mutex                             mtx_;

    // Version latch per frame (VMCache-style optimistic concurrency hint)
    std::vector<std::atomic<uint32_t>>             versions_;

    // Optional "disk" read/write hooks (simulate I/O)
    std::function<Page(PageID)>                    onPageIn_;
    std::function<void(const Page&)>               onPageOut_;

    // ── Internal helpers ──────────────────────────────────────────────────────
    std::optional<FrameID> freeFrame() const {
        for (auto& f : frames_)
            if (!f.occupied) return f.id;
        return std::nullopt;
    }

    void evictOne() {
        auto vic = policy_->victim();
        if (!vic) return;

        auto it = pageTable_.find(*vic);
        if (it == pageTable_.end()) return;

        FrameID fid  = it->second;
        Frame&  frame = frames_[fid];

        if (frame.page.dirty) {
            if (onPageOut_) onPageOut_(frame.page);
            ++stats_.dirtyW;
        }

        policy_->onEvict(*vic);
        pageTable_.erase(it);
        versions_[fid].fetch_add(1, std::memory_order_relaxed);
        frame.evict();
        ++stats_.evictions;
    }

public:
    MemoryManager(size_t capacity,
                  std::unique_ptr<ReplacementPolicy> policy)
        : capacity_(capacity)
        , frames_(capacity)
        , policy_(std::move(policy))
        , versions_(capacity)
    {
        for (size_t i = 0; i < capacity; ++i) frames_[i].id = static_cast<FrameID>(i);
    }

    // ── Configuration ─────────────────────────────────────────────────────────
    void setPageInHook (std::function<Page(PageID)>       f) { onPageIn_  = f; }
    void setPageOutHook(std::function<void(const Page&)>  f) { onPageOut_ = f; }

    void setPolicy(std::unique_ptr<ReplacementPolicy> p) {
        std::lock_guard lk(mtx_);
        policy_ = std::move(p);
        // Re-register all pages currently in memory so the new policy
        // knows about them and can compute ranks/order correctly.
        for (auto& [pid, fid] : pageTable_) {
            policy_->onLoad(pid);
        }
    }

    // ── Core API ──────────────────────────────────────────────────────────────

    // Access a page: returns reference to the in-memory frame's page.
    // Returns nullptr if something went wrong.
    Page* access(PageID pid) {
        std::lock_guard lk(mtx_);

        if (auto it = pageTable_.find(pid); it != pageTable_.end()) {
            ++stats_.hits;
            Frame& f = frames_[it->second];
            f.page.touch();
            policy_->onAccess(pid);
            return &f.page;
        }

        // Miss: load page
        ++stats_.misses;
        if (!freeFrame()) evictOne();

        auto freeOpt = freeFrame();
        if (!freeOpt) return nullptr;  // should not happen

        FrameID fid = *freeOpt;
        Frame&  f   = frames_[fid];

        Page newPage = onPageIn_ ? onPageIn_(pid) : Page(pid);
        newPage.touch();
        f.load(newPage);
        pageTable_[pid] = fid;
        policy_->onLoad(pid);

        return &f.page;
    }

    // Mark a page as dirty (modified)
    bool markDirty(PageID pid) {
        std::lock_guard lk(mtx_);
        if (auto it = pageTable_.find(pid); it != pageTable_.end()) {
            frames_[it->second].page.dirty = true;
            return true;
        }
        return false;
    }

    // Flush a specific page to "disk"
    bool flush(PageID pid) {
        std::lock_guard lk(mtx_);
        if (auto it = pageTable_.find(pid); it != pageTable_.end()) {
            auto& pg = frames_[it->second].page;
            if (pg.dirty) {
                if (onPageOut_) onPageOut_(pg);
                pg.dirty = false;
                ++stats_.dirtyW;
            }
            return true;
        }
        return false;
    }

    // Force-evict a page
    bool evict(PageID pid) {
        std::lock_guard lk(mtx_);
        auto it = pageTable_.find(pid);
        if (it == pageTable_.end()) return false;
        FrameID fid = it->second;
        Frame& frame = frames_[fid];
        if (frame.page.dirty && onPageOut_) onPageOut_(frame.page);
        policy_->onEvict(pid);
        pageTable_.erase(it);
        versions_[fid].fetch_add(1, std::memory_order_relaxed);
        frame.evict();
        ++stats_.evictions;
        return true;
    }

    // ── Introspection ─────────────────────────────────────────────────────────
    const Stats&  stats()    const { return stats_; }
    void          resetStats()     { stats_.reset(); }
    size_t        capacity() const { return capacity_; }
    size_t        used()     const { return pageTable_.size(); }
    size_t        free()     const { return capacity_ - pageTable_.size(); }
    std::string policyName() const { return policy_->name(); }

    // Snapshot of frames for display
    std::vector<std::pair<FrameID,Page>> snapshot() const {
        std::lock_guard lk(mtx_);
        std::vector<std::pair<FrameID,Page>> out;
        for (auto& f : frames_)
            if (f.occupied) out.emplace_back(f.id, f.page);
        return out;
    }

    // Policy's internal order (for visualisation)
    std::vector<PageID> policyOrder() const {
        std::lock_guard lk(mtx_);
        return policy_->order();
    }

    ReplacementPolicy* policy() const { return policy_.get(); }
};
