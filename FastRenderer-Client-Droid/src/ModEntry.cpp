#include <cstdio>
#include <chrono>
#include <ctime>
#include <thread>
#include <string>
#include <fstream>
#include <filesystem>
#include <dlfcn.h>

// ─── LeviLaunchroid Plugin API ───
#include <pl/cpp/mod/RegisterHelper.hpp>
#include <pl/c/PreloaderInput.h>

// ─── FastRenderer Core (reused from Desktop) ───
#include <gui/GuiService.h>
#include <gui/JsonGuiRenderer.h>
#include <input/KeybindService.h>
#include <theme/ThemeManager.h>

// ─── Android-specific modules ───
#include "EGLHook.h"
#include "ImGuiBackend.h"
#include "TouchInput.h"
#include <net/TcpClient.h>

// ─── Android Floating Menu ───
#include <ui/FRMenuStore.h>
#include <ui/FloatingTrigger.h>
#include <ui/FRAndroidMenu.h>
#include <ui/FloatingActionBar.h>

// ═══════════════════════════════════════════
//  Global state
// ═══════════════════════════════════════════

static JavaVM*      g_javaVM       = nullptr;
static std::string  g_modRootPath;   // plugin root directory
FrTcpClient         g_tcpClient;
FloatingTrigger     g_trigger;
FRAndroidMenu       g_menu;
FloatingActionBar   g_actionBar;

// ═══════════════════════════════════════════
//  Logging
// ═══════════════════════════════════════════

inline void logInit(const char* msg) {
    std::string path = g_modRootPath + "/data/FastRenderer_Android_Log.txt";
    std::ofstream out(path, std::ios::app);
    if (!out.is_open()) return;
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    char ts[32] = {};
    std::strftime(ts, sizeof(ts), "%H:%M:%S", localtime(&tt));
    out << "[" << ts << "] " << msg << "\n";
    out.flush();
}

// ═══════════════════════════════════════════
//  TCP message dispatcher
// ═══════════════════════════════════════════

void dispatchTcpMessage(const std::string& jsonMsg) {
    try {
        auto j = nlohmann::json::parse(jsonMsg);
        std::string type = j.value("type", "");

        if (type == "gui_register") {
            GuiService::registerGui(
                j.value("pluginId", ""),
                j.value("guiId", ""),
                j.value("definition", ""));
            logInit(("TCP gui_register: " + j.value("guiId", "")).c_str());
        }
        else if (type == "gui_unregister") {
            GuiService::unregisterGui(
                j.value("pluginId", ""),
                j.value("guiId", ""));
            logInit(("TCP gui_unregister: " + j.value("guiId", "")).c_str());
        }
        else if (type == "keybind_register") {
            FRMenuStore::getInstance().addOrUpdateItem(
                j.value("pluginId", ""),
                j.value("bindId", ""),
                j.value("bindName", ""),
                j.value("vkCode", 0));
        }
        else if (type == "keybind_unregister") {
            FRMenuStore::getInstance().removeItem(
                j.value("pluginId", ""),
                j.value("bindId", ""));
        }
        else if (type == "data_exchange") {
            // Not yet implemented on Android
        }
    } catch (std::exception& e) {
        logInit(("TCP dispatch exception: " + std::string(e.what())).c_str());
    }
}

// ═══════════════════════════════════════════
//  Send gui_event to TCP server
// ═══════════════════════════════════════════

void sendGuiEvent(const std::string& guiId, const std::string& controlId,
                  const std::string& eventType, const std::string& value)
{
    if (!g_tcpClient.isConnected()) return;
    nlohmann::json msg;
    msg["type"]     = "gui_event";
    msg["guiId"]    = guiId;
    msg["controlId"]= controlId;
    msg["eventType"]= eventType;
    msg["value"]    = value;
    g_tcpClient.send(msg.dump());
}

// ═══════════════════════════════════════════
//  Connect to TCP bridge server
// ═══════════════════════════════════════════

bool connectTcpBridge(const std::string& host, uint16_t port) {
    g_tcpClient.disconnect();

    g_tcpClient.setMessageCallback(dispatchTcpMessage);
    g_tcpClient.setStatusCallback([](bool connected) {
        logInit(connected ? "TCP Client connected" : "TCP Client disconnected");
    });
    g_tcpClient.enableAutoReconnect(5000, 10);

    if (g_tcpClient.connect(host, port)) {
        // Send identify
        nlohmann::json idMsg;
        idMsg["type"]     = "identify";
        idMsg["player"]   = "android_player";
        idMsg["platform"] = "android";
        g_tcpClient.send(idMsg.dump());
        g_tcpClient.startReceiveThread();
        logInit(("TCP Client connected to " + host + ":" + std::to_string(port)).c_str());
        return true;
    }
    logInit(("TCP Client connect FAILED to " + host + ":" + std::to_string(port)).c_str());
    g_tcpClient.startReceiveThread();  // will auto-reconnect
    return false;
}

// ═══════════════════════════════════════════
//  ImGui render callback (called from EGLHook::hkEglSwapBuffers)
// ═══════════════════════════════════════════

void onImGuiRender() {
    // 1. Server-pushed GUIs (reused Core logic)
    GuiService::renderAll();

    // 2. Floating action bar (user-pinned keybinds)
    if (g_actionBar.isVisible()) {
        g_actionBar.render(FRMenuStore::getInstance().getPinnedItems());
    }

    // 3. Floating trigger icon (always visible)
    g_trigger.render();

    // 4. Main menu (only visible when trigger is open)
    if (g_trigger.isOpen()) {
        g_menu.render();
    }
}

// ═══════════════════════════════════════════
//  Plugin class (LeviLaunchroid PL_REGISTER_MOD)
// ═══════════════════════════════════════════

class FRAndroidMod {
public:
    bool load() {
        logInit("FRAndroidMod::load() started");
        // PLMod_Load gives us the mod_info with root path
        // Saved by the PL_REGISTER_MOD macro infrastructure
        logInit("FRAndroidMod::load() completed");
        return true;
    }

    bool enable() {
        logInit("FRAndroidMod::enable() started");

        // 1. Load config (bridge address, trigger position, pinned binds)
        FRMenuStore::getInstance().loadConfig();

        // 2. Register touch input callback — resolved via dlsym to avoid link-time dependency
        using GetInputFn = PreloaderInput_Interface* (*)();
        auto* getInput = (GetInputFn)dlsym(RTLD_DEFAULT, "GetPreloaderInput");
        auto* input = getInput ? getInput() : nullptr;
        if (input) {
            input->RegisterTouchCallback(handleTouchEvent);
            input->RegisterKeyEventCallback(handleKeyEvent);
            logInit("  PreloaderInput callbacks registered");
        } else {
            logInit("  WARNING: GetPreloaderInput() returned null");
        }

        // 3. Hook eglSwapBuffers for ImGui rendering
        if (HookEglSwapBuffers()) {
            logInit("  eglSwapBuffers hook OK");
        } else {
            logInit("  WARNING: eglSwapBuffers hook FAILED");
        }

        // 4. Connect to TCP bridge (from config or default)
        std::string host;
        uint16_t port;
        FRMenuStore::getInstance().getBridgeConfig(host, port);
        if (!host.empty()) {
            connectTcpBridge(host, port);
        }

        logInit("FRAndroidMod::enable() completed");
        return true;
    }

    bool disable() {
        logInit("FRAndroidMod::disable() called");
        g_tcpClient.stop();
        UnhookEglSwapBuffers();
        DestroyImGuiBackend();
        return true;
    }

    bool unload() {
        logInit("FRAndroidMod::unload() called");
        return true;
    }
};

FRAndroidMod frAndroidInstance;
PL_REGISTER_MOD(FRAndroidMod, frAndroidInstance)