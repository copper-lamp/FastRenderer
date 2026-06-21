#include "ll/api/mod/NativeMod.h"
#include "ll/api/mod/RegisterHelper.h"
#include <DX11Hook.h>
#include <InputBlocker.h>
#include <gui/GuiService.h>
#include <input/KeybindService.h>
#include <theme/ThemeManager.h>
#include <hotload/HotReloadService.h>
#include <layout/DockLayoutService.h>
#include <fstream>
#include <chrono>
#include <ctime>

namespace fast_renderer {

inline void logInit(const char* msg) {
    std::ofstream out("FastRenderer_Init.txt", std::ios::app);
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    char ts[32] = {};
    tm local = {};
    localtime_s(&local, &tt);
    std::strftime(ts, sizeof(ts), "%H:%M:%S", &local);
    out << "[" << ts << "] " << msg << "\n";
    out.flush();
}

inline bool g_uiActive = false;

class FastRendererImpl {
public:
    static FastRendererImpl& getInstance();
    FastRendererImpl() : mSelf(*ll::mod::NativeMod::current()) {}
    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load() {
        logInit("FastRenderer::load() started");

        DX11Hook::g_imguiInitAttempted = false;

        InputBlocker::setActiveChecker([]() -> bool {
            return g_uiActive;
        });

        KeybindService::setActiveChecker([]() -> bool {
            return g_uiActive;
        });

        DX11Hook::setRenderCallback([]() {
            HotReloadService::poll();
            GuiService::renderAll();
            KeybindService::poll();
        });

        logInit("FastRenderer::load() completed");
        return true;
    }

    bool enable() {
        logInit("FastRenderer::enable() started");

        std::string guiDefsDir = (mSelf.getModDir() / "gui_defs").string();
        HotReloadService::init(guiDefsDir);

        ThemeManager::applyTheme(0);

        InputBlocker::init();

        if (!DX11Hook::init()) {
            logInit("FastRenderer::enable() FAILED - DX11Hook init");
            return false;
        }

        logInit("FastRenderer::enable() completed - DX11 Present Hook installed");
        return true;
    }

    bool disable() {
        logInit("FastRenderer::disable() called");
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
