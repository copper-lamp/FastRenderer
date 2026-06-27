#pragma once
#include <ll/api/memory/Hook.h>
#include <mc/client/renderer/screen/MinecraftUIRenderContext.h>
#include <DX11Hook.h>

LL_TYPE_INSTANCE_HOOK(
    UIRenderHook,
    ll::memory::HookPriority::Normal,
    MinecraftUIRenderContext,
    &MinecraftUIRenderContext::$flushText,
    void,
    float deltaTime,
    std::optional<float> obfuscateSwitchTime
) {
    origin(deltaTime, obfuscateSwitchTime);

    static bool s_hooked = false;
    if (!s_hooked) {
        s_hooked = true;
        DX11Hook::init();
    }
}
