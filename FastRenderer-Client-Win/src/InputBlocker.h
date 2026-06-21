#pragma once
#include <windows.h>
#include <MinHook.h>
#include <vector>
#include <functional>

namespace InputBlocker {

inline std::vector<int> g_whitelist = {0x4B, 0x4E, 0x4D, 0x55, VK_F11}; // K, N, M, U, F11
inline std::function<bool()> g_isActive = nullptr;

using GetAsyncKeyState_t = SHORT(WINAPI*)(int);
using GetKeyState_t = SHORT(WINAPI*)(int);

inline GetAsyncKeyState_t oGetAsyncKeyState = nullptr;
inline GetKeyState_t oGetKeyState = nullptr;

inline void setActiveChecker(std::function<bool()> checker) {
    g_isActive = std::move(checker);
}

inline void addToWhitelist(int vk) {
    for (auto k : g_whitelist) {
        if (k == vk) return;
    }
    g_whitelist.push_back(vk);
}

inline bool isWhitelisted(int vk) {
    for (auto k : g_whitelist) {
        if (k == vk) return true;
    }
    return false;
}

inline SHORT WINAPI hkGetAsyncKeyState(int vKey) {
    bool active = g_isActive ? g_isActive() : false;

    if (active && !isWhitelisted(vKey)) {
        return 0;
    }

    if (oGetAsyncKeyState) return oGetAsyncKeyState(vKey);
    return 0;
}

inline SHORT WINAPI hkGetKeyState(int vKey) {
    bool active = g_isActive ? g_isActive() : false;

    if (active && !isWhitelisted(vKey)) {
        return 0;
    }

    if (oGetKeyState) return oGetKeyState(vKey);
    return 0;
}

inline bool init() {
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (!hUser32) return false;

    void* pGetAsyncKeyState = (void*)GetProcAddress(hUser32, "GetAsyncKeyState");
    if (pGetAsyncKeyState && MH_CreateHook(pGetAsyncKeyState,
        (LPVOID)hkGetAsyncKeyState, (void**)&oGetAsyncKeyState) == MH_OK)
    {
        MH_EnableHook(pGetAsyncKeyState);
    }

    void* pGetKeyState = (void*)GetProcAddress(hUser32, "GetKeyState");
    if (pGetKeyState && MH_CreateHook(pGetKeyState,
        (LPVOID)hkGetKeyState, (void**)&oGetKeyState) == MH_OK)
    {
        MH_EnableHook(pGetKeyState);
    }

    return true;
}

}
