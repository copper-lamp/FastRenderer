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
#include <net/TcpServer.h>
#include <res/TextureManager.h>
#include <res/AudioManager.h>
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

// ─── 全局通信实例（前置声明，dispatchTcpMessage 的 lambda 中使用）───
static FrTcpServer g_embeddedServer;
FrTcpClient g_tcpClient;

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

        logInit(("  TCP recv type=" + type + " len=" + std::to_string(jsonMsg.size())).c_str());

        if (type == "gui_register") {
            std::string guiId = j.value("guiId", "");
            GuiService::registerGui(
                j.value("pluginId", ""),
                guiId,
                j.value("definition", ""));
            logInit(("  TCP gui_register: " + guiId).c_str());
        }
        else if (type == "gui_unregister") {
            std::string guiId = j.value("guiId", "");
            GuiService::unregisterGui(
                j.value("pluginId", ""),
                guiId);
            logInit(("  TCP gui_unregister: " + guiId).c_str());
        }
        else if (type == "keybind_register") {
            std::string pluginId = j.value("pluginId", "");
            std::string bindId = j.value("bindId", "");
            std::string bindName = j.value("bindName", "");
            int vkCode = j.value("vkCode", 0);
            // Register with a TCP callback — when key is pressed, send keybind_event
            KeybindService::registerKeybind(pluginId, bindId, bindName, vkCode,
                [pluginId, bindId, vkCode]() {
                    nlohmann::json evt;
                    evt["type"] = "gui_event";
                    evt["guiId"] = "keybind";
                    evt["controlId"] = bindId;
                    evt["eventType"] = "keydown";
                    evt["value"] = std::to_string(vkCode);
                    evt["pluginId"] = pluginId;
                    std::string payload = evt.dump();
                    if (g_embeddedServer.isRunning()) {
                        g_embeddedServer.broadcast(payload);
                    }
                    if (g_tcpClient.isConnected()) {
                        g_tcpClient.send(payload);
                    }
                    logInit(("  keybind fired: " + bindId).c_str());
                });
            logInit(("  TCP keybind_register: " + bindId + " vk=0x" + std::to_string(vkCode)).c_str());
        }
        else if (type == "keybind_unregister") {
            KeybindService::unregisterKeybind(
                j.value("pluginId", ""),
                j.value("bindId", ""));
            logInit(("  TCP keybind_unregister: " + j.value("bindId", "")).c_str());
        }
        else if (type == "data_exchange") {
            logInit(("  TCP data_exchange: channel=" + j.value("channel", "")).c_str());
        }
        else {
            logInit(("  TCP unknown type: " + type).c_str());
        }
    } catch (std::exception& e) {
        logInit(("  TCP dispatch exception: " + std::string(e.what()) + " raw=" + jsonMsg.substr(0, 120)).c_str());
    }
}

// ─── 发送 gui_event（双通道：内嵌Server广播 + 外部Client发送）───

inline void sendGuiEvent(const std::string& guiId, const std::string& controlId,
    const std::string& eventType, const std::string& value)
{
    nlohmann::json msg;
    msg["type"] = "gui_event";
    msg["guiId"] = guiId;
    msg["controlId"] = controlId;
    msg["eventType"] = eventType;
    msg["value"] = value;
    std::string payload = msg.dump();

    // 通道1：内嵌 Server 广播（给同进程的 FRTest-Native）
    if (g_embeddedServer.isRunning()) {
        g_embeddedServer.broadcast(payload);
    }
    // 通道2：外部 TCP Client 发送（给外部 FastRenderer-Server）
    if (g_tcpClient.isConnected()) {
        g_tcpClient.send(payload);
    }
}

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

        // ─── Hook GuiEvent to send via TCP ───
        // When user clicks a button in any GUI, fire TCP gui_event
        JsonGuiRenderer::setEventCallback([](const std::string& eventName) {
            // eventName = onClick action (e.g. "calc_1", "dash_refresh")
            // We need guiId from the current rendering context
            std::string guiId = JsonGuiRenderer::g_currentGuiId;
            if (guiId.empty() || eventName.empty()) return;

            sendGuiEvent(guiId, eventName, "click", "");
            logInit(("  GUI event: gui=" + guiId + " action=" + eventName).c_str());
        });

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

        // ════════════════════════════════════════════════════════
        //  TCP 通信层（双通道共存模式）
        //  通道A：内嵌 TCP Server — 供同进程 FRTest-Native 直连
        //  通道B：TCP Client — 连接外部 FastRenderer-Server
        //  两者独立运行，互不干扰
        // ════════════════════════════════════════════════════════

        // ── 通道A：启动内嵌 TCP Server ──
        // 先尝试 12345，被占用则自动递增端口（避免与外部 Server 冲突）
        uint16_t embeddedPort = 12345;
        while (embeddedPort <= 12350 && !g_embeddedServer.start(embeddedPort)) {
            embeddedPort++;
        }
        if (g_embeddedServer.isRunning()) {
            logInit(("  [5a/6] Embedded TCP Server started on port " + std::to_string(embeddedPort)).c_str());
            g_embeddedServer.setMessageHandler([](const std::string& /*player*/, const std::string& jsonMsg) {
                dispatchTcpMessage(jsonMsg);
            });
        } else {
            logInit("  [5a/6] Embedded TCP Server could not start on any port (12345-12350)");
        }

        // ── 通道B：连接外部 FastRenderer-Server ──
        g_tcpClient.setMessageCallback(dispatchTcpMessage);
        g_tcpClient.setStatusCallback([](bool connected) {
            logInit(connected ? "  TCP client connected" : "  TCP client disconnected");
            if (connected) {
                nlohmann::json idMsg;
                idMsg["type"] = "identify";
                idMsg["player"] = "local";
                idMsg["platform"] = "win";
                g_tcpClient.send(idMsg.dump());

                nlohmann::json syncMsg;
                syncMsg["type"] = "sync_request";
                syncMsg["player"] = "local";
                g_tcpClient.send(syncMsg.dump());
                logInit("  TCP reconnect: identify + sync_request sent");
            }
        });
        g_tcpClient.enableAutoReconnect(3000, 5);

        if (g_tcpClient.connect("127.0.0.1", 12345)) {
            logInit("  [5b/6] TcpClient connected to 127.0.0.1:12345");

            nlohmann::json idMsg;
            idMsg["type"] = "identify";
            idMsg["player"] = "local";
            idMsg["platform"] = "win";
            g_tcpClient.send(idMsg.dump());

            nlohmann::json syncMsg;
            syncMsg["type"] = "sync_request";
            syncMsg["player"] = "local";
            g_tcpClient.send(syncMsg.dump());
            logInit("  [5b2/6] sync_request sent to external server");

            g_tcpClient.startReceiveThread();
            logInit("  [5b3/6] TcpClient receive thread started");
        } else {
            logInit("  [5b/6] TcpClient connect FAILED (will auto-reconnect)");
            g_tcpClient.startReceiveThread();
        }
        logInit("  [5/6] TCP layer init OK");

        if (!DX11Hook::init()) {
            logInit("  [6/6] DX11Hook init FAILED (will retry via UIRenderHook)");
        } else {
            logInit("  [6/6] DX11Hook init OK");
            // Initialize TextureManager with D3D device
            if (DX11Hook::g_pd3dDevice && DX11Hook::g_pd3dDeviceContext) {
                TextureManager::init(DX11Hook::g_pd3dDevice, DX11Hook::g_pd3dDeviceContext);
                AudioManager::init(modDir + "/audio_cache");
                logInit("  [6b/6] Resource managers initialized");
            }
        }

        // Register deferred DX11 init via UIRenderContext hook
        UIRenderHook::hook();
        logInit("  [7/7] UIRenderHook registered");

        logInit("FastRenderer::enable() completed");
        return true;
    }

    bool disable() {
        logInit("FastRenderer::disable() called");
        if (g_embeddedServer.isRunning()) {
            g_embeddedServer.stop();
        }
        if (g_tcpClient.isConnected()) {
            g_tcpClient.stop();
        }
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