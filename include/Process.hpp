#pragma once
#include <unordered_map>
#include <vector>
#include <cstddef>

class Process {
private:
    int pid_;
    size_t requiredPages_;
    std::unordered_map<size_t, size_t> pageTable_;
    std::vector<size_t> referenceString_;
    size_t currentRef_ = 0;
    int arrivalTime_ = 0;
    int startTime_ = -1;
    int finishTime_ = -1;
    int pageFaults_ = 0;

public:
    Process(int pid, size_t requiredPages) : pid_(pid), requiredPages_(requiredPages) {}

    Process(int pid, size_t requiredPages, std::vector<size_t> refs, int arrivalTime)
        : pid_(pid), requiredPages_(requiredPages), referenceString_(std::move(refs)), arrivalTime_(arrivalTime) {}

    int getPid() const { return pid_; }
    size_t getRequiredPages() const { return requiredPages_; }

    void mapPage(size_t localPage, size_t globalVirtualPage) {
        pageTable_[localPage] = globalVirtualPage;
    }

    bool getGlobalPage(size_t localPage, size_t& globalVirtualPage) const {
        auto it = pageTable_.find(localPage);
        if (it != pageTable_.end()) {
            globalVirtualPage = it->second;
            return true;
        }
        return false;
    }

    const std::unordered_map<size_t, size_t>& getPageTable() const {
        return pageTable_;
    }

    bool hasMoreRefs() const { return currentRef_ < referenceString_.size(); }
    size_t nextRef() { return referenceString_[currentRef_++]; }
    
    int getArrivalTime() const { return arrivalTime_; }
    int getFinishTime() const { return finishTime_; }
    void setFinishTime(int t) { finishTime_ = t; }
    int getPageFaults() const { return pageFaults_; }
    void incrementPageFaults() { pageFaults_++; }
    
    int getWaitingTime() const {
        return finishTime_ - arrivalTime_ - (int)referenceString_.size();
    }
};