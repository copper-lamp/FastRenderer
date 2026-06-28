#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <cstdio>

using json = nlohmann::json;

// ─── Logging helper ───
inline void frLog(const char* msg) {
    FILE* f = fopen("FRTest_ComplexGui.txt", "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

namespace ComplexGui {

inline json text(const std::string& value, const std::string& color = "", const std::string& ref = "") {
    json j = {{"type", "text"}, {"props", json::object()}};
    if (!value.empty()) j["props"]["value"] = value;
    if (!color.empty()) j["props"]["color"] = color;
    if (!ref.empty()) j["props"]["ref"] = ref;
    return j;
}

inline json btn(const std::string& label, const std::string& color = "", const std::string& onClick = "") {
    json j = {{"type", "button"}, {"props", {{"text", label}}}};
    if (!color.empty()) j["props"]["color"] = color;
    if (!onClick.empty()) j["props"]["onClick"] = onClick;
    return j;
}

inline json chk(const std::string& label, bool def = false, const std::string& id = "") {
    json j = {{"type", "checkbox"}, {"props", {{"label", label}, {"default", def ? "true" : "false"}}}};
    if (!id.empty()) j["props"]["id"] = id;
    return j;
}

inline json row(const std::vector<json>& children, float spacing = 6.0f) {
    return {{"type", "row"}, {"props", {{"spacing", std::to_string(spacing)}}}, {"children", children}};
}

inline json sep() { return {{"type", "separator"}, {"props", json::object()}}; }

inline json input(const std::string& placeholder = "", const std::string& def = "", const std::string& w = "") {
    json j = {{"type", "input"}, {"props", json::object()}};
    if (!placeholder.empty()) j["props"]["placeholder"] = placeholder;
    if (!def.empty()) j["props"]["default"] = def;
    if (!w.empty()) j["props"]["width"] = w;
    return j;
}

inline json slider(float minV, float maxV, float defV, const std::string& label = "", const std::string& onChange = "") {
    json j = {{"type", "slider"}, {"props", {{"min", std::to_string((int)minV)}, {"max", std::to_string((int)maxV)}, {"default", std::to_string((int)defV)}, {"label", label}}}};
    if (!onChange.empty()) j["props"]["onChange"] = onChange;
    return j;
}

inline json panel(const json& children, const std::string& color = "", float padding = 6.0f, float round = 6.0f) {
    json j = {{"type", "panel"}, {"props", {{"padding", std::to_string((int)padding)}, {"round", std::to_string((int)round)}}}, {"children", children}};
    if (!color.empty()) j["props"]["color"] = color;
    return j;
}

inline json group(const std::string& label, const json& children, bool collapsible = false) {
    json j = {{"type", "group"}, {"props", {{"label", label}}}, {"children", children}};
    if (collapsible) j["props"]["collapsible"] = "true";
    return j;
}

inline json spacer(float w = 8.0f, float h = 8.0f) {
    return {{"type", "spacer"}, {"props", {{"width", std::to_string((int)w)}, {"height", std::to_string((int)h)}}}};
}

inline json progress(float val, float w = 0, float h = 18.0f) {
    return {{"type", "progress"}, {"props", {{"value", std::to_string(val)}, {"width", std::to_string((int)w)}, {"height", std::to_string((int)h)}}}};
}

inline json image(const std::string& texture = "", float w = 0, float h = 0, const std::string& tint = "") {
    json j = {{"type", "image"}, {"props", json::object()}};
    if (!texture.empty()) j["props"]["texture"] = texture;
    if (w > 0) j["props"]["width"] = std::to_string((int)w);
    if (h > 0) j["props"]["height"] = std::to_string((int)h);
    if (!tint.empty()) j["props"]["tint"] = tint;
    return j;
}

inline std::string vkToName(int vk) {
    if (vk >= 0x30 && vk <= 0x39) return std::string(1, (char)('0' + vk - 0x30));
    if (vk >= 0x41 && vk <= 0x5A) return std::string(1, (char)('A' + vk - 0x41));
    if (vk >= 0x70 && vk <= 0x7B) { const char* f[] = {"F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12"}; return f[vk-0x70]; }
    if (vk == 0x10) return "Shift"; if (vk == 0x11) return "Ctrl"; if (vk == 0x12) return "Alt";
    if (vk == 0x20) return "Space"; if (vk == 0x0D) return "Enter"; if (vk == 0x1B) return "Esc";
    char buf[16]; snprintf(buf, sizeof(buf), "0x%02X", vk); return buf;
}

// ══════════════════════════════════════════════════════════
//  图片查看器
//  参数 state: 包含当前显示的图片路径、缩放等信息
// ══════════════════════════════════════════════════════════

inline json makeImageViewer(const json& state) {
    std::string currentFile = state.value("currentFile", "");
    float zoom = state.value("zoom", 1.0f);
    bool fitToWindow = state.value("fitToWindow", true);
    int imgW = state.value("imgW", 0);
    int imgH = state.value("imgH", 0);

    return {{"type", "window"}, {"props", {{"title", "📷 图片查看器"}, {"width", "660"}, {"height", "480"}, {"flags", "no_resize"}}}, {"children", {
        // Toolbar
        row({
            btn("◀ 上一张", "#2196F3", "img_prev"),
            btn("下一张 ▶", "#2196F3", "img_next"),
            spacer(10, 0),
            text(currentFile.empty() ? "未选择" : currentFile, "#90CAF9"),
        }, 6.0f),
        sep(),

        // Image display area
        panel([&](){
            json children;
            if (!currentFile.empty()) {
                children.push_back(image(currentFile, (int)(imgW * zoom), (int)(imgH * zoom)));
                // Info overlay
                char buf[128];
                snprintf(buf, sizeof(buf), "%dx%d | %.0f%%", imgW, imgH, zoom * 100);
                children.push_back(text(buf, "#888888"));
            } else {
                children.push_back(text("拖动图片文件到窗口以加载", "#666666"));
            }
            return children;
        }(), "#1A1A2E", 8, 4),

        sep(),

        // Zoom controls
        row({
            btn("缩小", "#FF9800", "img_zoom_out"),
            btn("原始", "#4CAF50", "img_zoom_1x"),
            btn("放大", "#FF9800", "img_zoom_in"),
            spacer(10, 0),
            text(std::to_string((int)(zoom * 100)) + "%", "#FFFFFF", "img_zoom_label"),
            spacer(10, 0),
            chk("适应窗口", fitToWindow, "img_fit"),
            btn("加载文件", "#2196F3", "img_load"),
        }, 6.0f),
    }}};
}

// ══════════════════════════════════════════════════════════
//  音乐播放器
//  参数 state: 包含播放状态、音量、播放进度等
// ══════════════════════════════════════════════════════════

inline json makeMusicPlayer(const json& state) {
    std::string currentFile = state.value("currentFile", "");
    std::string status = state.value("status", "停止");
    int volume = state.value("volume", 80);
    int position = state.value("position", 0);
    int duration = state.value("duration", 100);

    std::string statusColor = "#81C784";
    if (status.find("播放") != std::string::npos) statusColor = "#4FC3F7";
    else if (status.find("暂停") != std::string::npos) statusColor = "#FFCC80";
    else if (status.find("停止") != std::string::npos) statusColor = "#AAAAAA";

    char posStr[64];
    snprintf(posStr, sizeof(posStr), "%02d:%02d / %02d:%02d",
        position / 60, position % 60, duration / 60, duration % 60);

    return {{"type", "window"}, {"props", {{"title", "🎵 音乐播放器"}, {"width", "480"}, {"height", "320"}, {"flags", "no_resize"}}}, {"children", {
        // Track info
        group("▸ 当前曲目", {
            panel({
                text(currentFile.empty() ? "未选择文件" : currentFile, "#FFFFFF", "music_title"),
                text(status, statusColor, "music_status"),
            }, "#1A1A2E", 8, 6),
        }),
        sep(),

        // Progress
        group("▸ 播放进度", {
            progress((float)position / (float)(duration > 0 ? duration : 1), 420, 14),
            row({
                text(posStr, "#888888", "music_time"),
                text("音量:", "#888888"),
                slider(0, 100, (float)volume, "%", "music_volume"),
            }, 6.0f),
        }),
        sep(),

        // Controls
        group("▸ 控制", {
            row({
                btn("⏮ 上一首", "#555555", "music_prev"),
                btn("▶ 播放", "#4CAF50", "music_play"),
                btn("⏸ 暂停", "#FF9800", "music_pause"),
                btn("⏹ 停止", "#F44336", "music_stop"),
                btn("⏭ 下一首", "#555555", "music_next"),
            }, 6.0f),
            row({
                btn("打开文件", "#2196F3", "music_open"),
                spacer(10, 0),
                text("测试 MP3 解码和播放性能", "#666666"),
            }, 6.0f),
        }),
    }}};
}

// ══════════════════════════════════════════════════════════
//  FR 设置主菜单 — 已移至 FR 模组内置 UI
//  移至此处的原 makeSettingsMenu 不再需要
//  设置菜单现在由 FR 模组的 BuiltinSettingsUI 实现
// ══════════════════════════════════════════════════════════

} // namespace ComplexGui
