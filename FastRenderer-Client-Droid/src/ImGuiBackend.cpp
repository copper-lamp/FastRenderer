#include "ImGuiBackend.h"
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <cstdio>
#include <string>
#include <fstream>

// ═══════════════════════════════════════════
//  ImGui OpenGL ES 3.0 backend
//  Runs inside Minecraft's existing GL context
// ═══════════════════════════════════════════

bool InitImGuiFromGLContext() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;        // Don't write imgui.ini
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // if docking branch
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // ─── Style (dark theme) ───
    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.WindowRounding    = 6.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding      = 4.0f;
    style.WindowBorderSize  = 0.0f;
    style.FrameBorderSize   = 0.0f;

    // ─── Init OpenGL3 backend ───
    if (!ImGui_ImplOpenGL3_Init("#version 300 es")) {
        std::fprintf(stderr, "ImGui_ImplOpenGL3_Init failed\n");
        return false;
    }

    // ─── Font (use default, embedded font for now) ───
    // On Android, system font loading requires NDK asset manager.
    // For now, ImGui's default Proggy font is used.
    // To support Chinese characters, embed NotoSansSC via:
    //   io.Fonts->AddFontFromFileTTF("/data/data/.../NotoSansSC-Regular.ttf", ...)
    // Or use plugin's resources directory.

    return true;
}

bool BeginImGuiFrame(int width, int height, float deltaTime) {
    ImGui_ImplOpenGL3_NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    io.DeltaTime = (deltaTime > 0.0f) ? deltaTime : 0.016f;

    ImGui::NewFrame();
    return true;
}

void EndImGuiFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DestroyImGuiBackend() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}