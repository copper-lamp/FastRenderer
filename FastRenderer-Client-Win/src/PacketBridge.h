#pragma once
#include <string>
#include <ll/api/network/packet/Packet.h>
#include <ll/api/network/packet/PacketRegistrar.h>
#include <ll/api/utils/HashUtils.h>
#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/deps/core/utility/ReadOnlyBinaryStream.h>
#include <mc/platform/Result.h>
#include <gui/GuiService.h>
#include <input/KeybindService.h>
#include <fstream>

// Packet::getRuntimeId is not exported from LeviLamina.dll
namespace ll::network {
inline PacketRuntimeId Packet::getRuntimeId() const {
    return ll::hash_utils::doHash(getName());
}
}

namespace fast_packets {

// ═══════════════════════════════════════════
// Client-side packet definitions (mirror server)
// These MUST have identical binary layout
// ═══════════════════════════════════════════

class ClientGuiRegistrationPacket : public ll::network::Packet {
public:
    std::string pluginId;
    std::string guiId;
    std::string jsonContent;

    void write(::BinaryStream& stream) const override {}
    Bedrock::Result<void> read(::ReadOnlyBinaryStream& stream) override {
        auto rP = stream.getString(64);    if (!rP) return nonstd::make_unexpected(rP.error()); pluginId = rP.value();
        auto rG = stream.getString(64);    if (!rG) return nonstd::make_unexpected(rG.error()); guiId = rG.value();
        auto rJ = stream.getString(65536); if (!rJ) return nonstd::make_unexpected(rJ.error()); jsonContent = rJ.value();
        return {};
    }
    std::string_view getName() const override { static std::string n = "FR_GuiRegistration"; return n; }
};

class ClientGuiUnregistrationPacket : public ll::network::Packet {
public:
    std::string pluginId;
    std::string guiId;

    void write(::BinaryStream& stream) const override {}
    Bedrock::Result<void> read(::ReadOnlyBinaryStream& stream) override {
        auto rP = stream.getString(64); if (!rP) return nonstd::make_unexpected(rP.error()); pluginId = rP.value();
        auto rG = stream.getString(64); if (!rG) return nonstd::make_unexpected(rG.error()); guiId = rG.value();
        return {};
    }
    std::string_view getName() const override { static std::string n = "FR_GuiUnregistration"; return n; }
};

class ClientKeybindRegistrationPacket : public ll::network::Packet {
public:
    std::string pluginId;
    std::string bindId;
    std::string bindName;
    int vkCode;

    void write(::BinaryStream& stream) const override {}
    Bedrock::Result<void> read(::ReadOnlyBinaryStream& stream) override {
        auto rP = stream.getString(64); if (!rP) return nonstd::make_unexpected(rP.error()); pluginId = rP.value();
        auto rI = stream.getString(64); if (!rI) return nonstd::make_unexpected(rI.error()); bindId = rI.value();
        auto rN = stream.getString(64); if (!rN) return nonstd::make_unexpected(rN.error()); bindName = rN.value();
        auto rV = stream.getVarInt();   if (!rV) return nonstd::make_unexpected(rV.error()); vkCode = rV.value();
        return {};
    }
    std::string_view getName() const override { static std::string n = "FR_KeybindRegistration"; return n; }
};

class ClientKeybindUnregistrationPacket : public ll::network::Packet {
public:
    std::string pluginId;
    std::string bindId;

    void write(::BinaryStream& stream) const override {}
    Bedrock::Result<void> read(::ReadOnlyBinaryStream& stream) override {
        auto rP = stream.getString(64); if (!rP) return nonstd::make_unexpected(rP.error()); pluginId = rP.value();
        auto rI = stream.getString(64); if (!rI) return nonstd::make_unexpected(rI.error()); bindId = rI.value();
        return {};
    }
    std::string_view getName() const override { static std::string n = "FR_KeybindUnregistration"; return n; }
};

class ClientGuiEventPacket : public ll::network::Packet {
public:
    std::string guiId;
    std::string controlId;
    std::string eventType;
    std::string value;

    void write(::BinaryStream& stream) const override {
        stream.writeString(guiId, nullptr, nullptr);
        stream.writeString(controlId, nullptr, nullptr);
        stream.writeString(eventType, nullptr, nullptr);
        stream.writeString(value, nullptr, nullptr);
    }
    Bedrock::Result<void> read(::ReadOnlyBinaryStream& stream) override { return {}; }
    std::string_view getName() const override { static std::string n = "FR_GuiEvent"; return n; }
};

class ClientDataExchangePacket : public ll::network::Packet {
public:
    std::string channel;
    std::string data;

    void write(::BinaryStream& stream) const override {
        stream.writeString(channel, nullptr, nullptr);
        stream.writeString(data, nullptr, nullptr);
    }
    Bedrock::Result<void> read(::ReadOnlyBinaryStream& stream) override { return {}; }
    std::string_view getName() const override { static std::string n = "FR_DataExchange"; return n; }
};

}

// ─── Handlers ───

class ClientGuiRegHandler : public ll::network::IPacketHandler {
public:
    void handle(const NetworkIdentifier&, NetEventCallback&,
                const ll::network::Packet& packet) const override
    {
        auto& pkt = static_cast<const fast_packets::ClientGuiRegistrationPacket&>(packet);
        GuiService::registerGui(pkt.pluginId, pkt.guiId, pkt.jsonContent);

        // Also persist to disk for hot-reload persistence
        auto defs = GuiService::getDefinitions();
        std::string dir = HotReloadService::getWatchDir();
        if (!dir.empty() && !pkt.guiId.empty()) {
            std::string path = dir + "/" + pkt.guiId + ".json";
            std::ofstream f(path);
            if (f.is_open()) {
                f << pkt.jsonContent;
                f.close();
            }
        }
    }
};

class ClientGuiUnregHandler : public ll::network::IPacketHandler {
public:
    void handle(const NetworkIdentifier&, NetEventCallback&,
                const ll::network::Packet& packet) const override
    {
        auto& pkt = static_cast<const fast_packets::ClientGuiUnregistrationPacket&>(packet);
        GuiService::unregisterGui(pkt.pluginId, pkt.guiId);
    }
};

class ClientKeyRegHandler : public ll::network::IPacketHandler {
public:
    void handle(const NetworkIdentifier&, NetEventCallback&,
                const ll::network::Packet& packet) const override
    {
        auto& pkt = static_cast<const fast_packets::ClientKeybindRegistrationPacket&>(packet);
        KeybindService::registerKeybind(pkt.pluginId, pkt.bindId, pkt.bindName, pkt.vkCode, nullptr);
    }
};

class ClientKeyUnregHandler : public ll::network::IPacketHandler {
public:
    void handle(const NetworkIdentifier&, NetEventCallback&,
                const ll::network::Packet& packet) const override
    {
        auto& pkt = static_cast<const fast_packets::ClientKeybindUnregistrationPacket&>(packet);
        KeybindService::unregisterKeybind(pkt.pluginId, pkt.bindId);
    }
};

class ClientDataExHandler : public ll::network::IPacketHandler {
public:
    void handle(const NetworkIdentifier&, NetEventCallback&,
                const ll::network::Packet& packet) const override {}
};

// ═══════════════════════════════════════════
// Bridge initialization - called once at startup
// ═══════════════════════════════════════════

namespace PacketBridge {

inline bool g_initialized = false;

inline void init() {
    if (g_initialized) return;

    auto& reg = ll::network::PacketRegistrar::getInstance();
    auto nh = [](const std::string& n) -> uint64 {
        return std::hash<std::string_view>{}(n);
    };

    static ClientGuiRegHandler guiRegH;
    static ClientGuiUnregHandler guiUnregH;
    static ClientKeyRegHandler keyRegH;
    static ClientKeyUnregHandler keyUnregH;
    static ClientDataExHandler dataExH;

    reg.registerPacket("FR_GuiRegistration", nh("FR_GuiRegistration"),
        []() -> std::unique_ptr<ll::network::Packet> {
            return std::make_unique<fast_packets::ClientGuiRegistrationPacket>();
        }, &guiRegH);

    reg.registerPacket("FR_GuiUnregistration", nh("FR_GuiUnregistration"),
        []() -> std::unique_ptr<ll::network::Packet> {
            return std::make_unique<fast_packets::ClientGuiUnregistrationPacket>();
        }, &guiUnregH);

    reg.registerPacket("FR_KeybindRegistration", nh("FR_KeybindRegistration"),
        []() -> std::unique_ptr<ll::network::Packet> {
            return std::make_unique<fast_packets::ClientKeybindRegistrationPacket>();
        }, &keyRegH);

    reg.registerPacket("FR_KeybindUnregistration", nh("FR_KeybindUnregistration"),
        []() -> std::unique_ptr<ll::network::Packet> {
            return std::make_unique<fast_packets::ClientKeybindUnregistrationPacket>();
        }, &keyUnregH);

    reg.registerPacket("FR_GuiEvent", nh("FR_GuiEvent"), nullptr, nullptr);
    reg.registerPacket("FR_DataExchange", nh("FR_DataExchange"),
        []() -> std::unique_ptr<ll::network::Packet> {
            return std::make_unique<fast_packets::ClientDataExchangePacket>();
        }, &dataExH);

    g_initialized = true;
}

}
