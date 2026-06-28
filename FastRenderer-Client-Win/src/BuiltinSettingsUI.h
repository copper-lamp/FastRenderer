// ════════════════════════════════════════════════════════════
//  BuiltinSettingsUI — FR 内置设置菜单 (ImGui 直写版)
//  替代旧 FastUI JSON 方式，使用直接 ImGui 渲染
//  包含：仪表盘、资源管理、按键绑定、设置 共4个页面
// ════════════════════════════════════════════════════════════

#pragma once
#include <imgui.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include <filesystem>
#include <gui/GuiService.h>
#include <gui/JsonGuiRenderer.h>
#include <input/KeybindService.h>
#include <theme/ThemeManager.h>
#include <hotload/HotReloadService.h>
#include <res/ResourceManager.h>

namespace BuiltinSettingsUI {

struct LogEntry { std::string time; std::string text; };

struct State {
    bool visible = false;
    int page = 0;
    int fps = 0, guiCount = 0, pluginCount = 0;
    bool tcpConnected = false;
    std::vector<LogEntry> events;
    std::string resFilter;
    std::string captureBindId, conflictMsg;
    std::vector<std::string> conflictDetails;
    int pendingVk = 0;
    std::string pluginFilter;
    bool showFps = true, showOverlay = false;
};
inline State g;
inline std::function<bool()> g_tcpChecker = nullptr;

inline void addLog(const std::string& msg) {
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    tm local = {}; localtime_s(&local, &tt);
    char buf[32]; snprintf(buf, sizeof(buf), "%02d:%02d:%02d", local.tm_hour, local.tm_min, local.tm_sec);
    g.events.insert(g.events.begin(), {std::string(buf), msg});
    if (g.events.size() > 100) g.events.pop_back();
    FILE* f = nullptr; fopen_s(&f, "FastRenderer_BS.txt", "a");
    if (f) { fprintf(f, "[%s] %s\n", buf, msg.c_str()); fclose(f); }
}

inline std::string vkName(int vk) {
    if (vk >= 0x30 && vk <= 0x39) return std::string(1, (char)('0' + vk - 0x30));
    if (vk >= 0x41 && vk <= 0x5A) return std::string(1, (char)('A' + vk - 0x41));
    if (vk >= 0x70 && vk <= 0x7B) { const char* f[] = {"F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12"}; return f[vk-0x70]; }
    if (vk == 0x10) return "Shift"; if (vk == 0x11) return "Ctrl"; if (vk == 0x12) return "Alt";
    if (vk == 0x20) return "Space"; if (vk == 0x0D) return "Enter"; if (vk == 0x1B) return "Esc";
    char buf[16]; snprintf(buf, sizeof(buf), "0x%02X", vk); return buf;
}

inline void drawCard(const char* val, const char* label, ImU32 valCol, ImU32 bgCol, float w) {
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, 56));
    auto dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p0, ImVec2(p0.x+w, p0.y+56), bgCol, 6);
    dl->AddText(ImVec2(p0.x+8, p0.y+6), valCol, val);
    dl->AddText(ImVec2(p0.x+8, p0.y+30), IM_COL32(135,135,135,255), label);
}

// ════════════════════════════════════════════════════════════
//  仪表盘
// ════════════════════════════════════════════════════════════
inline void renderDashboard() {
    if (g_tcpChecker) g.tcpConnected = g_tcpChecker();
    g.fps = (int)ImGui::GetIO().Framerate;
    auto defs = GuiService::getDefinitions();
    g.guiCount = (int)defs.size();
    std::set<std::string> plugins;
    for (auto& d : defs) plugins.insert(d.pluginId);
    auto allKb = KeybindService::getAll();
    for (auto& kb : allKb) plugins.insert(kb.pluginId);
    g.pluginCount = (int)plugins.size();

    ImGui::TextColored(ImVec4(1,1,1,1), "FR 仪表盘");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0,4));

    float avail = ImGui::GetContentRegionAvail().x;
    float cw = (avail - 24) / 4.0f; if (cw < 100) cw = 100;
    ImU32 cG = IM_COL32(130,200,135,255), cO = IM_COL32(255,205,128,255);
    ImU32 cB = IM_COL32(128,210,250,255), cP = IM_COL32(208,145,218,255);
    ImU32 bG = IM_COL32(25,90,30,70), bO = IM_COL32(230,80,0,70);
    ImU32 bB = IM_COL32(0,88,155,70), bP = IM_COL32(75,20,140,70);
    ImU32 cR = IM_COL32(240,155,155,255), bR = IM_COL32(240,90,90,70);

    drawCard(g.tcpConnected ? "TCP: OK" : "TCP: DISCONNECTED", "TCP", g.tcpConnected ? cG : cR, g.tcpConnected ? bG : bR, cw);
    ImGui::SameLine(0,8);
    char buf[32]; snprintf(buf, sizeof(buf), "%d plugins", g.pluginCount);
    drawCard(std::to_string(g.pluginCount).c_str(), buf, cO, bO, cw);
    ImGui::SameLine(0,8);
    snprintf(buf, sizeof(buf), "%d GUIs", g.guiCount);
    drawCard(std::to_string(g.guiCount).c_str(), buf, cB, bB, cw);
    ImGui::SameLine(0,8);
    snprintf(buf, sizeof(buf), "%d FPS", g.fps);
    drawCard(std::to_string(g.fps).c_str(), buf, cP, bP, cw);

    ImGui::Dummy(ImVec2(0,8));
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f,0.7f,0.7f,1), "-- Recent Events --");
    ImGui::Separator();
    ImGui::BeginChild("##evtlist", ImVec2(0, 140), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    if (g.events.empty()) ImGui::TextColored(ImVec4(0.4f,0.4f,0.4f,1), "(no events)");
    else { int n=0; for (auto& e : g.events) { if (n++>=25) break; ImGui::TextColored(ImVec4(0.65f,0.65f,0.65f,1), "  [%s]  %s", e.time.c_str(), e.text.c_str()); } }
    ImGui::EndChild();
    ImGui::Dummy(ImVec2(0,4));

    if (ImGui::Button("  Refresh  ", ImVec2(100,28))) { addLog("refresh"); }
    ImGui::SameLine(0,8);
    if (ImGui::Button("  Export Log  ", ImVec2(120,28))) {
        FILE* f = nullptr; fopen_s(&f, "FastRenderer_Events.log", "w");
        if (f) { for (auto& e : g.events) fprintf(f, "[%s] %s\n", e.time.c_str(), e.text.c_str()); fclose(f); }
        addLog("log exported");
    }
    ImGui::SameLine(0,8);
    ImGui::TextColored(ImVec4(0.4f,0.4f,0.4f,1), "resources: %d  |  keybinds: %zu", ResourceManager::totalCount(), KeybindService::getAll().size());
}

// ════════════════════════════════════════════════════════════
//  资源管理
// ════════════════════════════════════════════════════════════
inline void renderResources() {
    ImGui::TextColored(ImVec4(1,1,1,1), "Resource Manager");
    ImGui::Separator();

    int imgLoc = ResourceManager::countBySource(ResourceManager::Source::Local);
    int imgRem = ResourceManager::countBySource(ResourceManager::Source::Remote);
    int audLoc = 0, audRem = 0;
    for (auto& e : ResourceManager::getByType(ResourceManager::Type::Audio))
        if (e.source == ResourceManager::Source::Local) audLoc++; else audRem++;

    int totalRM = ResourceManager::totalCount();
    ImGui::Text("Images: %d (local %d | remote %d)", imgLoc+imgRem, imgLoc, imgRem);
    ImGui::Text("Audio:  %d (local %d | remote %d)", audLoc+audRem, audLoc, audRem);
    ImGui::TextColored(ImVec4(0.4f,0.4f,0.4f,1), "ResourceManager total entries: %d", totalRM);
    if (totalRM == 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,0.6f,0,1));
        ImGui::Text("WARNING: ResourceManager has 0 entries - resources not recognized!");
        ImGui::PopStyleColor();
    }
    ImGui::Dummy(ImVec2(0,4));

    if (ImGui::Button("  Scan Dir  ", ImVec2(120,26))) { ResourceManager::scanAll(); addLog("scan done"); }
    ImGui::SameLine(0,6);
    ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1), "total: %d", ResourceManager::totalCount());
    ImGui::Dummy(ImVec2(0,4));

    auto plugins = ResourceManager::getPlugins();
    ImGui::Text("filter:"); ImGui::SameLine(0,6);
    if (ImGui::SmallButton("all##rf")) { g.resFilter = ""; }
    for (auto& p : plugins) {
        ImGui::SameLine(0,3);
        bool sel = (g.resFilter == p);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.13f,0.59f,0.95f,0.8f));
        if (ImGui::SmallButton((p+"##rf").c_str())) { g.resFilter = (g.resFilter==p?"":p); }
        if (sel) ImGui::PopStyleColor();
    }
    ImGui::Separator();

    ImGui::BeginChild("##reslist", ImVec2(0,0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::TextColored(ImVec4(0.5f,0.83f,0.98f,1), "  type  | filename");
    ImGui::SameLine(120); ImGui::TextColored(ImVec4(0.5f,0.83f,0.98f,1), "plugin");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x-160); ImGui::TextColored(ImVec4(0.5f,0.83f,0.98f,1), "source    size");
    ImGui::Separator();

    auto all = ResourceManager::getAll();
    int cnt = 0;
    for (auto& e : all) {
        if (!g.resFilter.empty() && e.pluginId!=g.resFilter) continue;
        cnt++;
        const char* ts = (e.type==ResourceManager::Type::Image)?"[IMG]":"[AUD]";
        ImVec4 tc = (e.type==ResourceManager::Type::Image)?ImVec4(0.3f,0.8f,0.5f,1):ImVec4(0.8f,0.5f,0.3f,1);
        const char* ss = (e.source==ResourceManager::Source::Remote)?"remote":"local";
        ImVec4 sc = (e.source==ResourceManager::Source::Remote)?ImVec4(0.5f,0.78f,0.52f,1):ImVec4(1,0.8f,0.5f,1);
        ImGui::TextColored(tc, " %s ", ts); ImGui::SameLine();
        ImGui::Text("%s", e.name.c_str()); ImGui::SameLine(120);
        ImGui::TextColored(ImVec4(0.55f,0.55f,0.55f,1), "%s", e.pluginId.c_str());
        char sz[32];
        if (e.fileSize>1048576) snprintf(sz,sizeof(sz),"%.1f MB",e.fileSize/1048576.0f);
        else if (e.fileSize>1024) snprintf(sz,sizeof(sz),"%.1f KB",e.fileSize/1024.0f);
        else snprintf(sz,sizeof(sz),"%lld B",(long long)e.fileSize);
        ImGui::SameLine(ImGui::GetContentRegionAvail().x-160);
        ImGui::TextColored(sc,"%s",ss); ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1),"%s",sz);
    }
    if (cnt==0) { ImGui::Dummy(ImVec2(0,20)); ImGui::TextColored(ImVec4(0.4f,0.4f,0.4f,1),"  no resources (scan or wait for TCP transfer)"); }
    ImGui::EndChild();
}

// ════════════════════════════════════════════════════════════
//  按键绑定
// ════════════════════════════════════════════════════════════
inline void renderKeybinds() {
    ImGui::TextColored(ImVec4(1,1,1,1), "Key Bindings");
    ImGui::Separator();

    auto allKb = KeybindService::getAll();
    std::set<std::string> plugins;
    for (auto& kb : allKb) plugins.insert(kb.pluginId);

    ImGui::Text("filter:"); ImGui::SameLine(0,6);
    if (ImGui::SmallButton("all##kb")) { g.pluginFilter = ""; }
    for (auto& p : plugins) {
        ImGui::SameLine(0,3);
        bool sel = (g.pluginFilter == p);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.13f,0.59f,0.95f,0.8f));
        if (ImGui::SmallButton((p+"##kbf").c_str())) { g.pluginFilter = (g.pluginFilter==p?"":p); }
        if (sel) ImGui::PopStyleColor();
    }
    ImGui::Dummy(ImVec2(0,4));

    if (!g.captureBindId.empty()) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.9f,0.32f,0,0.27f));
        ImGui::BeginChild("##capbar", ImVec2(0,34), true);
        ImGui::TextColored(ImVec4(1,0.8f,0.5f,1), " [CAPTURE] Press a key...  (click Cancel to exit)");
        ImGui::EndChild(); ImGui::PopStyleColor(); ImGui::Dummy(ImVec2(0,4));
    }

    if (!g.conflictMsg.empty()) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.9f,0.32f,0,0.27f));
        ImGui::BeginChild("##conflict", ImVec2(0,0), true);
        ImGui::TextColored(ImVec4(1,0.8f,0.5f,1), " [CONFLICT] %s", g.conflictMsg.c_str());
        for (auto& d : g.conflictDetails) ImGui::TextColored(ImVec4(1,0.67f,0.57f,1), "    %s", d.c_str());
        ImGui::Separator();
        if (ImGui::Button("  Override  ", ImVec2(100,22))) {
            if (g.pendingVk>0 && !g.captureBindId.empty()) {
                auto kb2 = KeybindService::getAll();
                for (auto& kb : kb2) if (kb.vk==g.pendingVk && kb.id!=g.captureBindId) { KeybindService::unregisterKeybind(kb.pluginId,kb.id); break; }
                for (auto& kb : kb2) if (kb.id==g.captureBindId) {
                    KeybindService::unregisterKeybind(kb.pluginId,kb.id);
                    KeybindService::registerKeybind(kb.pluginId,kb.id,kb.name,g.pendingVk,nullptr);
                    addLog("override: "+kb.id+" -> "+vkName(g.pendingVk)); break;
                }
            }
            g.captureBindId.clear(); g.conflictMsg.clear(); g.conflictDetails.clear(); g.pendingVk=0;
        }
        ImGui::SameLine(0,8);
        if (ImGui::Button("  Cancel  ", ImVec2(100,22))) { g.captureBindId.clear(); g.conflictMsg.clear(); g.conflictDetails.clear(); g.pendingVk=0; }
        ImGui::EndChild(); ImGui::PopStyleColor();
    }

    ImGui::Separator();
    ImGui::BeginChild("##kblist", ImVec2(0,0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar|ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextColored(ImVec4(0.31f,0.76f,0.97f,1), "  %-8s %-24s %-24s %-16s","key","ID","name","plugin");
    ImGui::SameLine(); ImGui::TextColored(ImVec4(0.31f,0.76f,0.97f,1), "action");
    ImGui::Separator();

    int cnt = 0;
    for (auto& kb : allKb) {
        if (!g.pluginFilter.empty() && kb.pluginId!=g.pluginFilter) continue;
        cnt++;
        bool cap = (g.captureBindId==kb.id);
        std::string ks = cap ? "[...]" : vkName(kb.vk);
        ImVec4 kc = cap ? ImVec4(1,0.8f,0.5f,1) : ImVec4(1,1,1,1);
        ImGui::TextColored(kc,"  %-8s",ks.c_str()); ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.56f,0.81f,0.98f,1),"%-24s",kb.id.c_str()); ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f,0.7f,0.7f,1),"%-24s",kb.name.c_str()); ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1),"%-16s",kb.pluginId.c_str()); ImGui::SameLine();
        if (cap) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(1,0.6f,0,1));
        else ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.13f,0.59f,0.95f,1));
        if (ImGui::SmallButton((std::string(cap?"Cancel":"Modify")+"##"+kb.id).c_str())) {
            if (cap) g.captureBindId.clear();
            else { g.captureBindId=kb.id; g.conflictMsg.clear(); addLog("capture: "+kb.id); }
        }
        ImGui::PopStyleColor();
    }
    if (cnt==0) { ImGui::Dummy(ImVec2(0,20)); ImGui::TextColored(ImVec4(0.4f,0.4f,0.4f,1),"  no matching bindings"); }
    ImGui::EndChild();
}

// ════════════════════════════════════════════════════════════
//  设置
// ════════════════════════════════════════════════════════════
inline void renderSettings() {
    ImGui::TextColored(ImVec4(1,1,1,1), "Settings");
    ImGui::Separator();

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f,0.7f,0.7f,1), "-- Network --");
    static char host[128] = "127.0.0.1";
    static char port[32] = "12345";
    ImGui::Text("TCP Host:"); ImGui::SameLine(0,8); ImGui::InputText("##host",host,sizeof(host));
    ImGui::Text("TCP Port:"); ImGui::SameLine(0,8); ImGui::InputText("##port",port,sizeof(port));
    ImGui::Checkbox("Auto Reconnect", &g.showFps);

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f,0.7f,0.7f,1), "-- Rendering --");
    ImGui::Checkbox("Show FPS", &g.showFps);
    ImGui::Checkbox("Show Debug Overlay", &g.showOverlay);

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f,0.7f,0.7f,1), "-- Hot Reload --");
    std::string dir = HotReloadService::getWatchDir();
    ImGui::Text("GUI Dir: %s", dir.c_str());
    ImGui::Text("Loaded: %zu GUI definitions", GuiService::getDefinitions().size());

    ImGui::Separator(); ImGui::Dummy(ImVec2(0,4));
    if (ImGui::Button("  Save  ", ImVec2(100,28))) { addLog("settings saved"); }
    ImGui::SameLine(0,8);
    if (ImGui::Button("  Reset  ", ImVec2(100,28))) { addLog("settings reset"); }
}

// ════════════════════════════════════════════════════════════
//  主渲染
// ════════════════════════════════════════════════════════════
inline void render() {
    if (!g.visible) return;
    ImGui::SetNextWindowSize(ImVec2(800,520), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(100,100), ImGuiCond_FirstUseEver);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoResize;
    if (!ImGui::Begin("FR Settings", &g.visible, flags)) { ImGui::End(); return; }

    float navW = 160.0f, sbH = 30.0f;

    // ──导航栏──
    ImGui::BeginChild("##nav", ImVec2(navW, -(sbH+2)), false);
    ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1), "NAV"); ImGui::Separator(); ImGui::Dummy(ImVec2(0,2));
    struct NavItem { const char* label; int idx; };
    NavItem navItems[] = {{"Dashboard",0},{"Resources",1},{"Keybinds",2},{"Settings",3}};
    for (auto& n : navItems) {
        bool sel = (g.page == n.idx);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.13f,0.59f,0.95f,0.7f));
        else ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.12f,0.12f,0.14f,0.6f));
        ImGui::PushStyleColor(ImGuiCol_Text, sel?ImVec4(1,1,1,1):ImVec4(0.8f,0.8f,0.8f,1));
        if (ImGui::Button(n.label, ImVec2(navW-12,32))) { g.page=n.idx; g.captureBindId.clear(); g.conflictMsg.clear(); }
        ImGui::PopStyleColor(2); ImGui::Dummy(ImVec2(0,2));
    }
    ImGui::Dummy(ImVec2(0,12));
    ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.85f,0.25f,0.22f,1));
    ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1,1,1,1));
    if (ImGui::Button("Close", ImVec2(navW-12,32))) { g.visible = false; }
    ImGui::PopStyleColor(2);
    ImGui::EndChild();

    ImGui::SameLine(0,0);

    // ──内容区──
    ImGui::BeginChild("##content", ImVec2(-1, -(sbH+2)), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::Dummy(ImVec2(4,4));
    switch (g.page) {
        case 0: renderDashboard(); break;
        case 1: renderResources(); break;
        case 2: renderKeybinds(); break;
        case 3: renderSettings(); break;
    }
    ImGui::Dummy(ImVec2(0,8)); ImGui::EndChild();

    // ──状态栏──
    if (g_tcpChecker) g.tcpConnected = g_tcpChecker();
    g.fps = (int)ImGui::GetIO().Framerate;
    g.guiCount = (int)GuiService::getDefinitions().size();
    ImGui::Separator();
    ImGui::BeginChild("##statbar", ImVec2(0, sbH), false);
    if (g.tcpConnected) ImGui::TextColored(ImVec4(0.5f,0.78f,0.52f,1),"  TCP: connected");
    else ImGui::TextColored(ImVec4(0.9f,0.55f,0.55f,1),"  TCP: disconnected");
    ImGui::SameLine(0,20);
    ImGui::TextColored(ImVec4(0.56f,0.81f,0.98f,1),"GUI: %d",g.guiCount);
    ImGui::SameLine(0,16);
    ImGui::TextColored(ImVec4(0.81f,0.57f,0.85f,1),"FPS: %d",g.fps);
    ImGui::SameLine(0,16);
    ImGui::TextColored(ImVec4(0.65f,0.84f,0.65f,1),"resources: %d",ResourceManager::totalCount());
    ImGui::SameLine(0,16);
    ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1),"keybinds: %zu",KeybindService::getAll().size());
    ImGui::EndChild();

    ImGui::End();

    // ──按键捕获──
    if (!g.captureBindId.empty()) {
        static std::map<int,bool> lastKeyState;
        for (int vk=0x08; vk<=0xFF; vk++) {
            if (vk>=0x01 && vk<=0x06) continue;
            if (vk==0x77) continue;
            bool cur = (GetAsyncKeyState(vk)&0x8000)!=0;
            bool& lst = lastKeyState[vk];
            if (cur && !lst) {
                lst = true;
                auto allKb = KeybindService::getAll();
                g.conflictDetails.clear(); g.conflictMsg.clear(); g.pendingVk=vk;
                for (auto& kb : allKb)
                    if (kb.vk==vk && kb.id!=g.captureBindId)
                        { g.conflictDetails.push_back(kb.name+" ("+kb.pluginId+")"); g.conflictMsg="key "+vkName(vk)+" already in use"; }
                if (g.conflictMsg.empty()) {
                    for (auto& kb : allKb) if (kb.id==g.captureBindId) {
                        KeybindService::unregisterKeybind(kb.pluginId,kb.id);
                        KeybindService::registerKeybind(kb.pluginId,kb.id,kb.name,vk,nullptr);
                        addLog("bound: "+kb.id+" -> "+vkName(vk)); break;
                    }
                    g.captureBindId.clear();
                } else { addLog("conflict: "+g.conflictMsg); }
                return;
            }
            if (!cur) lst = false;
        }
    }
}

inline void openMenu() { g.visible = !g.visible; addLog(g.visible?"menu opened":"menu closed"); }
inline void closeMenu() { g.visible = false; }
inline bool isVisible() { return g.visible; }
inline bool isCapturing() { return !g.captureBindId.empty(); }
inline void setTcpChecker(std::function<bool()> cb) { g_tcpChecker = std::move(cb); }
inline void addEvent(const std::string& text) { addLog(text); }

} // namespace BuiltinSettingsUI
