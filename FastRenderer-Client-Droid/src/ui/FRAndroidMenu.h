#pragma once
#include <imgui.h>
#include <algorithm>
#include "FRMenuStore.h"
#include "FloatingActionBar.h"
#include <input/KeybindService.h>

// ═══════════════════════════════════════════
//  FR Android Main Menu
//  Tabs: Keybinds | Settings
// ═══════════════════════════════════════════

class FRAndroidMenu {
public:
    FRAndroidMenu() = default;

    void render() {
        ImGuiIO& io = ImGui::GetIO();
        float w = std::min(420.0f, io.DisplaySize.x - 32.0f);
        float h = std::min(520.0f, io.DisplaySize.y - 64.0f);

        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2),
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));

        if (ImGui::Begin("FR Android Menu", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoNavFocus))
        {
            if (ImGui::BeginTabBar("FRTabs", ImGuiTabBarFlags_None)) {
                if (ImGui::BeginTabItem("⏎ 按键")) {
                    renderKeybindsTab();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("⚙ 设置")) {
                    renderSettingsTab();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
    }

private:
    void renderKeybindsTab() {
        auto items = FRMenuStore::getInstance().getAllItems();

        if (items.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::TextWrapped("暂无按键绑定。\n连接到服务器后，服务端注册的按键将自动出现在此处。");
            ImGui::PopStyleColor();
            return;
        }

        ImGui::Text("共 %d 个按键绑定 — 勾选后显示到快捷栏", (int)items.size());
        ImGui::Separator();
        ImGui::BeginChild("##bindList", ImVec2(0, -1), false,
            ImGuiWindowFlags_AlwaysVerticalScrollbar);

        for (auto& item : items) {
            ImGui::PushID(item.bindId.c_str());

            ImVec2 area = ImGui::GetContentRegionAvail();
            float btnX = area.x - 40.0f;

            // Checkbox (pin toggle)
            bool pinned = item.pinned;
            if (ImGui::Checkbox("##pin", &pinned)) {
                FRMenuStore::getInstance().togglePin(item.bindId);
            }
            ImGui::SameLine();

            // Name + subtitle
            ImGui::BeginGroup();
            ImGui::Text("%s", item.name.c_str());
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 0.7f));
            ImGui::Text("%s / %s", item.pluginId.c_str(), item.bindId.c_str());
            ImGui::PopStyleColor();
            ImGui::EndGroup();

            ImGui::SameLine(area.x - 32.0f);

            // Execute button ▶
            if (ImGui::Button("▶", ImVec2(32, 32))) {
                KeybindService::execute(item.bindId);
            }

            ImGui::PopID();
            ImGui::Separator();
        }

        ImGui::EndChild();
    }

    void renderSettingsTab() {
        std::string host;
        uint16_t port;
        FRMenuStore::getInstance().getBridgeConfig(host, port);

        // Server address input
        ImGui::Text("桥接地址");
        char buf[256];
        std::string addrStr = host + ":" + std::to_string(port);
        strncpy(buf, addrStr.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';

        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##bridgeAddr", buf, sizeof(buf),
            ImGuiInputTextFlags_EnterReturnsTrue))
        {
            // Parse host:port
            std::string input(buf);
            size_t colon = input.find(':');
            if (colon != std::string::npos) {
                host = input.substr(0, colon);
                port = (uint16_t)std::stoi(input.substr(colon + 1));
                FRMenuStore::getInstance().setBridgeConfig(host, port);
            }
        }

        ImGui::Spacing();

        // Connect / disconnect buttons
        extern FrTcpClient g_tcpClient;  // from ModEntry.cpp
        bool connected = g_tcpClient.isConnected();

        ImGui::BeginDisabled(connected);
        if (ImGui::Button("连接", ImVec2(100, 36))) {
            // Trigger connection
            extern bool connectTcpBridge(const std::string&, uint16_t);
            connectTcpBridge(host, port);
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!connected);
        if (ImGui::Button("断开", ImVec2(100, 36))) {
            g_tcpClient.disconnect();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        // Status indicator
        if (connected) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "● 已连接");
        } else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "● 未连接");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Action bar toggle
        bool abVisible = FRMenuStore::getInstance().isActionBarVisible();
        if (ImGui::Checkbox("显示浮动快捷栏", &abVisible)) {
            FRMenuStore::getInstance().setActionBarVisible(abVisible);
            extern FloatingActionBar g_actionBar;
            g_actionBar.setVisible(abVisible);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Version info
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "FastRenderer-Android v0.1.0");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "基于 LeviLaunchroid Preloader");
    }
};