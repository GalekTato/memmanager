#pragma once
#include "MemoryManager.hpp"
#include "ARCPolicy.hpp"
#include "ClockPolicy.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdio>
#include <termios.h>
#include <unistd.h>

// ─── ANSI helpers ─────────────────────────────────────────────────────────────
namespace ansi {
    inline const char* RESET  = "\033[0m";
    inline const char* BOLD   = "\033[1m";
    inline const char* DIM    = "\033[2m";
    inline const char* ITALIC = "\033[3m";

    inline const char* FG_RED     = "\033[31m";
    inline const char* FG_GREEN   = "\033[32m";
    inline const char* FG_YELLOW  = "\033[33m";
    inline const char* FG_MAGENTA = "\033[35m";
    inline const char* FG_CYAN    = "\033[36m";
    inline const char* FG_BRIGHT_RED     = "\033[91m";
    inline const char* FG_BRIGHT_BLACK   = "\033[90m";
    inline const char* FG_BRIGHT_WHITE   = "\033[97m";
    inline const char* FG_BRIGHT_CYAN    = "\033[96m";
    inline const char* FG_BRIGHT_GREEN   = "\033[92m";
    inline const char* FG_BRIGHT_YELLOW  = "\033[93m";
    inline const char* FG_BRIGHT_MAGENTA = "\033[95m";

    inline const char* BG_BRIGHT_BLACK = "\033[100m";

    inline void clear()      { std::cout << "\033[2J\033[H"; }
    inline void hideCursor() { std::cout << "\033[?25l"; }
    inline void showCursor() { std::cout << "\033[?25h"; }
}

// ─── RawTerminal ──────────────────────────────────────────────────────────────
class RawTerminal {
    termios old_;
public:
    RawTerminal() {
        tcgetattr(STDIN_FILENO, &old_);
        termios raw = old_;
        raw.c_lflag &= ~(ECHO | ICANON);
        raw.c_cc[VMIN]  = 0;
        raw.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }
    ~RawTerminal() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_); }

    int getChar() {
        char c = 0;
        if (read(STDIN_FILENO, &c, 1) <= 0) return -1;
        return static_cast<unsigned char>(c);
    }
};

// ─── TUI ──────────────────────────────────────────────────────────────────────
class TUI {
    MemoryManager& mm_;
    std::vector<std::string> log_;
    static constexpr size_t  MAX_LOG = 14;
    std::string inputBuf_;
    std::string statusMsg_;
    bool        running_ = true;
    int         ticker_  = 0;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void addLog(const std::string& msg) {
        log_.push_back(msg);
        if (log_.size() > MAX_LOG) log_.erase(log_.begin());
    }

    static std::string hbar(double pct, int width = 28) {
        int filled = std::min((int)(pct / 100.0 * width), width);
        const char* color = pct >= 80 ? ansi::FG_BRIGHT_GREEN
                          : pct >= 40 ? ansi::FG_BRIGHT_YELLOW
                          :             ansi::FG_BRIGHT_CYAN;
        std::string s = std::string(ansi::BOLD) + color;
        for (int i = 0; i < filled; ++i) s += "█";
        s += ansi::DIM;
        for (int i = filled; i < width; ++i) s += "░";
        s += ansi::RESET;
        return s;
    }

    static std::string padRight(const std::string& s, int w) {
        if ((int)s.size() >= w) return s.substr(0, w);
        return s + std::string(w - s.size(), ' ');
    }

    // ── Header ────────────────────────────────────────────────────────────────
    void drawHeader() {
        const char* spin[] = {"◐","◓","◑","◒"};
        std::cout << ansi::BOLD << ansi::FG_BRIGHT_CYAN
                  << "╔══════════════════════════════════════════════════════════════════════════╗\n"
                  << "║  " << spin[ticker_ % 4] << "  "
                  << ansi::FG_BRIGHT_WHITE  << "MEM MANAGER"
                  << ansi::FG_BRIGHT_CYAN   << " ▸ "
                  << ansi::FG_BRIGHT_YELLOW << "C++ Buffer Pool  "
                  << ansi::FG_BRIGHT_CYAN   << "│ Policy: "
                  << ansi::FG_BRIGHT_MAGENTA << padRight(mm_.policyName(), 8)
                  << ansi::FG_BRIGHT_CYAN   << "│ Frames: " << mm_.capacity()
                  << "                      ║\n"
                  << "╚══════════════════════════════════════════════════════════════════════════╝"
                  << ansi::RESET << "\n";
    }

    // ── Statistics ────────────────────────────────────────────────────────────
    void drawStats() {
        auto& s  = mm_.stats();
        auto  hr = s.hitRate();
        double occ = mm_.capacity() ? (double)mm_.used() / mm_.capacity() * 100 : 0;

        std::cout << "\n" << ansi::BOLD << ansi::FG_BRIGHT_CYAN
                  << " ┌─ Statistics "
                  << ansi::RESET << ansi::DIM << ansi::FG_BRIGHT_BLACK
                  << "──────────────────────────────────────────────────────\n"
                  << ansi::RESET;

        std::cout << "   Hit Rate   " << hbar(hr) << "  "
                  << ansi::BOLD << ansi::FG_BRIGHT_GREEN
                  << std::fixed << std::setprecision(1) << hr << "%"
                  << ansi::RESET << "\n";

        std::cout << "   Occupancy  " << hbar(occ) << "  "
                  << ansi::FG_BRIGHT_CYAN << mm_.used() << "/" << mm_.capacity()
                  << ansi::RESET << "\n\n";

        std::cout << "   "
                  << ansi::FG_BRIGHT_GREEN  << "✓ Hits: "  << ansi::BOLD << s.hits.load()
                  << ansi::RESET << "   "
                  << ansi::FG_BRIGHT_RED    << "✗ Miss: "  << ansi::BOLD << s.misses.load()
                  << ansi::RESET << "   "
                  << ansi::FG_BRIGHT_YELLOW << "⇡ Evict: " << ansi::BOLD << s.evictions.load()
                  << ansi::RESET << "   "
                  << ansi::FG_MAGENTA       << "✎ DirtyW: "<< ansi::BOLD << s.dirtyW.load()
                  << ansi::RESET << "\n";

        // ── Panel ARC ─────────────────────────────────────────────────────────
        if (mm_.policyName() == "ARC") {
            auto* arc = dynamic_cast<ARCPolicy*>(mm_.policy());
            if (arc) {
                std::cout << "\n   "
                          << ansi::BOLD << ansi::FG_BRIGHT_CYAN << "ARC" << ansi::RESET
                          << ansi::DIM  << " → "
                          << ansi::FG_BRIGHT_GREEN  << "T1(recientes):" << arc->t1Size()
                          << "  "
                          << ansi::FG_BRIGHT_CYAN   << "T2(frecuentes):" << arc->t2Size()
                          << "  "
                          << ansi::FG_BRIGHT_BLACK  << "B1(ghost):" << arc->b1Size()
                          << "  B2(ghost):" << arc->b2Size()
                          << "  p=" << arc->pTarget()
                          << ansi::RESET << "\n";
            }
        }

        // ── Panel Clock ───────────────────────────────────────────────────────
        if (mm_.policyName() == "Clock") {
            auto* clk = dynamic_cast<ClockPolicy*>(mm_.policy());
            if (clk) {
                size_t refsNext = clk->refsUntilNextCycle();
                // Barra de progreso hacia el próximo ciclo (de 3)
                std::string prog = "";
                for (size_t i = 0; i < 3; ++i) {
                    if (i < (3 - refsNext))
                        prog += std::string(ansi::FG_BRIGHT_YELLOW) + "●" + ansi::RESET;
                    else
                        prog += std::string(ansi::DIM) + std::string(ansi::FG_BRIGHT_BLACK) + "○" + ansi::RESET;
                }

                std::cout << "\n   "
                          << ansi::BOLD << ansi::FG_BRIGHT_YELLOW << "Clock" << ansi::RESET
                          << ansi::DIM  << " → "
                          << ansi::RESET
                          << ansi::FG_BRIGHT_CYAN   << "Ciclos: " << ansi::BOLD << clk->cycles()
                          << ansi::RESET << "   "
                          << ansi::FG_BRIGHT_WHITE  << "Refs totales: " << ansi::BOLD << clk->refs()
                          << ansi::RESET << "   "
                          << ansi::FG_BRIGHT_YELLOW << "Hand→ P";

                // Muestra a qué página apunta el puntero actualmente
                const auto& ring = clk->ring();
                if (!ring.empty()) {
                    size_t h = clk->hand() % ring.size();
                    std::cout << ring[h].pid;
                } else {
                    std::cout << "-";
                }
                std::cout << ansi::RESET << "\n";

                std::cout << "   "
                          << ansi::FG_BRIGHT_BLACK << "Próximo ciclo en: "
                          << ansi::RESET << prog << " "
                          << ansi::DIM << "(" << refsNext << " ref"
                          << (refsNext == 1 ? "" : "s") << " restante"
                          << (refsNext == 1 ? "" : "s") << ")"
                          << ansi::RESET << "\n";
            }
        }
    }

    // ── Frame Pool ────────────────────────────────────────────────────────────
    void drawFrames() {
        auto snap = mm_.snapshot();
        auto ord  = mm_.policyOrder();

        std::unordered_map<PageID, size_t> rank;
        for (size_t i = 0; i < ord.size(); ++i) rank[ord[i]] = i;

        // Para Clock: obtener bit R real de la política
        ClockPolicy* clk = nullptr;
        if (mm_.policyName() == "Clock")
            clk = dynamic_cast<ClockPolicy*>(mm_.policy());

        std::cout << "\n" << ansi::BOLD << ansi::FG_BRIGHT_CYAN
                  << " ┌─ Frame Pool "
                  << ansi::RESET << ansi::DIM << ansi::FG_BRIGHT_BLACK
                  << "───────────────────────────────────────────────────────\n"
                  << ansi::RESET;

        // Cabecera de columnas
        std::cout << "   " << ansi::DIM << ansi::FG_BRIGHT_BLACK
                  << " Frame  Página  Dirty  BitR  Freq  Rango"
                  << ansi::RESET << "\n";

        if (mm_.free()) {
            std::cout << "   " << ansi::DIM << ansi::FG_BRIGHT_BLACK
                      << "[ " << mm_.free() << " frame(s) vacío(s) ]"
                      << ansi::RESET << "\n";
        }

        int shown = 0;
        for (auto& [fid, pg] : snap) {
            if (shown >= 12) {
                std::cout << "   " << ansi::DIM << "…(+" << snap.size()-12 << " más)\n" << ansi::RESET;
                break;
            }
            size_t rnk    = rank.count(pg.id) ? rank[pg.id] : 99;
            bool   isHand = false;

            // ¿Es la víctima actual del Clock?
            bool rBit = pg.referenced;
            if (clk) {
                rBit = clk->getRbit(pg.id);
                const auto& ring = clk->ring();
                if (!ring.empty())
                    isHand = (ring[clk->hand() % ring.size()].pid == pg.id);
            }

            // Color de la página
            const char* pColor = pg.dirty ? ansi::FG_BRIGHT_YELLOW : ansi::FG_BRIGHT_WHITE;

            // Prefijo: puntero del reloj
            std::cout << "   ";
            if (isHand)
                std::cout << ansi::FG_BRIGHT_YELLOW << ansi::BOLD << "►" << ansi::RESET << " ";
            else
                std::cout << "  ";

            std::cout << ansi::FG_BRIGHT_BLACK << "[F" << std::setw(2) << fid << "] "
                      << ansi::RESET << pColor << ansi::BOLD << "P" << std::setw(3) << pg.id
                      << ansi::RESET;

            // Dirty
            if (pg.dirty) std::cout << ansi::FG_BRIGHT_YELLOW << "  ✎ " << ansi::RESET;
            else          std::cout << "    ";

            // Bit R — siempre visible con label
            if (rBit)
                std::cout << ansi::BOLD << ansi::FG_BRIGHT_GREEN << " R=1●" << ansi::RESET;
            else
                std::cout << ansi::DIM  << ansi::FG_BRIGHT_BLACK << " R=0○" << ansi::RESET;

            std::cout << ansi::DIM
                      << "  freq:" << pg.frequency
                      << "  rank:" << rnk
                      << ansi::RESET << "\n";
            ++shown;
        }
    }

    // ── Event Log ─────────────────────────────────────────────────────────────
    void drawLog() {
        std::cout << "\n" << ansi::BOLD << ansi::FG_BRIGHT_CYAN
                  << " ┌─ Event Log "
                  << ansi::RESET << ansi::DIM << ansi::FG_BRIGHT_BLACK
                  << "────────────────────────────────────────────────────────\n"
                  << ansi::RESET;
        for (auto it = log_.rbegin(); it != log_.rend(); ++it)
            std::cout << "   " << *it << "\n";
    }

    // ── Command input ─────────────────────────────────────────────────────────
    void drawInput() {
        std::cout << "\n" << ansi::BOLD << ansi::FG_BRIGHT_CYAN
                  << " ┌─ Comando "
                  << ansi::RESET << ansi::DIM << ansi::FG_BRIGHT_BLACK
                  << "──────────────────────────────────────────────────────────\n"
                  << ansi::RESET;
        if (!statusMsg_.empty())
            std::cout << "   " << ansi::FG_BRIGHT_YELLOW << statusMsg_ << ansi::RESET << "\n";

        std::cout << "   " << ansi::BOLD << ansi::FG_BRIGHT_WHITE << "> " << ansi::RESET
                  << inputBuf_ << ansi::FG_BRIGHT_CYAN << "█" << ansi::RESET << "\n\n"
                  << ansi::DIM << ansi::FG_BRIGHT_BLACK
                  << "   [a <id>] acceder  [d <id>] dirty  [e <id>] evictar  "
                     "[f <id>] flush  [r] reset  [p lru|arc|clock]  [q] salir\n"
                  << ansi::RESET;
    }

    void processCommand(const std::string& cmd);

public:
    explicit TUI(MemoryManager& mm) : mm_(mm) {}
    void run();
};
