#pragma once
#include <imgui.h>
#include <string>
#include <vector>
#include <gui/GuiService.h>
#include <theme/ThemeManager.h>
#include <hotload/HotReloadService.h>

namespace VerificationUI {

inline bool g_showConsole = false;
inline int g_selectedTheme = 0;

inline void renderConsole() {
    if (!g_showConsole) return;

    ImGui::SetNextWindowSize(ImVec2(480, 360), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("FastRenderer 控制台", &g_showConsole,
        ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("系统信息", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& io = ImGui::GetIO();
        ImGui::Text("FPS:      %.1f", io.Framerate);
        ImGui::Text("显示尺寸: %.0f x %.0f", io.DisplaySize.x, io.DisplaySize.y);
        ImGui::Text("活跃 GUI: %zu 个", GuiService::getDefinitions().size());
        ImGui::Text("GUI 目录: %s", HotReloadService::getWatchDir().c_str());
    }

    if (ImGui::CollapsingHeader("主题切换", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto names = ThemeManager::getThemeNames();
        for (int i = 0; i < (int)names.size(); i++) {
            if (i > 0) ImGui::SameLine();
            if (ImGui::Button(names[i])) {
                g_selectedTheme = i;
                ThemeManager::applyTheme(i);
            }
        }
        ImGui::Text("当前主题: %s", ThemeManager::getThemeName(g_selectedTheme));
    }

    if (ImGui::CollapsingHeader("已加载的 GUI", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto defs = GuiService::getDefinitions();
        if (defs.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                "(无 - 将 JSON 文件放入 gui_defs/ 目录)");
        }
        for (auto& def : defs) {
            ImGui::BulletText("%s (%s)", def.id.c_str(),
                def.sourceFile.substr(0, 40).c_str());
        }
    }

    if (ImGui::CollapsingHeader("快捷键")) {
        ImGui::Text("N - 打开/关闭此控制台");
        ImGui::Text("F11 - 全屏切换");
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
        "FastRenderer v0.1.0 | 前置渲染框架");
    ImGui::End();
}

inline bool isActive() {
    return g_showConsole;
}

}
