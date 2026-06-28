#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <map>
#include <mutex>
#include <vector>
#include <functional>
#include <cstring>
#include <chrono>
#include <nlohmann/json.hpp>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// ─── TCP Server — accepts multiple clients, broadcast/定向 ───

class FrTcpServer {
public:
    using MessageCallback = std::function<void(const std::string& player, const std::string& jsonMsg)>;

    FrTcpServer() = default;
    ~FrTcpServer() { stop(); }

    // ─── Lifecycle ───

    bool start(uint16_t port) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;

        m_listenSock = socket(AF_INET, SOCK_STREAM, 0);
        if (m_listenSock == INVALID_SOCKET) { WSACleanup(); return false; }

        // Allow address reuse
        int opt = 1;
        setsockopt(m_listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 0.0.0.0 - accept any connection
        addr.sin_port = htons(port);

        if (bind(m_listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            closesocket(m_listenSock); WSACleanup(); return false;
        }

        if (listen(m_listenSock, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(m_listenSock); WSACleanup(); return false;
        }

        m_port = port;
        m_running = true;
        m_acceptThread = std::thread(&FrTcpServer::acceptLoop, this);

        return true;
    }

    void stop() {
        m_running = false;

        // Close all client connections
        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            for (auto& [_, client] : m_clients) {
                shutdown(client.sock, SD_BOTH);
                closesocket(client.sock);
            }
            m_clients.clear();
            m_playerMap.clear();
        }

        // Close listen socket to unblock accept()
        if (m_listenSock != INVALID_SOCKET) {
            closesocket(m_listenSock);
            m_listenSock = INVALID_SOCKET;
        }

        if (m_acceptThread.joinable()) m_acceptThread.join();
        WSACleanup();
    }

    uint16_t getPort() const { return m_port; }
    bool isRunning() const { return m_running; }

    // ─── Broadcast to all clients ───

    bool broadcast(const std::string& jsonMessage) {
        std::string line = jsonMessage + "\n";
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (auto& [_, client] : m_clients) {
            sendToSock(client.sock, line);
        }
        return true;
    }

    // ─── Send to a specific player ───

    bool sendToPlayer(const std::string& player, const std::string& jsonMessage) {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = m_playerMap.find(player);
        if (it == m_playerMap.end()) return false;

        auto cit = m_clients.find(it->second);
        if (cit == m_clients.end()) return false;

        return sendToSock(cit->second.sock, jsonMessage + "\n");
    }

    // ─── Register a handler for messages from clients ───

    void setMessageHandler(MessageCallback handler) {
        m_onMessage = std::move(handler);
    }

    // ─── Get connected player list ───

    std::vector<std::string> getConnectedPlayers() {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        std::vector<std::string> players;
        for (auto& [player, _] : m_playerMap) {
            players.push_back(player);
        }
        return players;
    }

    int getClientCount() {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        return (int)m_clients.size();
    }

private:
    struct ClientInfo {
        SOCKET sock;
        std::string playerName;
        std::thread recvThread;
        bool identified = false;
    };

    SOCKET m_listenSock = INVALID_SOCKET;
    uint16_t m_port = 0;
    std::atomic<bool> m_running{false};
    std::thread m_acceptThread;

    std::mutex m_clientsMutex;
    std::map<SOCKET, ClientInfo> m_clients;     // sock → client info
    std::map<std::string, SOCKET> m_playerMap;   // player → sock

    MessageCallback m_onMessage;

    // ─── Accept loop ───

    void acceptLoop() {
        fd_set readfds;
        while (m_running) {
            FD_ZERO(&readfds);
            FD_SET(m_listenSock, &readfds);

            timeval tv = {1, 0};  // 1 second timeout
            int ret = select(0, &readfds, nullptr, nullptr, &tv);
            if (ret == SOCKET_ERROR) break;
            if (ret == 0) continue;  // timeout, check m_running

            sockaddr_in clientAddr;
            int addrLen = sizeof(clientAddr);
            SOCKET clientSock = accept(m_listenSock, (sockaddr*)&clientAddr, &addrLen);
            if (clientSock == INVALID_SOCKET) continue;

            // Start receive thread for this client
            ClientInfo info;
            info.sock = clientSock;
            info.recvThread = std::thread(&FrTcpServer::clientRecvLoop, this, clientSock);

            {
                std::lock_guard<std::mutex> lock(m_clientsMutex);
                m_clients[clientSock] = std::move(info);
            }
        }
    }

    // ─── Per-client receive loop ───

    void clientRecvLoop(SOCKET sock) {
        std::string buffer;
        char buf[4096];

        while (m_running) {
            int ret = recv(sock, buf, sizeof(buf) - 1, 0);
            if (ret <= 0) {
                removeClient(sock);
                return;
            }

            buf[ret] = '\0';
            buffer.append(buf, ret);

            // Split by newline
            size_t pos;
            while ((pos = buffer.find('\n')) != std::string::npos) {
                std::string msg = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);
                if (msg.empty()) continue;

                // Handle identify message
                try {
                    auto j = nlohmann::json::parse(msg);
                    if (j.contains("type") && j["type"] == "identify") {
                        handleIdentify(sock, j);
                        continue;
                    }
                } catch (...) {}

                // Route to message handler with player name
                if (m_onMessage) {
                    std::string player = getPlayerForSock(sock);
                    m_onMessage(player, msg);
                }
            }
        }
    }

    // ─── Identify handling ───

    void handleIdentify(SOCKET sock, const nlohmann::json& j) {
        std::string player = j.value("player", "");
        std::lock_guard<std::mutex> lock(m_clientsMutex);

        auto it = m_clients.find(sock);
        if (it == m_clients.end()) return;

        // Remove old mapping if player reconnected
        auto oldSock = m_playerMap.find(player);
        if (oldSock != m_playerMap.end() && oldSock->second != sock) {
            // Close old connection
            shutdown(oldSock->second, SD_BOTH);
            closesocket(oldSock->second);
            m_clients.erase(oldSock->second);
        }

        it->second.playerName = player;
        it->second.identified = true;
        m_playerMap[player] = sock;
    }

    // ─── Helpers ───

    void removeClient(SOCKET sock) {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = m_clients.find(sock);
        if (it == m_clients.end()) return;

        // Remove from player map
        if (!it->second.playerName.empty()) {
            m_playerMap.erase(it->second.playerName);
        }

        // Detach thread (it will exit on its own)
        if (it->second.recvThread.joinable()) {
            it->second.recvThread.detach();
        }

        closesocket(sock);
        m_clients.erase(it);
    }

    std::string getPlayerForSock(SOCKET sock) {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = m_clients.find(sock);
        if (it != m_clients.end()) {
            return it->second.playerName;
        }
        return "";
    }

    bool sendToSock(SOCKET sock, const std::string& data) {
        int total = (int)data.size();
        int sent = 0;
        while (sent < total) {
            int n = ::send(sock, data.data() + sent, total - sent, 0);
            if (n == SOCKET_ERROR) return false;
            sent += n;
        }
        return true;
    }
};
