#pragma once
#include "MemoryManager.hpp"
#include <deque>
#include <vector>
#include <optional>

class Dispatcher {
public:
    struct FinalReport {
        int pid, arrivalTime, finishTime, waitingTime, pageFaults;
    };

    Dispatcher(MemoryManager& mm, int quantum = 5, int clockCycle = 3);
    void addProcess(Process p);         // add to ready queue at any time
    struct StepResult {
        bool running = false;
        int pid = -1;
        size_t ref = 0;
        int result = 0; // 1=hit, 2=fault, 0=error
    };

    StepResult step();                        // execute ONE reference of the current process; return false if nothing to run
    bool isDone() const;
    int currentTime() const;
    int currentQuantumUsed() const;
    int currentPid() const;             // -1 if idle
    std::vector<FinalReport> getReport() const; // only finished processes
    int totalPageFaults() const;
    int getQuantum() const { return quantum_; }
    int getClockCycle() const { return clockCycle_; }

private:
    MemoryManager& mm_;
    int quantum_;
    int clockCycle_;    // stored but exposed for future use
    int time_ = 0;
    int quantumUsed_ = 0;
    std::deque<Process> readyQueue_;
    std::optional<Process> running_;
    std::vector<FinalReport> finished_;
    void rotateProcess();
};