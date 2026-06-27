#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <cstdio>

using json = nlohmann::json;

// ─── Logging helper (only on plugin side) ───
inline void frLog(const char* msg) {
    FILE* f = fopen("FRTest_ComplexGui.txt", "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

namespace ComplexGui {

inline json text(const std::string& value, const std::string& color = "", const std::string& ref = "") {
    json j = {{"type", "text"}, {"props", json::object()}};
    if (!value.empty()) j["props"]["value"] = value;
    if (!color.empty()) j["props"]["color"] = color;
    if (!ref.empty()) j["props"]["ref"] = ref;
    return j;
}

inline json btn(const std::string& label, const std::string& color = "", const std::string& onClick = "") {
    json j = {{"type", "button"}, {"props", {{"text", label}}}};
    if (!color.empty()) j["props"]["color"] = color;
    if (!onClick.empty()) j["props"]["onClick"] = onClick;
    return j;
}

inline json chk(const std::string& label, bool def = false, const std::string& id = "") {
    json j = {{"type", "checkbox"}, {"props", {{"label", label}, {"default", def ? "true" : "false"}}}};
    if (!id.empty()) j["props"]["id"] = id;
    return j;
}

inline json row(const std::vector<json>& children, float spacing = 6.0f) {
    return {{"type", "row"}, {"props", {{"spacing", std::to_string(spacing)}}}, {"children", children}};
}

inline json sep() {
    return {{"type", "separator"}, {"props", json::object()}};
}

inline json input(const std::string& placeholder = "", const std::string& def = "", const std::string& w = "") {
    json j = {{"type", "input"}, {"props", json::object()}};
    if (!placeholder.empty()) j["props"]["placeholder"] = placeholder;
    if (!def.empty()) j["props"]["default"] = def;
    if (!w.empty()) j["props"]["width"] = w;
    return j;
}

inline json slider(float minV, float maxV, float defV, const std::string& label = "", const std::string& onChange = "") {
    json j = {{"type", "slider"}, {"props", {{"min", std::to_string((int)minV)}, {"max", std::to_string((int)maxV)}, {"default", std::to_string((int)defV)}, {"label", label}}}};
    if (!onChange.empty()) j["props"]["onChange"] = onChange;
    return j;
}

inline json combo(const std::string& options, const std::string& label = "", const std::string& def = "0") {
    return {{"type", "combo"}, {"props", {{"options", options}, {"label", label}, {"default", def}}}};
}

inline json panel(const std::vector<json>& children, const std::string& color = "", float padding = 6.0f, float round = 6.0f) {
    json j = {{"type", "panel"}, {"props", {{"padding", std::to_string((int)padding)}, {"round", std::to_string((int)round)}}}, {"children", children}};
    if (!color.empty()) j["props"]["color"] = color;
    return j;
}

inline json group(const std::string& label, const std::vector<json>& children, bool collapsible = false) {
    json j = {{"type", "group"}, {"props", {{"label", label}}}, {"children", children}};
    if (collapsible) j["props"]["collapsible"] = "true";
    return j;
}

inline json spacer(float w = 8.0f, float h = 8.0f) {
    return {{"type", "spacer"}, {"props", {{"width", std::to_string((int)w)}, {"height", std::to_string((int)h)}}}};
}

inline json progress(float val, float w = 0, float h = 18.0f) {
    return {{"type", "progress"}, {"props", {{"value", std::to_string(val)}, {"width", std::to_string((int)w)}, {"height", std::to_string((int)h)}}}};
}

// ══════════════════════════════════════════════════════════
//  PAGE 1 — 仪表盘 (原有 + TCP 连接状态)
// ══════════════════════════════════════════════════════════

inline json makeDashboard() {
    return {{"type", "window"}, {"props", {{"title", "FR 仪表盘 [测试项: 基本渲染]"}, {"width", "680"}, {"height", "480"}}}, {"children", {
        group("▸ 系统状态", {
            row({
                panel({text("连接状态", "white"), text("已连接", "#81c784", "tcp_status")}, "#1b5e2088"),
                panel({text("已注册 GUI", "white"), text("8", "#4fc3f7", "gui_count")}, "#0d47a188"),
                panel({text("按键绑定", "white"), text("4", "#ffcc80", "keybind_count")}, "#e6510088"),
                panel({text("渲染 FPS", "white"), text("60", "#ce93d8", "fps")}, "#4a148c88")
            }, 8.0f),
        }),
        sep(),
        group("▸ 性能指标 (模拟)", {
            row({text("帧渲染耗时:", "white"), progress(0.22f, 500, 16)}, 8.0f),
            row({text("GUI 解析耗时:", "white"), progress(0.08f, 500, 16)}, 8.0f),
            row({text("TCP 延迟:", "white"), progress(0.05f, 500, 16)}, 8.0f),
        }),
        sep(),
        row({
            btn("刷新状态", "#2196f3", "dash_refresh"),
            btn("发送 Ping", "#4caf50", "dash_ping"),
            btn("注册所有 GUI", "#ff9800", "dash_reload_all"),
            btn("卸载所有 GUI", "#f44336", "dash_unload_all")
        }, 8.0f),
        chk("自动刷新 (5s)", true, "dash_auto_refresh")
    }}};
}

// ══════════════════════════════════════════════════════════
//  PAGE 2 — 控件沙盒 (测试所有控件类型)
// ══════════════════════════════════════════════════════════

inline json makeWidgetSandbox() {
    return {{"type", "window"}, {"props", {{"title", "控件沙盒 [测试项: 全部控件]"}, {"width", "560"}, {"height", "520"}}}, {"children", {
        group("▸ 按钮测试", {
            row({btn("左键点击", "#2196f3", "sb_click_left"), btn("右键点击", "#ff5722", "sb_click_right"), btn("提交", "#4caf50", "sb_submit"), btn("删除", "#f44336", "sb_delete")}, 6.0f),
        }),
        sep(),
        group("▸ 输入框测试", {
            row({text("用户名:", "white"), input("输入用户名...", "", "180")}, 6.0f),
            row({text("密  码:", "white"), input("输入密码...", "", "180")}, 6.0f),
            row({text("备  注:", "white"), input("", "", "300")}, 6.0f),
        }),
        sep(),
        group("▸ 滑块与选择", {
            row({text("音量:", "white"), slider(0, 100, 70, "%", "sb_volume")}, 6.0f),
            row({text("亮度:", "white"), slider(0, 20, 12, "", "sb_brightness")}, 6.0f),
            row({text("游戏模式:", "white"), combo("生存,创造,冒险,观察者", "", "0")}, 6.0f),
            row({text("语言:", "white"), combo("简体中文,English,日本語,한국어", "", "0")}, 6.0f),
        }),
        sep(),
        group("▸ 复选框与开关", {
            chk("启用声音", true, "sb_sound"),
            chk("显示粒子效果", true, "sb_particles"),
            chk("启用光影", false, "sb_shaders"),
            chk("FPS 显示", true, "sb_fps"),
        }),
        sep(),
        group("▸ 进度条与面板", {
            progress(0.42f, 480, 18),
            progress(0.78f, 480, 18),
            panel({text("此面板测试嵌套 panel 渲染", "#81c784")}, "#1b5e2044", 8, 8),
        }),
        sep(),
        row({
            btn("发送测试事件", "#2196f3", "sb_send_event"),
            text("  结果将写入 FRTest_R.txt", "#888888"),
        })
    }}};
}

// ══════════════════════════════════════════════════════════
//  PAGE 3 — TCP 桥通信测试
// ══════════════════════════════════════════════════════════

inline json makeTcpTestPanel() {
    return {{"type", "window"}, {"props", {{"title", "TCP 桥通信测试 [测试项: 协议]"}, {"width", "580"}, {"height", "440"}}}, {"children", {
        group("▸ 消息类型测试", {
            row({btn("gui_register", "#4caf50", "tcp_gui_register"), btn("gui_unregister", "#f44336", "tcp_gui_unregister")}, 6.0f),
            row({btn("keybind_register", "#4caf50", "tcp_keybind_register"), btn("keybind_unregister", "#f44336", "tcp_keybind_unregister")}, 6.0f),
            row({btn("data_exchange", "#2196f3", "tcp_data_exchange"), btn("identify", "#ff9800", "tcp_identify")}, 6.0f),
        }),
        sep(),
        group("▸ 连接管理", {
            row({btn("连接 TCP", "#4caf50", "tcp_connect"), btn("断开 TCP", "#f44336", "tcp_disconnect"), btn("重新连接", "#ff9800", "tcp_reconnect")}, 6.0f),
        }),
        sep(),
        group("▸ 数据交换测试", {
            row({text("频道:", "white"), input("channel_name", "test", "120"), btn("订阅", "#2196f3", "tcp_subscribe")}, 6.0f),
            row({text("数据:", "white"), input("JSON data", "", "300"), btn("发送", "#4caf50", "tcp_send_data")}, 6.0f),
        }),
        sep(),
        panel({
            text("[INFO] 点击按钮发送相应 TCP 消息", "#81c784"),
            text("[INFO] 查看 FRTest_TCP.txt 查看详细日志", "#90caf9"),
        }, "#00000044"),
    }}};
}

// ══════════════════════════════════════════════════════════
//  PAGE 4 — 压力测试 (模拟大量控件)
// ══════════════════════════════════════════════════════════

inline json makeStressTest() {
    json listChildren = json::array();
    for (int i = 1; i <= 30; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "项目 #%d", i);
        listChildren.push_back(row({
            text(std::string("▶ ") + buf, "#81c784"),
            progress((i % 100) / 100.0f, 200, 12),
            btn("操作", "#2196f3", std::string("stress_act_") + std::to_string(i)),
            chk("", i % 3 == 0, std::string("stress_chk_") + std::to_string(i))
        }, 4.0f));
    }

    return {{"type", "window"}, {"props", {{"title", "压力测试 [测试项: 大量控件]"}, {"width", "620"}, {"height", "500"}}}, {"children", {
        group("▸ 配置", {
            row({text("控件数量:", "white"), slider(1, 200, 30, "个", "stress_count")}, 6.0f),
            row({btn("刷新", "#4caf50", "stress_refresh"), btn("全选", "#2196f3", "stress_select_all"), btn("取消全选", "#ff9800", "stress_deselect_all"), btn("冻结/解冻", "#f44336", "stress_freeze")}, 6.0f),
        }),
        sep(),
        group("▸ 批量列表 (30 项)", listChildren),
    }}};
}

// ══════════════════════════════════════════════════════════
//  PAGE 5 — 插件互通演示
// ══════════════════════════════════════════════════════════

inline json makeInteropDemo() {
    return {{"type", "window"}, {"props", {{"title", "插件互通演示 [测试项: 跨插件]"}, {"width", "540"}, {"height", "400"}}}, {"children", {
        text("本页模拟其他 LL 插件通过 FR API 注册的 GUI", "#ffcc80"),
        sep(),
        group("▸ 商店插件 (shop)", {
            panel({
                row({text("商品: 钻石剑", "white"), text("价格: 32 绿宝石", "#ffcc80"), btn("购买", "#4caf50", "shop_buy_sword")}, 6.0f),
                row({text("商品: 铁胸甲", "white"), text("价格: 16 绿宝石", "#ffcc80"), btn("购买", "#4caf50", "shop_buy_chest")}, 6.0f),
                row({text("商品: 金苹果", "white"), text("价格: 8 绿宝石", "#ffcc80"), btn("购买", "#4caf50", "shop_buy_gapple")}, 6.0f),
                row({text("商品: 附魔书", "white"), text("价格: 24 绿宝石", "#ffcc80"), btn("购买", "#4caf50", "shop_buy_book")}, 6.0f),
            }, "#1b5e2044"),
        }),
        sep(),
        group("▸ 聊天插件 (fakechat)", {
            row({input("输入消息...", "", "340"), btn("发送", "#2196f3", "fc_send")}, 6.0f),
            chk("启用表情补全", true, "fc_emoji"),
            chk("显示发送时间", true, "fc_timestamp"),
            chk("消息过滤", false, "fc_filter"),
        }),
        sep(),
        group("▸ 插件通信事件流", {
            panel({
                text("[事件] shop → FR-Server: gui_event(buy_sword)", "#81c784"),
                text("[事件] FR-Server → FRTest: data_exchange(shop_result)", "#90caf9"),
                text("[事件] FRTest → FR-Server: gui_event(settings_save)", "#ffcc80"),
                text("[状态] 互通链路完整 ✓", "#4caf50"),
            }, "#00000044"),
        }),
    }}};
}

// ══════════════════════════════════════════════════════════
//  PAGE 6 — 设置 (原有, 增强)
// ══════════════════════════════════════════════════════════

inline json makeSettings() {
    return {{"type", "window"}, {"props", {{"title", "FR 设置 [测试项: 表单控件]"}, {"width", "560"}, {"height", "460"}}}, {"children", {
        group("▸ 基本设置", {
            row({text("服务器名称:", "white"), input("输入服务器名...", "Minecraft Server", "300")}, 8.0f),
            row({text("最大玩家:", "white"), slider(5, 100, 20, "", "settings_maxplayers"), text("20", "white", "max_players_display")}, 8.0f),
            row({text("游戏模式:", "white"), combo("生存,创造,冒险,观察者", "", "0")}, 8.0f),
            row({text("难度:", "white"), combo("和平,简单,普通,困难", "", "2")}, 8.0f),
        }, true),
        group("▸ 聊天设置", {
            chk("启用聊天过滤", true, "settings_chat_filter"),
            chk("显示玩家加入/离开消息", true, "settings_join_msg"),
            chk("启用表情符号", false, "settings_emoji"),
            row({text("消息保留:", "white"), slider(10, 200, 50, "条", "settings_msg_retain")}, 8.0f)
        }, true),
        group("▸ 通知设置", {
            chk("允许服务器推送", true, "settings_srv_push"),
            chk("声音提示", true, "settings_sound"),
            row({text("通知音量:", "white"), slider(0, 100, 70, "%", "settings_volume")}, 8.0f)
        }, true),
        group("▸ 渲染设置", {
            chk("启用 ImGui 渲染", true, "settings_render"),
            chk("显示 FPS 叠加层", true, "settings_fps_overlay"),
            row({text("UI 缩放:", "white"), slider(80, 150, 100, "%", "settings_ui_scale")}, 8.0f)
        }, true),
        sep(),
        row({btn("保存设置", "#4caf50", "settings_save"), btn("恢复默认", "#ff9800", "settings_reset"), btn("取消", "#9e9e9e")}, 8.0f)
    }}};
}

// ══════════════════════════════════════════════════════════
//  PAGE 7 — 数据面板 (原有, 增强)
// ══════════════════════════════════════════════════════════

inline json makeDataPanel() {
    return {{"type", "window"}, {"props", {{"title", "FR 数据 [测试项: 实时监控]"}, {"width", "500"}, {"height", "400"}}}, {"children", {
        group("▸ 命令输入", {
            row({input("输入命令...", "", "340"), btn("执行", "#2196f3", "data_execute")}, 4.0f)
        }),
        group("▸ 数据监控", {
            row({
                panel({text("每秒发包: 256", "", "packet_per_sec"), text("延迟: 45ms", "", "latency")}, "#1a237e44"),
                panel({text("实体数: 142", "", "entity_count"), text("区块加载: 64", "", "chunks_loaded")}, "#1a237e44")
            }, 8.0f),
            sep(),
            chk("实时刷新", true, "data_realtime"),
            slider(1, 10, 5, "刷新间隔 (秒)", "data_refresh_interval")
        }),
        group("▸ 最近日志", {
            panel({
                text("[INFO] FRTest-Native 加载完成", "#81c784"),
                text("[OK]   TCP 桥连接成功", "#4caf50"),
                text("[DATA] 8 个 GUI 已注册", "#90caf9"),
                text("[WARN] 压力测试控件数 > 50", "#ffcc80"),
                text("[INFO] 等待用户交互...", "#81c784")
            }, "#00000066")
        }, true)
    }}};
}

// ══════════════════════════════════════════════════════════
//  PAGE 8 — 控制台 (原有)
// ══════════════════════════════════════════════════════════

inline json makeConsole() {
    return {{"type", "window"}, {"props", {{"title", "FR 控制台 [测试项: 日志]"}, {"width", "540"}, {"height", "340"}}}, {"children", {
        row({combo("全部,信息,警告,错误,调试", "级别:", "0"), input("搜索日志...", "", "260"), btn("清空", "#ff5722", "console_clear")}, 4.0f),
        sep(),
        panel({
            text("[15:30:12] [INFO]  FastRenderer 启动完成", "#4caf50"),
            text("[15:30:12] [INFO]  GUI 定义加载: 8 个面板", "#4caf50"),
            text("[15:30:13] [WARN]  主题管理器延迟加载", "#ffcc80"),
            text("[15:30:13] [INFO]  TCP 桥初始化完成", "#4caf50"),
            text("[15:30:14] [OK]    端到端测试全部通过 (10/10)", "#81c784"),
            text("[15:30:15] [DATA] 玩家 Steve 数据同步", "#90caf9"),
            text("[15:30:16] [ERROR] 压力测试阈值超限", "#ef9a9a"),
            text("[15:30:17] [WARN]  内存使用率 82%", "#ffcc80"),
            text("[15:30:18] [DATA] 插件互通测试通过", "#90caf9"),
            text("[15:30:19] [INFO] 全部 8 个 GUI 渲染正常", "#4caf50"),
        }, "#0d0d0d66")
    }}};
}

// ══════════════════════════════════════════════════════════
//  PAGE 9 — 玩家列表 (原有)
// ══════════════════════════════════════════════════════════

inline json makePlayerPanel() {
    json entry = json::array();
    auto addPlayer = [&](const std::string& name, const std::string& hp, const std::string& hpColor, const std::string& nameColor = "#81c784", const std::string& dim = "主世界") {
        entry.push_back(row({
            text("▶ " + name, nameColor), spacer(30, 0),
            text("HP: " + hp, hpColor), spacer(20, 0),
            text("[" + dim + "]", "#666666"),
            btn("传送", "#2196f3", std::string("player_tp_") + name),
            btn("查看", "#4caf50", std::string("player_info_") + name)
        }, 4.0f));
    };
    addPlayer("Steve",   "20/20", "#90caf9");
    addPlayer("Alex",    "18/20", "#ffcc80");
    addPlayer("Dream",   "20/20", "#90caf9");
    addPlayer("Notch",   "6/20",  "#ef9a9a", "#81c784", "下界");
    addPlayer("Herobrine", "??", "#b39ddb", "#ce93d8", "末地");

    return {{"type", "window"}, {"props", {{"title", "FR 在线玩家 [测试项: 列表]"}, {"width", "520"}, {"height", "340"}}}, {"children", {
        text("在线玩家 (5 人)", "", "player_list_title"),
        sep(),
        group("▸ 玩家列表", entry),
        sep(),
        row({btn("刷新列表", "#2196f3", "player_refresh"), combo("全部,主世界,下界,末地", "维度:", "0")}, 8.0f)
    }}};
}

// ══════════════════════════════════════════════════════════
//  PAGE 10 — 系统诊断
// ══════════════════════════════════════════════════════════

inline json makeDiagnostics() {
    return {{"type", "window"}, {"props", {{"title", "系统诊断 [测试项: 自检]"}, {"width", "540"}, {"height", "420"}}}, {"children", {
        group("▸ 渲染引擎", {
            row({text("ImGui 版本:", "white"), text("1.92+", "#81c784", "diag_imgui_ver")}, 6.0f),
            row({text("JSON 解析器:", "white"), text("nlohmann_json 3.11", "#81c784")}, 6.0f),
            row({text("DX 后端:", "white"), text("D3D11 + D3D12(RenderDragon)", "#90caf9")}, 6.0f),
            row({text("中文字体:", "white"), text("已加载 (微软雅黑)", "#81c784")}, 6.0f),
        }),
        sep(),
        group("▸ TCP 桥", {
            row({text("连接目标:", "white"), text("127.0.0.1:12345", "#ffcc80", "diag_tcp_target")}, 6.0f),
            row({text("连接状态:", "white"), text("检测中...", "#ffcc80", "diag_tcp_status")}, 6.0f),
            row({text("重连机制:", "white"), text("3s间隔, 最多5次", "#81c784")}, 6.0f),
            row({text("协议版本:", "white"), text("v1.0 (JSON over TCP)", "#81c784")}, 6.0f),
        }),
        sep(),
        group("▸ 自检结果", {
            panel({
                text("[OK]   Basic rendering — passed", "#4caf50"),
                text("[PEND] TCP bridge  — waiting for connection", "#ffcc80"),
                text("[PEND] Data exchange — waiting for server", "#ffcc80"),
                text("[PEND] Keybind poll — waiting for input", "#ffcc80"),
            }, "#00000044"),
        }),
    }}};
}

} // namespace ComplexGui
