# FastRenderer

> Minecraft Bedrock 前置渲染框架插件 — 为其他插件提供声明式 GUI 渲染能力

FastRenderer 是一个基于 LeviLamina/LeviLaunchroid 生态的**跨平台客户端前置渲染框架**。它不直接提供游戏功能，而是为其他客户端插件（C++ / JS / 任意语言）提供统一的 **GUI 渲染能力** 和 **TCP 桥通信**，让插件开发者专注于业务逻辑，无需关心底层渲染、网络通信和跨平台适配。

---

## 核心特性

- **声明式 GUI** — JSON 定义界面布局，支持 14 种控件类型，插件只需声明布局
- **TCP 桥通信** — 统一的跨进程通信协议（JSON over TCP），替代旧式文件桥和 MC 自定义 Packet
- **跨平台** — Windows (D3D11 + D3D12 RenderDragon) + Android (OpenGL ES 3.0) 双端支持
- **文件热加载** — `gui_defs/` 目录下的 JSON 文件修改即时生效，无需重启
- **keybinds 文件加载** — `keybinds/` 目录下 JSON 按键绑定自动注册
- **6 种主题** — 暗色经典/亮色简约/赛博朋克/樱花粉韵/深森幽绿/太空深蓝
- **N 键控制台** — 游戏内按 N 键打开 FR 调试面板，查看 FPS、GUI 数量、切换主题
- **端到端验证** — FRTest-Native 插件覆盖 10 个 GUI 面板、全部控件类型、TCP 协议测试
- **服务端可选增强** — 当 BDS 服务端安装 FR-Server 时，插件可通过 TCP 桥向客户端推送 UI

---

## 快速开始

### 前置依赖

- Windows 10+ 或 Android 12+
- Minecraft Bedrock 1.26.10.04+
- [LeviLamina](https://github.com/LiteLDev/LeviLamina) 客户端版本（Win）/ [LeviLaunchroid](https://github.com/LiteLDev/LeviLaunchroid)（Android）
- XMake (构建工具)

### 构建 Windows 客户端

```bash
cd FastRenderer-Client-Win
xmake f --target_type=client -c -y
xmake -y
```

产物：`build/windows/x64/release/FastRenderer-Client.dll`

### 构建 Android 客户端

```bash
cd FastRenderer-Client-Droid
cmake -B build/android-release -DCMAKE_TOOLCHAIN_FILE=<ndk>/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=24
cmake --build build/android-release
```

产物：`build/libs/arm64/libFastRenderer-Android.so`

### 安装

1. 将编译产物复制到 `Minecraft/mods/FastRenderer/`
2. 最终目录结构应为：
   ```
   mods/FastRenderer/
   ├── FastRenderer-Client.dll
   ├── FastRenderer-Client.pdb   （可选）
   ├── manifest.json
   ├── gui_defs/                  ← 热加载 GUI JSON
   │   └── example.json
   └── keybinds/                  ← 按键绑定 JSON
       └── keybinds.json
   ```
3. 启动游戏，按 **N 键** 打开 FR 控制台

### 编写第一个 GUI

在 `gui_defs/` 目录下创建 `example.json`：

```json
{
    "root": {
        "type": "window",
        "props": {
            "title": "你好，FastRenderer！",
            "width": 300,
            "height": 200
        },
        "children": [
            {
                "type": "text",
                "props": {
                    "value": "欢迎使用 FastRenderer",
                    "color": "#FFD700"
                }
            },
            {
                "type": "button",
                "props": {
                    "text": "点击我",
                    "onClick": "hello_clicked",
                    "color": "#3D85E0"
                }
            }
        ]
    }
}
```

### 编写按键绑定

在 `keybinds/` 目录下创建 `test_keybinds.json`：

```json
{
    "keybinds": [
        {
            "pluginId": "my_plugin",
            "bindId": "open_panel",
            "bindName": "打开面板",
            "vkCode": 79
        }
    ]
}
```

---

## 项目结构

```
FastRenderer/
├── FastRenderer-SDK/include/         ← 公共 SDK（插件开发者引用）
│   ├── FastRendererAPI.h             ← IFastRendererAPI 接口
│   ├── GuiDefinition.h               ← GuiNode/GuiDefinition 类型
│   └── IPlatform.h                   ← 跨平台抽象接口
│
├── FastRenderer-Core/src/            ← 共享核心逻辑（header-only）
│   ├── gui/                          ← JsonGuiParser / JsonGuiRenderer / GuiService
│   ├── input/                        ← KeybindService
│   ├── theme/                        ← ThemeManager（6 种主题）
│   ├── net/                          ← TcpClient（跨平台 TCP 客户端）
│   ├── hotload/                      ← HotReloadService
│   └── layout/                       ← DockLayoutService
│
├── FastRenderer-Client-Win/          ← Windows 客户端 DLL
│   ├── src/
│   │   ├── DX11Hook.h                ← D3D11 Present Hook + D3D12 RenderDragon 兼容
│   │   ├── InputBlocker.h            ← GetAsyncKeyState Hook
│   │   ├── UIRenderHook.h            ← LL Hook: MinecraftUIRenderContext::flushText
│   │   ├── ModEntry.cpp              ← 模组入口 + 网络分发
│   │   └── VerificationUI.h          ← N 键控制台 UI
│   ├── gui_defs/                     ← 示例 GUI 定义
│   ├── manifest.json
│   └── xmake.lua
│
├── FastRenderer-Client-Droid/        ← Android 客户端 SO
│   ├── src/
│   │   ├── ModEntry.cpp              ← PL 插件入口 + ImGui 渲染循环
│   │   ├── EGLHook.h / .cpp          ← eglSwapBuffers Hook
│   │   ├── ImGuiBackend.h / .cpp     ← OpenGL ES3 ImGui 后端
│   │   ├── TouchInput.h / .cpp       ← 触摸/按键事件处理
│   │   └── ui/                       ← FRAndroidMenu / FloatingTrigger / FloatingActionBar
│   ├── sdk/                          ← LeviLaunchroid SDK
│   └── CMakeLists.txt
│
├── FastRenderer-Server/              ← BDS 服务端 DLL（可选）
│   ├── src/
│   │   ├── TcpServer.h               ← TCP Server 多客户端管理
│   │   ├── ServerPluginApi.h         ← 服务端 API 实现
│   │   └── ModEntry.cpp              ← 服务端入口
│   ├── manifest.json
│   └── xmake.lua
│
├── FRTest-Native/                    ← 端到端验证插件
│   ├── src/
│   │   ├── ComplexGui.h              ← 10 个测试 GUI 面板
│   │   └── ModEntry.cpp              ← TCP 桥注册 + 离线文件回退
│   ├── manifest.json
│   └── xmake.lua
│
├── FastUI-Compiler/                  ← XML → JSON 编译器
│   ├── src/main.cpp
│   └── gui_defs/
│
└── docs/                             ← 设计文档与规范
    ├── specs/
    │   ├── FR-TCP桥接通信协议.md      ← TCP 桥协议规范
    │   ├── FR-Android端适配方案.md    ← Android 移植指南
    │   ├── FastRenderer-架构设计文档.md
    │   ├── FastRenderer-实施计划.md
    │   └── FastRenderer-端到端测试方案.md
    └── FastUI-语言规范.md
```

---

## TCP 桥通信协议

FR 的跨进程统一通信协议（JSON over TCP），替代旧式文件桥和 MC 自定义 Packet。

```
Client 应用/游戏                BDS 服务器
    │                               │
    │  ── identify ──────────────→  │
    │  ── gui_register ──────────→  │  (广播/定向)
    │  ←─ gui_register ───────────  │
    │  ←─ keybind_register ────────  │
    │  ── gui_event ─────────────→  │
    │  ── data_exchange ─────────→  │  (双向)
```

详见：[FR-TCP桥接通信协议.md](docs/specs/FR-TCP桥接通信协议.md)

---

## 端到端验证

FRTest-Native 是一个完整的端到端验证插件，部署后自动注册以下内容：

| GUI 面板 | 测试目标 | 控件数 |
|----------|---------|--------|
| 仪表盘 | 基本渲染、动态数据 | 14+ |
| 控件沙盒 | 全部 14 种控件 | 25+ |
| TCP 桥通信 | 协议消息类型 | 10 |
| 压力测试 | 大量控件渲染 (30 行) | 120+ |
| 插件互通 | 跨插件事件流 | 12+ |
| 设置 | 表单控件 | 20+ |
| 数据面板 | 实时监控 | 8+ |
| 控制台 | 日志渲染 | 10 |
| 玩家列表 | 列表渲染 | 15+ |
| 系统诊断 | 自检状态 | 10+ |

部署 `FRTest-Native.dll` 到 `mods/FRTest-Native/` 后启动游戏即可验证。

---

## 技术栈

| 组件 | 技术 |
|------|------|
| 模组框架 | LeviLamina (Win) / LeviLaunchroid (Android) |
| 渲染引擎 | Dear ImGui v1.92+ |
| Hook 框架 | MinHook v1.3+ (Win) / GlossHook (Android) |
| JSON 解析 | nlohmann_json v3.11+ |
| XML 解析 | pugixml（FastUI-Compiler） |
| 网络通信 | TCP Socket (跨平台 header-only) |
| 构建工具 | XMake + MSVC (Win) / CMake + NDK (Android) |
| 语言标准 | C++20 |

---

## 许可

Copyright © 2026 FastRenderer Team

Licensed under the LGPL-3.0 License.
