#pragma once
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <atomic>
#include "../include/MemoryManager.hpp"
#include "../include/Dispatcher.hpp"
#include "ThreadPool.hpp"

struct ServerStats {
    int totalConnections;
    int activeConnections;
    size_t pendingTasks;
    size_t activeWorkers;
    size_t poolSize;
    int totalProcessesSubmitted;
    int totalProcessesFinished;
    int totalPageFaults;
};

class Server {
public:
    explicit Server(int port = 9000,
                    size_t vmCapacity = 256,
                    size_t ramCapacity = 16,
                    int quantum = 5,
                    int clockCycle = 3,
                    size_t threadPoolSize = 0);
    ~Server();
    void run();
    ServerStats stats() const;

private:
    void acceptLoop();
    void handleClient(int clientSocket);
    void simulationLoop();
    void sendDoneMessage(int pid, const Dispatcher::FinalReport& report);

    int serverSocket_;
    int port_;
    std::atomic<bool> running_;
    
    std::mutex dispatcherMutex_;
    std::unique_ptr<MemoryManager> mm_;
    std::unique_ptr<Dispatcher> dispatcher_;
    
    std::mutex clientMutex_;
    std::map<int, int> pidToSocket_;
    
    std::thread acceptThread_;
    std::thread simThread_;
    
    ThreadPool pool_;
    std::atomic<int> totalConnections_{0};
    std::atomic<int> activeConnections_{0};
    
    std::atomic<int> totalProcessesSubmitted_{0};
    std::atomic<int> totalProcessesFinished_{0};
    std::atomic<int> totalPageFaults_{0};
    size_t poolSize_;
};