#pragma once
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include "Process.hpp"
#include "ReplacementPolicy.hpp"

class MemoryManager {
private:
    size_t virtualCapacity_;
    size_t freeVirtualPages_;
    
    size_t ramCapacity_; 
    size_t ramUsed_;

    std::vector<bool> bitmap_; // Ocupación de Memoria Virtual
    std::vector<bool> inRam_;  
    
    std::unordered_map<int, Process> processes_;
    std::unique_ptr<ReplacementPolicy> policy_;

public:
    explicit MemoryManager(size_t virtualCapacity, size_t ramCapacity, std::unique_ptr<ReplacementPolicy> policy)
        : virtualCapacity_(virtualCapacity), freeVirtualPages_(virtualCapacity), 
          ramCapacity_(ramCapacity), ramUsed_(0), policy_(std::move(policy)) {
        bitmap_.resize(virtualCapacity_, false); 
        inRam_.resize(virtualCapacity_, false);
    }

    bool admitProcess(int pid, size_t requiredPages) {
        if (processes_.find(pid) != processes_.end() || requiredPages > freeVirtualPages_) return false; 

        Process newProc(pid, requiredPages);
        size_t pagesAssigned = 0;
        
        // BÚSQUEDA Y DESPACHO (First-Fit en Memoria Virtual)
        for (size_t globalVirtualPage = 0; globalVirtualPage < virtualCapacity_ && pagesAssigned < requiredPages; ++globalVirtualPage) {
            if (!bitmap_[globalVirtualPage]) {
                bitmap_[globalVirtualPage] = true;
                newProc.mapPage(pagesAssigned, globalVirtualPage); 
                pagesAssigned++;
                freeVirtualPages_--;
            }
        }
        processes_.emplace(pid, newProc);
        return true; 
    }

    void terminateProcess(int pid) {
        auto it = processes_.find(pid);
        if (it != processes_.end()) {
            for (const auto& [localPage, globalVirtualPage] : it->second.getPageTable()) {
                bitmap_[globalVirtualPage] = false; // Liberar Virtual
                if (inRam_[globalVirtualPage]) {
                    inRam_[globalVirtualPage] = false; // Liberar RAM
                    ramUsed_--;
                    if (policy_) policy_->onEvict(globalVirtualPage);
                }
                freeVirtualPages_++;
            }
            processes_.erase(it);
        }
    }

    // Retorna: 1 (Hit en RAM), 2 (Page Fault / Traído de Swap), 0 (Segfault / Error)
    int accessProcessPage(int pid, size_t localPage) {
        auto it = processes_.find(pid);
        if (it == processes_.end()) return 0;

        size_t globalVirtualPage;
        if (it->second.getGlobalPage(localPage, globalVirtualPage)) {
            
            if (inRam_[globalVirtualPage]) {
                // HIT: Ya estaba en RAM
                if (policy_) policy_->onAccess(globalVirtualPage);
                return 1; 
            } else {
                // PAGE FAULT (Fallo de página): Traer del disco a la RAM
                if (ramUsed_ >= ramCapacity_) {
                    // ALGORTIMO DE REEMPLAZO: RAM LLENA, Buscar Víctima
                    if (policy_) {
                        auto vicOpt = policy_->victim();
                        if (vicOpt.has_value()) {
                            size_t victimPage = vicOpt.value();
                            inRam_[victimPage] = false; // Mandar víctima al Swap (Disco)
                            policy_->onEvict(victimPage);
                        }
                    }
                } else {
                    ramUsed_++; // Aún hay espacio en RAM
                }
                
                inRam_[globalVirtualPage] = true; // Cargar nueva a RAM
                if (policy_) policy_->onLoad(globalVirtualPage);
                return 2; 
            }
        }
        return 0; // Segfault
    }

    size_t getVirtualCapacity() const { return virtualCapacity_; }
    size_t getFreeVirtualPages() const { return freeVirtualPages_; }
    size_t getRamCapacity() const { return ramCapacity_; }
    size_t getRamUsed() const { return ramUsed_; }
    const std::vector<bool>& getBitmap() const { return bitmap_; }
    bool isPageInRam(size_t vPage) const { return inRam_[vPage]; }
    const std::unordered_map<int, Process>& getProcesses() const { return processes_; }
    ReplacementPolicy* policy() const { return policy_.get(); }
};