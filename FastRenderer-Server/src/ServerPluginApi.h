#pragma once
#include <FastRendererAPI.h>
#include "TcpServer.h"
#include <functional>
#include <map>
#include <vector>
#include <algorithm>
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

    // ─── Registration cache: replay to late-joining clients ───
    // This solves the timing gap where FRTest-Native registers GUIs before
    // any client connects. New clients send sync_request to retrieve cached registrations.

    void cacheRegistration(const std::string& jsonMsg) {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_registrationCache.push_back(jsonMsg);
        // Limit cache size to prevent memory leak
        if (m_registrationCache.size() > 500) {
            m_registrationCache.erase(m_registrationCache.begin());
        }
    }

    void removeFromCache(const std::string& type, const std::string& pluginId, const std::string& id) {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_registrationCache.erase(
            std::remove_if(m_registrationCache.begin(), m_registrationCache.end(),
                [&](const std::string& cached) {
                    try {
                        auto j = nlohmann::json::parse(cached);
                        return j.value("type", "") == type
                            && j.value("pluginId", "") == pluginId
                            && j.value("guiId", j.value("bindId", "")) == id;
                    } catch (...) { return false; }
                }),
            m_registrationCache.end()
        );
    }

    // Replay all cached registrations to a specific player/client
    void replayToClient(const std::string& player) {
        if (!m_tcpServer) return;
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        for (const auto& msg : m_registrationCache) {
            m_tcpServer->sendToPlayer(player, msg);
        }
    }

    // Replay all cached registrations to ALL connected clients
    void replayToAll() {
        if (!m_tcpServer) return;
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        for (const auto& msg : m_registrationCache) {
            m_tcpServer->broadcast(msg);
        }
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

        // Cache the registration for late-joining clients
        cacheRegistration(msg.dump());

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

        // Remove from cache
        removeFromCache("gui_register", pluginId, guiId);

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

        // Cache the registration for late-joining clients
        cacheRegistration(msg.dump());

        m_tcpServer->broadcast(msg.dump());
        return true;
    }

    bool unregisterKeybind(const std::string& pluginId,
        const std::string& bindId) override
    {
        if (!m_tcpServer) return false;

        // Remove from cache
        removeFromCache("keybind_register", pluginId, bindId);

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
            } else if (type == "gui_register" || type == "gui_unregister" ||
                       type == "keybind_register" || type == "keybind_unregister") {
                // Relay registration messages from test plugins (e.g. FRTest-Native)
                // to all connected Win/Android clients
                if (m_tcpServer) {
                    std::string target = j.value("targetPlayer", "");
                    if (target.empty()) {
                        m_tcpServer->broadcast(jsonMsg);
                    } else {
                        m_tcpServer->sendToPlayer(target, jsonMsg);
                    }
                }
            } else if (type == "sync_request") {
                // Client requests replay of all cached registrations
                // This solves the timing issue where registrations were sent before client connected
                if (m_tcpServer) {
                    replayToClient(player);
                }
            }
            // Unknown types are silently ignored
        } catch (...) {}
    }

private:
    FrTcpServer* m_tcpServer = nullptr;
    std::string m_lastTargetPlayer;  // cleared after use in registerGui

    std::mutex m_cacheMutex;
    std::vector<std::string> m_registrationCache;  // cached gui_register/keybind_register messages

    std::mutex m_subMutex;
    std::map<std::string, std::vector<std::function<void(const std::string&)>>> m_subscriptions;

    std::mutex m_eventMutex;
    std::map<std::string, std::function<void(const std::string&, const std::string&, const std::string&)>> m_eventCallbacks;
};