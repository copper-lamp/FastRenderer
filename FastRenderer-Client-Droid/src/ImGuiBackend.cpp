#include "ImGuiBackend.h"
#include "FileLog.h"
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
    FileLog("ImGui", "InitImGuiFromGLContext enter");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    // Mobile: disable keyboard nav, enable touch mode
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;  // platform has cursors
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    // Highly sensitive on mobile: make drag threshold smaller
    io.MouseDragThreshold = 4.0f;

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
    FileLog("ImGui", "Init OpenGL3 backend...");
    if (!ImGui_ImplOpenGL3_Init("#version 300 es")) {
        FileLog("ImGui", "ImGui_ImplOpenGL3_Init failed");
        return false;
    }

    FileLog("ImGui", "init OK");
    return true;
}

bool BeginImGuiFrame(int width, int height, float deltaTime) {
    FileLog("ImGui", "BeginImGuiFrame %dx%f", width, deltaTime);
    ImGui_ImplOpenGL3_NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    io.DeltaTime = (deltaTime > 0.0f) ? deltaTime : 0.016f;

    ImGui::NewFrame();
    return true;
}

void EndImGuiFrame() {
    FileLog("ImGui", "EndImGuiFrame");
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DestroyImGuiBackend() {
    FileLog("ImGui", "DestroyImGuiBackend");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}