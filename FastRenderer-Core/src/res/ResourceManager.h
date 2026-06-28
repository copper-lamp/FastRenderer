// ════════════════════════════════════════════════════════════
//  ResourceManager — 统一资源管理
//  支持本地/远程资源，按模组分类存放
//  为图片查看器、音乐播放器等提供资源浏览 API
// ════════════════════════════════════════════════════════════

#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <cstdio>
#include <filesystem>
#include <chrono>
#include <algorithm>

namespace ResourceManager {

// ─── 资源来源 ───
enum class Source { Local, Remote };

// ─── 资源类型 ───
enum class Type { Image, Audio, Unknown };

// ─── 资源条目 ───
struct Entry {
    std::string id;           // 唯一标识
    std::string name;         // 文件名
    std::string filePath;     // 磁盘路径
    std::string pluginId;     // 所属模组
    Type type = Type::Unknown;
    Source source = Source::Local;
    int64_t fileSize = 0;
    std::chrono::system_clock::time_point addedTime;
};

// ─── 内部状态 ───
inline std::vector<Entry> g_entries;
inline std::mutex g_mutex;
inline std::string g_resourceBaseDir;

// ─── 日志 ───
inline void rmLog(const char* msg) {
    FILE* f = nullptr;
    fopen_s(&f, "FastRenderer_RM.txt", "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

// ─── 初始化 ───
inline void init(const std::string& baseDir) {
    g_resourceBaseDir = baseDir;
    std::filesystem::create_directories(baseDir);
    rmLog(("ResourceManager init: " + baseDir).c_str());
}

// ─── 获取资源来源字符串 ───
inline const char* sourceStr(Source s) { return s == Source::Remote ? "REMOTE" : "LOCAL"; }

// ─── 根据扩展名判断资源类型 ───
inline Type detectType(const std::string& ext) {
    std::string e;
    for (auto c : ext) e.push_back((char)tolower(c));
    if (e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".bmp" || e == ".gif") return Type::Image;
    if (e == ".mp3" || e == ".wav" || e == ".ogg" || e == ".flac" || e == ".aac") return Type::Audio;
    return Type::Unknown;
}

// ─── 扫描目录添加资源 ───
inline void scanDirectory(const std::string& dirPath, const std::string& pluginId, Source source) {
    if (!std::filesystem::exists(dirPath)) {
        rmLog(("[RM] scanDirectory SKIP: not found - " + dirPath).c_str());
        return;
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    int added = 0, skipped = 0;

    for (auto& entry : std::filesystem::recursive_directory_iterator(dirPath)) {
        if (!entry.is_regular_file()) continue;
        auto path = entry.path();
        auto ext = path.extension().string();
        auto type = detectType(ext);
        if (type == Type::Unknown) { skipped++; continue; }

        std::string fpath = path.string();
        std::string fname = path.filename().string();

        bool exists = false;
        for (auto& e : g_entries) { if (e.filePath == fpath) { exists = true; break; } }
        if (exists) { skipped++; continue; }

        Entry e;
        e.id = pluginId + ":" + fname;
        e.name = fname;
        e.filePath = fpath;
        e.pluginId = pluginId;
        e.type = type;
        e.source = source;
        e.fileSize = entry.file_size();
        e.addedTime = std::chrono::system_clock::now();
        g_entries.push_back(e);
        added++;
    }

    if (added > 0 || skipped > 0)
        rmLog((std::string("[RM] scanDirectory: ") + dirPath + " plugin=" + pluginId + " added=" + std::to_string(added) + " skipped=" + std::to_string(skipped)).c_str());
    else
        rmLog(("[RM] scanDirectory: empty dir - " + dirPath).c_str());
}

// ─── 手动注册资源 ───
inline void registerFile(const std::string& filePath, const std::string& pluginId, Source source) {
    if (!std::filesystem::exists(filePath)) {
        rmLog(("[RM] registerFile FAILED: not found - " + filePath).c_str());
        return;
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    auto path = std::filesystem::path(filePath);
    auto ext = path.extension().string();
    auto type = detectType(ext);
    if (type == Type::Unknown) {
        rmLog(("[RM] registerFile SKIP: unknown type " + ext + " - " + filePath).c_str());
        return;
    }

    std::string fname = path.filename().string();

    // 去重
    for (auto& e : g_entries) {
        if (e.filePath == filePath) {
            rmLog(("[RM] registerFile SKIP: duplicate - " + filePath).c_str());
            return;
        }
    }

    auto fsize = std::filesystem::file_size(path);
    const char* typeName = (type == Type::Image) ? "IMG" : (type == Type::Audio) ? "AUD" : "UNK";

    Entry e;
    e.id = pluginId + ":" + fname;
    e.name = fname;
    e.filePath = filePath;
    e.pluginId = pluginId;
    e.type = type;
    e.source = source;
    e.fileSize = fsize;
    e.addedTime = std::chrono::system_clock::now();
    g_entries.push_back(e);

    char sz[32];
    if (fsize > 1048576) snprintf(sz, sizeof(sz), "%.1fMB", fsize / 1048576.0f);
    else if (fsize > 1024) snprintf(sz, sizeof(sz), "%.1fKB", fsize / 1024.0f);
    else snprintf(sz, sizeof(sz), "%lldB", (long long)fsize);
    rmLog((std::string("[RM] REGISTER OK: [") + typeName + "] " + fname + " (" + sz + ") plugin=" + pluginId + " source=" + sourceStr(source) + " path=" + filePath).c_str());
}

// ─── 按模组获取资源列表 ───
inline std::vector<Entry> getByPlugin(const std::string& pluginId) {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::vector<Entry> result;
    for (auto& e : g_entries) {
        if (e.pluginId == pluginId) result.push_back(e);
    }
    return result;
}

// ─── 按来源获取资源列表 ───
inline std::vector<Entry> getBySource(Source source) {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::vector<Entry> result;
    for (auto& e : g_entries) {
        if (e.source == source) result.push_back(e);
    }
    return result;
}

// ─── 按类型获取资源列表 ───
inline std::vector<Entry> getByType(Type type) {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::vector<Entry> result;
    for (auto& e : g_entries) {
        if (e.type == type) result.push_back(e);
    }
    return result;
}

// ─── 获取所有资源 ───
inline std::vector<Entry> getAll() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_entries;
}

// ─── 获取所有模组列表 ───
inline std::vector<std::string> getPlugins() {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::set<std::string> plugins;
    for (auto& e : g_entries) plugins.insert(e.pluginId);
    return std::vector<std::string>(plugins.begin(), plugins.end());
}

// ─── 统计信息 ───
inline int countByType(Type type) {
    std::lock_guard<std::mutex> lock(g_mutex);
    return (int)std::count_if(g_entries.begin(), g_entries.end(),
        [type](const Entry& e) { return e.type == type; });
}

inline int countBySource(Source source) {
    std::lock_guard<std::mutex> lock(g_mutex);
    return (int)std::count_if(g_entries.begin(), g_entries.end(),
        [source](const Entry& e) { return e.source == source; });
}

inline int totalCount() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return (int)g_entries.size();
}

// ─── 扫描 FR 资源目录（按模组子目录）───
inline void scanAll() {
    if (g_resourceBaseDir.empty()) { rmLog("[RM] scanAll SKIP: baseDir not set"); return; }
    rmLog((std::string("[RM] scanAll: scanning ") + g_resourceBaseDir).c_str());

    int before = totalCount();
    scanDirectory(g_resourceBaseDir, "FR", Source::Remote);

    if (std::filesystem::exists(g_resourceBaseDir)) {
        for (auto& entry : std::filesystem::directory_iterator(g_resourceBaseDir)) {
            if (entry.is_directory()) {
                std::string pluginId = entry.path().filename().string();
                scanDirectory(entry.path().string(), pluginId, Source::Remote);
            }
        }
    }

    int after = totalCount();
    rmLog((std::string("[RM] scanAll complete: before=") + std::to_string(before) + " after=" + std::to_string(after) + " new=" + std::to_string(after-before)).c_str());
}

} // namespace ResourceManager
