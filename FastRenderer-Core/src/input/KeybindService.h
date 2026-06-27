#pragma once
#include <map>
#include <string>
#include <vector>
#include <functional>
#ifdef _WIN32
#include <windows.h>
#else
static inline short GetAsyncKeyState(int) { return 0; }
#endif

namespace KeybindService {

struct Entry {
    int vk;
    std::string id;
    std::string name;
    std::string pluginId;
    std::function<void()> callback;
};

inline std::map<int, Entry> g_keybinds;
inline std::map<int, bool> g_lastKeyState;
inline std::function<bool()> g_activeChecker = nullptr;

inline void setActiveChecker(std::function<bool()> checker) {
    g_activeChecker = std::move(checker);
}

inline bool registerKeybind(const std::string& pluginId, const std::string& id,
    const std::string& name, int vk, std::function<void()> callback)
{
    if (g_keybinds.find(vk) != g_keybinds.end()) {
        return false;
    }
    g_keybinds[vk] = {vk, id, name, pluginId, std::move(callback)};
    return true;
}

inline bool unregisterKeybind(const std::string& pluginId, const std::string& id) {
    for (auto it = g_keybinds.begin(); it != g_keybinds.end(); ) {
        if (it->second.pluginId == pluginId && it->second.id == id) {
            g_lastKeyState.erase(it->first);
            it = g_keybinds.erase(it);
            return true;
        } else {
            ++it;
        }
    }
    return false;
}

inline void unregisterPlugin(const std::string& pluginId) {
    for (auto it = g_keybinds.begin(); it != g_keybinds.end(); ) {
        if (it->second.pluginId == pluginId) {
            g_lastKeyState.erase(it->first);
            it = g_keybinds.erase(it);
        } else {
            ++it;
        }
    }
}

inline bool isRegistered(int vk) {
    return g_keybinds.find(vk) != g_keybinds.end();
}

inline void poll() {
    bool active = g_activeChecker ? g_activeChecker() : false;
    if (!active) return;

    for (auto& [vk, entry] : g_keybinds) {
        bool current = (GetAsyncKeyState(vk) & 0x8000) != 0;
        bool& last = g_lastKeyState[vk];
        if (current && !last && entry.callback) {
            entry.callback();
        }
        last = current;
    }
}

inline std::vector<Entry> getAll() {
    std::vector<Entry> result;
    for (auto& [vk, entry] : g_keybinds) {
        result.push_back(entry);
    }
    return result;
}

inline void execute(const std::string& bindId) {
    for (auto& [vk, entry] : g_keybinds) {
        if (entry.id == bindId && entry.callback) {
            entry.callback();
            return;
        }
    }
}

}
