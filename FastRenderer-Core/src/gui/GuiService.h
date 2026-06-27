#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <cstdio>
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

inline void gsLog(const char* msg) {
    FILE* f = nullptr;
    f = fopen("FastRenderer_GS.txt", "a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}

inline void registerGui(const std::string& pluginId, const std::string& guiId,
    const std::string& jsonContent)
{
    auto def = JsonGuiParser::parseString(jsonContent);
    def.pluginId = pluginId;
    def.id = guiId;
    def.sourceFile = "api:" + guiId;

    gsLog(("registerGui: " + guiId + " plugin=" + pluginId + " root.type=" + def.root.type + " root.props.size=" + std::to_string(def.root.props.size()) + " children=" + std::to_string(def.root.children.size())).c_str());

    auto& s = getState();
    std::lock_guard<std::mutex> lock(s.mutex);

    for (auto& existing : s.definitions) {
        if (existing.sourceFile == def.sourceFile) {
            existing = def;
            s.dirty = true;
            JsonGuiRenderer::clearControlStates();
            gsLog(("  -> replaced existing, total=" + std::to_string(s.definitions.size())).c_str());
            return;
        }
    }

    s.definitions.push_back(def);
    s.dirty = true;
    JsonGuiRenderer::clearControlStates();
    gsLog(("  -> added new, total=" + std::to_string(s.definitions.size())).c_str());
}

inline bool unregisterGui(const std::string& pluginId, const std::string& guiId) {
    auto& s = getState();
    std::lock_guard<std::mutex> lock(s.mutex);

    std::string targetSource = "api:" + guiId;
    auto before = s.definitions.size();
    s.definitions.erase(
        std::remove_if(s.definitions.begin(), s.definitions.end(),
            [&](const GuiDefinition& d) {
                return d.sourceFile == targetSource && d.pluginId == pluginId;
            }),
        s.definitions.end()
    );
    bool removed = (s.definitions.size() < before);
    if (removed) s.dirty = true;
    return removed;
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

inline int g_renderCount = 0;

inline void renderAll() {
    auto defs = getDefinitions();
    if (defs.empty()) {
        if (++g_renderCount % 300 == 0) gsLog("renderAll: NO definitions to render");
        return;
    }

    if (++g_renderCount % 300 == 0) {
        gsLog(("renderAll: rendering " + std::to_string(defs.size()) + " defs").c_str());
        for (auto& d : defs) {
            gsLog(("  def: " + d.id + " type=" + d.root.type + " children=" + std::to_string(d.root.children.size())).c_str());
        }
    }

    for (auto& def : defs) {
        JsonGuiRenderer::renderDefinition(def);
    }
}

}
