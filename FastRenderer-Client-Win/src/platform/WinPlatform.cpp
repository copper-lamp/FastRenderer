#pragma once
#include <IPlatform.h>
#include <DX11Hook.h>
#include <InputBlocker.h>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include <filesystem>
#include <string>

class WinPlatform : public IPlatform {
public:
    bool init() override {
        return DX11Hook::init();
    }

    void shutdown() override {
        DX11Hook::ShutdownImGui();
    }

    void initImGui() override {
        // ImGui initialization is automatic in DX11Hook::hkPresent
    }

    void newFrame() override {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void renderDrawData() override {
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void shutdownImGui() override {
        DX11Hook::ShutdownImGui();
    }

    float getFrameTime() override {
        return ImGui::GetIO().DeltaTime;
    }

    Vec2 getDisplaySize() override {
        auto& io = ImGui::GetIO();
        return Vec2{io.DisplaySize.x, io.DisplaySize.y};
    }

    bool isKeyDown(int vk) override {
        return (GetAsyncKeyState(vk) & 0x8000) != 0;
    }

    Vec2 getMousePos() override {
        POINT pt;
        GetCursorPos(&pt);
        return Vec2{(float)pt.x, (float)pt.y};
    }

    void setCursorVisible(bool visible) override {
        ShowCursor(visible ? TRUE : FALSE);
    }

    void setInputBlocked(bool blocked) override {
        if (blocked) {
            InputBlocker::addToWhitelist(0x4E); // N
        }
    }

    void sendPacket(std::string_view channel, std::string_view data) override {
        // Network layer - implemented in Phase 1.5
    }

    void onPacket(std::string_view channel,
        std::function<void(std::string_view)> callback) override
    {
        // Network layer - implemented in Phase 1.5
    }

    std::filesystem::path getDataDir() override {
        return std::filesystem::current_path() / "FastRenderer";
    }

    std::filesystem::path getGuiDefsDir() override {
        return getDataDir() / "gui_defs";
    }

    std::filesystem::path getConfigDir() override {
        return getDataDir() / "config";
    }

    bool loadFont(const std::string& name, const std::string& path,
        float size) override
    {
        ImGuiIO& io = ImGui::GetIO();
        return io.Fonts->AddFontFromFileTTF(path.c_str(), size) != nullptr;
    }
};
