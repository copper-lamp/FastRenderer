#include "ll/api/mod/NativeMod.h"
#include "ll/api/mod/RegisterHelper.h"
#include "FRPackets.h"
#include "ServerPluginApi.h"
#include <fstream>
#include <chrono>
#include <ctime>
#include <ll/api/network/packet/PacketRegistrar.h>

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

        auto& reg = ll::network::PacketRegistrar::getInstance();

        auto nameHash = [](const std::string& n) -> uint64 {
            return std::hash<std::string_view>{}(n);
        };

        static fast_packets::GuiRegistrationPacket  guiRegPkt;
        static fast_packets::GuiUnregistrationPacket guiUnregPkt;
        static fast_packets::KeybindRegistrationPacket keyRegPkt;
        static fast_packets::KeybindUnregistrationPacket keyUnregPkt;
        static GuiEventHandler guiEvtHandler;
        static DataExchangeHandler dataExHandler;

        reg.registerPacket("FR_GuiRegistration", nameHash("FR_GuiRegistration"),
            []() -> std::unique_ptr<ll::network::Packet> {
                return std::make_unique<fast_packets::GuiRegistrationPacket>();
            }, nullptr);

        reg.registerPacket("FR_GuiUnregistration", nameHash("FR_GuiUnregistration"),
            []() -> std::unique_ptr<ll::network::Packet> {
                return std::make_unique<fast_packets::GuiUnregistrationPacket>();
            }, nullptr);

        reg.registerPacket("FR_KeybindRegistration", nameHash("FR_KeybindRegistration"),
            []() -> std::unique_ptr<ll::network::Packet> {
                return std::make_unique<fast_packets::KeybindRegistrationPacket>();
            }, nullptr);

        reg.registerPacket("FR_KeybindUnregistration", nameHash("FR_KeybindUnregistration"),
            []() -> std::unique_ptr<ll::network::Packet> {
                return std::make_unique<fast_packets::KeybindUnregistrationPacket>();
            }, nullptr);

        reg.registerPacket("FR_GuiEvent", nameHash("FR_GuiEvent"),
            []() -> std::unique_ptr<ll::network::Packet> {
                return std::make_unique<fast_packets::GuiEventPacket>();
            }, &guiEvtHandler);

        reg.registerPacket("FR_DataExchange", nameHash("FR_DataExchange"),
            []() -> std::unique_ptr<ll::network::Packet> {
                return std::make_unique<fast_packets::DataExchangePacket>();
            }, &dataExHandler);

        logInit("FastRenderer-Server::enable() completed - 6 packets registered");
        return true;
    }

    bool disable() {
        logInit("FastRenderer-Server::disable() called");
        return true;
    }

private:
    ll::mod::NativeMod& mSelf;
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
