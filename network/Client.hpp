#pragma once
#include <string>

class Client {
public:
    Client(const std::string& host, int port);
    ~Client();
    bool connectToServer(bool retry = true);
    bool createProcess(int pid, int numPages, const std::string& refs);
    void waitForDone();
    bool requestStats();

private:
    std::string host_;
    int port_;
    int socket_;
};