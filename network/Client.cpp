#include "Client.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <thread>
#include <chrono>

Client::Client(const std::string& host, int port) : host_(host), port_(port), socket_(-1) {}

Client::~Client() {
    if (socket_ != -1) close(socket_);
}

bool Client::connectToServer(bool retry) {
    int maxRetries = retry ? 5 : 1;
    for (int i = 0; i < maxRetries; ++i) {
        socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_ == -1) return false;

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        if (inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) return false;

        if (connect(socket_, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            return true;
        }

        close(socket_);
        socket_ = -1;

        if (i < maxRetries - 1) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    return false;
}

bool Client::createProcess(int pid, int numPages, const std::string& refs) {
    std::string msg = "CREATE " + std::to_string(pid) + " " + std::to_string(numPages) + " " + refs + "\n";
    if (send(socket_, msg.c_str(), msg.size(), 0) < 0) return false;

    char buffer[1024];
    ssize_t n = recv(socket_, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) return false;
    buffer[n] = '\0';
    std::cout << "Server: " << buffer;
    return std::string(buffer).find("OK") != std::string::npos;
}

void Client::waitForDone() {
    char buffer[1024];
    while (true) {
        ssize_t n = recv(socket_, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;
        buffer[n] = '\0';
        std::string line(buffer);
        if (line.find("DONE") != std::string::npos) {
            std::cout << "Result: " << line;
            break;
        }
    }
}

bool Client::requestStats() {
    std::string msg = "STATS\n";
    if (send(socket_, msg.c_str(), msg.size(), 0) < 0) return false;

    char buffer[1024];
    ssize_t n = recv(socket_, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) return false;
    buffer[n] = '\0';
    std::cout << buffer;
    return true;
}