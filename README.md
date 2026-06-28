# FastRenderer

> Minecraft Bedrock 前置渲染框架插件 — 为其他插件提供声明式 GUI 渲染能力

FastRenderer 是一个基于 LeviLamina 生态的**跨平台客户端前置渲染框架**。它不直接提供游戏功能，而是为其他客户端插件提供统一的 **GUI 渲染能力** 和 **TCP 桥通信**，让插件开发者专注于业务逻辑，无需关心底层渲染、网络通信和跨平台适配。

---

## 核心特性

- **声明式 GUI** — JSON 定义界面布局，支持 16 种节点类型、80+ 属性，插件只需声明布局
- **双模式 TCP 通信** — 纯本地开发（内嵌 TCP Server）+ 生产模式（外部 BDS Server），自动切换
- **跨平台** — Windows (D3D11 + D3D12 RenderDragon) + Android (OpenGL ES 3.0) 双端支持
- **文件热加载** — `gui_defs/` 目录下的 JSON 文件修改即时生效，无需重启
- **按键绑定系统** — 按键注册→捕获→冲突检测，支持修改和恢复默认
- **6 种主题** — 暗色经典/亮色简约/赛博朋克/樱花粉韵/深森幽绿/太空深蓝
- **远程资源加载** — 纹理管理器支持 HTTP 下载 + 本地文件加载，音频管理器支持网播
- **N 键控制台** — 游戏内按 N 键打开 FR 调试面板
- **端到端验证** — FRTest-Native 插件覆盖图片查看器、音乐播放器、设置菜单

---

## 快速开始

### 前置依赖

- Windows 10+
- Minecraft Bedrock 1.26.10.04+
- [LeviLamina](https://github.com/LiteLDev/LeviLamina) 客户端版本
- XMake (构建工具)

### 构建

```bash
cd FastRenderer-Client-Win
xmake f -c -y
xmake -y
```

产物：`bin/FastRenderer/FastRenderer-Client.dll`

### 安装

1. 将 `bin/FastRenderer/` 完整复制到 `Minecraft/mods/FastRenderer/`
2. 最终目录结构：
   ```
   mods/FastRenderer/
   ├── FastRenderer-Client.dll
   ├── manifest.json
   ├── gui_defs/                  ← 热加载 GUI JSON
   └── keybinds/                  ← 按键绑定 JSON
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
            {"type": "text", "props": {"value": "欢迎使用 FastRenderer", "color": "#FFD700"}},
            {"type": "button", "props": {"text": "点击我", "onClick": "hello_clicked", "color": "#3D85E0"}}
        ]
    }
}
```

---

## 通信架构

### 纯本地开发模式（无 Server）

```
FRTest-Native ──TCP:12345──▶ 内嵌 TcpServer (Client 进程)
                               └─ dispatchTcpMessage → GuiService/KeybindService
```

不需要启动 FastRenderer-Server，FRTest-Native 自动连接内嵌 Server。

### 生产模式（有 FastRenderer-Server）

```
FRTest-Native ──TCP──▶ FastRenderer-Server ──broadcast──▶ FastRenderer-Client
(BDS)                                                        (MC 客户端)
```

双通道共存：内嵌 Server + 外部 Client 同时运行，互不干扰。

---

## 项目结构

```
FastRenderer/
├── FastRenderer-SDK/include/         ← 公共 SDK
│   ├── FastRendererAPI.h             ← IFastRendererAPI 接口
│   ├── GuiDefinition.h               ← GuiNode/GuiDefinition 类型
│   └── IPlatform.h                   ← 跨平台抽象接口
│
├── FastRenderer-Core/src/            ← 共享核心逻辑 (header-only)
│   ├── gui/                          ← JsonGuiParser / JsonGuiRenderer / GuiService
│   ├── input/                        ← KeybindService
│   ├── theme/                        ← ThemeManager（6 种主题）
│   ├── net/                          ← TcpClient / TcpServer（跨平台）
│   ├── hotload/                      ← HotReloadService
│   ├── res/                          ← TextureManager / AudioManager
│   └── layout/                       ← DockLayoutService
│
├── FastRenderer-Client-Win/          ← Windows 客户端 DLL
│   ├── src/
│   │   ├── DX11Hook.h                ← D3D11 Present Hook + D3D12 RenderDragon 兼容
│   │   ├── InputBlocker.h            ← GetAsyncKeyState Hook
│   │   ├── UIRenderHook.h            ← LL Hook: MinecraftUIRenderContext::flushText
│   │   ├── ModEntry.cpp              ← 模组入口 + 双通道网络分发
│   │   └── VerificationUI.h          ← N 键控制台 UI
│   ├── manifest.json
│   └── xmake.lua
│
├── FastRenderer-Client-Droid/        ← Android 客户端 SO
│   ├── src/
│   │   ├── ModEntry.cpp              ← PL 插件入口 + ImGui 渲染循环
│   │   ├── EGLHook.h / .cpp          ← eglSwapBuffers Hook
│   │   ├── ImGuiBackend.h / .cpp     ← OpenGL ES3 ImGui 后端
│   │   ├── TouchInput.h / .cpp       ← 触摸/按键事件处理
│   │   └── ui/                       ← FRAndroidMenu / FloatingTrigger
│   └── CMakeLists.txt
│
├── FastRenderer-Server/              ← BDS 服务端 DLL（可选）
│   ├── src/
│   │   ├── TcpServer.h               ← TCP Server 多客户端管理
│   │   ├── ServerPluginApi.h         ← 服务端 API 实现 + 注册缓存同步
│   │   └── ModEntry.cpp              ← 服务端入口
│   ├── manifest.json
│   └── xmake.lua
│
├── FRTest-Native/                    ← 端到端测试插件
│   ├── src/
│   │   ├── ComplexGui.h              ← 图片查看器 / 音乐播放器 / 设置菜单
│   │   └── ModEntry.cpp              ← TCP 注册 + 事件路由
│   ├── pack/                         ← 测试资源（图片、音频）
│   ├── manifest.json
│   └── xmake.lua
│
└── docs/
    ├── specs/
    │   ├── FR-TCP桥接通信协议.md
    │   ├── FR-Android端适配方案.md
    │   ├── FR-设置菜单设计文档.md
    │   └── FastRenderer-GUI-属性完全规范.md
    └── 新手开发指南/
        ├── FastUI-语言规范.md
        └── FastUI-开发者快速上手指南.md
```

---

## TCP 桥通信协议

FR 的跨进程统一通信协议（JSON over TCP）。

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

## FRTest 测试插件

按以下键位操作：

| 键位 | 功能 | 说明 |
|------|------|------|
| `I` | 图片查看器 | 加载 `pack/*.png`，支持缩放/适配/原始尺寸 |
| `M` | 音乐播放器 | MCI 播放 `pack/*.mp3`，支持播放/暂停/停止 |
| `F8` | 设置菜单 | 仪表盘、按键绑定、系统设置 |
| `F3` | 调试叠加 | FPS、调试状态文字显示 |

部署 `FRTest-Native.dll` 到 `mods/FRTest-Native/` 后启动游戏即可测试。

---

## 技术栈

| 组件 | 技术 |
|------|------|
| 模组框架 | LeviLamina (Win) / LeviLaunchroid (Android) |
| 渲染引擎 | Dear ImGui v1.92+ |
| Hook 框架 | MinHook v1.3+ (Win) / GlossHook (Android) |
| JSON 解析 | nlohmann_json v3.11+ |
| 网络通信 | TCP Socket (跨平台 header-only) |
| 资源加载 | WinHTTP / stb_image / MCI (Win) |
| 构建工具 | XMake + MSVC (Win) / CMake + NDK (Android) |
| 语言标准 | C++20 |

---

## 许可

Copyright © 2026 FastRenderer Team

Licensed under the LGPL-3.0 License.
