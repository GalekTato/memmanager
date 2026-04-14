#include "TUI.hpp"
#include "LRUPolicy.hpp"
#include "ClockPolicy.hpp"
#include "ARCPolicy.hpp"
#include <sstream>
#include <cctype>
#include <thread>

// ─── TUI::processCommand ──────────────────────────────────────────────────────
void TUI::processCommand(const std::string& raw) {
    std::istringstream ss(raw);
    std::string cmd;
    ss >> cmd;
    for (auto& c : cmd) c = std::tolower(c);

    if (cmd == "q" || cmd == "quit") {
        running_ = false;
        return;
    }

    if (cmd == "r" || cmd == "reset") {
        mm_.resetStats();
        addLog(std::string(ansi::FG_BRIGHT_YELLOW) + "Stats reset." + ansi::RESET);
        statusMsg_ = "Stats reset.";
        return;
    }

    if (cmd == "p") {
        std::string pol;
        ss >> pol;
        for (auto& c : pol) c = std::tolower(c);
        if (pol == "lru") {
            mm_.setPolicy(std::make_unique<LRUPolicy>());
            addLog(std::string(ansi::FG_BRIGHT_MAGENTA) + "Policy → LRU" + ansi::RESET);
            statusMsg_ = "Policy changed to LRU.";
        } else if (pol == "arc") {
            mm_.setPolicy(std::make_unique<ARCPolicy>(mm_.capacity()));
            addLog(std::string(ansi::FG_BRIGHT_MAGENTA) + "Policy → ARC" + ansi::RESET);
            statusMsg_ = "Policy changed to ARC.";
        } else if (pol == "clock") {
            mm_.setPolicy(std::make_unique<ClockPolicy>());
            addLog(std::string(ansi::FG_BRIGHT_MAGENTA) + "Policy → Clock" + ansi::RESET);
            statusMsg_ = "Policy changed to Clock.";
        } else {
            statusMsg_ = "Unknown policy. Use: lru | arc | clock";
        }
        return;
    }

    // Commands that take a PageID
    uint32_t pid = 0;
    bool hasPid = static_cast<bool>(ss >> pid);

    if (cmd == "a" || cmd == "access") {
        if (!hasPid) { statusMsg_ = "Usage: a <page_id>"; return; }
        auto* p = mm_.access(pid);
        if (p) {
            addLog(std::string(ansi::FG_BRIGHT_GREEN) + "ACCESS P" + std::to_string(pid) +
                   " → frame hit/loaded" + ansi::RESET);
            statusMsg_ = "Accessed page " + std::to_string(pid);
        } else {
            statusMsg_ = "Failed to access page " + std::to_string(pid);
        }
        return;
    }

    if (cmd == "d" || cmd == "dirty") {
        if (!hasPid) { statusMsg_ = "Usage: d <page_id>"; return; }
        bool ok = mm_.markDirty(pid);
        addLog(ok ? (std::string(ansi::FG_YELLOW) + "DIRTY  P" + std::to_string(pid) + ansi::RESET)
                  : (std::string(ansi::FG_RED)    + "P" + std::to_string(pid) + " not in pool" + ansi::RESET));
        statusMsg_ = ok ? "Page " + std::to_string(pid) + " marked dirty."
                        : "Page not in pool.";
        return;
    }

    if (cmd == "e" || cmd == "evict") {
        if (!hasPid) { statusMsg_ = "Usage: e <page_id>"; return; }
        bool ok = mm_.evict(pid);
        addLog(ok ? (std::string(ansi::FG_BRIGHT_RED) + "EVICT  P" + std::to_string(pid) + ansi::RESET)
                  : (std::string(ansi::FG_RED)        + "P" + std::to_string(pid) + " not in pool" + ansi::RESET));
        statusMsg_ = ok ? "Page " + std::to_string(pid) + " evicted."
                        : "Page not in pool.";
        return;
    }

    if (cmd == "f" || cmd == "flush") {
        if (!hasPid) { statusMsg_ = "Usage: f <page_id>"; return; }
        bool ok = mm_.flush(pid);
        addLog(ok ? (std::string(ansi::FG_BRIGHT_CYAN) + "FLUSH  P" + std::to_string(pid) + ansi::RESET)
                  : (std::string(ansi::FG_RED)         + "P" + std::to_string(pid) + " not in pool" + ansi::RESET));
        statusMsg_ = ok ? "Page " + std::to_string(pid) + " flushed."
                        : "Page not in pool.";
        return;
    }

    statusMsg_ = "Unknown command: " + raw;
}

// ─── TUI::run ─────────────────────────────────────────────────────────────────
void TUI::run() {
    RawTerminal rt;
    ansi::hideCursor();
    ansi::clear();

    // Seed a few demo pages
    for (PageID i = 1; i <= 5; ++i) mm_.access(i);

    addLog(std::string(ansi::FG_BRIGHT_CYAN) + "Welcome! Type commands below." + ansi::RESET);

    while (running_) {
        ansi::clear();
        drawHeader();
        drawStats();
        drawFrames();
        drawLog();
        drawInput();
        std::cout << std::flush;

        // Non-blocking key read (~100 ms)
        int ch = rt.getChar();
        if (ch == -1) { ++ticker_; std::this_thread::sleep_for(std::chrono::milliseconds(80)); continue; }
        ++ticker_;

        if (ch == '\n' || ch == '\r') {
            if (!inputBuf_.empty()) {
                processCommand(inputBuf_);
                inputBuf_.clear();
            }
        } else if (ch == 127 || ch == 8) {  // backspace
            if (!inputBuf_.empty()) inputBuf_.pop_back();
            statusMsg_.clear();
        } else if (ch >= 32 && ch < 127) {
            inputBuf_ += static_cast<char>(ch);
            statusMsg_.clear();
        }
    }

    ansi::showCursor();
    ansi::clear();
    std::cout << ansi::BOLD << ansi::FG_BRIGHT_CYAN
              << "\n  Goodbye from MemManager!\n\n" << ansi::RESET;
}
