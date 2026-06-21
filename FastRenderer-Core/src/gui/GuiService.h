#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <GuiDefinition.h>
#include <gui/JsonGuiParser.h>
#include <gui/JsonGuiRenderer.h>

namespace GuiService {

struct State {
    std::vector<GuiDefinition> definitions;
    std::mutex mutex;
    bool dirty = false;
};

inline State& getState() {
    static State s;
    return s;
}

inline void registerGui(const std::string& pluginId, const std::string& guiId,
    const std::string& jsonContent)
{
    auto def = JsonGuiParser::parseString(jsonContent);
    def.pluginId = pluginId;
    def.id = guiId;
    def.sourceFile = "api:" + guiId;

    auto& s = getState();
    std::lock_guard<std::mutex> lock(s.mutex);

    for (auto& existing : s.definitions) {
        if (existing.sourceFile == def.sourceFile) {
            existing = def;
            s.dirty = true;
            JsonGuiRenderer::clearControlStates();
            return;
        }
    }

    s.definitions.push_back(def);
    s.dirty = true;
    JsonGuiRenderer::clearControlStates();
}

inline void unregisterGui(const std::string& pluginId, const std::string& guiId) {
    auto& s = getState();
    std::lock_guard<std::mutex> lock(s.mutex);

    std::string targetSource = "api:" + guiId;
    s.definitions.erase(
        std::remove_if(s.definitions.begin(), s.definitions.end(),
            [&](const GuiDefinition& d) {
                return d.sourceFile == targetSource && d.pluginId == pluginId;
            }),
        s.definitions.end()
    );
    s.dirty = true;
}

inline std::vector<GuiDefinition> getDefinitions() {
    auto& s = getState();
    std::lock_guard<std::mutex> lock(s.mutex);
    return s.definitions;
}

inline void setDefinitions(const std::vector<GuiDefinition>& defs) {
    auto& s = getState();
    std::lock_guard<std::mutex> lock(s.mutex);
    s.definitions = defs;
    s.dirty = true;
    JsonGuiRenderer::clearControlStates();
}

inline bool consumeDirty() {
    auto& s = getState();
    std::lock_guard<std::mutex> lock(s.mutex);
    bool wasDirty = s.dirty;
    s.dirty = false;
    return wasDirty;
}

inline void renderAll() {
    auto defs = getDefinitions();
    if (defs.empty()) return;

    for (auto& def : defs) {
        JsonGuiRenderer::renderDefinition(def);
    }
}

}
