#pragma once
#include "Page.hpp"
#include <vector>
#include <optional>
#include <string>

// ─── ReplacementPolicy (abstract) ────────────────────────────────────────────
class ReplacementPolicy {
public:
    virtual ~ReplacementPolicy() = default;

    // Called when a page is accessed (hit or after load)
    virtual void onAccess(PageID pid)  = 0;

    // Called when a page is loaded into a frame
    virtual void onLoad(PageID pid)    = 0;

    // Called when a page is evicted
    virtual void onEvict(PageID pid)   = 0;

    // Returns the PageID that should be evicted next
    virtual std::optional<PageID> victim() const = 0;

    // Human-readable name
    virtual std::string name() const = 0;

    // Ordered list of page IDs currently tracked (for display)
    virtual std::vector<PageID> order() const = 0;
    
    virtual std::string getPageInfo(PageID /*pid*/) { return "N/A"; }
};
