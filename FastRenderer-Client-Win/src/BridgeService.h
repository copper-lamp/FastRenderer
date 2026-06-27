#pragma once
// ═══════════════════════════════════════════════════════════
//  DEPRECATED — File-based bridge protocol.
//  Replaced by TCP Bridge (FrTcpClient + FrTcpServer).
//  Kept for reference; no longer used in ModEntry.
//  See docs/specs/FR-TCP桥接通信协议.md
// ═══════════════════════════════════════════════════════════
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <gui/GuiService.h>
#include <input/KeybindService.h>

namespace BridgeService {

using json = nlohmann::json;

inline std::string g_bridgeDir;
inline std::atomic<bool> g_running{false};
inline std::thread g_thread;

inline void logMsg(const char* msg) {
    FILE* flog = nullptr;
    if (fopen_s(&flog, "FastRenderer_Bridge.txt", "a") == 0 && flog) {
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        tm local = {};
        localtime_s(&local, &tt);
        fprintf(flog, "[%02d:%02d:%02d] %s\n", local.tm_hour, local.tm_min, local.tm_sec, msg);
        fclose(flog);
    }
}

inline std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return {};
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

inline void deleteFile(const std::string& path) {
    std::filesystem::remove(path);
}

// ─── Register a single GUI from parsed JSON ───

inline bool tryRegisterGui(const json& j, const std::string& pluginId, const std::string& guiId) {
    if (j.contains("definition") && j["definition"].is_string()) {
        std::string defStr = j["definition"].get<std::string>();
        logMsg(("  tryRegisterGui: def string, size=" + std::to_string(defStr.size())).c_str());
        if (defStr.size() < 200) logMsg(("    content: " + defStr).c_str());
        else logMsg(("    content[0..199]: " + defStr.substr(0, 200)).c_str());
        GuiService::registerGui(pluginId, guiId, defStr);
        return true;
    }
    if (j.contains("definition")) {
        std::string defStr = j["definition"].dump();
        logMsg(("  tryRegisterGui: def object, size=" + std::to_string(defStr.size())).c_str());
        GuiService::registerGui(pluginId, guiId, defStr);
        return true;
    }
    if (j.contains("root") && j["root"].is_object()) {
        logMsg("  tryRegisterGui: root object format");
        GuiService::registerGui(pluginId, guiId, j["root"].dump());
        return true;
    }
    if (j.contains("type")) {
        logMsg(("  tryRegisterGui: top-level type=" + j["type"].get<std::string>()).c_str());
        GuiService::registerGui(pluginId, guiId, j.dump());
        return true;
    }
    logMsg(("  tryRegisterGui: NO MATCH, keys: " + std::to_string(j.size())).c_str());
    return false;
}

// ─── Process a single GUI entry (object) ───

inline void processGuiEntry(const json& j, bool deleteAfter = true) {
    std::string pluginId = j.value("pluginId", "bridge");
    std::string guiId = j.value("guiId", j.value("id", ""));
    if (guiId.empty()) return;

    logMsg(("  entry: pluginId=" + pluginId + " guiId=" + guiId).c_str());
    tryRegisterGui(j, pluginId, guiId);
}

// ─── Process gui_pending.json (object or array) ───

inline void processGuiPending(const std::string& path) {
    logMsg(("processGuiPending: " + path).c_str());
    auto content = readFile(path);
    if (content.empty()) { deleteFile(path); return; }

    try {
        auto j = json::parse(content);

        if (j.is_array()) {
            logMsg(("  batch: " + std::to_string(j.size()) + " entries").c_str());
            for (auto& entry : j) {
                processGuiEntry(entry);
            }
        } else {
            processGuiEntry(j);
        }

        int count = (int)GuiService::getDefinitions().size();
        logMsg(("  done, GuiService has " + std::to_string(count) + " defs").c_str());
    } catch (std::exception& e) {
        logMsg(("  EXCEPTION: " + std::string(e.what())).c_str());
    }
    deleteFile(path);
}

// ─── Process gui_unreg.json (object or array) ───

inline void processGuiUnreg(const std::string& path) {
    auto content = readFile(path);
    if (content.empty()) { deleteFile(path); return; }
    try {
        auto j = json::parse(content);
        auto entries = j.is_array() ? j : json::array({j});
        for (auto& entry : entries) {
            std::string pluginId = entry.value("pluginId", "bridge");
            std::string guiId = entry.value("guiId", entry.value("id", ""));
            if (!guiId.empty()) {
                GuiService::unregisterGui(pluginId, guiId);
                logMsg(("  unreg: " + guiId).c_str());
            }
        }
    } catch (...) {}
    deleteFile(path);
}

// ─── Process keybind_pending.json (object or array) ───

inline void processKeybindPending(const std::string& path) {
    auto content = readFile(path);
    if (content.empty()) { deleteFile(path); return; }
    try {
        auto j = json::parse(content);
        auto entries = j.is_array() ? j : json::array({j});
        for (auto& entry : entries) {
            std::string pluginId = entry.value("pluginId", "bridge");
            std::string bindId = entry.value("bindId", "");
            std::string name = entry.value("name", entry.value("bindName", ""));
            int vk = entry.value("vk", 0);
            if (!bindId.empty() && vk > 0) {
                KeybindService::registerKeybind(pluginId, bindId, name, vk, nullptr);
                logMsg(("  keybind: " + bindId + " vk=" + std::to_string(vk)).c_str());
            }
        }
    } catch (std::exception& e) {
        logMsg(("  EXCEPTION: " + std::string(e.what())).c_str());
    }
    deleteFile(path);
}

inline void pollOnce() {
    std::string guiPending = g_bridgeDir + "/gui_pending.json";
    std::string guiUnreg = g_bridgeDir + "/gui_unreg.json";
    std::string keyPending = g_bridgeDir + "/keybind_pending.json";

    if (std::filesystem::exists(guiPending)) processGuiPending(guiPending);
    if (std::filesystem::exists(guiUnreg)) processGuiUnreg(guiUnreg);
    if (std::filesystem::exists(keyPending)) processKeybindPending(keyPending);
}

inline void bridgeLoop() {
    logMsg("bridgeLoop: started");
    int count = 0;
    while (g_running) {
        pollOnce();
        if (++count % 50 == 0) {
            int defCount = (int)GuiService::getDefinitions().size();
            logMsg(("bridgeLoop: alive, GuiService has " + std::to_string(defCount) + " defs").c_str());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    logMsg("bridgeLoop: ended");
}

inline void init(const std::string& modDir) {
    g_bridgeDir = modDir + "/gui_bridge";
    logMsg(("init: bridgeDir=" + g_bridgeDir).c_str());
    std::filesystem::create_directories(g_bridgeDir);
    logMsg("init: starting bridge thread");
    g_running = true;
    g_thread = std::thread(bridgeLoop);
}

inline void shutdown() {
    g_running = false;
    if (g_thread.joinable()) g_thread.join();
}

}
