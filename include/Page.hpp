#pragma once
#include <cstdint>
#include <string>
#include <chrono>


using PageID = uint32_t;

struct Page {
    PageID  id;
    bool    dirty      = false;
    bool    referenced = false;          
    uint32_t frequency = 0;             
    std::chrono::steady_clock::time_point last_access{};

    explicit Page(PageID pid = 0) : id(pid) {
        last_access = std::chrono::steady_clock::now();
    }

    void touch() {
        referenced  = true;
        last_access = std::chrono::steady_clock::now();
        ++frequency;
    }

    std::string toString() const {
        return "P" + std::to_string(id);
    }
};
