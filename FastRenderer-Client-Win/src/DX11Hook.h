#pragma once
#include <windows.h>
#define D3D12_FEATURE_DATA_D3D12_OPTIONS  D3D12_FEATURE_DATA_D3D12_OPTIONS_LEGACY
#define D3D12_FEATURE_DATA_ARCHITECTURE  D3D12_FEATURE_DATA_ARCHITECTURE_LEGACY
#define D3D12_RAYTRACING_GEOMETRY_DESC  D3D12_RAYTRACING_GEOMETRY_DESC_LEGACY
#include <d3d11.h>
#include <d3d12.h>
#include <d3d11on12.h>
#include <dxgi1_4.h>
#undef D3D12_FEATURE_DATA_D3D12_OPTIONS
#undef D3D12_FEATURE_DATA_ARCHITECTURE
#undef D3D12_RAYTRACING_GEOMETRY_DESC
#include <MinHook.h>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include <functional>
#include <atomic>
#include <fstream>
#include <chrono>
#include <ctime>
#include <cstdio>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace DX11Hook {

using Present_t = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);

inline Present_t oPresent = nullptr;

inline ID3D11Device* g_pd3dDevice = nullptr;
inline ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
inline HWND g_hWnd = nullptr;
inline WNDPROC oWndProc = nullptr;
inline bool g_imguiInitialized = false;
inline bool g_imguiInitAttempted = false;

inline std::function<void()> g_renderCallback = nullptr;

inline void setRenderCallback(std::function<void()> cb) {
    g_renderCallback = std::move(cb);
}

inline void ShutdownImGui() {
    if (g_imguiInitialized) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_imguiInitialized = false;
    }
}

inline LRESULT __stdcall WndProcHook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (g_imguiInitialized) {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
    }
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

inline void InitImGuiFromSwapChain(IDXGISwapChain* pSwapChain) {
    if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice))) {
        g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);

        DXGI_SWAP_CHAIN_DESC sd;
        pSwapChain->GetDesc(&sd);
        g_hWnd = sd.OutputWindow;
        if (!g_hWnd) g_hWnd = FindWindowW(L"Minecraft", NULL);

        oWndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)WndProcHook);

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;

        // Load CJK-capable font for Chinese text support
        struct CjkFontEntry { const char* path; int index; };
        CjkFontEntry cjkFonts[] = {
            {"C:\\Windows\\Fonts\\msyh.ttc", 0},
            {"C:\\Windows\\Fonts\\simsun.ttc", 0},
            {"C:\\Windows\\Fonts\\msyhbd.ttc", 0},
            {"C:\\Windows\\Fonts\\SimHei.ttf", 0},
        };
        bool cjkLoaded = false;
        for (auto& entry : cjkFonts) {
            FILE* test = nullptr;
            if (fopen_s(&test, entry.path, "r") == 0 && test) {
                fclose(test);
                ImFontConfig cfg;
                cfg.FontDataOwnedByAtlas = true;
                cfg.FontNo = entry.index;
                io.Fonts->AddFontFromFileTTF(entry.path, 16.0f, &cfg, io.Fonts->GetGlyphRangesChineseFull());
                cjkLoaded = true;
                break;
            }
        }
        if (!cjkLoaded) {
            io.Fonts->AddFontDefault();
        }

        ImGui_ImplWin32_Init(g_hWnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
        g_imguiInitialized = true;
    }
}

inline void RenderFrame() {
    if (!g_imguiInitialized) return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (g_renderCallback) {
        g_renderCallback();
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

inline HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!g_imguiInitialized && !g_imguiInitAttempted) {
        g_imguiInitAttempted = true;
        InitImGuiFromSwapChain(pSwapChain);
    }

    if (g_imguiInitialized) {
        ID3D11Texture2D* pBackBuffer = nullptr;
        if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer))) {
            ID3D11RenderTargetView* rtv = nullptr;
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &rtv);
            pBackBuffer->Release();
            if (rtv) {
                g_pd3dDeviceContext->OMSetRenderTargets(1, &rtv, NULL);
                RenderFrame();
                rtv->Release();
                g_pd3dDeviceContext->ClearState();
                g_pd3dDeviceContext->Flush();
            }
        }
    }

    return oPresent(pSwapChain, SyncInterval, Flags);
}

inline bool init() {
    __try {
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = GetDesktopWindow();
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        ID3D11Device* dummyDevice = nullptr;
        IDXGISwapChain* dummySwapChain = nullptr;
        ID3D11DeviceContext* dummyContext = nullptr;

        if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
            &featureLevel, 1, D3D11_SDK_VERSION, &sd,
            &dummySwapChain, &dummyDevice, NULL, &dummyContext)))
        {
            return false;
        }

        void** pVTable = *reinterpret_cast<void***>(dummySwapChain);

        MH_STATUS status = MH_Initialize();
        if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
            dummySwapChain->Release();
            dummyDevice->Release();
            dummyContext->Release();
            return false;
        }

        MH_CreateHook(pVTable[8], (LPVOID)hkPresent, (void**)&oPresent);
        MH_EnableHook(pVTable[8]);

        dummySwapChain->Release();
        dummyDevice->Release();
        dummyContext->Release();
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}
