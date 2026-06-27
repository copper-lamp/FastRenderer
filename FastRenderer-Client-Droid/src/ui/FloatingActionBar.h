#pragma once
#include <imgui.h>
#include <algorithm>
#include "FRMenuStore.h"
#include <input/KeybindService.h>

// ═══════════════════════════════════════════
//  Floating Action Bar
//  Bottom bar showing user-pinned keybinds
// ═══════════════════════════════════════════

class FloatingActionBar {
public:
    FloatingActionBar() = default;

    bool isVisible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }

    void render(const std::vector<MenuItem>& pinnedItems) {
        if (pinnedItems.empty() || !m_visible) return;

        ImGuiIO& io = ImGui::GetIO();
        float barHeight = 56.0f;
        float screenW   = io.DisplaySize.x;
        float screenH   = io.DisplaySize.y;
        float btnH      = barHeight - 8.0f;
        float btnW      = std::min(140.0f, (screenW - 16.0f) / pinnedItems.size() - 8.0f);

        ImGui::SetNextWindowPos(ImVec2(0, screenH - barHeight));
        ImGui::SetNextWindowSize(ImVec2(screenW, barHeight));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.45f));

        if (ImGui::Begin("##FRActionBar", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus))
        {
            float startX = (screenW - (pinnedItems.size() * btnW + (pinnedItems.size()-1) * 6.0f)) / 2.0f;
            if (startX < 4) startX = 4;
            ImGui::SetCursorPosX(startX);

            for (size_t i = 0; i < pinnedItems.size(); i++) {
                if (i > 0) {
                    ImGui::SameLine(0, 6);
                }
                const auto& item = pinnedItems[i];
                if (ImGui::Button(item.name.c_str(), ImVec2(btnW, btnH))) {
                    KeybindService::execute(item.bindId);
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }

private:
    bool m_visible = true;
};