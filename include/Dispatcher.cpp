#include "Dispatcher.hpp"
#include <numeric>

Dispatcher::Dispatcher(MemoryManager& mm, int quantum, int clockCycle)
    : mm_(mm), quantum_(quantum), clockCycle_(clockCycle) {}

void Dispatcher::addProcess(Process p) {
    readyQueue_.push_back(std::move(p));
}

Dispatcher::StepResult Dispatcher::step() {
    if (!running_) {
        if (readyQueue_.empty()) return {false, -1, 0, 0};
        running_ = std::move(readyQueue_.front());
        readyQueue_.pop_front();
        quantumUsed_ = 0;
    }

    size_t ref = running_->nextRef();
    int pid = running_->getPid();
    int res = mm_.accessProcessPage(pid, ref);
    if (res == 2) {
        running_->incrementPageFaults();
    }

    time_++;
    quantumUsed_++;

    StepResult stepRes = {true, pid, ref, res};

    if (quantumUsed_ >= quantum_ || !running_->hasMoreRefs()) {
        rotateProcess();
    }

    return stepRes;
}

void Dispatcher::rotateProcess() {
    if (!running_) return;

    if (!running_->hasMoreRefs()) {
        running_->setFinishTime(time_);
        finished_.push_back({
            running_->getPid(),
            running_->getArrivalTime(),
            running_->getFinishTime(),
            running_->getWaitingTime(),
            running_->getPageFaults()
        });
    } else {
        readyQueue_.push_back(std::move(*running_));
    }
    running_ = std::nullopt;
    quantumUsed_ = 0;
}

bool Dispatcher::isDone() const {
    return !running_ && readyQueue_.empty();
}

int Dispatcher::currentTime() const {
    return time_;
}

int Dispatcher::currentQuantumUsed() const {
    return quantumUsed_;
}

int Dispatcher::currentPid() const {
    return running_ ? running_->getPid() : -1;
}

std::vector<Dispatcher::FinalReport> Dispatcher::getReport() const {
    return finished_;
}

int Dispatcher::totalPageFaults() const {
    int total = 0;
    for (const auto& f : finished_) {
        total += f.pageFaults;
    }
    return total;
}