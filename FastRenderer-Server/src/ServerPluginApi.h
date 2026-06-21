#pragma once
#include "FRPackets.h"
#include <FastRendererAPI.h>
#include <ll/api/network/packet/PacketRegistrar.h>
#include <functional>
#include <map>
#include <string>
#include <fstream>

// ─── Server-side PluginApi implementation ───
// Delegates API calls to network packets broadcast to clients

class ServerFRPluginApi : public IFastRendererAPI {
public:
    static ServerFRPluginApi& getInstance() {
        static ServerFRPluginApi instance;
        return instance;
    }

    bool registerGui(const std::string& pluginId,
        const std::string& guiId, const nlohmann::json& definition) override
    {
        using namespace fast_packets;
        GuiRegistrationPacket pkt;
        pkt.pluginId = pluginId;
        pkt.guiId = guiId;
        pkt.jsonContent = definition.dump();
        pkt.sendBroadcast();
        return true;
    }

    bool unregisterGui(const std::string& pluginId,
        const std::string& guiId) override
    {
        using namespace fast_packets;
        GuiUnregistrationPacket pkt;
        pkt.pluginId = pluginId;
        pkt.guiId = guiId;
        pkt.sendBroadcast();
        return true;
    }

    bool registerKeybind(const std::string& pluginId,
        const std::string& bindId, const std::string& name, int vkCode) override
    {
        using namespace fast_packets;
        KeybindRegistrationPacket pkt;
        pkt.pluginId = pluginId;
        pkt.bindId = bindId;
        pkt.bindName = name;
        pkt.vkCode = vkCode;
        pkt.sendBroadcast();
        return true;
    }

    bool unregisterKeybind(const std::string& pluginId,
        const std::string& bindId) override
    {
        using namespace fast_packets;
        KeybindUnregistrationPacket pkt;
        pkt.pluginId = pluginId;
        pkt.bindId = bindId;
        pkt.sendBroadcast();
        return true;
    }

    void publish(const std::string& channel,
        const std::string& data) override
    {
        using namespace fast_packets;
        DataExchangePacket pkt;
        pkt.channel = channel;
        pkt.data = data;
        pkt.sendBroadcast();
    }

    void subscribe(const std::string& channel,
        std::function<void(const std::string&)> callback) override
    {
        std::lock_guard<std::mutex> lock(m_subMutex);
        m_subscriptions[channel].push_back(std::move(callback));
    }

    void onGuiEvent(const std::string& guiId,
        std::function<void(const std::string& controlId,
            const std::string& eventType, const std::string& value)> callback) override
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_eventCallbacks[guiId] = std::move(callback);
    }

    bool isServerConnected() override {
        return true; // We ARE the server
    }

    // ─── Internal dispatch methods ───

    void dispatchGuiEvent(const std::string& guiId, const std::string& controlId,
        const std::string& eventType, const std::string& value)
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        auto it = m_eventCallbacks.find(guiId);
        if (it != m_eventCallbacks.end() && it->second) {
            it->second(controlId, eventType, value);
        }
    }

    void dispatchData(const std::string& channel, const std::string& data) {
        std::lock_guard<std::mutex> lock(m_subMutex);
        auto it = m_subscriptions.find(channel);
        if (it != m_subscriptions.end()) {
            for (auto& cb : it->second) {
                if (cb) cb(data);
            }
        }
    }

private:
    std::mutex m_subMutex;
    std::map<std::string, std::vector<std::function<void(const std::string&)>>> m_subscriptions;

    std::mutex m_eventMutex;
    std::map<std::string, std::function<void(const std::string&, const std::string&, const std::string&)>> m_eventCallbacks;
};

// ─── Packet Handlers (server side receives from clients) ───

class GuiEventHandler : public ll::network::IPacketHandler {
public:
    void handle(const NetworkIdentifier& source, NetEventCallback&,
                const ll::network::Packet& packet) const override
    {
        auto& pkt = static_cast<const fast_packets::GuiEventPacket&>(packet);
        ServerFRPluginApi::getInstance().dispatchGuiEvent(
            pkt.guiId, pkt.controlId, pkt.eventType, pkt.value);
    }
};

class DataExchangeHandler : public ll::network::IPacketHandler {
public:
    void handle(const NetworkIdentifier& source, NetEventCallback&,
                const ll::network::Packet& packet) const override
    {
        auto& pkt = static_cast<const fast_packets::DataExchangePacket&>(packet);
        ServerFRPluginApi::getInstance().dispatchData(pkt.channel, pkt.data);
    }
};
