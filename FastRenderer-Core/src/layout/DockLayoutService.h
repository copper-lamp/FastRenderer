#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>

namespace DockLayoutService {

inline void initDockSpace(const std::string& id, const std::string& configPath) {
    // DockSpace support requires ImGui with Docking enabled
    // Will be implemented when xmake imgui package has docking=true
}

inline void beginDockSpace(const std::string& id) {
    // Stub - requires ImGui Docking feature
}

inline void saveLayout(const std::string& id) {
    // Stub
}

}
