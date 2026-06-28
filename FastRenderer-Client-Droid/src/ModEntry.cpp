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
#include "FloatOverlay.h"
#include "ImGuiBackend.h"
#include "TouchInput.h"
#include <net/TcpClient.h>

// ─── Android Floating Menu ───
#include <ui/FRMenuStore.h>
#include <ui/FRAndroidMenu.h>
#include <ui/FloatingActionBar.h>

// ═══════════════════════════════════════════
//  Global state
// ═══════════════════════════════════════════

static JavaVM*      g_javaVM       = nullptr;
static std::string  g_modRootPath;   // plugin root directory
FrTcpClient         g_tcpClient;
FRAndroidMenu       g_menu;
FloatingActionBar   g_actionBar;

// ═══════════════════════════════════════════
//  Logging
// ═══════════════════════════════════════════

inline void logInit(const char* msg) {
    FileLog("FRMod", "%s", msg);
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
    FileLog("FRMod", "onImGuiRender enter");

    try {
        GuiService::renderAll();
    } catch (std::exception& e) {
        FileLog("FRMod", "GuiService exception: %s", e.what());
    }

    if (g_actionBar.isVisible()) {
        try {
            g_actionBar.render(FRMenuStore::getInstance().getPinnedItems());
        } catch (std::exception& e) {
            FileLog("FRMod", "actionbar exception: %s", e.what());
        }
    }

    if (GetFloatOverlay().isOpen()) {
        try {
            g_menu.render();
        } catch (std::exception& e) {
            FileLog("FRMod", "menu exception: %s", e.what());
        }
    }

    FileLog("FRMod", "onImGuiRender leave");
}

// ═══════════════════════════════════════════
//  Plugin class (LeviLaunchroid PL_REGISTER_MOD)
// ═══════════════════════════════════════════

class FRAndroidMod {
public:
    bool load() {
        logInit("load() started");
        logInit("load() completed");
        return true;
    }

    bool enable() {
        logInit("enable() started");

        try {
            logInit("  loadConfig...");
            FRMenuStore::getInstance().loadConfig();
            logInit("  loadConfig done");
        } catch (std::exception& e) {
            logInit((std::string("  loadConfig exception: ") + e.what()).c_str());
        }

        try {
            logInit("  GetPreloaderInput...");
            using GetInputFn = PreloaderInput_Interface* (*)();
            auto* getInput = (GetInputFn)dlsym(RTLD_DEFAULT, "GetPreloaderInput");
            auto* input = getInput ? getInput() : nullptr;
            if (input) {
                input->RegisterTouchCallback(handleTouchEvent);
                input->RegisterKeyEventCallback(handleKeyEvent);
                logInit("  callbacks registered");
            } else {
                logInit("  GetPreloaderInput() returned null");
            }
        } catch (std::exception& e) {
            logInit((std::string("  input exception: ") + e.what()).c_str());
        }

        try {
            logInit("  HookEglSwapBuffers...");
            if (HookEglSwapBuffers()) {
                logInit("  hook OK");
            } else {
                logInit("  hook FAILED");
            }
        } catch (std::exception& e) {
            logInit((std::string("  hook exception: ") + e.what()).c_str());
        }

        try {
            std::string host;
            uint16_t port;
            FRMenuStore::getInstance().getBridgeConfig(host, port);
            if (!host.empty()) {
                connectTcpBridge(host, port);
            }
        } catch (std::exception& e) {
            logInit((std::string("  tcp exception: ") + e.what()).c_str());
        }

        logInit("enable() completed");
        return true;
    }

    bool disable() {
        logInit("disable() called");
        try {
            g_tcpClient.stop();
            UnhookEglSwapBuffers();
            DestroyImGuiBackend();
        } catch (...) {}
        return true;
    }

    bool unload() {
        logInit("unload() called");
        return true;
    }
};

FRAndroidMod frAndroidInstance;
PL_REGISTER_MOD(FRAndroidMod, frAndroidInstance)