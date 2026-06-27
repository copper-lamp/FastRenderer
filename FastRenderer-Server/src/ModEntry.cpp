#include "ll/api/mod/NativeMod.h"
#include "ll/api/mod/RegisterHelper.h"
#include "TcpServer.h"
#include "ServerPluginApi.h"
#include <fstream>
#include <chrono>
#include <ctime>
#include <thread>

namespace fast_renderer {

inline void logInit(const char* msg) {
    std::ofstream out("FastRenderer_Server_Init.txt", std::ios::app);
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    char ts[32] = {};
    tm local = {};
    localtime_s(&local, &tt);
    std::strftime(ts, sizeof(ts), "%H:%M:%S", &local);
    out << "[" << ts << "] " << msg << "\n";
    out.flush();
}

class FRServerImpl {
public:
    static FRServerImpl& getInstance();
    FRServerImpl() : mSelf(*ll::mod::NativeMod::current()) {}
    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load() {
        logInit("FastRenderer-Server::load() started");
        logInit("FastRenderer-Server::load() completed");
        return true;
    }

    bool enable() {
        logInit("FastRenderer-Server::enable() started");

        // ─── Start TCP Server ───
        uint16_t port = 12345;

        if (!m_tcpServer.start(port)) {
            logInit(("  TCP Server FAILED to start on port " + std::to_string(port)).c_str());
            logInit("FastRenderer-Server::enable() completed with errors");
            return true;  // don't prevent server from loading
        }

        logInit(("  TCP Server started on port " + std::to_string(port)).c_str());

        // Inject TcpServer into PluginApi
        ServerFRPluginApi& api = ServerFRPluginApi::getInstance();
        api.setTcpServer(&m_tcpServer);

        // ─── TCP message handler ───
        // Routes gui_event and data_exchange from clients to PluginApi dispatch
        m_tcpServer.setMessageHandler([](const std::string& player, const std::string& jsonMsg) {
            ServerFRPluginApi::getInstance().handleTcpMessage(player, jsonMsg);
        });

        logInit("FastRenderer-Server::enable() completed - TCP Server ready");
        return true;
    }

    bool disable() {
        logInit("FastRenderer-Server::disable() called");
        m_tcpServer.stop();
        return true;
    }

private:
    FrTcpServer m_tcpServer;
};

FRServerImpl& FRServerImpl::getInstance() {
    static FRServerImpl instance;
    return instance;
}

}

// Provide getInstance for server-side plugins
IFastRendererAPI* IFastRendererAPI::getInstance() {
    return &ServerFRPluginApi::getInstance();
}

LL_REGISTER_MOD(fast_renderer::FRServerImpl, fast_renderer::FRServerImpl::getInstance());