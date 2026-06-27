#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <cstring>
#include <chrono>

// ─── Platform-specific socket headers ───

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using tcp_socket_t = SOCKET;
inline constexpr tcp_socket_t TCP_INVALID_SOCKET = INVALID_SOCKET;
inline constexpr int TCP_SOCKET_ERROR = SOCKET_ERROR;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
using tcp_socket_t = int;
inline constexpr tcp_socket_t TCP_INVALID_SOCKET = -1;
inline constexpr int TCP_SOCKET_ERROR = -1;
inline int closesocket(tcp_socket_t s) { return close(s); }
inline int WSAGetLastError() { return errno; }
#endif

// ─── TCP Client — cross platform (Win / Android) ───

class FrTcpClient {
public:
    using MessageCallback = std::function<void(const std::string& jsonMsg)>;
    using StatusCallback = std::function<void(bool connected)>;

    FrTcpClient() = default;
    ~FrTcpClient() { stop(); }

    // ─── Lifecycle ───

    bool connect(const std::string& host, uint16_t port) {
        m_host = host;
        m_port = port;
        return connectInternal();
    }

    void disconnect() {
        m_running = false;
        if (m_sock != TCP_INVALID_SOCKET) {
            #ifdef _WIN32
            shutdown(m_sock, SD_BOTH);
            closesocket(m_sock);
            #else
            shutdown(m_sock, SHUT_RDWR);
            closesocket(m_sock);
            #endif
            m_sock = TCP_INVALID_SOCKET;
        }
        if (m_recvThread.joinable()) m_recvThread.join();
        notifyStatus(false);
    }

    void stop() {
        disconnect();
        #ifdef _WIN32
        if (m_wsaStarted) { WSACleanup(); m_wsaStarted = false; }
        #endif
    }

    // ─── Send ───

    bool send(const std::string& jsonMessage) {
        if (m_sock == TCP_INVALID_SOCKET) return false;
        std::string line = jsonMessage + "\n";

        int sent = 0;
        int total = (int)line.size();
        while (sent < total) {
            int n = ::send(m_sock, line.data() + sent, total - sent, 0);
            if (n == TCP_SOCKET_ERROR) {
                disconnect();
                return false;
            }
            sent += n;
        }
        return true;
    }

    // ─── Callbacks ───

    void setMessageCallback(MessageCallback cb) { m_onMessage = std::move(cb); }
    void setStatusCallback(StatusCallback cb) { m_onStatus = std::move(cb); }

    bool isConnected() const { return m_sock != TCP_INVALID_SOCKET && m_running; }

    // ─── Auto reconnect (optional) ───

    void enableAutoReconnect(int intervalMs = 3000, int maxRetries = 5) {
        m_autoReconnect = true;
        m_reconnectInterval = intervalMs;
        m_maxRetries = maxRetries;
    }

    void startReceiveThread() {
        m_running = true;
        m_recvThread = std::thread(&FrTcpClient::receiveLoop, this);
    }

private:
    tcp_socket_t m_sock = TCP_INVALID_SOCKET;
    std::string m_host;
    uint16_t m_port = 0;
    std::thread m_recvThread;
    std::atomic<bool> m_running{false};

    MessageCallback m_onMessage;
    StatusCallback m_onStatus;

    bool m_autoReconnect = false;
    int m_reconnectInterval = 3000;
    int m_maxRetries = 5;
    #ifdef _WIN32
    bool m_wsaStarted = false;
    #endif

    bool connectInternal() {
        #ifdef _WIN32
        if (!m_wsaStarted) {
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;
            m_wsaStarted = true;
        }
        #endif

        m_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (m_sock == TCP_INVALID_SOCKET) return false;

        // Resolve host
        struct addrinfo hints = {}, *addrs = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        std::string portStr = std::to_string(m_port);
        int ret = getaddrinfo(m_host.c_str(), portStr.c_str(), &hints, &addrs);
        if (ret != 0) {
            closesocket(m_sock);
            m_sock = TCP_INVALID_SOCKET;
            return false;
        }

        // Connect (non-blocking with timeout would be better, but blocking is simpler)
        ret = ::connect(m_sock, addrs->ai_addr, (int)addrs->ai_addrlen);
        freeaddrinfo(addrs);

        if (ret == TCP_SOCKET_ERROR) {
            closesocket(m_sock);
            m_sock = TCP_INVALID_SOCKET;
            return false;
        }

        notifyStatus(true);
        return true;
    }

    void receiveLoop() {
        std::string buffer;
        char buf[4096];

        while (m_running) {
            int ret = (int)recv(m_sock, buf, sizeof(buf) - 1, 0);
            if (ret <= 0) {
                notifyStatus(false);
                // Attempt reconnect if enabled
                if (m_autoReconnect && m_running) {
                    attemptReconnect();
                } else {
                    break;
                }
                continue;
            }

            buf[ret] = '\0';
            buffer.append(buf, ret);

            // Split by newline
            size_t pos;
            while ((pos = buffer.find('\n')) != std::string::npos) {
                std::string msg = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);
                if (!msg.empty() && m_onMessage) {
                    m_onMessage(msg);
                }
            }
        }
    }

    void attemptReconnect() {
        closesocket(m_sock);
        m_sock = TCP_INVALID_SOCKET;

        for (int i = 0; i < m_maxRetries && m_running; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_reconnectInterval));
            if (!m_running) break;
            if (connectInternal()) {
                // Re-send identify message (handled by caller)
                notifyStatus(true);
                // Restart receive loop in current thread
                receiveLoop();
                return;
            }
        }
    }

    void notifyStatus(bool connected) {
        if (m_onStatus) m_onStatus(connected);
    }
};