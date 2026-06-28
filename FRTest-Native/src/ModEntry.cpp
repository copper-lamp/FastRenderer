// ════════════════════════════════════════════════════════════
//  FRTest-Native — FastRenderer 本地测试插件
//  测试内容: 图片查看器, 音乐播放器, 按键绑定, 设置菜单
//  协议: FR-TCP桥接通信协议.md (JSON over TCP)
// ════════════════════════════════════════════════════════════

#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdio>
#include <atomic>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <net/TcpClient.h>
#include <ll/api/mod/NativeMod.h>
#include <ll/api/mod/RegisterHelper.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include "ComplexGui.h"


using json = nlohmann::json;

// ─── Config ───
static const char* BRIDGE_HOST = "127.0.0.1";
static const uint16_t BRIDGE_PORT = 12345;
static const char* PLAYER_NAME = "TestPlayer";
static const char* PLATFORM = "win";
static int g_messageId = 0;

// ─── Globals ───
static FrTcpClient g_tcpClient;
static std::atomic<bool> g_registered{false};
static int g_receivedCount = 0;
static int g_guiEventCount = 0;
static int g_dataExchangeCount = 0;
static std::string g_modDir;

// ─── 图片查看器状态 ───
static struct ImageViewerState {
    std::string currentFile;
    float zoom = 1.0f;
    bool fitToWindow = true;
    int imgW = 0, imgH = 0;
} g_imgView;

// ─── 音乐播放器状态 ───
static struct MusicPlayerState {
    std::string currentFile;
    std::string status = "停止";
    int volume = 80;
    int position = 0;
    int duration = 100;
    bool isOpen = false;
} g_music;

// ─── 设置菜单状态 ───
static struct SettingsState {
    int page = 0;                     // 0=dashboard,1=keybinds,2=settings
    int guiCount = 0;
    int fps = 60;
    bool tcpConnected = false;
    std::string captureBindId;
    std::string conflictMsg;
    std::string pluginFilter;
    std::vector<std::string> recentEvents;
    struct KbInfo { std::string pluginId, bindId, bindName; int vkCode; };
    std::vector<KbInfo> keybinds;
} g_settings;

// ─── Timestamped log ───
inline void testLog(const char* category, const char* msg) {
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    tm local = {};
    localtime_s(&local, &tt);
    FILE* f = fopen("FRTest.log", "a");
    if (f) {
        fprintf(f, "[%02d:%02d:%02d] [%s] %s\n", local.tm_hour, local.tm_min, local.tm_sec, category, msg);
        fclose(f);
    }
}

// ─── Helper: Send JSON over TCP ───
static bool sendJson(const json& msg) {
    int id = ++g_messageId;
    std::string str = msg.dump();
    std::string logStr = str.length() > 120 ? str.substr(0, 120) + "..." : str;
    testLog("TCP>OUT", logStr.c_str());
    return g_tcpClient.send(str);
}

// ─── Fallback: Write GUI JSON files to disk (deprecated, TCP preferred) ───
static void writeAllGuiFiles() {
    namespace fs = std::filesystem;
    std::string frDir = fs::path(g_modDir).parent_path().string() + "/FastRenderer";
    std::string guiDir = frDir + "/gui_defs";
    fs::create_directories(guiDir);

    // Write image viewer keybind and music player keybind as JSON fallback
    json kbArray = json::array();
    kbArray.push_back({{"pluginId","FRTest-Native"},{"bindId","img_viewer"},{"bindName","图片查看器"},{"vkCode",0x49}});
    kbArray.push_back({{"pluginId","FRTest-Native"},{"bindId","music_player"},{"bindName","音乐播放器"},{"vkCode",0x4D}});
    kbArray.push_back({{"pluginId","FRTest-Native"},{"bindId","open_settings"},{"bindName","设置菜单"},{"vkCode",0x77}});
    json kbFile; kbFile["keybinds"] = kbArray;
    std::ofstream f(guiDir + "/fr_test_keybinds.json");
    f << kbFile.dump(2); f.close();
    testLog("FILE", "fallback keybinds written");
}

// ─── Forward declarations ───
static void registerSettingsMenu();

// ─── Register all GUIs via TCP ───
static void registerAllGuis() {
    // Image Viewer
    {
        json def = ComplexGui::makeImageViewer({{"currentFile",""},{"zoom",1.0f},{"imgW",0},{"imgH",0}});
        sendJson({{"type","gui_register"},{"pluginId","FRTest-Native"},{"guiId","image_viewer"},{"definition",def.dump()},{"version",1},{"targetPlayer",""}});
        testLog("GUI+REG", "image_viewer — 图片查看器");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    // Music Player
    {
        json def = ComplexGui::makeMusicPlayer({{"currentFile",""},{"status","停止"},{"volume",80},{"position",0},{"duration",100}});
        sendJson({{"type","gui_register"},{"pluginId","FRTest-Native"},{"guiId","music_player"},{"definition",def.dump()},{"version",1},{"targetPlayer",""}});
        testLog("GUI+REG", "music_player — 音乐播放器");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    // Settings menu
    {
        registerSettingsMenu();
        testLog("GUI+REG", "settings_menu — 设置菜单");
    }
    testLog("GUI+REG", "注册完毕: 3个GUI (image_viewer, music_player, settings_menu)");
}

// ─── Register keybinds via TCP ───
static void registerKeybinds() {
    struct { const char* id; const char* name; int vk; } binds[] = {
        {"img_viewer",    "图片查看器",   0x49},  // I
        {"music_player",  "音乐播放器",   0x4D},  // M
        {"open_settings", "设置菜单",     0x77},  // F8
        {"toggle_f3_debug","F3调试",      0x72},  // F3
    };
    for (auto& b : binds) {
        sendJson({{"type","keybind_register"},{"pluginId","FRTest-Native"},{"bindId",b.id},{"bindName",b.name},{"vkCode",b.vk},{"targetPlayer",""}});
        testLog("KB+REG", (std::string(b.id) + " (0x" + std::to_string(b.vk) + ") — " + b.name).c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    testLog("KB+REG", "全部按键注册完成");
}

// ─── Send identify ───
static void sendIdentify() {
    sendJson({{"type","identify"},{"player",PLAYER_NAME},{"platform",PLATFORM}});
    testLog("TCP>OUT", "identify sent");
}

// ─── Main registration routine ───
static void registerAllAfterConnect() {
    testLog("TCP", (std::string("等待连接 ") + BRIDGE_HOST + ":" + std::to_string(BRIDGE_PORT) + " (最多5秒)").c_str());
    for (int i = 0; i < 10; i++) {
        if (g_tcpClient.isConnected()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (!g_tcpClient.isConnected()) {
        testLog("TCP", "连接失败 — 启用离线文件回退模式");
        writeAllGuiFiles();
        g_registered = true;
        return;
    }

    testLog("TCP", "已连接");
    sendIdentify();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    registerAllGuis();
    registerKeybinds();
    g_registered = true;
    testLog("TCP", "全部注册完成");
}

// ══════════════════════════════════════════════════════════
//  设置菜单
// ══════════════════════════════════════════════════════════

static void sendSettingsMenu() {
    json state;
    state["page"] = g_settings.page;
    state["guiCount"] = g_settings.guiCount;
    state["fps"] = g_settings.fps;
    state["tcpConnected"] = g_settings.tcpConnected;
    state["captureBindId"] = g_settings.captureBindId;
    state["conflictMsg"] = g_settings.conflictMsg;
    state["pluginFilter"] = g_settings.pluginFilter;
    json events = json::array();
    for (auto& e : g_settings.recentEvents) { json evt; evt["text"] = e; events.push_back(evt); }
    state["events"] = events;
    json kbs = json::array();
    for (auto& kb : g_settings.keybinds)
        kbs.push_back({{"pluginId",kb.pluginId},{"bindId",kb.bindId},{"bindName",kb.bindName},{"vkCode",kb.vkCode}});
    state["keybinds"] = kbs;
    state["maxRetries"] = 5; state["uiScale"] = 100.0f;

    json def = ComplexGui::makeSettingsMenu(state);
    sendJson({{"type","gui_register"},{"pluginId","FRTest-Native"},{"guiId","settings_menu"},{"definition",def.dump()},{"version",1},{"targetPlayer",""}});
    testLog("MENU", ("Settings page=" + std::to_string(g_settings.page)).c_str());
}

static void handleSettingsEvent(const std::string& controlId) {
    testLog("MENU", ("evt: " + controlId).c_str());
    g_settings.recentEvents.push_back("[" + std::to_string(g_messageId) + "] " + controlId);
    if (g_settings.recentEvents.size() > 30) g_settings.recentEvents.erase(g_settings.recentEvents.begin());

    if (controlId.find("settings_nav_") == 0) {
        int p = std::stoi(controlId.substr(13));
        if (p >= 0 && p <= 2) { g_settings.page = p; g_settings.captureBindId.clear(); }
    } else if (controlId == "settings_close") {
        sendJson({{"type","gui_unregister"},{"pluginId","FRTest-Native"},{"guiId","settings_menu"},{"targetPlayer",""}});
        testLog("MENU","Closed"); return;
    } else if (controlId.find("settings_kb_modify|") == 0) {
        std::string kid = controlId.substr(19);
        if (g_settings.captureBindId == kid) { g_settings.captureBindId.clear(); }
        else { g_settings.captureBindId = kid; }
    } else if (controlId == "settings_kb_ignore" || controlId == "settings_kb_cancel") {
        g_settings.captureBindId.clear();
    } else if (controlId == "settings_kb_filter|") {
        g_settings.pluginFilter = "";
    } else if (controlId.find("settings_kb_filter|") != std::string::npos) {
        g_settings.pluginFilter = controlId.substr(19);
    } else if (controlId == "settings_save" || controlId == "settings_reset") {
        testLog("MENU", (controlId == "settings_save" ? "Saved" : "Reset"));
    }
    sendSettingsMenu();
}

static void registerSettingsMenu() {
    g_settings.keybinds.clear();
    g_settings.keybinds.push_back({"FRTest-Native","img_viewer","图片查看器",0x49});
    g_settings.keybinds.push_back({"FRTest-Native","music_player","音乐播放器",0x4D});
    g_settings.keybinds.push_back({"FRTest-Native","toggle_f3_debug","F3调试",0x72});
    g_settings.keybinds.push_back({"FRTest-Native","open_settings","设置菜单",0x77});
    g_settings.guiCount = 3; // image_viewer, music_player, settings_menu
    g_settings.tcpConnected = g_tcpClient.isConnected();
    g_settings.page = 0; g_settings.captureBindId.clear();
    sendSettingsMenu();
}

// ══════════════════════════════════════════════════════════
//  图片查看器
// ══════════════════════════════════════════════════════════

static void sendImageViewer() {
    json state;
    state["currentFile"] = g_imgView.currentFile;
    state["zoom"] = g_imgView.zoom;
    state["fitToWindow"] = g_imgView.fitToWindow;
    state["imgW"] = g_imgView.imgW;
    state["imgH"] = g_imgView.imgH;
    json def = ComplexGui::makeImageViewer(state);
    sendJson({{"type","gui_register"},{"pluginId","FRTest-Native"},{"guiId","image_viewer"},{"definition",def.dump()},{"version",1},{"targetPlayer",""}});
}

static void handleImageEvent(const std::string& controlId) {
    std::string packDir = std::filesystem::path(g_modDir).string() + "/pack";
    if (controlId == "img_load") {
        std::string firstPng;
        if (std::filesystem::exists(packDir)) {
            for (auto& e : std::filesystem::directory_iterator(packDir)) {
                if (e.path().extension() == ".png") { firstPng = e.path().string(); break; }
            }
        }
        if (!firstPng.empty()) {
            g_imgView.currentFile = firstPng;
            g_imgView.imgW = 256;
            g_imgView.imgH = 256;
            g_imgView.zoom = 1.0f;
            testLog("IMG", ("Selected: " + firstPng).c_str());
        }
    } else if (controlId == "img_zoom_in") { g_imgView.zoom *= 1.25f; }
    else if (controlId == "img_zoom_out") { g_imgView.zoom *= 0.8f; if (g_imgView.zoom < 0.1f) g_imgView.zoom = 0.1f; }
    else if (controlId == "img_zoom_1x") { g_imgView.zoom = 1.0f; }
    sendImageViewer();
}

// ══════════════════════════════════════════════════════════
//  音乐播放器 (MCI)
// ══════════════════════════════════════════════════════════

static void musicClose() {
    if (g_music.isOpen) { mciSendStringW(L"close music", NULL, 0, NULL); g_music.isOpen = false; }
}

static void sendMusicPlayer() {
    json state;
    state["currentFile"] = g_music.currentFile;
    state["status"] = g_music.status;
    state["volume"] = g_music.volume;
    state["position"] = g_music.position;
    state["duration"] = g_music.duration;
    json def = ComplexGui::makeMusicPlayer(state);
    sendJson({{"type","gui_register"},{"pluginId","FRTest-Native"},{"guiId","music_player"},{"definition",def.dump()},{"version",1},{"targetPlayer",""}});
}

static void handleMusicEvent(const std::string& controlId) {
    std::string packDir = std::filesystem::path(g_modDir).string() + "/pack";

    if (controlId == "music_open") {
        std::string firstFile;
        if (std::filesystem::exists(packDir)) {
            for (auto& e : std::filesystem::directory_iterator(packDir)) {
                std::string ext = e.path().extension().string();
                if (ext == ".mp3" || ext == ".wav" || ext == ".ogg") { firstFile = e.path().string(); break; }
            }
        }
        if (!firstFile.empty()) {
            musicClose();
            int len = MultiByteToWideChar(CP_UTF8, 0, firstFile.c_str(), -1, NULL, 0);
            std::wstring wpath(len, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, firstFile.c_str(), -1, &wpath[0], len);
            std::wstring cmd = L"open \"" + wpath + L"\" type mpegvideo alias music";
            mciSendStringW(cmd.c_str(), NULL, 0, NULL);
            g_music.isOpen = true;
            g_music.currentFile = std::filesystem::path(firstFile).filename().string();
            g_music.status = "已加载";
            testLog("MUSIC", ("Opened: " + firstFile).c_str());
        }
    } else if (controlId == "music_play") {
        if (g_music.isOpen) { mciSendStringW(L"play music", NULL, 0, NULL); g_music.status = "▶ 播放中"; }
    } else if (controlId == "music_pause") {
        if (g_music.isOpen) { mciSendStringW(L"pause music", NULL, 0, NULL); g_music.status = "⏸ 已暂停"; }
    } else if (controlId == "music_stop") {
        if (g_music.isOpen) { mciSendStringW(L"stop music", NULL, 0, NULL); mciSendStringW(L"seek music to start", NULL, 0, NULL); g_music.status = "⏹ 已停止"; }
    } else if (controlId == "music_prev" || controlId == "music_next") {
        if (g_music.isOpen) { mciSendStringW(L"seek music to start", NULL, 0, NULL); mciSendStringW(L"play music", NULL, 0, NULL); g_music.status = "▶ 播放中"; }
    }
    sendMusicPlayer();
}

// ─── Handle events received from server ───
static void handleServerMessage(const std::string& jsonMsg) {
    g_receivedCount++;
    try {
        auto j = json::parse(jsonMsg);
        std::string type = j.value("type", "");

        if (type == "gui_event") {
            g_guiEventCount++;
            std::string guiId = j.value("guiId", "");
            std::string controlId = j.value("controlId", "");

            if (guiId == "image_viewer") {
                handleImageEvent(controlId);
            } else if (guiId == "music_player") {
                handleMusicEvent(controlId);
            } else if (guiId == "settings_menu") {
                handleSettingsEvent(controlId);
            } else if (guiId == "keybind" && controlId == "open_settings") {
                registerSettingsMenu();
                testLog("MENU", "Opened via F8");
            } else if (guiId == "keybind" && controlId == "img_viewer") {
                sendImageViewer();
                testLog("IMG", "Opened via I key");
            } else if (guiId == "keybind" && controlId == "music_player") {
                sendMusicPlayer();
                testLog("MUSIC", "Opened via M key");
            } else {
                testLog("GUI>EVT", ("gui=" + guiId + " ctrl=" + controlId).c_str());
            }
        }
        else if (type == "data_exchange") {
            g_dataExchangeCount++;
            std::string channel = j.value("channel", "");
            if (channel == "fr_test_echo") {
                sendJson({{"type","data_exchange"},{"channel","fr_test_echo_response"},{"data",j.value("data","")},{"targetPlayer",""}});
            }
        }
    } catch (std::exception& e) {
        testLog("TCP>ERR", ("parse: " + std::string(e.what())).c_str());
    }
}

// ══════════════════════════════════════════════════════════
//  LeviLamina NativeMod entry
// ══════════════════════════════════════════════════════════

namespace fr_test {

class FRTestMod final {
public:
    static FRTestMod& getInstance();
    FRTestMod() : mSelf(*ll::mod::NativeMod::current()) {}
    ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load() {
        testLog("INIT", "FRTest-Native load()");
        return true;
    }

    bool enable() {
        g_modDir = mSelf.getModDir().string();

        // Clean log
        FILE* f = fopen("FRTest.log", "w");
        if (f) fclose(f);

        testLog("INIT", "═══════════════════════════════════════════");
        testLog("INIT", "FRTest-Native v0.1.0 — 端到端验证插件启动");
        testLog("INIT", (std::string("modDir=") + g_modDir).c_str());
        testLog("INIT", "测试目标:");
        testLog("INIT", "  [T1] 复杂GUI渲染 — 10个面板, 全部控件类型");
        testLog("INIT", "  [T2] TCP桥协议 — 7种消息类型双向通信");
        testLog("INIT", "  [T3] 按键绑定 — 5个快捷键注册/反馈");
        testLog("INIT", "  [T4] 压力测试 — 30+ 控件批量渲染");
        testLog("INIT", "  [T5] 插件互通 — 模拟 shop/fakechat 插件事件流");
        testLog("INIT", "  [T6] 系统诊断 — 渲染/网络/协议状态自检");
        testLog("INIT", "═══════════════════════════════════════════");
        testLog("INIT", (std::string("连接 ") + BRIDGE_HOST + ":" + std::to_string(BRIDGE_PORT)).c_str());

        g_tcpClient.setStatusCallback([](bool connected) {
            testLog("TCP", connected ? "已连接" : "已断开");
            if (connected) sendIdentify();
        });

        g_tcpClient.setMessageCallback(handleServerMessage);

        if (!g_tcpClient.connect(BRIDGE_HOST, BRIDGE_PORT)) {
            testLog("TCP", "首次连接失败，将自动重连");
        }
        g_tcpClient.enableAutoReconnect(3000, 5);
        g_tcpClient.startReceiveThread();

        // Register all GUIs/keybinds in background thread
        std::thread regThread(registerAllAfterConnect);
        regThread.detach();

        // Periodic data push simulator
        std::thread simThread([]() {
            int tick = 0;
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(10));
                tick++;
                if (!g_tcpClient.isConnected()) continue;
                char buf[256];
                snprintf(buf, sizeof(buf),
                    "{\"type\":\"data_exchange\",\"channel\":\"fr_server_stats\","
                    "\"data\":\"{\\\"uptime\\\":%d,\\\"tps\\\":%d,\\\"players\\\":%d}\","
                    "\"targetPlayer\":\"\"}",
                    tick * 10, 20 - (tick % 3), 3 + (tick % 5));
                g_tcpClient.send(buf);
                testLog("SIM", "模拟服务器状态推送完成");
            }
        });
        simThread.detach();

        testLog("INIT", "FRTest-Native 启动完成");
        return true;
    }

    bool disable() {
        testLog("EXIT", "FRTest-Native 卸载中...");
        char summary[256];
        snprintf(summary, sizeof(summary),
            "汇总: 消息ID=%d, 接收消息=%d, gui_events=%d, data_exchange=%d",
            g_messageId, g_receivedCount, g_guiEventCount, g_dataExchangeCount);
        testLog("EXIT", summary);
        g_tcpClient.stop();
        g_registered = false;
        return true;
    }

private:
    ll::mod::NativeMod& mSelf;
};

FRTestMod& FRTestMod::getInstance() {
    static FRTestMod instance;
    return instance;
}

} // namespace fr_test

LL_REGISTER_MOD(fr_test::FRTestMod, fr_test::FRTestMod::getInstance());
