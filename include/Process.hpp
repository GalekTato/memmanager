#pragma once
#include <unordered_map>
#include <vector>
#include <cstddef>

class Process {
private:
    int pid_;
    size_t requiredPages_;
    std::unordered_map<size_t, size_t> pageTable_;

public:
    Process(int pid, size_t requiredPages) : pid_(pid), requiredPages_(requiredPages) {}

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
};