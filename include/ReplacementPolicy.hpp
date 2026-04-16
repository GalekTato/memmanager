#pragma once
#include "Page.hpp"
#include <vector>
#include <optional>
#include <string>

// ─── ReplacementPolicy (abstract) ────────────────────────────────────────────
class ReplacementPolicy {
public:
    virtual ~ReplacementPolicy() = default;
    
    virtual void onAccess(PageID pid)  = 0;

    virtual void onLoad(PageID pid)    = 0;

    virtual void onEvict(PageID pid)   = 0;

    virtual std::optional<PageID> victim() const = 0;

    virtual std::string name() const = 0;

    virtual std::vector<PageID> order() const = 0;
    
    virtual std::string getPageInfo(PageID /*pid*/) { return "N/A"; }
};
