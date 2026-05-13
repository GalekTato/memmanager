#include "Client.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cerr << "Usage: ./memclient <host> <port> <pid> <numPages> <ref1,ref2,...>" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    int port = std::stoi(argv[2]);
    int pid = std::stoi(argv[3]);
    int numPages = std::stoi(argv[4]);
    std::string refs = argv[5];

    Client client(host, port);
    if (!client.connectToServer()) {
        std::cerr << "Could not connect to server" << std::endl;
        return 1;
    }

    if (client.createProcess(pid, numPages, refs)) {
        client.waitForDone();
    } else {
        std::cerr << "Failed to create process" << std::endl;
    }

    return 0;
}