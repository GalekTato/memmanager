#include "MemoryManager.hpp"
#include "HybridPolicy.hpp"
#include "TUI.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    size_t capacity = 8; // Frames por defecto

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--frames" && i + 1 < argc) {
            capacity = std::stoul(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Usage: memmanager [--frames N]\n"
            "  --frames N   Number of physical frames (default: 8)\n";
            return 0;
        }
    }

    // Instanciar motor híbrido único
    auto policy = std::make_unique<HybridPolicy>(capacity);
    MemoryManager mm(capacity, std::move(policy));

    mm.setPageInHook([](PageID pid) -> Page { return Page(pid); });
    mm.setPageOutHook([](const Page& pg) { (void)pg; });

    TUI tui(mm);
    tui.run();

    return 0;
}
