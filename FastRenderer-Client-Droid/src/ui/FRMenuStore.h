#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <mutex>
#include <fstream>
#include <nlohmann/json.hpp>

// ═══════════════════════════════════════════
//  Menu item + state management
// ═══════════════════════════════════════════

struct MenuItem {
    std::string pluginId;
    std::string bindId;
    std::string name;
    int vkCode = 0;
    bool pinned = false;  // user has pinned to action bar
};

class FRMenuStore {
public:
    static FRMenuStore& getInstance() {
        static FRMenuStore instance;
        return instance;
    }

    // ─── Called from TCP dispatch (keybind_register) ───
    void addOrUpdateItem(const std::string& pluginId,
                         const std::string& bindId,
                         const std::string& name,
                         int vkCode)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& item : m_items) {
            if (item.bindId == bindId && item.pluginId == pluginId) {
                item.name  = name;   // update name in case it changed
                item.vkCode = vkCode;
                return;
            }
        }
        // New item: check if it was previously pinned (restore from config)
        bool wasPinned = m_pinnedBinds.count(bindId) > 0;
        m_items.push_back({pluginId, bindId, name, vkCode, wasPinned});
    }

    void removeItem(const std::string& pluginId, const std::string& bindId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::remove_if(m_items.begin(), m_items.end(),
            [&](const MenuItem& item) {
                return item.pluginId == pluginId && item.bindId == bindId;
            });
        if (it != m_items.end()) {
            m_pinnedBinds.erase(bindId);
            m_items.erase(it, m_items.end());
        }
    }

    // ─── User actions ───
    void togglePin(const std::string& bindId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& item : m_items) {
            if (item.bindId == bindId) {
                item.pinned = !item.pinned;
                if (item.pinned)
                    m_pinnedBinds.insert(bindId);
                else
                    m_pinnedBinds.erase(bindId);
                return;
            }
        }
    }

    bool isPinned(const std::string& bindId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& item : m_items) {
            if (item.bindId == bindId) return item.pinned;
        }
        return false;
    }

    std::vector<MenuItem> getAllItems() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_items;
    }

    std::vector<MenuItem> getPinnedItems() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<MenuItem> result;
        for (auto& item : m_items) {
            if (item.pinned) result.push_back(item);
        }
        return result;
    }

    int count() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return (int)m_items.size();
    }

    // ─── Bridge config ───
    void getBridgeConfig(std::string& host, uint16_t& port) {
        host = m_serverHost;
        port = m_serverPort;
    }

    void setBridgeConfig(const std::string& host, uint16_t port) {
        m_serverHost = host;
        m_serverPort = port;
        saveConfig();
    }

    // ─── Trigger position ───
    void getTriggerPos(float& x, float& y) { x = m_triggerX; y = m_triggerY; }
    void setTriggerPos(float x, float y)    { m_triggerX = x; m_triggerY = y; }

    // ─── Action bar visibility ───
    bool isActionBarVisible() const { return m_actionBarVisible; }
    void setActionBarVisible(bool v) { m_actionBarVisible = v; }

    // ─── Persistence ───
    void loadConfig() {
        // TODO: load from actual config path (requires mod root)
        // For now, use defaults
        m_serverHost  = "127.0.0.1";
        m_serverPort  = 12345;
        m_triggerX    = -1.0f;  // Right-center default (set in render)
        m_triggerY    = -1.0f;
        m_actionBarVisible = true;
    }

    void saveConfig() {
        // TODO: write to config file
    }

private:
    FRMenuStore() = default;

    std::mutex m_mutex;
    std::vector<MenuItem> m_items;
    std::set<std::string> m_pinnedBinds;

    std::string m_serverHost = "127.0.0.1";
    uint16_t    m_serverPort = 12345;
    float       m_triggerX   = -1.0f;
    float       m_triggerY   = -1.0f;
    bool        m_actionBarVisible = true;
};