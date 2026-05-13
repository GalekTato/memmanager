#include "Server.hpp"
#include "../include/HybridPolicy.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

Server::Server(int port) : port_(port), running_(true) {
    auto pol = std::make_unique<HybridPolicy>(16); // Default MF=16
    mm_ = std::make_unique<MemoryManager>(256, 16, std::move(pol));
    dispatcher_ = std::make_unique<Dispatcher>(*mm_, 5, 3);
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

    std::cout << "Server listening on port " << port_ << std::endl;

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
        std::thread(&Server::handleClient, this, clientSocket).detach();
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

        if (cmd == "CREATE") {
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

            std::lock_guard<std::mutex> lock(mmMutex_);
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
    // Note: We don't close the socket here because we might need to send DONE later.
    // However, if the client disconnects, we should probably handle it.
    // For this prototype, we'll keep the socket open or the client waits.
}

void Server::simulationLoop() {
    while (running_) {
        std::vector<Dispatcher::FinalReport> newFinished;
        {
            std::lock_guard<std::mutex> lock(mmMutex_);
            int oldFinishedCount = dispatcher_->getReport().size();
            dispatcher_->step();
            int newFinishedCount = dispatcher_->getReport().size();
            if (newFinishedCount > oldFinishedCount) {
                newFinished = dispatcher_->getReport();
            }
        }

        for (const auto& report : newFinished) {
            std::lock_guard<std::mutex> clock(clientMutex_);
            auto it = pidToSocket_.find(report.pid);
            if (it != pidToSocket_.end()) {
                sendDoneMessage(it->first, report);
                // After sending DONE, we can close the socket if no other processes are pending for this client.
                // But simplified for now: just send.
            }
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