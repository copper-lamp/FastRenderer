#pragma once
#include <imgui.h>
#include <string>
#include <vector>

namespace ThemeManager {

enum ThemeId : int {
    DarkClassic = 0,
    LightClean,
    Cyberpunk,
    SakuraPink,
    DeepForest,
    SpaceBlue,
    Count
};

inline std::vector<const char*> getThemeNames() {
    return {"暗色经典", "亮色简约", "赛博朋克", "樱花粉韵", "深森幽绿", "太空深蓝"};
}

inline const char* getThemeName(int id) {
    auto names = getThemeNames();
    if (id >= 0 && id < (int)names.size()) return names[id];
    return "未知";
}

inline void applyDarkClassic() {
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(6, 4);

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.24f, 0.52f, 0.88f, 0.80f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.60f, 0.95f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.35f, 0.50f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.24f, 0.52f, 0.88f, 0.50f);
    colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.30f, 0.35f, 0.50f);
}

inline void applyLightClean() {
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.97f, 0.97f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.10f, 0.50f, 0.85f, 0.90f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.10f, 0.50f, 0.85f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.55f, 0.90f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.92f, 0.92f, 0.94f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.88f, 0.88f, 0.92f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.85f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.10f, 0.50f, 0.85f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.10f, 0.50f, 0.85f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.10f, 0.50f, 0.85f, 0.40f);
    colors[ImGuiCol_Separator] = ImVec4(0.80f, 0.80f, 0.85f, 0.80f);
}

inline void applyCyberpunk() {
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.85f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.00f, 0.90f, 1.00f, 0.80f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.90f, 1.00f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.95f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.00f, 0.90f, 1.00f, 0.50f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.92f, 0.23f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.90f, 1.00f, 1.00f);

    auto& style = ImGui::GetStyle();
    style.WindowRounding = 2.0f;
    style.FrameRounding = 0.0f;
    style.WindowBorderSize = 2.0f;
    style.FrameBorderSize = 1.0f;
}

inline void applySakuraPink() {
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.88f, 0.90f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.16f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.91f, 0.55f, 0.69f, 0.80f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.91f, 0.55f, 0.69f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.95f, 0.60f, 0.75f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.91f, 0.55f, 0.69f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.91f, 0.55f, 0.69f, 1.00f);

    auto& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.FrameRounding = 10.0f;
    style.GrabRounding = 10.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
}

inline void applyDeepForest() {
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.85f, 0.88f, 0.82f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.13f, 0.09f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.42f, 0.77f, 0.32f, 0.80f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.42f, 0.77f, 0.32f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.48f, 0.82f, 0.38f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.16f, 0.12f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.30f, 0.50f, 0.25f, 0.40f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.42f, 0.77f, 0.32f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.42f, 0.77f, 0.32f, 1.00f);

    auto& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.WindowBorderSize = 1.0f;
}

inline void applySpaceBlue() {
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.88f, 0.92f, 0.96f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.08f, 0.13f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.29f, 0.67f, 1.00f, 0.80f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.29f, 0.67f, 1.00f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.72f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.10f, 0.16f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.30f, 0.40f, 0.60f, 0.40f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.29f, 0.67f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.29f, 0.67f, 1.00f, 1.00f);

    auto& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
}

inline int g_pendingTheme = -1;

inline void applyTheme(int themeId) {
    g_pendingTheme = themeId;
    if (!ImGui::GetCurrentContext()) return;
    switch (themeId) {
        case DarkClassic: applyDarkClassic(); break;
        case LightClean: applyLightClean(); break;
        case Cyberpunk: applyCyberpunk(); break;
        case SakuraPink: applySakuraPink(); break;
        case DeepForest: applyDeepForest(); break;
        case SpaceBlue: applySpaceBlue(); break;
        default: applyDarkClassic(); break;
    }
}

inline void applyPendingTheme() {
    if (g_pendingTheme >= 0 && ImGui::GetCurrentContext()) {
        applyTheme(g_pendingTheme);
        g_pendingTheme = -1;
    }
}

}
