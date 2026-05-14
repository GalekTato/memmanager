#include "Server.hpp"
#include "../include/HybridPolicy.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

Server::Server(int port, size_t vmCapacity, size_t ramCapacity, int quantum, int clockCycle, size_t threadPoolSize) 
    : port_(port), running_(true), 
      pool_(threadPoolSize == 0 ? std::thread::hardware_concurrency() : threadPoolSize),
      poolSize_(threadPoolSize == 0 ? std::thread::hardware_concurrency() : threadPoolSize) {
    auto pol = std::make_unique<HybridPolicy>(ramCapacity);
    mm_ = std::make_unique<MemoryManager>(vmCapacity, ramCapacity, std::move(pol));
    dispatcher_ = std::make_unique<Dispatcher>(*mm_, quantum, clockCycle);
}

Server::~Server() {
    running_ = false;
    if (serverSocket_ != -1) close(serverSocket_);
    if (acceptThread_.joinable()) acceptThread_.join();
    if (simThread_.joinable()) simThread_.join();
}

void Server::run() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ == -1) {
        perror("Socket creation failed");
        return;
    }

    int opt = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        return;
    }

    if (listen(serverSocket_, 5) < 0) {
        perror("Listen failed");
        return;
    }

    std::cout << "[SERVER] Escuchando en puerto " << port_ << std::endl;
    std::cout << "[SERVER] Thread pool: " << poolSize_ << " workers activos" << std::endl;
    // Hardcoded initial parameters for logging
    std::cout << "[SERVER] Dispatcher iniciado (quantum=5, ciclo=3)" << std::endl;

    simThread_ = std::thread(&Server::simulationLoop, this);
    acceptLoop();
}

void Server::acceptLoop() {
    while (running_) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            if (running_) perror("Accept failed");
            continue;
        }
        
        pool_.enqueue([this, clientSocket]() { handleClient(clientSocket); });
        ++totalConnections_;
        ++activeConnections_;
    }
}

void Server::handleClient(int clientSocket) {
    char buffer[1024];
    while (running_) {
        ssize_t n = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;
        buffer[n] = '\0';
        
        std::string line(buffer);
        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if (cmd == "STATS") {
            ServerStats s = stats();
            std::string resp = "STATS connections=" + std::to_string(s.totalConnections) +
                               " active=" + std::to_string(s.activeConnections) +
                               " workers=" + std::to_string(s.activeWorkers) +
                               " pending=" + std::to_string(s.pendingTasks) +
                               " processes=" + std::to_string(s.totalProcessesSubmitted) +
                               " finished=" + std::to_string(s.totalProcessesFinished) +
                               " faults=" + std::to_string(s.totalPageFaults) + "\n";
            send(clientSocket, resp.c_str(), resp.size(), 0);
            break; // Read-only probe, close connection
        } else if (cmd == "CREATE") {
            int pid;
            size_t numPages;
            std::string refsStr;
            ss >> pid >> numPages >> refsStr;

            std::vector<size_t> refs;
            std::stringstream rss(refsStr);
            std::string r;
            while (std::getline(rss, r, ',')) {
                try { refs.push_back(std::stoul(r)); } catch (...) {}
            }

            std::lock_guard<std::mutex> lock(dispatcherMutex_);
            if (mm_->admitProcess(pid, numPages)) {
                Process p(pid, numPages, refs, dispatcher_->currentTime());
                const auto& mmProcs = mm_->getProcesses();
                auto it = mmProcs.find(pid);
                if (it != mmProcs.end()) {
                    for (const auto& [loc, glob] : it->second.getPageTable()) {
                        p.mapPage(loc, glob);
                    }
                }
                dispatcher_->addProcess(std::move(p));
                ++totalProcessesSubmitted_;
                
                {
                    std::lock_guard<std::mutex> clock(clientMutex_);
                    pidToSocket_[pid] = clientSocket;
                }
                
                std::string resp = "OK " + std::to_string(pid) + "\n";
                send(clientSocket, resp.c_str(), resp.size(), 0);
            } else {
                std::string resp = "ERR Capacity or duplicate PID\n";
                send(clientSocket, resp.c_str(), resp.size(), 0);
            }
        }
    }
    close(clientSocket);
    --activeConnections_;
}

void Server::simulationLoop() {
    while (running_) {
        std::vector<Dispatcher::FinalReport> newFinished;
        {
            std::unique_lock<std::mutex> lk(dispatcherMutex_, std::try_to_lock);
            if (lk.owns_lock()) {
                int oldFinishedCount = dispatcher_->getReport().size();
                dispatcher_->step();
                int newFinishedCount = dispatcher_->getReport().size();
                if (newFinishedCount > oldFinishedCount) {
                    newFinished = dispatcher_->getReport();
                    // We only want the newly finished ones.
                    // The simplest is to just figure out which ones are new.
                    // But Dispatcher::getReport() returns all reports. 
                    // Let's just track the processed ones.
                }
            }
        }

        if (!newFinished.empty()) {
            std::lock_guard<std::mutex> clock(clientMutex_);
            // Send to all new finished that we haven't seen. 
            // We use totalProcessesFinished_ to track offset.
            for (size_t i = totalProcessesFinished_.load(); i < newFinished.size(); ++i) {
                const auto& report = newFinished[i];
                auto it = pidToSocket_.find(report.pid);
                if (it != pidToSocket_.end()) {
                    sendDoneMessage(it->first, report);
                }
                totalPageFaults_ += report.pageFaults;
            }
            totalProcessesFinished_ = newFinished.size();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Server::sendDoneMessage(int pid, const Dispatcher::FinalReport& report) {
    int sock = -1;
    {
        auto it = pidToSocket_.find(pid);
        if (it != pidToSocket_.end()) sock = it->second;
    }
    if (sock != -1) {
        std::string msg = "DONE " + std::to_string(pid) + " " +
                          std::to_string(report.finishTime) + " " +
                          std::to_string(report.waitingTime) + " " +
                          std::to_string(report.pageFaults) + "\n";
        send(sock, msg.c_str(), msg.size(), 0);
    }
}

ServerStats Server::stats() const {
    ServerStats s;
    s.totalConnections = totalConnections_.load();
    s.activeConnections = activeConnections_.load();
    s.pendingTasks = pool_.pendingTasks();
    s.activeWorkers = pool_.activeWorkers();
    s.poolSize = poolSize_;
    s.totalProcessesSubmitted = totalProcessesSubmitted_.load();
    s.totalProcessesFinished = totalProcessesFinished_.load();
    s.totalPageFaults = totalPageFaults_.load();
    return s;
}