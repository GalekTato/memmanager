#pragma once
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <atomic>
#include "../include/MemoryManager.hpp"
#include "../include/Dispatcher.hpp"

class Server {
public:
    Server(int port = 9000);
    ~Server();
    void run();

private:
    void acceptLoop();
    void handleClient(int clientSocket);
    void simulationLoop();
    void sendDoneMessage(int pid, const Dispatcher::FinalReport& report);

    int serverSocket_;
    int port_;
    std::atomic<bool> running_;
    
    std::mutex mmMutex_;
    std::unique_ptr<MemoryManager> mm_;
    std::unique_ptr<Dispatcher> dispatcher_;
    
    std::mutex clientMutex_;
    std::map<int, int> pidToSocket_; // Maps PID to client socket
    
    std::thread acceptThread_;
    std::thread simThread_;
};