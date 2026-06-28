#pragma once
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstdio>
#include <GuiDefinition.h>
#include <gui/JsonGuiParser.h>
#include <gui/GuiService.h>

namespace HotReloadService {

struct State {
    std::string watchDir;
    std::map<std::string, std::filesystem::file_time_type> fileTimes;
    std::atomic<bool> running{false};
    std::thread watcherThread;
};

inline State& getState() {
    static State s;
    return s;
}

inline void hrLog(const char* msg) {
    FILE* f = nullptr;
    f = fopen("FastRenderer_HR.txt", "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

inline void scanDirectory() {
    auto& s = getState();
    if (!std::filesystem::exists(s.watchDir)) return;

    std::vector<GuiDefinition> fileDefs;
    std::map<std::string, std::filesystem::file_time_type> newTimes;
    bool changed = false;

    for (auto& entry : std::filesystem::directory_iterator(s.watchDir)) {
        if (!entry.is_regular_file()) continue;
        auto path = entry.path();
        if (path.extension() != ".json") continue;

        std::string fname = path.filename().string();
        auto mtime = std::filesystem::last_write_time(path);
        newTimes[fname] = mtime;

        auto it = s.fileTimes.find(fname);
        if (it == s.fileTimes.end() || it->second != mtime) {
            changed = true;
            auto def = JsonGuiParser::parseFile(path.string());
            def.sourceFile = path.string();
            fileDefs.push_back(def);
        } else {
            // Preserve existing file-based definition
            auto existingDefs = GuiService::getDefinitions();
            for (auto& oldDef : existingDefs) {
                if (oldDef.sourceFile == path.string()) {
                    fileDefs.push_back(oldDef);
                    break;
                }
            }
        }
    }

    // ═══ 合并：保留 API 注册的定义 + 文件定义 ═══
    // API 注册的定义 sourceFile 以 "api:" 开头
    // 文件定义 sourceFile 是绝对路径
    auto currentDefs = GuiService::getDefinitions();
    std::vector<GuiDefinition> mergedDefs;

    int keptApi = 0, keptFile = 0;
    for (auto& def : currentDefs) {
        if (def.sourceFile.find("api:") == 0) {
            mergedDefs.push_back(def);
            keptApi++;
        }
    }
    for (auto& def : fileDefs) {
        mergedDefs.push_back(def);
        keptFile++;
    }

    bool sizeChanged = (mergedDefs.size() != currentDefs.size());
    if (changed || sizeChanged) {
        char buf[256];
        snprintf(buf, sizeof(buf), "scan: api=%d files=%d total=%d (changed=%d sizeChanged=%d)",
            keptApi, keptFile, (int)mergedDefs.size(), (int)changed, (int)sizeChanged);
        hrLog(buf);
        GuiService::setDefinitions(mergedDefs);
        s.fileTimes = newTimes;
    }
}

inline void watcherLoop() {
    auto& s = getState();
    while (s.running) {
        scanDirectory();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

inline void init(const std::string& dir) {
    auto& s = getState();
    s.watchDir = dir;
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }
    scanDirectory();
    s.running = true;
    s.watcherThread = std::thread(watcherLoop);
    s.watcherThread.detach();
}

inline void shutdown() {
    getState().running = false;
}

inline void poll() {
    // Deliberately empty - scanning is done in background thread
    // The actual scan happens in watcherLoop every 500ms
}

inline std::string getWatchDir() {
    return getState().watchDir;
}

}
