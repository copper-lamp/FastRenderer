#pragma once
#include <imgui.h>
#include <algorithm>
#include "FRMenuStore.h"

// ═══════════════════════════════════════════
//  Floating Trigger Icon
//  A small semi-transparent ⚙ icon that can be dragged
//  Tap to open/close FRAndroidMenu
// ═══════════════════════════════════════════

class FloatingTrigger {
public:
    FloatingTrigger();
    void render();
    bool isOpen() const { return m_open; }
    void setOpen(bool v) { m_open = v; }
    void toggle()        { m_open = !m_open; }

private:
    bool m_open       = false;
    bool m_dragging   = false;
    bool m_wasDragged = false;
    ImVec2 m_pos;
    ImVec2 m_dragOffset;
    float  m_size = 48.0f;
    float  m_alpha = 0.65f;
};

inline FloatingTrigger::FloatingTrigger() {
    // Use stored position or default to right-center
    float x, y;
    FRMenuStore::getInstance().getTriggerPos(x, y);
    if (x < 0 || y < 0) {
        m_pos = ImVec2(-1, -1);  // Will be set on first render
    } else {
        m_pos = ImVec2(x, y);
    }
}

inline void FloatingTrigger::render() {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 screen(io.DisplaySize.x, io.DisplaySize.y);

    // First render: default to right-center
    if (m_pos.x < 0) m_pos = ImVec2(screen.x - m_size - 16, screen.y / 2 - m_size / 2);

    // Clamp to screen
    m_pos.x = std::clamp(m_pos.x, 0.0f, screen.x - m_size);
    m_pos.y = std::clamp(m_pos.y, 0.0f, screen.y - m_size);

    ImGui::SetNextWindowPos(m_pos);
    ImGui::SetNextWindowSize(ImVec2(m_size, m_size));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0)); // transparent

    if (ImGui::Begin("##FRTrigger", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus))
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 winMin = ImGui::GetWindowPos();
        ImVec2 winMax = ImVec2(winMin.x + m_size, winMin.y + m_size);
        ImVec2 center = ImVec2(winMin.x + m_size/2, winMin.y + m_size/2);

        // Background circle
        draw->AddCircleFilled(center, m_size/2,
            IM_COL32(30, 30, 30, (int)(m_alpha * 255)));

        // Gear icon "⚙" drawn as text
        float fontSz = m_size * 0.7f;
        const char* gear = "\u2699"; // ⚙

        ImGui::SetCursorPos(ImVec2((m_size - fontSz) / 2, (m_size - fontSz) / 2));
        ImGui::PushFont(io.Fonts->Fonts[0]);
        // Use larger font size for the gear
        ImGui::SetWindowFontScale(fontSz / ImGui::GetFontSize());
        ImGui::TextColored(ImVec4(1,1,1,m_alpha), "%s", gear);
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopFont();

        // ─── Drag handling ───
        ImGui::InvisibleButton("##drag", ImVec2(m_size, m_size));

        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
            if (!m_dragging) {
                m_dragging = true;
                m_dragOffset = ImVec2(
                    io.MousePos.x - m_pos.x,
                    io.MousePos.y - m_pos.y);
            }
            m_pos.x = io.MousePos.x - m_dragOffset.x;
            m_pos.y = io.MousePos.y - m_dragOffset.y;
            m_wasDragged = true;
        }

        if (ImGui::IsItemDeactivated()) {
            if (m_dragging) {
                m_dragging = false;
                // Save position
                FRMenuStore::getInstance().setTriggerPos(m_pos.x, m_pos.y);
            }
            // Click (not drag) → toggle menu
            if (!m_wasDragged) {
                toggle();
            }
            m_wasDragged = false;
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}