#pragma once
#include <cstdint>
#include <string>
#include <chrono>

using PageID  = uint32_t;
using FrameID = uint32_t;

// ─── Page ────────────────────────────────────────────────────────────────────
struct Page {
    PageID  id;
    bool    dirty      = false;
    bool    referenced = false;          // Clock bit
    uint32_t frequency = 0;             // LFU / ARC use
    std::chrono::steady_clock::time_point last_access{};

    explicit Page(PageID pid = 0) : id(pid) {
        last_access = std::chrono::steady_clock::now();
    }

    void touch() {
        referenced  = true;
        last_access = std::chrono::steady_clock::now();
        ++frequency;
    }

    std::string toString() const {
        return "P" + std::to_string(id);
    }
};

// ─── Frame ───────────────────────────────────────────────────────────────────
struct Frame {
    FrameID id;
    bool    occupied = false;
    Page    page{};

    explicit Frame(FrameID fid = 0) : id(fid) {}

    void load(const Page& p) {
        page     = p;
        occupied = true;
    }

    void evict() {
        occupied = false;
        page     = Page{};
    }
};
