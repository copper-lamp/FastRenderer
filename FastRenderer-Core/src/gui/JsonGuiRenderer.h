#pragma once
#include <imgui.h>
#include <string>
#include <vector>
#include <functional>
#include <cstdio>
#include <sstream>
#include <GuiDefinition.h>
#include <gui/JsonGuiParser.h>
#include <gui/DynamicText.h>
#include <res/TextureManager.h>

namespace JsonGuiRenderer {

inline std::function<void(const std::string&)> g_eventCallback = nullptr;

inline void rLog(const char* msg) {
    FILE* f = nullptr;
    f = fopen("FastRenderer_R.txt", "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

inline int g_windowRenderCount = 0;

inline void setEventCallback(std::function<void(const std::string&)> cb) {
    g_eventCallback = std::move(cb);
}

inline ImVec4 parseColor(const std::string& str, const ImVec4& fallback = ImVec4(1,1,1,1)) {
    if (str.empty()) return fallback;
    if (str[0] == '#') {
        unsigned int r=255, g=255, b=255, a=255;
        if (str.length() >= 7) sscanf(str.c_str() + 1, "%02x%02x%02x", &r, &g, &b);
        if (str.length() >= 9) sscanf(str.c_str() + 7, "%02x", &a);
        return ImVec4(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
    }
    if (str == "red")    return ImVec4(0.85f, 0.25f, 0.25f, 1.0f);
    if (str == "green")  return ImVec4(0.25f, 0.75f, 0.30f, 1.0f);
    if (str == "blue")   return ImVec4(0.25f, 0.45f, 0.85f, 1.0f);
    if (str == "orange") return ImVec4(0.95f, 0.60f, 0.15f, 1.0f);
    if (str == "yellow") return ImVec4(0.95f, 0.85f, 0.20f, 1.0f);
    if (str == "white")  return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    if (str == "black")  return ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    return fallback;
}

inline void fireEvent(const std::string& eventName) {
    if (g_eventCallback && !eventName.empty()) g_eventCallback(eventName);
}

inline std::string g_currentGuiId;

struct ControlState {
    static inline std::map<std::string, bool> checkboxStates;
    static inline std::map<std::string, float> sliderStates;
    static inline std::map<std::string, std::string> inputStates;
    static inline std::map<std::string, int> comboStates;
};

inline void clearControlStates() {
    ControlState::checkboxStates.clear();
    ControlState::sliderStates.clear();
    ControlState::inputStates.clear();
    ControlState::comboStates.clear();
}

// ─── Parse window flags from comma-separated string ───
inline ImGuiWindowFlags parseWindowFlags(const std::string& flagsStr) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    if (flagsStr.empty()) return flags;
    std::istringstream ss(flagsStr);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // Trim whitespace
        token.erase(0, token.find_first_not_of(" \t\r\n"));
        token.erase(token.find_last_not_of(" \t\r\n") + 1);
        if (token == "no_collapse")            flags |= ImGuiWindowFlags_NoCollapse;
        else if (token == "no_scrollbar")       flags |= ImGuiWindowFlags_NoScrollbar;
        else if (token == "no_resize")          flags |= ImGuiWindowFlags_NoResize;
        else if (token == "no_move")            flags |= ImGuiWindowFlags_NoMove;
        else if (token == "no_title_bar")       flags |= ImGuiWindowFlags_NoTitleBar;
        else if (token == "always_auto_resize")  flags |= ImGuiWindowFlags_AlwaysAutoResize;
        else if (token == "no_scroll_with_mouse") flags |= ImGuiWindowFlags_NoScrollWithMouse;
        else if (token == "no_saved_settings")  flags |= ImGuiWindowFlags_NoSavedSettings;
        else if (token == "no_bring_to_front")  flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
        else if (token == "no_nav_focus")       flags |= ImGuiWindowFlags_NoNavFocus;
        else if (token == "no_inputs")          flags |= ImGuiWindowFlags_NoInputs;
    }
    return flags;
}

// ─── Apply font size via window font scale ───
struct FontSizeScope {
    FontSizeScope(int fontSize) : active(fontSize > 0), savedScale(1.0f) {
        if (active) {
            savedScale = ImGui::GetFont()->Scale;
            float scale = (float)fontSize / ImGui::GetFontSize();
            ImGui::SetWindowFontScale(scale);
        }
    }
    ~FontSizeScope() { if (active) ImGui::SetWindowFontScale(savedScale); }
    bool active;
    float savedScale;
};

// ─── Apply text align via cursor offset ───
inline void applyTextAlign(const std::string& align, const std::string& text) {
    if (align == "center" || align == "right") {
        float tw = ImGui::CalcTextSize(text.c_str()).x;
        float avail = ImGui::GetContentRegionAvail().x;
        if (align == "center") ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - tw) * 0.5f);
        else ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - tw);
    }
}

// ─── Draw text with shadow ───
inline void drawTextWithShadow(const std::string& value, const ImVec4& color,
    bool shadow, const ImVec4& shadowColor, float sx, float sy)
{
    if (shadow) {
        auto dl = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        dl->AddText(ImVec2(pos.x + sx, pos.y + sy), IM_COL32(0,0,0,(int)(shadowColor.w*255)), value.c_str());
    }
    ImGui::TextColored(color, "%s", value.c_str());
}

// ═══════════════════════════════════════════════════════════
//  Main render node
// ═══════════════════════════════════════════════════════════

inline void renderNode(const GuiNode& node, const std::string& path = "root") {

    // ──────────────────────────────────
    //  WINDOW
    // ──────────────────────────────────
    if (node.type == "window") {
        std::string title = JsonGuiParser::getProp(node, "title", "窗口");
        int w = JsonGuiParser::getPropInt(node, "width", 400);
        int h = JsonGuiParser::getPropInt(node, "height", 300);
        bool fullscreen = JsonGuiParser::getPropBool(node, "fullscreen", false);
        bool modal = JsonGuiParser::getPropBool(node, "modal", false);
        std::string flagsStr = JsonGuiParser::getProp(node, "flags", "");
        std::string bgColorStr = JsonGuiParser::getProp(node, "bg_color", "");
        std::string borderStr = JsonGuiParser::getProp(node, "border", "true");
        float borderRadius = JsonGuiParser::getPropFloat(node, "border_radius", 0);

        if (fullscreen) {
            auto vp = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(vp->Pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(vp->Size, ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        } else {
            ImGui::SetNextWindowSize(ImVec2((float)w, (float)h), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiCond_FirstUseEver);
        }

        // Modal overlay
        if (modal) {
            auto vp = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(vp->Pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(vp->Size, ImGuiCond_Always);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.55f));
            ImGuiWindowFlags mf = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
                | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
            if (ImGui::Begin((title + "_modal").c_str(), nullptr, mf)) {
                ImVec2 cs = ImVec2((float)w, (float)h);
                ImVec2 vs = vp->Size;
                ImGui::SetCursorPos(ImVec2((vs.x - cs.x) * 0.5f, (vs.y - cs.y) * 0.5f));
                ImGui::BeginChild((title + "_content").c_str(), cs, false);
                int idx = 0;
                for (auto& ch : node.children) renderNode(ch, path + "/" + std::to_string(idx++));
                ImGui::EndChild();
            }
            ImGui::End();
            ImGui::PopStyleColor();
            return;
        }

        if (++g_windowRenderCount % 300 == 0)
            rLog(("window: title='" + title + "' w=" + std::to_string(w) + " h=" + std::to_string(h) + " path=" + path).c_str());

        ImGuiWindowFlags flags = parseWindowFlags(flagsStr);
        if (fullscreen) flags |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize
                               | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

        // Window background color
        if (!bgColorStr.empty()) ImGui::PushStyleColor(ImGuiCol_WindowBg, parseColor(bgColorStr));

        // Border radius
        if (borderRadius > 0 && !fullscreen) ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, borderRadius);
        if (borderStr == "false") ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        if (ImGui::Begin(title.c_str(), nullptr, flags)) {
            int idx = 0;
            for (auto& ch : node.children) renderNode(ch, path + "/" + std::to_string(idx++));
        }
        ImGui::End();

        if (borderStr == "false") ImGui::PopStyleVar();
        if (borderRadius > 0 && !fullscreen) ImGui::PopStyleVar();
        if (!bgColorStr.empty()) ImGui::PopStyleColor();
        if (fullscreen) ImGui::PopStyleVar(2);

    // ──────────────────────────────────
    //  COLUMN
    // ──────────────────────────────────
    } else if (node.type == "column") {
        float spacing = JsonGuiParser::getPropFloat(node, "spacing", 6.0f);
        if (spacing > 0) ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
        int idx = 0;
        for (auto& ch : node.children) renderNode(ch, path + "/col" + std::to_string(idx++));
        if (spacing > 0) ImGui::PopStyleVar();

    // ──────────────────────────────────
    //  ROW
    // ──────────────────────────────────
    } else if (node.type == "row") {
        float spacing = JsonGuiParser::getPropFloat(node, "spacing", 6.0f);
        bool wrap = JsonGuiParser::getPropBool(node, "wrap", false);
        float startX = 0;
        if (wrap) startX = ImGui::GetCursorPosX();
        int idx = 0;
        for (auto& ch : node.children) {
            if (idx > 0) {
                if (wrap) {
                    ImGui::SameLine(0.0f, spacing);
                    // Check if we need to wrap (next item would go off screen)
                    float curX = ImGui::GetCursorPosX();
                    float availW = ImGui::GetContentRegionAvail().x;
                    if (curX > startX + availW) {
                        ImGui::NewLine();
                    }
                } else {
                    ImGui::SameLine(0.0f, spacing);
                }
            }
            renderNode(ch, path + "/row" + std::to_string(idx));
            idx++;
        }

    // ──────────────────────────────────
    //  BUTTON
    // ──────────────────────────────────
    } else if (node.type == "button") {
        std::string text = JsonGuiParser::getProp(node, "text", "按钮");
        std::string colorStr = JsonGuiParser::getProp(node, "color", "");
        std::string bgHover = JsonGuiParser::getProp(node, "bg_hover", "");
        std::string bgActive = JsonGuiParser::getProp(node, "bg_active", "");
        std::string textColorStr = JsonGuiParser::getProp(node, "text_color", "");
        std::string onClick = JsonGuiParser::getProp(node, "onClick", "");
        float w = JsonGuiParser::getPropFloat(node, "width", 0.0f);
        float h = JsonGuiParser::getPropFloat(node, "height", 0.0f);
        bool disabled = JsonGuiParser::getPropBool(node, "disabled", false);
        float borderRadius = JsonGuiParser::getPropFloat(node, "border_radius", 0);
        int fontSize = JsonGuiParser::getPropInt(node, "font_size", 0);

        int pushedColors = 0;
        if (!colorStr.empty()) {
            ImVec4 col = parseColor(colorStr);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(col.x, col.y, col.z, col.w * 0.8f));
            pushedColors++;
            if (bgHover.empty()) { ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col); pushedColors++; }
            if (bgActive.empty()) { ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(col.x*1.1f, col.y*1.1f, col.z*1.1f, col.w)); pushedColors++; }
        }
        if (!bgHover.empty())     { ImGui::PushStyleColor(ImGuiCol_ButtonHovered, parseColor(bgHover)); pushedColors++; }
        if (!bgActive.empty())    { ImGui::PushStyleColor(ImGuiCol_ButtonActive, parseColor(bgActive)); pushedColors++; }
        if (!textColorStr.empty()) { ImGui::PushStyleColor(ImGuiCol_Text, parseColor(textColorStr)); pushedColors++; }
        if (borderRadius > 0)     { ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, borderRadius); }

        FontSizeScope fss(fontSize);
        if (disabled) ImGui::BeginDisabled();

        std::string btnId = text + "##" + path;
        if (ImGui::Button(btnId.c_str(), ImVec2(w > 0 ? w : 0, h > 0 ? h : 0))) {
            fireEvent(onClick);
        }

        if (disabled) ImGui::EndDisabled();
        if (borderRadius > 0) ImGui::PopStyleVar();
        if (pushedColors > 0) ImGui::PopStyleColor(pushedColors);

    // ──────────────────────────────────
    //  TEXT (with font_size, font_weight, align, shadow)
    // ──────────────────────────────────
    } else if (node.type == "text") {
        std::string value = JsonGuiParser::getProp(node, "value", "");
        std::string ref = JsonGuiParser::getProp(node, "ref", "");
        if (!ref.empty()) value = DynamicText::get(ref);
        if (value.empty()) { ImGui::Dummy(ImVec2(0,0)); return; }

        std::string colorStr = JsonGuiParser::getProp(node, "color", "");
        std::string fontWeight = JsonGuiParser::getProp(node, "font_weight", "normal");
        std::string align = JsonGuiParser::getProp(node, "align", "left");
        bool shadow = JsonGuiParser::getPropBool(node, "shadow", false);
        std::string shadowColorStr = JsonGuiParser::getProp(node, "shadow_color", "#00000080");
        std::string shadowOffsetStr = JsonGuiParser::getProp(node, "shadow_offset", "");
        int fontSize = JsonGuiParser::getPropInt(node, "font_size", 0);

        ImVec4 col = colorStr.empty() ? ImVec4(0.88f, 0.88f, 0.88f, 1.0f) : parseColor(colorStr);

        // Bold: use ImGui::PushFont if we had a bold font; simplest is to use TextColored with a bold style
        // Since we don't have a bold font loaded, we'll use a visual approximation

        FontSizeScope fss(fontSize);
        applyTextAlign(align, value);

        // Shadow
        float sx = 1.0f, sy = 1.0f;
        if (!shadowOffsetStr.empty()) {
            sscanf(shadowOffsetStr.c_str(), "%f,%f", &sx, &sy);
        }
        ImVec4 shadowCol = parseColor(shadowColorStr, ImVec4(0,0,0,0.5f));

        if (shadow) {
            auto dl = ImGui::GetWindowDrawList();
            ImVec2 pos = ImGui::GetCursorScreenPos();
            dl->AddText(ImVec2(pos.x + sx, pos.y + sy), IM_COL32(0,0,0,(int)(shadowCol.w*255)), value.c_str());
        }

        if (fontWeight == "bold") {
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            ImGui::TextUnformatted(value.c_str());
            ImGui::PopStyleColor();
        } else {
            ImGui::TextColored(col, "%s", value.c_str());
        }

    // ──────────────────────────────────
    //  SEPARATOR
    // ──────────────────────────────────
    } else if (node.type == "separator") {
        ImGui::Separator();

    // ──────────────────────────────────
    //  CHECKBOX
    // ──────────────────────────────────
    } else if (node.type == "checkbox") {
        std::string label = JsonGuiParser::getProp(node, "label", "");
        bool def = JsonGuiParser::getPropBool(node, "default", false);
        std::string onChange = JsonGuiParser::getProp(node, "onChange", "");
        bool disabled = JsonGuiParser::getPropBool(node, "disabled", false);
        std::string colorStr = JsonGuiParser::getProp(node, "color", "");

        auto& state = ControlState::checkboxStates[path];
        if (!ControlState::checkboxStates.count(path)) state = def;

        if (disabled) ImGui::BeginDisabled();
        if (!colorStr.empty()) ImGui::PushStyleColor(ImGuiCol_Text, parseColor(colorStr));

        bool old = state;
        std::string cbId = label + "##" + path;
        ImGui::Checkbox(cbId.c_str(), &state);
        if (old != state && !onChange.empty()) fireEvent(onChange);

        if (!colorStr.empty()) ImGui::PopStyleColor();
        if (disabled) ImGui::EndDisabled();

    // ──────────────────────────────────
    //  SLIDER
    // ──────────────────────────────────
    } else if (node.type == "slider") {
        std::string label = JsonGuiParser::getProp(node, "label", "");
        float minVal = JsonGuiParser::getPropFloat(node, "min", 0.0f);
        float maxVal = JsonGuiParser::getPropFloat(node, "max", 100.0f);
        float def = JsonGuiParser::getPropFloat(node, "default", minVal);
        std::string onChange = JsonGuiParser::getProp(node, "onChange", "");
        bool disabled = JsonGuiParser::getPropBool(node, "disabled", false);

        auto& state = ControlState::sliderStates[path];
        if (state < minVal || state > maxVal) state = def;

        if (disabled) ImGui::BeginDisabled();
        float old = state;
        if (!label.empty()) {
            char buf[128];
            snprintf(buf, sizeof(buf), "%s: %.0f##%s", label.c_str(), state, path.c_str());
            ImGui::SliderFloat(buf, &state, minVal, maxVal, "%.0f");
        } else {
            std::string sId = "##slider" + path;
            ImGui::SliderFloat(sId.c_str(), &state, minVal, maxVal, "%.0f");
        }
        if (old != state && !onChange.empty()) fireEvent(onChange);
        if (disabled) ImGui::EndDisabled();

    // ──────────────────────────────────
    //  INPUT
    // ──────────────────────────────────
    } else if (node.type == "input") {
        std::string placeholder = JsonGuiParser::getProp(node, "placeholder", "");
        std::string def = JsonGuiParser::getProp(node, "default", "");
        float w = JsonGuiParser::getPropFloat(node, "width", 0.0f);
        bool readonly = JsonGuiParser::getPropBool(node, "readonly", false);
        bool password = JsonGuiParser::getPropBool(node, "password", false);
        int maxLen = JsonGuiParser::getPropInt(node, "max_length", 255);
        std::string align = JsonGuiParser::getProp(node, "align", "left");
        int fontSize = JsonGuiParser::getPropInt(node, "font_size", 0);

        auto& state = ControlState::inputStates[path];
        if (state.empty() && !def.empty()) state = def;

        char buf[512];
        strncpy(buf, state.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';

        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;
        if (readonly) flags |= ImGuiInputTextFlags_ReadOnly;
        if (password) flags |= ImGuiInputTextFlags_Password;

        // Clamp max length
        if (maxLen > (int)sizeof(buf) - 1) maxLen = (int)sizeof(buf) - 1;

        std::string inputId = "##input" + path;
        FontSizeScope fss(fontSize);

        if (w > 0) ImGui::SetNextItemWidth(w);

        if (placeholder.empty()) {
            ImGui::InputText(inputId.c_str(), buf, (size_t)maxLen + 1, flags);
        } else {
            ImGui::InputTextWithHint(inputId.c_str(), placeholder.c_str(), buf, (size_t)maxLen + 1, flags);
        }

        if (align != "left") {
            // Align the text inside the input -- limited support; we can use SetCursorPosX
            // but InputText alignment is handled differently
        }

        state = buf;

    // ──────────────────────────────────
    //  COMBO
    // ──────────────────────────────────
    } else if (node.type == "combo") {
        std::string label = JsonGuiParser::getProp(node, "label", "");
        auto& state = ControlState::comboStates[path];
        bool disabled = JsonGuiParser::getPropBool(node, "disabled", false);

        std::string optsStr = JsonGuiParser::getProp(node, "options", "");
        if (optsStr.empty()) optsStr = "选项1,选项2,选项3";
        std::vector<std::string> opts;
        size_t start = 0, end;
        while ((end = optsStr.find(',', start)) != std::string::npos) {
            opts.push_back(optsStr.substr(start, end - start));
            start = end + 1;
        }
        if (start < optsStr.length()) opts.push_back(optsStr.substr(start));

        std::string items;
        for (auto& o : opts) { items += o; items.push_back('\0'); }
        items.push_back('\0');

        if (disabled) ImGui::BeginDisabled();
        if (!label.empty()) {
            std::string comboId = label + "##" + path;
            ImGui::Combo(comboId.c_str(), &state, items.c_str());
        } else {
            std::string comboId = "##combo" + path;
            ImGui::Combo(comboId.c_str(), &state, items.c_str());
        }
        if (disabled) ImGui::EndDisabled();

    // ──────────────────────────────────
    //  GROUP
    // ──────────────────────────────────
    } else if (node.type == "group") {
        std::string label = JsonGuiParser::getProp(node, "label", "");
        bool collapsible = JsonGuiParser::getPropBool(node, "collapsible", false);
        bool defaultOpen = JsonGuiParser::getPropBool(node, "default_open", false);
        float indent = JsonGuiParser::getPropFloat(node, "indent", 0.0f);

        if (collapsible) {
            std::string headerId = label + "##" + path;
            if (defaultOpen) ImGui::SetNextItemOpen(true);
            if (ImGui::CollapsingHeader(headerId.c_str())) {
                if (indent > 0) ImGui::Indent(indent);
                int idx = 0;
                for (auto& ch : node.children) renderNode(ch, path + "/grp" + std::to_string(idx++));
                if (indent > 0) ImGui::Unindent(indent);
            }
        } else {
            if (!label.empty()) { ImGui::TextUnformatted(label.c_str()); ImGui::Separator(); }
            if (indent > 0) ImGui::Indent(indent);
            int idx = 0;
            for (auto& ch : node.children) renderNode(ch, path + "/grp" + std::to_string(idx++));
            if (indent > 0) ImGui::Unindent(indent);
        }

    // ──────────────────────────────────
    //  SPACER
    // ──────────────────────────────────
    } else if (node.type == "spacer") {
        float w = JsonGuiParser::getPropFloat(node, "width", 0.0f);
        float h = JsonGuiParser::getPropFloat(node, "height", 8.0f);
        ImGui::Dummy(ImVec2(w, h));

    // ──────────────────────────────────
    //  PANEL
    // ──────────────────────────────────
    } else if (node.type == "panel") {
        std::string colorStr = JsonGuiParser::getProp(node, "color", "");
        float pad = JsonGuiParser::getPropFloat(node, "padding", 8.0f);
        float round = JsonGuiParser::getPropFloat(node, "round", 6.0f);
        std::string borderColorStr = JsonGuiParser::getProp(node, "border_color", "");
        float borderWidth = JsonGuiParser::getPropFloat(node, "border_width", 0);
        bool scrollable = JsonGuiParser::getPropBool(node, "scrollable", false);

        ImVec4 bgColor = parseColor(colorStr, ImVec4(0.12f, 0.12f, 0.14f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
        if (round > 0) ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, round);
        if (borderWidth > 0) ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, borderWidth);
        if (!borderColorStr.empty()) ImGui::PushStyleColor(ImGuiCol_Border, parseColor(borderColorStr));

        ImGuiWindowFlags pnlFlags = ImGuiWindowFlags_None;
        if (scrollable) pnlFlags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;

        ImGui::BeginChild(path.c_str(), ImVec2(0, 0), borderWidth >= 0, pnlFlags);
        if (round > 0) ImGui::PopStyleVar();
        if (borderWidth > 0) ImGui::PopStyleVar();
        ImGui::PopStyleColor(); // ChildBg
        if (!borderColorStr.empty()) ImGui::PopStyleColor();

        if (pad > 0) ImGui::Indent(pad);
        int idx = 0;
        for (auto& ch : node.children) renderNode(ch, path + "/pnl" + std::to_string(idx++));
        if (pad > 0) ImGui::Unindent(pad);
        ImGui::EndChild();

    // ──────────────────────────────────
    //  PROGRESS
    // ──────────────────────────────────
    } else if (node.type == "progress") {
        float val = JsonGuiParser::getPropFloat(node, "value", 0.5f);
        float maxVal = JsonGuiParser::getPropFloat(node, "max", 1.0f);
        std::string label = JsonGuiParser::getProp(node, "label", "");
        float w = JsonGuiParser::getPropFloat(node, "width", 0.0f);
        float h = JsonGuiParser::getPropFloat(node, "height", 0.0f);

        float pct = (maxVal > 0.0f) ? (val / maxVal) : 0.0f;
        ImGui::ProgressBar(pct, ImVec2(w > 0 ? w : 0, h > 0 ? h : 0), label.empty() ? nullptr : label.c_str());

    // ──────────────────────────────────
    //  IMAGE (with TextureManager support)
    // ──────────────────────────────────
    } else if (node.type == "image") {
        std::string texture = JsonGuiParser::getProp(node, "texture", "");
        float w = JsonGuiParser::getPropFloat(node, "width", 32);
        float h = JsonGuiParser::getPropFloat(node, "height", 32);
        float round = JsonGuiParser::getPropFloat(node, "round", 4.0f);
        std::string tintStr = JsonGuiParser::getProp(node, "tint", "");

        ImTextureID texId = (ImTextureID)0;
        bool isUrl = (texture.find("://") != std::string::npos);
        bool isLoading = false;

        if (isUrl) {
            // Remote texture from URL
            texId = TextureManager::get(texture);
            if (!texId && !TextureManager::isLoading(texture) && !TextureManager::hasFailed(texture)) {
                TextureManager::loadFromUrlAsync(texture);
            }
            isLoading = TextureManager::isLoading(texture);
        } else if (!texture.empty()) {
            // Local pre-registered texture
            texId = TextureManager::get(texture);
        }

        ImVec2 size(w > 0 ? w : 32, h > 0 ? h : 32);
        ImGui::InvisibleButton(("##img" + path).c_str(), size);

        auto dl = ImGui::GetWindowDrawList();
        ImVec2 pMin = ImGui::GetItemRectMin();
        ImVec2 pMax = ImGui::GetItemRectMax();

        if (texId) {
            // Render actual texture
            if (round > 0) {
                dl->AddImageRounded(texId, pMin, pMax, ImVec2(0,0), ImVec2(1,1),
                    IM_COL32(255,255,255,255), round);
            } else {
                dl->AddImage(texId, pMin, pMax);
            }
        } else if (isLoading) {
            // Loading indicator: spinning rect
            dl->AddRectFilled(pMin, pMax, IM_COL32(40, 40, 50, 200), round);
            dl->AddText(ImVec2(pMin.x + 4, pMin.y + (h-ImGui::GetFontSize())*0.5f),
                IM_COL32(128,128,128,255), "DL..");
        } else if (!tintStr.empty()) {
            // Tinted placeholder
            ImVec4 tint = parseColor(tintStr);
            dl->AddRectFilled(pMin, pMax, IM_COL32(
                (int)(tint.x*255), (int)(tint.y*255), (int)(tint.z*255), (int)(tint.w*255)), round);
        } else {
            // Default placeholder
            dl->AddRectFilled(pMin, pMax, IM_COL32(60, 60, 80, 200), round);
            if (texture.empty()) {
                dl->AddText(ImVec2(pMin.x + 4, pMin.y + (h-ImGui::GetFontSize())*0.5f),
                    IM_COL32(128,128,128,255), "img");
            }
        }

    // ──────────────────────────────────
    //  OVERLAY (F3-style debug)
    // ──────────────────────────────────
    } else if (node.type == "overlay") {
        float x = JsonGuiParser::getPropFloat(node, "x", 10.0f);
        float y = JsonGuiParser::getPropFloat(node, "y", 10.0f);
        bool showBg = JsonGuiParser::getPropBool(node, "showBg", true);
        std::string bgColorStr = JsonGuiParser::getProp(node, "bgColor", "#00000088");

        auto dl = ImGui::GetForegroundDrawList();
        float lh = ImGui::GetTextLineHeightWithSpacing();

        float maxW = 0;
        int lineCount = 0;
        for (auto& ch : node.children) {
            if (ch.type == "text") {
                std::string v = JsonGuiParser::getProp(ch, "value", "");
                std::string r = JsonGuiParser::getProp(ch, "ref", "");
                if (!r.empty()) v = DynamicText::get(r);
                if (!v.empty()) {
                    ImVec2 sz = ImGui::CalcTextSize(v.c_str());
                    if (sz.x > maxW) maxW = sz.x;
                    lineCount++;
                } else { lineCount++; }
            } else { lineCount++; }
        }

        if (showBg && lineCount > 0) {
            float pad = 8.0f;
            ImVec4 bgCol = parseColor(bgColorStr, ImVec4(0,0,0,0.4f));
            dl->AddRectFilled(ImVec2(x-pad, y-pad), ImVec2(x+maxW+pad, y+lineCount*lh+pad),
                IM_COL32((int)(bgCol.x*255),(int)(bgCol.y*255),(int)(bgCol.z*255),(int)(bgCol.w*255)), 4.0f);
        }

        float curY = y;
        for (auto& ch : node.children) {
            if (ch.type == "text") {
                std::string v = JsonGuiParser::getProp(ch, "value", "");
                std::string r = JsonGuiParser::getProp(ch, "ref", "");
                std::string cs = JsonGuiParser::getProp(ch, "color", "");
                if (!r.empty()) v = DynamicText::get(r);
                if (v.empty()) { curY += lh; continue; }
                ImVec4 col = cs.empty() ? ImVec4(0.8f,0.8f,0.8f,1.0f) : parseColor(cs);
                dl->AddText(ImVec2(x, curY), IM_COL32((int)(col.x*255),(int)(col.y*255),(int)(col.z*255),(int)(col.w*255)), v.c_str());
                curY += lh;
            } else if (ch.type == "separator") {
                if (maxW > 0) dl->AddLine(ImVec2(x, curY+lh*0.3f), ImVec2(x+maxW, curY+lh*0.3f), IM_COL32(128,128,128,128));
                curY += lh;
            } else { curY += lh; }
        }
    }
}

inline void renderDefinition(const GuiDefinition& def) {
    g_currentGuiId = def.id;
    try {
        renderNode(def.root, def.id);
    } catch (std::exception& e) {
        rLog(("CRASH_GUARD: renderNode exception for " + def.id + ": " + std::string(e.what())).c_str());
    } catch (...) {
        rLog(("CRASH_GUARD: renderNode unknown exception for " + def.id).c_str());
    }
}

}
