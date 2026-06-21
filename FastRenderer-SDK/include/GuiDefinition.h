#pragma once
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

struct GuiNode {
    std::string type;
    std::map<std::string, std::string> props;
    std::vector<GuiNode> children;
};

struct GuiDefinition {
    GuiNode root;
    std::string id;
    std::string pluginId;
    std::string sourceFile;
    int version = 1;
};
