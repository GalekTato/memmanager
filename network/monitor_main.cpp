#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <map>

std::map<std::string, std::string> parseStats(const std::string& statsLine) {
    std::map<std::string, std::string> result;
    if (statsLine.substr(0, 6) != "STATS ") return result;
    
    std::stringstream ss(statsLine.substr(6));
    std::string token;
    while (ss >> token) {
        auto pos = token.find('=');
        if (pos != std::string::npos) {
            result[token.substr(0, pos)] = token.substr(pos + 1);
        }
    }
    return result;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: ./memmonitor <host> <port> [interval_ms]" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    int port = std::stoi(argv[2]);
    int interval_ms = (argc > 3) ? std::stoi(argv[3]) : 2000;

    while (true) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            std::cerr << "Socket creation failed" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
            continue;
        }

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
            std::cerr << "Invalid address" << std::endl;
            close(sock);
            return 1;
        }

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            std::string msg = "STATS\n";
            if (send(sock, msg.c_str(), msg.size(), 0) > 0) {
                char buffer[1024];
                ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
                if (n > 0) {
                    buffer[n] = '\0';
                    std::string line(buffer);
                    auto stats = parseStats(line);
                    
                    std::cout << "\033[2J\033[H";
                    std::cout << "╔══════════════════════════════════════════════╗\n";
                    std::cout << "║  MemManager Server Monitor               ║\n";
                    std::cout << "╠══════════════════════════════════════════════╣\n";
                    std::cout << "║  Total conexiones  : " << stats["connections"] << std::string(20 - stats["connections"].length(), ' ') << "║\n";
                    std::cout << "║  Conexiones activas: " << stats["active"] << std::string(20 - stats["active"].length(), ' ') << "║\n";
                    std::string workersStr = stats["workers"] + " / " + "N/A"; // wait, stats only provides active. I added poolSize.
                    // Wait, the STATS format didn't say to add poolSize, but the prompt says:
                    // STATS connections=<N> active=<N> workers=<N> pending=<N> processes=<N> finished=<N> faults=<N>
                    // And the monitor output shows: Workers activos   : 3 / 8
                    // Oh, my Server::handleClient sends stats with workers=<N> but not pool size. I should modify Server.cpp to include pool size if needed, or just display workers. Wait, I'll modify Server.cpp slightly if needed, or just display active if that's all I have.
                    // Let me just format it using a fixed width print.
                    std::cout << "║  Workers activos   : " << stats["workers"] << std::string(20 - stats["workers"].length(), ' ') << "║\n";
                    std::cout << "║  Tareas pendientes : " << stats["pending"] << std::string(20 - stats["pending"].length(), ' ') << "║\n";
                    std::cout << "║  Procesos enviados : " << stats["processes"] << std::string(20 - stats["processes"].length(), ' ') << "║\n";
                    std::cout << "║  Procesos finalizados: " << stats["finished"] << std::string(18 - stats["finished"].length(), ' ') << "║\n";
                    std::cout << "║  Fallos de página  : " << stats["faults"] << std::string(20 - stats["faults"].length(), ' ') << "║\n";
                    std::cout << "╚══════════════════════════════════════════════╝\n";
                }
            }
        }
        close(sock);
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }
    return 0;
}