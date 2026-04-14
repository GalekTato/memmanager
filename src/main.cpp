#include "MemoryManager.hpp"
#include "LRUPolicy.hpp"
#include "ClockPolicy.hpp"
#include "ARCPolicy.hpp"
#include "TUI.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    // Defaults
    size_t      capacity  = 8;
    std::string policyStr = "arc";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--frames" && i + 1 < argc)
            capacity = std::stoul(argv[++i]);
        else if (arg == "--policy" && i + 1 < argc)
            policyStr = argv[++i];
        else if (arg == "--help") {
            std::cout << "Usage: memmanager [--frames N] [--policy lru|arc|clock]\n"
                         "  --frames N   Number of physical frames (default: 8)\n"
                         "  --policy X   Replacement policy: lru, arc, clock (default: arc)\n";
            return 0;
        }
    }

    // Build policy
    std::unique_ptr<ReplacementPolicy> policy;
    if      (policyStr == "lru"  ) policy = std::make_unique<LRUPolicy>();
    else if (policyStr == "clock") policy = std::make_unique<ClockPolicy>();
    else                           policy = std::make_unique<ARCPolicy>(capacity);

    // Build manager
    MemoryManager mm(capacity, std::move(policy));

    // Simulate page-in from "disk"
    mm.setPageInHook([](PageID pid) -> Page {
        // Simulate a small I/O delay (not actually blocking here)
        return Page(pid);
    });

    // Simulate page-out to "disk"
    mm.setPageOutHook([](const Page& pg) {
        // In a real system, write pg to disk/storage
        (void)pg;
    });

    // Launch TUI
    TUI tui(mm);
    tui.run();

    return 0;
}
