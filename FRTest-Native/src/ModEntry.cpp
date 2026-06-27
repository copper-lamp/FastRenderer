// ════════════════════════════════════════════════════════════
//  FRTest-Native — FastRenderer 端到端验证插件
//  覆盖: 复杂GUI渲染, TCP桥协议, 按键绑定, 插件互通, 压力测试
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

// ─── Fallback: Write GUI JSON files to gui_defs dir (when TCP unavailable) ───
static std::string findGuiDefsDir() {
    // Try relative to cwd: ./mods/FastRenderer/gui_defs/
    std::vector<std::string> candidates = {
        "./mods/FastRenderer/gui_defs/",
        "../mods/FastRenderer/gui_defs/",
        "gui_defs/",
    };
    // Also try relative to this DLL's mod dir
    std::string selfPath = __FILE__;
    auto pos = selfPath.find("FRTest-Native");
    if (pos != std::string::npos) {
        candidates.push_back(selfPath.substr(0, pos) + "FastRenderer/gui_defs/");
    }
    return candidates[0]; // default to first
}

static void writeAllGuiFiles(const std::string& guiDefsDir) {
    namespace fs = std::filesystem;
    fs::create_directories(guiDefsDir);

    struct {
        const char* guiId;
        json (*builder)();
    } guis[] = {
        {"dashboard",     ComplexGui::makeDashboard},
        {"sandbox",       ComplexGui::makeWidgetSandbox},
        {"tcp_test",       nullptr}, // skip — TCP bridge UI
        {"stress",        ComplexGui::makeStressTest},
        {"interop",       ComplexGui::makeInteropDemo},
        {"settings",      ComplexGui::makeSettings},
        {"data_panel",    ComplexGui::makeDataPanel},
        {"console",       ComplexGui::makeConsole},
        {"players",       ComplexGui::makePlayerPanel},
        {"diagnostics",   ComplexGui::makeDiagnostics},
    };

    int written = 0;
    for (auto& g : guis) {
        if (!g.builder) continue;
        json root = g.builder();
        json wrapper = {{"root", root}};

        std::string path = guiDefsDir + g.guiId + ".json";
        std::ofstream f(path);
        f << wrapper.dump(2);
        f.close();
        testLog("FILE", (std::string("wrote ") + path).c_str());
        written++;
    }
    testLog("FILE", (std::to_string(written) + " GUI JSON files written to " + guiDefsDir).c_str());

    // Write keybinds.json
    json kbArray = json::array();
    struct { const char* id; const char* name; int vk; } binds[] = {
        {"toggle_dashboard",  "切换仪表盘",   0x4B},
        {"toggle_sandbox",    "切换控件沙盒", 0x4E},
        {"toggle_tcp_test",   "切换TCP测试",  0x4D},
        {"toggle_diagnostics","切换诊断",     0x55},
        {"reconnect_tcp",     "重连TCP",      0x50},
    };
    for (auto& b : binds) {
        kbArray.push_back({
            {"pluginId", "FRTest-Native"},
            {"bindId", b.id},
            {"bindName", b.name},
            {"vkCode", b.vk}
        });
    }
    json kbFile;
    kbFile["keybinds"] = kbArray;
    // Write to both gui_defs/ and a separate keybinds/ dir
    std::string kbPath = guiDefsDir + "../keybinds/";
    fs::create_directories(kbPath);
    std::ofstream f(kbPath + "fr_test_keybinds.json");
    f << kbFile.dump(2);
    f.close();
    testLog("FILE", (std::string("keybinds written to ") + kbPath).c_str());
}

// ─── Simulate game data refresh (for dynamic text/ref fields) ───
// In a real game scenario, this would come from game state updates.
static void simulateDataRefresh() {
    char buf[256];

    // Simulate dashboard data changes
    static int counter = 0;
    counter++;
    int tickRate = 20 - (counter % 5); // fluctuate 16-20

    snprintf(buf, sizeof(buf),
        "{\"type\":\"gui_register\",\"pluginId\":\"FRTest-Native\",\"guiId\":\"dashboard\","
        "\"version\":%d,\"definition\":%s,\"targetPlayer\":\"\"}",
        100 + counter,
        ComplexGui::makeDashboard().dump().c_str());
    // In real operation this would be an update message — for now we track the intent
    testLog("SIM", (std::string("dashboard data refreshed tick=") + std::to_string(tickRate)).c_str());
}

// ─── Register all 10 test GUIs via TCP ───
static void registerAllGuis() {
    struct GuiDef {
        const char* guiId;
        const char* label;
        json (*builder)();
    };
    GuiDef guis[] = {
        {"dashboard",   "仪表盘",     ComplexGui::makeDashboard},
        {"sandbox",     "控件沙盒",   ComplexGui::makeWidgetSandbox},
        {"tcp_test",    "TCP桥通信",  ComplexGui::makeTcpTestPanel},
        {"stress",      "压力测试",   ComplexGui::makeStressTest},
        {"interop",     "插件互通",   ComplexGui::makeInteropDemo},
        {"settings",    "设置",       ComplexGui::makeSettings},
        {"data_panel",  "数据",       ComplexGui::makeDataPanel},
        {"console",     "控制台",     ComplexGui::makeConsole},
        {"players",     "在线玩家",   ComplexGui::makePlayerPanel},
        {"diagnostics", "系统诊断",   ComplexGui::makeDiagnostics},
    };

    int total = sizeof(guis) / sizeof(guis[0]);
    int ok = 0, fail = 0;

    for (int i = 0; i < total; i++) {
        auto& g = guis[i];
        json definition = g.builder();
        json msg = {
            {"type", "gui_register"},
            {"pluginId", "FRTest-Native"},
            {"guiId", g.guiId},
            {"definition", definition.dump()},
            {"version", 1},
            {"targetPlayer", ""}
        };
        if (sendJson(msg)) {
            ok++;
            char buf[128];
            snprintf(buf, sizeof(buf), "gui_register [%d/%d] %s — %s", i+1, total, g.guiId, g.label);
            testLog("GUI+REG", buf);
        } else {
            fail++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    char result[128];
    snprintf(result, sizeof(result), "注册完毕: %d成功 / %d失败 / 共%d", ok, fail, total);
    testLog("GUI+REG", result);
}

// ─── Register keybinds via TCP ───
static void registerKeybinds() {
    struct {
        const char* id;
        const char* name;
        int vk;
    } binds[] = {
        {"toggle_dashboard",  "切换仪表盘",   0x4B},  // K
        {"toggle_sandbox",    "切换控件沙盒", 0x4E},  // N
        {"toggle_tcp_test",   "切换TCP测试",  0x4D},  // M
        {"toggle_diagnostics","切换诊断",     0x55},  // U
        {"reconnect_tcp",     "重连TCP",      0x50},  // P
    };

    for (auto& b : binds) {
        json msg = {
            {"type", "keybind_register"},
            {"pluginId", "FRTest-Native"},
            {"bindId", b.id},
            {"bindName", b.name},
            {"vkCode", b.vk},
            {"targetPlayer", ""}
        };
        sendJson(msg);
        char buf[128];
        snprintf(buf, sizeof(buf), "keybind %s (0x%02X) — %s", b.id, b.vk, b.name);
        testLog("KB+REG", buf);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    testLog("KB+REG", "全部按键绑定注册完成 (5个)");
}

// ─── Send identify ───
static void sendIdentify() {
    json msg = {
        {"type", "identify"},
        {"player", PLAYER_NAME},
        {"platform", PLATFORM}
    };
    sendJson(msg);
    testLog("TCP>OUT", "identify sent");
}

// ─── Main registration routine (runs in background thread) ───
static void registerAllAfterConnect() {
    for (int i = 0; i < 10; i++) {
        if (g_tcpClient.isConnected()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (!g_tcpClient.isConnected()) {
        testLog("TCP", "连接失败 (5秒超时) — 启用离线文件回退模式");
        std::string guiDir = findGuiDefsDir();
        writeAllGuiFiles(guiDir);
        g_registered = true;
        return;
    }

    testLog("TCP", "已连接到 TCP 桥");
    sendIdentify();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    registerAllGuis();
    registerKeybinds();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Subscribe to data_exchange channel
    {
        json msg = {
            {"type", "data_exchange"},
            {"channel", "fr_test_subscribe"},
            {"data", "{\"action\":\"subscribe\"}"},
            {"targetPlayer", ""}
        };
        sendJson(msg);
    }

    g_registered = true;
    testLog("TCP", "全部注册完成");
}

// ─── Handle events received from server ───
static void handleServerMessage(const std::string& jsonMsg) {
    g_receivedCount++;
    try {
        auto j = json::parse(jsonMsg);
        std::string type = j.value("type", "");
        testLog("TCP>IN", (std::string("type=") + type).c_str());

        if (type == "data_exchange") {
            g_dataExchangeCount++;
            std::string channel = j.value("channel", "");
            std::string data = j.value("data", "");

            // Echo back if from test channel
            if (channel == "fr_test_echo") {
                json reply = {
                    {"type", "data_exchange"},
                    {"channel", "fr_test_echo_response"},
                    {"data", data},
                    {"targetPlayer", ""}
                };
                sendJson(reply);
                testLog("TCP>OUT", "data_exchange echo response sent");
            }
        }
        else if (type == "gui_register") {
            testLog("TCP>IN", (std::string("gui_register from pluginId=") + j.value("pluginId", "")).c_str());
        }
        else if (type == "gui_event") {
            g_guiEventCount++;
            testLog("TCP>IN", (std::string("gui_event: ") + j.dump()).c_str());
        }
        else {
            testLog("TCP>IN", (std::string("unknown type, raw=") + jsonMsg).c_str());
        }
    } catch (std::exception& e) {
        testLog("TCP>ERR", (std::string("parse error: ") + e.what()).c_str());
    }
}

// ─── Plugin entry point ───
extern "C" __declspec(dllexport) void onEnable() {
    // Clean log
    FILE* f = fopen("FRTest.log", "w");
    if (f) fclose(f);

    testLog("INIT", "═══════════════════════════════════════════");
    testLog("INIT", "FRTest-Native v0.1.0 — 端到端验证插件启动");
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
        if (connected) {
            sendIdentify();
        }
    });

    g_tcpClient.setMessageCallback(handleServerMessage);

    if (!g_tcpClient.connect(BRIDGE_HOST, BRIDGE_PORT)) {
        testLog("TCP", "首次连接失败，将自动重连");
    }

    g_tcpClient.startReceiveThread();

    // Register all GUIs/keybinds in background thread
    std::thread regThread(registerAllAfterConnect);
    regThread.detach();

    // Start a simulator that sends periodic updates
    // (demonstrates how a real game plugin would push data)
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

    testLog("INIT", "FRTest-Native 启动完成，等待连接...");
}

extern "C" __declspec(dllexport) void onDisable() {
    testLog("EXIT", "FRTest-Native 卸载中...");

    char summary[256];
    snprintf(summary, sizeof(summary),
        "汇总: 消息ID=%d, 接收消息=%d, gui_events=%d, data_exchange=%d",
        g_messageId, g_receivedCount, g_guiEventCount, g_dataExchangeCount);
    testLog("EXIT", summary);
    testLog("EXIT", "═══════════════════════════════════════════");

    g_tcpClient.stop();
    g_registered = false;
}
