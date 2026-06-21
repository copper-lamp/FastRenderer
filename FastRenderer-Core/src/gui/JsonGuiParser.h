#pragma once
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <GuiDefinition.h>

namespace JsonGuiParser {

inline std::string getProp(const GuiNode& node, const std::string& key, const std::string& def = "") {
    auto it = node.props.find(key);
    return (it != node.props.end()) ? it->second : def;
}

inline int getPropInt(const GuiNode& node, const std::string& key, int def = 0) {
    auto it = node.props.find(key);
    return (it != node.props.end()) ? std::stoi(it->second) : def;
}

inline float getPropFloat(const GuiNode& node, const std::string& key, float def = 0.0f) {
    auto it = node.props.find(key);
    return (it != node.props.end()) ? std::stof(it->second) : def;
}

inline bool getPropBool(const GuiNode& node, const std::string& key, bool def = false) {
    auto it = node.props.find(key);
    if (it == node.props.end()) return def;
    std::string v = it->second;
    for (auto& c : v) c = (char)tolower(c);
    return (v == "true" || v == "1");
}

inline GuiNode parseNode(const nlohmann::json& j) {
    GuiNode node;
    if (j.contains("type") && j["type"].is_string()) {
        node.type = j["type"].get<std::string>();
    }
    if (j.contains("props") && j["props"].is_object()) {
        for (auto& [key, val] : j["props"].items()) {
            if (val.is_string()) {
                node.props[key] = val.get<std::string>();
            } else {
                node.props[key] = val.dump();
            }
        }
    }
    if (j.contains("children") && j["children"].is_array()) {
        for (auto& child : j["children"]) {
            node.children.push_back(parseNode(child));
        }
    }
    return node;
}

inline GuiDefinition parseString(const std::string& jsonStr) {
    GuiDefinition def;
    def.sourceFile = "<string>";
    try {
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        if (j.contains("id") && j["id"].is_string()) {
            def.id = j["id"].get<std::string>();
        } else {
            def.id = "unnamed";
        }
        if (j.contains("root") && j["root"].is_object()) {
            def.root = parseNode(j["root"]);
        } else {
            def.root = parseNode(j);
        }
    } catch (const std::exception& e) {
        def.id = "parse_error";
        GuiNode errNode;
        errNode.type = "text";
        errNode.props["value"] = std::string("解析错误: ") + e.what();
        def.root = errNode;
    }
    return def;
}

inline GuiDefinition parseFile(const std::string& filePath) {
    GuiDefinition def;
    def.sourceFile = filePath;

    std::ifstream file(filePath);
    if (!file.is_open()) {
        def.id = "error";
        GuiNode errNode;
        errNode.type = "text";
        errNode.props["value"] = "无法加载文件: " + filePath;
        def.root = errNode;
        return def;
    }

    try {
        nlohmann::json j;
        file >> j;

        if (j.contains("id") && j["id"].is_string()) {
            def.id = j["id"].get<std::string>();
        } else {
            def.id = "unnamed";
        }

        if (j.contains("root") && j["root"].is_object()) {
            def.root = parseNode(j["root"]);
        } else {
            def.root = parseNode(j);
        }
    } catch (const std::exception& e) {
        def.id = "parse_error";
        GuiNode errNode;
        errNode.type = "text";
        errNode.props["value"] = std::string("解析错误: ") + e.what();
        def.root = errNode;
    }

    return def;
}

}
