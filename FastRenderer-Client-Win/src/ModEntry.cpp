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
#include <PacketBridge.h>
#include <BridgeService.h>
#include <MinHook.h>
#include <windows.h>
#include <chrono>
#include <ctime>
#include <cstdio>

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
            return VerificationUI::isActive();
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
            if (VerificationUI::isActive()) {
                KeybindService::poll();
            }
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

        BridgeService::init(modDir);
        logInit("  [3/6] BridgeService init OK");

        ThemeManager::applyTheme(0);
        logInit("  [4/6] Theme applied");

        InputBlocker::init();
        logInit("  [5/6] InputBlocker init OK");

        PacketBridge::init();
        logInit("  [5/6] PacketBridge init OK");

        if (!DX11Hook::init()) {
            logInit("  [6/6] DX11Hook init FAILED (will retry on first Present)");
        } else {
            logInit("  [6/6] DX11Hook init OK");
        }

        logInit("FastRenderer::enable() completed");
        return true;
    }

    bool disable() {
        logInit("FastRenderer::disable() called");
        BridgeService::shutdown();
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
