#pragma once
#include <imgui.h>
#include <string>
#include <vector>
#include <functional>
#include <GuiDefinition.h>
#include <gui/JsonGuiParser.h>
#include <gui/DynamicText.h>

namespace JsonGuiRenderer {

inline std::function<void(const std::string&)> g_eventCallback = nullptr;

inline void setEventCallback(std::function<void(const std::string&)> cb) {
    g_eventCallback = cb;
}

inline ImVec4 parseColor(const std::string& str, const ImVec4& fallback = ImVec4(1,1,1,1)) {
    if (str.empty()) return fallback;
    if (str[0] == '#') {
        unsigned int r=255, g=255, b=255, a=255;
        if (str.length() >= 7) {
            sscanf_s(str.c_str() + 1, "%02x%02x%02x", &r, &g, &b);
        }
        if (str.length() >= 9) {
            sscanf_s(str.c_str() + 7, "%02x", &a);
        }
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
    if (g_eventCallback && !eventName.empty()) {
        g_eventCallback(eventName);
    }
}

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

inline void renderNode(const GuiNode& node, const std::string& path = "root") {
    if (node.type == "window") {
        std::string title = JsonGuiParser::getProp(node, "title", "窗口");
        int w = JsonGuiParser::getPropInt(node, "width", 400);
        int h = JsonGuiParser::getPropInt(node, "height", 300);
        ImGui::SetNextWindowSize(ImVec2((float)w, (float)h), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(title.c_str())) {
            int idx = 0;
            for (auto& child : node.children) {
                renderNode(child, path + "/" + std::to_string(idx++));
            }
        }
        ImGui::End();

    } else if (node.type == "column") {
        float spacing = JsonGuiParser::getPropFloat(node, "spacing", 6.0f);
        if (spacing > 0) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
        }
        int idx = 0;
        for (auto& child : node.children) {
            renderNode(child, path + "/col" + std::to_string(idx++));
        }
        if (spacing > 0) {
            ImGui::PopStyleVar();
        }

    } else if (node.type == "row") {
        float spacing = JsonGuiParser::getPropFloat(node, "spacing", 6.0f);
        int idx = 0;
        for (auto& child : node.children) {
            if (idx > 0) ImGui::SameLine(0.0f, spacing);
            renderNode(child, path + "/row" + std::to_string(idx));
            idx++;
        }

    } else if (node.type == "button") {
        std::string text = JsonGuiParser::getProp(node, "text", "按钮");
        std::string colorStr = JsonGuiParser::getProp(node, "color", "");
        std::string onClick = JsonGuiParser::getProp(node, "onClick", "");
        float w = JsonGuiParser::getPropFloat(node, "width", 0.0f);
        float h = JsonGuiParser::getPropFloat(node, "height", 0.0f);
        bool disabled = JsonGuiParser::getPropBool(node, "disabled", false);

        if (!colorStr.empty()) {
            ImVec4 col = parseColor(colorStr);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(col.x, col.y, col.z, col.w * 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(col.x * 1.1f, col.y * 1.1f, col.z * 1.1f, col.w));
        }

        if (disabled) ImGui::BeginDisabled();

        if (ImGui::Button(text.c_str(), ImVec2(w > 0 ? w : 0, h > 0 ? h : 0))) {
            fireEvent(onClick);
        }

        if (disabled) ImGui::EndDisabled();

        if (!colorStr.empty()) ImGui::PopStyleColor(3);

    } else if (node.type == "text") {
        std::string value = JsonGuiParser::getProp(node, "value", "");
        std::string ref = JsonGuiParser::getProp(node, "ref", "");
        if (!ref.empty()) {
            value = DynamicText::get(ref);
        }
        std::string colorStr = JsonGuiParser::getProp(node, "color", "");
        if (!colorStr.empty()) {
            ImVec4 col = parseColor(colorStr);
            ImGui::TextColored(col, "%s", value.c_str());
        } else {
            ImGui::TextUnformatted(value.c_str());
        }

    } else if (node.type == "separator") {
        ImGui::Separator();

    } else if (node.type == "checkbox") {
        std::string label = JsonGuiParser::getProp(node, "label", "");
        bool def = JsonGuiParser::getPropBool(node, "default", false);
        std::string onChange = JsonGuiParser::getProp(node, "onChange", "");

        auto& state = ControlState::checkboxStates[path];
        if (!ControlState::checkboxStates.count(path)) {
            state = def;
        }

        bool old = state;
        ImGui::Checkbox(label.c_str(), &state);
        if (old != state && !onChange.empty()) {
            fireEvent(onChange);
        }

    } else if (node.type == "slider") {
        std::string label = JsonGuiParser::getProp(node, "label", "");
        float minVal = JsonGuiParser::getPropFloat(node, "min", 0.0f);
        float maxVal = JsonGuiParser::getPropFloat(node, "max", 100.0f);
        float def = JsonGuiParser::getPropFloat(node, "default", minVal);
        std::string onChange = JsonGuiParser::getProp(node, "onChange", "");

        auto& state = ControlState::sliderStates[path];
        if (state < minVal || state > maxVal) {
            state = def;
        }

        float old = state;
        char labelBuf[128];
        if (!label.empty()) {
            snprintf(labelBuf, sizeof(labelBuf), "%s: %.0f", label.c_str(), state);
            ImGui::SliderFloat(labelBuf, &state, minVal, maxVal, "%.0f");
        } else {
            ImGui::SliderFloat("##slider", &state, minVal, maxVal, "%.0f");
        }
        if (old != state && !onChange.empty()) {
            fireEvent(onChange);
        }

    } else if (node.type == "input") {
        std::string placeholder = JsonGuiParser::getProp(node, "placeholder", "");
        std::string def = JsonGuiParser::getProp(node, "default", "");

        auto& state = ControlState::inputStates[path];
        if (state.empty() && !def.empty()) {
            state = def;
        }

        char buf[256];
        strncpy_s(buf, sizeof(buf), state.c_str(), sizeof(buf) - 1);
        if (placeholder.empty()) {
            ImGui::InputText("##input", buf, sizeof(buf));
        } else {
            ImGui::InputTextWithHint("##input", placeholder.c_str(), buf, sizeof(buf));
        }
        state = buf;

    } else if (node.type == "combo") {
        std::string label = JsonGuiParser::getProp(node, "label", "");
        auto& state = ControlState::comboStates[path];

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
        for (auto& o : opts) {
            items += o;
            items.push_back('\0');
        }
        items.push_back('\0');

        if (!label.empty()) {
            ImGui::Combo(label.c_str(), &state, items.c_str());
        } else {
            ImGui::Combo("##combo", &state, items.c_str());
        }

    } else if (node.type == "group") {
        std::string label = JsonGuiParser::getProp(node, "label", "");
        bool collapsible = JsonGuiParser::getPropBool(node, "collapsible", false);

        if (collapsible) {
            if (ImGui::CollapsingHeader(label.c_str())) {
                ImGui::Indent();
                int idx = 0;
                for (auto& child : node.children) {
                    renderNode(child, path + "/grp" + std::to_string(idx++));
                }
                ImGui::Unindent();
            }
        } else {
            if (!label.empty()) {
                ImGui::TextUnformatted(label.c_str());
                ImGui::Separator();
            }
            int idx = 0;
            for (auto& child : node.children) {
                renderNode(child, path + "/grp" + std::to_string(idx++));
            }
        }

    } else if (node.type == "spacer") {
        float w = JsonGuiParser::getPropFloat(node, "width", 0.0f);
        float h = JsonGuiParser::getPropFloat(node, "height", 8.0f);
        ImGui::Dummy(ImVec2(w, h));

    } else if (node.type == "panel") {
        std::string colorStr = JsonGuiParser::getProp(node, "color", "");
        float pad = JsonGuiParser::getPropFloat(node, "padding", 8.0f);
        float round = JsonGuiParser::getPropFloat(node, "round", 6.0f);

        ImVec4 bgColor = parseColor(colorStr, ImVec4(0.12f, 0.12f, 0.14f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, round);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);

        ImGui::BeginChild(path.c_str(), ImVec2(0, 0), true);
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        if (pad > 0) ImGui::Indent(pad);

        int idx = 0;
        for (auto& child : node.children) {
            renderNode(child, path + "/pnl" + std::to_string(idx++));
        }

        if (pad > 0) ImGui::Unindent(pad);
        ImGui::EndChild();

    } else if (node.type == "progress") {
        float val = JsonGuiParser::getPropFloat(node, "value", 0.5f);
        float maxVal = JsonGuiParser::getPropFloat(node, "max", 1.0f);
        std::string label = JsonGuiParser::getProp(node, "label", "");
        float w = JsonGuiParser::getPropFloat(node, "width", 0.0f);
        float h = JsonGuiParser::getPropFloat(node, "height", 0.0f);

        float pct = (maxVal > 0.0f) ? (val / maxVal) : 0.0f;
        ImGui::ProgressBar(pct, ImVec2(w > 0 ? w : 0, h > 0 ? h : 0), label.empty() ? nullptr : label.c_str());
    }
}

inline void renderDefinition(const GuiDefinition& def) {
    renderNode(def.root, def.id);
}

}
