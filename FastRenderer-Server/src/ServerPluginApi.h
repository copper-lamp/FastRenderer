#pragma once
#include <FastRendererAPI.h>
#include "TcpServer.h"
#include <functional>
#include <map>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

// ─── Server-side PluginApi implementation ───
// Delegates API calls to TCP Server broadcast to all connected clients

class ServerFRPluginApi : public IFastRendererAPI {
public:
    static ServerFRPluginApi& getInstance() {
        static ServerFRPluginApi instance;
        return instance;
    }

    // Inject the TcpServer reference (called from ModEntry::enable)
    void setTcpServer(FrTcpServer* server) {
        m_tcpServer = server;
    }

    bool registerGui(const std::string& pluginId,
        const std::string& guiId, const nlohmann::json& definition) override
    {
        if (!m_tcpServer) return false;

        nlohmann::json msg;
        msg["type"] = "gui_register";
        msg["pluginId"] = pluginId;
        msg["guiId"] = guiId;
        msg["definition"] = definition.dump();

        if (m_lastTargetPlayer.empty()) {
            m_tcpServer->broadcast(msg.dump());
        } else {
            m_tcpServer->sendToPlayer(m_lastTargetPlayer, msg.dump());
            m_lastTargetPlayer.clear();
        }
        return true;
    }

    bool unregisterGui(const std::string& pluginId,
        const std::string& guiId) override
    {
        if (!m_tcpServer) return false;

        nlohmann::json msg;
        msg["type"] = "gui_unregister";
        msg["pluginId"] = pluginId;
        msg["guiId"] = guiId;
        m_tcpServer->broadcast(msg.dump());
        return true;
    }

    bool registerKeybind(const std::string& pluginId,
        const std::string& bindId, const std::string& name, int vkCode) override
    {
        if (!m_tcpServer) return false;

        nlohmann::json msg;
        msg["type"] = "keybind_register";
        msg["pluginId"] = pluginId;
        msg["bindId"] = bindId;
        msg["bindName"] = name;
        msg["vkCode"] = vkCode;
        m_tcpServer->broadcast(msg.dump());
        return true;
    }

    bool unregisterKeybind(const std::string& pluginId,
        const std::string& bindId) override
    {
        if (!m_tcpServer) return false;

        nlohmann::json msg;
        msg["type"] = "keybind_unregister";
        msg["pluginId"] = pluginId;
        msg["bindId"] = bindId;
        m_tcpServer->broadcast(msg.dump());
        return true;
    }

    void publish(const std::string& channel,
        const std::string& data) override
    {
        if (!m_tcpServer) return;

        nlohmann::json msg;
        msg["type"] = "data_exchange";
        msg["channel"] = channel;
        msg["data"] = data;
        m_tcpServer->broadcast(msg.dump());
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
        return m_tcpServer != nullptr && m_tcpServer->isRunning();
    }

    // ─── Internal dispatch methods (called from TCP message handler) ───

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

    // ─── Handle incoming TCP messages from clients ───

    void handleTcpMessage(const std::string& player, const std::string& jsonMsg) {
        try {
            auto j = nlohmann::json::parse(jsonMsg);
            std::string type = j.value("type", "");

            if (type == "gui_event") {
                dispatchGuiEvent(
                    j.value("guiId", ""),
                    j.value("controlId", ""),
                    j.value("eventType", ""),
                    j.value("value", ""));
            } else if (type == "data_exchange") {
                dispatchData(
                    j.value("channel", ""),
                    j.value("data", ""));
            }
            // Unknown types are silently ignored
        } catch (...) {}
    }

private:
    FrTcpServer* m_tcpServer = nullptr;
    std::string m_lastTargetPlayer;  // cleared after use in registerGui

    std::mutex m_subMutex;
    std::map<std::string, std::vector<std::function<void(const std::string&)>>> m_subscriptions;

    std::mutex m_eventMutex;
    std::map<std::string, std::function<void(const std::string&, const std::string&, const std::string&)>> m_eventCallbacks;
};