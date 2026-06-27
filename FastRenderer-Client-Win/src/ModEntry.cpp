#include "ll/api/mod/NativeMod.h"
#include "ll/api/mod/RegisterHelper.h"
#include <DX11Hook.h>
#include <InputBlocker.h>
#include <gui/GuiService.h>
#include <gui/JsonGuiRenderer.h>
#include <input/KeybindService.h>
#include <theme/ThemeManager.h>
#include <hotload/HotReloadService.h>
#include <layout/DockLayoutService.h>
#include <VerificationUI.h>
#include <net/TcpClient.h>
#include <UIRenderHook.h>
#include <MinHook.h>
#include <windows.h>
#include <chrono>
#include <ctime>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fast_renderer {

inline void logInit(const char* msg) {
    FILE* flog = nullptr;
    if (fopen_s(&flog, "FastRenderer_Init.txt", "a") == 0 && flog) {
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        tm local = {};
        localtime_s(&local, &tt);
        fprintf(flog, "[%02d:%02d:%02d] %s\n", local.tm_hour, local.tm_min, local.tm_sec, msg);
        fclose(flog);
    }
}

inline bool g_lastNState = false;

// ─── TCP Client message dispatcher ───
// Routes incoming TCP messages to the appropriate service

inline void dispatchTcpMessage(const std::string& jsonMsg) {
    try {
        auto j = nlohmann::json::parse(jsonMsg);
        std::string type = j.value("type", "");

        if (type == "gui_register") {
            GuiService::registerGui(
                j.value("pluginId", ""),
                j.value("guiId", ""),
                j.value("definition", ""));
            logInit(("  TCP gui_register: " + j.value("guiId", "")).c_str());
        }
        else if (type == "gui_unregister") {
            GuiService::unregisterGui(
                j.value("pluginId", ""),
                j.value("guiId", ""));
            logInit(("  TCP gui_unregister: " + j.value("guiId", "")).c_str());
        }
        else if (type == "keybind_register") {
            KeybindService::registerKeybind(
                j.value("pluginId", ""),
                j.value("bindId", ""),
                j.value("bindName", ""),
                j.value("vkCode", 0),
                nullptr);
        }
        else if (type == "keybind_unregister") {
            KeybindService::unregisterKeybind(
                j.value("pluginId", ""),
                j.value("bindId", ""));
        }
        else if (type == "data_exchange") {
            // data_exchange from Server is handled silently
            // (Client side data_exchange subscription not yet implemented)
        }
        // Unknown types are silently ignored
    } catch (std::exception& e) {
        logInit(("  TCP dispatch exception: " + std::string(e.what())).c_str());
    }
}

// ─── Send a gui_event to the TCP Server ───

inline void sendGuiEvent(const std::string& guiId, const std::string& controlId,
    const std::string& eventType, const std::string& value)
{
    extern FrTcpClient g_tcpClient;
    if (!g_tcpClient.isConnected()) return;

    nlohmann::json msg;
    msg["type"] = "gui_event";
    msg["guiId"] = guiId;
    msg["controlId"] = controlId;
    msg["eventType"] = eventType;
    msg["value"] = value;
    g_tcpClient.send(msg.dump());
}

// ─── Global TCP Client instance ───
FrTcpClient g_tcpClient;

class FastRendererImpl {
public:
    static FastRendererImpl& getInstance();
    FastRendererImpl() : mSelf(*ll::mod::NativeMod::current()) {}
    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load() {
        logInit("FastRenderer::load() started");

        DX11Hook::g_imguiInitAttempted = false;

        InputBlocker::setActiveChecker([]() -> bool {
            return VerificationUI::isActive();
        });

        KeybindService::setActiveChecker([]() -> bool {
            return true;
        });

        // ─── Hook GuiEvent to also send via TCP ───
        // The JsonGuiRenderer::fireEvent callback will send to TCP server
        // This is handled via the existing KeybindService/guiEvent pipeline

        DX11Hook::setRenderCallback([]() {
            ThemeManager::applyPendingTheme();

            bool nDown = (GetAsyncKeyState(0x4E) & 0x8000) != 0;
            if (nDown && !g_lastNState) {
                VerificationUI::g_showConsole = !VerificationUI::g_showConsole;
            }
            g_lastNState = nDown;

            HotReloadService::poll();
            GuiService::renderAll();
            KeybindService::poll();
            VerificationUI::renderConsole();
        });

        logInit("FastRenderer::load() completed");
        return true;
    }

    bool enable() {
        logInit("FastRenderer::enable() started");

        MH_Initialize();

        std::string modDir = mSelf.getModDir().string();
        logInit(("  [1/6] modDir: " + modDir).c_str());

        std::string guiDefsDir = modDir + "/gui_defs";
        HotReloadService::init(guiDefsDir);
        logInit("  [2/6] HotReload init OK");

        // [3/6] BridgeService removed — replaced by TCP bridge (step 5)

        ThemeManager::applyTheme(0);
        logInit("  [4/6] Theme applied");

        // Load keybinds from keybinds/ directory
        std::string kbDir = modDir + "/keybinds";
        if (std::filesystem::exists(kbDir)) {
            int kbLoaded = 0;
            for (auto& entry : std::filesystem::directory_iterator(kbDir)) {
                if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;
                try {
                    std::ifstream f(entry.path());
                    nlohmann::json j; f >> j;
                    auto& binds = j["keybinds"];
                    for (auto& kb : binds) {
                        KeybindService::registerKeybind(
                            kb.value("pluginId", ""),
                            kb.value("bindId", ""),
                            kb.value("bindName", ""),
                            kb.value("vkCode", 0),
                            nullptr
                        );
                        kbLoaded++;
                    }
                } catch (...) {}
            }
            logInit(("  [4b/6] Keybinds loaded from keybinds/: " + std::to_string(kbLoaded)).c_str());
        } else {
            std::filesystem::create_directories(kbDir);
            logInit("  [4b/6] keybinds/ directory created (empty)");
        }

        InputBlocker::init();
        logInit("  [5/6] InputBlocker init OK");

        // ─── TCP Client connection (replaces PacketBridge) ───
        // Connect to localhost:12345 where BDS's FR-Server is listening
        // In the future, this could be configured via config file
        g_tcpClient.setMessageCallback(dispatchTcpMessage);
        g_tcpClient.setStatusCallback([](bool connected) {
            logInit(connected ? "  TCP Client connected" : "  TCP Client disconnected");
        });
        g_tcpClient.enableAutoReconnect(3000, 5);

        if (g_tcpClient.connect("127.0.0.1", 12345)) {
            logInit("  [5a/6] TcpClient connected to 127.0.0.1:12345");

            // Send identify message
            nlohmann::json idMsg;
            idMsg["type"] = "identify";
            idMsg["player"] = "local";  // Will be updated when we know the actual player name
            idMsg["platform"] = "win";
            g_tcpClient.send(idMsg.dump());

            g_tcpClient.startReceiveThread();
            logInit("  [5b/6] TcpClient receive thread started");
        } else {
            logInit("  [5a/6] TcpClient connect FAILED (will auto-reconnect)");
            g_tcpClient.startReceiveThread();
        }
        logInit("  [5/6] TcpClient init OK");

        if (!DX11Hook::init()) {
            logInit("  [6/6] DX11Hook init FAILED (will retry via UIRenderHook)");
        } else {
            logInit("  [6/6] DX11Hook init OK");
        }

        // Register deferred DX11 init via UIRenderContext hook
        UIRenderHook::hook();
        logInit("  [7/7] UIRenderHook registered");

        logInit("FastRenderer::enable() completed");
        return true;
    }

    bool disable() {
        logInit("FastRenderer::disable() called");
        g_tcpClient.stop();
        HotReloadService::shutdown();
        return true;
    }

private:
    ll::mod::NativeMod& mSelf;
};

FastRendererImpl& FastRendererImpl::getInstance() {
    static FastRendererImpl instance;
    return instance;
}

}

LL_REGISTER_MOD(fast_renderer::FastRendererImpl, fast_renderer::FastRendererImpl::getInstance());