#pragma once
#include <string>
#include <ll/api/network/packet/Packet.h>
#include <ll/api/network/packet/PacketRegistrar.h>
#include <ll/api/utils/HashUtils.h>
#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/deps/core/utility/ReadOnlyBinaryStream.h>
#include <mc/platform/Result.h>
#include <string>

// Packet::getRuntimeId is not exported from LeviLamina.dll
namespace ll::network {
inline PacketRuntimeId Packet::getRuntimeId() const {
    return ll::hash_utils::doHash(getName());
}
}

namespace fast_packets {

// ==========================================
// GuiRegistrationPacket: Server → Client
// Server plugin registers a GUI definition
// ==========================================
class GuiRegistrationPacket : public ll::network::Packet {
public:
    std::string pluginId;
    std::string guiId;
    std::string jsonContent;

    GuiRegistrationPacket() : Packet() {}

    void write(::BinaryStream& stream) const override {
        stream.writeString(pluginId, nullptr, nullptr);
        stream.writeString(guiId, nullptr, nullptr);
        stream.writeString(jsonContent, nullptr, nullptr);
    }

    Bedrock::Result<void> read(::ReadOnlyBinaryStream& stream) override {
        auto rP = stream.getString(64);    if (!rP) return nonstd::make_unexpected(rP.error()); pluginId = rP.value();
        auto rG = stream.getString(64);    if (!rG) return nonstd::make_unexpected(rG.error()); guiId = rG.value();
        auto rJ = stream.getString(65536); if (!rJ) return nonstd::make_unexpected(rJ.error()); jsonContent = rJ.value();
        return {};
    }

    std::string_view getName() const override { static std::string n = "FR_GuiRegistration"; return n; }
};

// ==========================================
// GuiUnregistrationPacket: Server → Client
// ==========================================
class GuiUnregistrationPacket : public ll::network::Packet {
public:
    std::string pluginId;
    std::string guiId;

    GuiUnregistrationPacket() : Packet() {}

    void write(::BinaryStream& stream) const override {
        stream.writeString(pluginId, nullptr, nullptr);
        stream.writeString(guiId, nullptr, nullptr);
    }

    Bedrock::Result<void> read(::ReadOnlyBinaryStream& stream) override {
        auto rP = stream.getString(64); if (!rP) return nonstd::make_unexpected(rP.error()); pluginId = rP.value();
        auto rG = stream.getString(64); if (!rG) return nonstd::make_unexpected(rG.error()); guiId = rG.value();
        return {};
    }

    std::string_view getName() const override { static std::string n = "FR_GuiUnregistration"; return n; }
};

// ==========================================
// KeybindRegistrationPacket: Server → Client
// ==========================================
class KeybindRegistrationPacket : public ll::network::Packet {
public:
    std::string pluginId;
    std::string bindId;
    std::string bindName;
    int vkCode;

    KeybindRegistrationPacket() : Packet() { vkCode = 0; }

    void write(::BinaryStream& stream) const override {
        stream.writeString(pluginId, nullptr, nullptr);
        stream.writeString(bindId, nullptr, nullptr);
        stream.writeString(bindName, nullptr, nullptr);
        stream.writeVarInt(vkCode, nullptr, nullptr);
    }

    Bedrock::Result<void> read(::ReadOnlyBinaryStream& stream) override {
        auto rP = stream.getString(64); if (!rP) return nonstd::make_unexpected(rP.error()); pluginId = rP.value();
        auto rI = stream.getString(64); if (!rI) return nonstd::make_unexpected(rI.error()); bindId = rI.value();
        auto rN = stream.getString(64); if (!rN) return nonstd::make_unexpected(rN.error()); bindName = rN.value();
        auto rV = stream.getVarInt();   if (!rV) return nonstd::make_unexpected(rV.error()); vkCode = rV.value();
        return {};
    }

    std::string_view getName() const override { static std::string n = "FR_KeybindRegistration"; return n; }
};

// ==========================================
// KeybindUnregistrationPacket: Server → Client
// ==========================================
class KeybindUnregistrationPacket : public ll::network::Packet {
public:
    std::string pluginId;
    std::string bindId;

    KeybindUnregistrationPacket() : Packet() {}

    void write(::BinaryStream& stream) const override {
        stream.writeString(pluginId, nullptr, nullptr);
        stream.writeString(bindId, nullptr, nullptr);
    }

    Bedrock::Result<void> read(::ReadOnlyBinaryStream& stream) override {
        auto rP = stream.getString(64); if (!rP) return nonstd::make_unexpected(rP.error()); pluginId = rP.value();
        auto rI = stream.getString(64); if (!rI) return nonstd::make_unexpected(rI.error()); bindId = rI.value();
        return {};
    }

    std::string_view getName() const override { static std::string n = "FR_KeybindUnregistration"; return n; }
};

// ==========================================
// GuiEventPacket: Client → Server
// User clicked a button → notify server plugin
// ==========================================
class GuiEventPacket : public ll::network::Packet {
public:
    std::string guiId;
    std::string controlId;
    std::string eventType;
    std::string value;

    GuiEventPacket() : Packet() {}

    void write(::BinaryStream& stream) const override {
        stream.writeString(guiId, nullptr, nullptr);
        stream.writeString(controlId, nullptr, nullptr);
        stream.writeString(eventType, nullptr, nullptr);
        stream.writeString(value, nullptr, nullptr);
    }

    Bedrock::Result<void> read(::ReadOnlyBinaryStream& stream) override {
        auto rG = stream.getString(64);    if (!rG) return nonstd::make_unexpected(rG.error()); guiId = rG.value();
        auto rC = stream.getString(64);    if (!rC) return nonstd::make_unexpected(rC.error()); controlId = rC.value();
        auto rT = stream.getString(64);    if (!rT) return nonstd::make_unexpected(rT.error()); eventType = rT.value();
        auto rV = stream.getString(1024);  if (!rV) return nonstd::make_unexpected(rV.error()); value = rV.value();
        return {};
    }

    std::string_view getName() const override { static std::string n = "FR_GuiEvent"; return n; }
};

// ==========================================
// DataExchangePacket: Bidirectional
// ==========================================
class DataExchangePacket : public ll::network::Packet {
public:
    std::string channel;
    std::string data;

    DataExchangePacket() : Packet() {}

    void write(::BinaryStream& stream) const override {
        stream.writeString(channel, nullptr, nullptr);
        stream.writeString(data, nullptr, nullptr);
    }

    Bedrock::Result<void> read(::ReadOnlyBinaryStream& stream) override {
        auto rC = stream.getString(128);   if (!rC) return nonstd::make_unexpected(rC.error()); channel = rC.value();
        auto rD = stream.getString(65536); if (!rD) return nonstd::make_unexpected(rD.error()); data = rD.value();
        return {};
    }

    std::string_view getName() const override { static std::string n = "FR_DataExchange"; return n; }
};

}
