# FastRenderer

> Minecraft Bedrock 前置渲染框架插件 — 为其他插件提供声明式 GUI 渲染能力

FastRenderer 是一个基于 LeviLamina 生态的客户端前置渲染框架插件。它不直接提供游戏功能，而是为其他客户端插件（C++ / JS）提供统一的 **GUI 渲染能力** 和 **API 接口**，让插件开发者可以专注于业务逻辑，无需关心底层渲染和跨平台适配。

---

## 核心特性

- **前置框架** — 客户端插件通过 `IFastRendererAPI::getInstance()` 同进程调用 FR API
- **独立运行** — FR Client 不依赖服务端，单机/局域网/联服均可独立使用
- **声明式 GUI** — JSON 定义界面布局，支持 12 种控件类型，插件只需声明布局
- **文件热加载** — `gui_defs/` 目录下的 JSON 文件修改即时生效，无需重启
- **6 种主题** — 暗色经典/亮色简约/赛博朋克/樱花粉韵/深森幽绿/太空深蓝
- **按键注册** — 插件可注册按键绑定，边缘检测 + 白名单管理
- **动态文本** — 通过 `DynamicText::values` 实时更新 UI 文本
- **中文支持** — ImGui 默认加载微软雅黑字体，原生支持 CJK 字符
- **N 键验证控制台** — 游戏内按 N 键打开 FR 调试面板
- **跨平台** — Windows (D3D11) 优先，Android (OpenGL ES 3.0) 规划中
- **服务端可选增强** — 当服务端也安装了 FR 时，服务端插件可推送 UI 到客户端（Phase 1.5）

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
xmake f --target_type=client -c -y
xmake -y
```

成功编译后，`bin/FastRenderer/` 目录包含部署所需全部文件。

### 安装

1. 将 `bin/FastRenderer` 整个目录复制到 `Minecraft/mods/FastRenderer/`
2. 最终目录结构应为：
   ```
   mods/FastRenderer/
   ├── FastRenderer-Client.dll
   ├── FastRenderer-Client.pdb   （可选）
   ├── manifest.json
   └── gui_defs/
       └── example.json
   ```
3. 启动游戏，按 **N 键** 打开 FR 控制台

### 编写第一个 GUI

在 `gui_defs/` 目录下创建 `hello.json`：

```json
{
    "id": "hello_world",
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

### 从插件调用 API

```cpp
#include <FastRendererAPI.h>

// 在 xmake.lua 中添加：
// add_links("FastRenderer-Client")
// add_linkdirs("path/to/fastrenderer/build/windows/x64/release")
// add_includedirs("path/to/FastRenderer-SDK/include")

bool MyPlugin::enable() {
    auto* fr = IFastRendererAPI::getInstance();
    if (!fr) {
        getSelf().getLogger().error("FastRenderer not found!");
        return false;
    }

    fr->registerGui("my_plugin", "status_panel", jsonContent);
    fr->registerKeybind("my_plugin", "open_panel", "状态面板", VK_F6);

    return true;
}
```

---

## 项目结构

```
FastRenderer/
├── FastRenderer-SDK/include/        ← 共享头文件（插件开发者引用）
│   ├── FastRendererAPI.h            ← IFastRendererAPI 接口 + FRAPI 导出宏
│   ├── GuiDefinition.h              ← GuiNode/GuiDefinition 类型
│   ├── IPlatform.h                  ← 跨平台抽象接口
│   └── PacketTypes.h                ← 网络 Packet 类型定义
│
├── FastRenderer-Core/src/           ← 共享核心逻辑（头文件 only）
│   ├── gui/                         ← JsonGuiParser / JsonGuiRenderer / GuiService / DynamicText
│   ├── input/                       ← KeybindService（按键注册与边缘检测）
│   ├── theme/                       ← ThemeManager（6 种主题）
│   ├── layout/                      ← DockLayoutService（桩实现，等待 ImGui Docking）
│   └── hotload/                     ← HotReloadService（文件监控自动重载）
│
├── FastRenderer-Client-Win/         ← Windows 客户端 DLL
│   ├── src/
│   │   ├── DX11Hook.h               ← D3D11 Present Hook + ImGui 初始化
│   │   ├── InputBlocker.h           ← GetAsyncKeyState Hook + 键盘白名单
│   │   ├── WinPlatform.cpp          ← IPlatform 实现
│   │   ├── MemoryOperators.cpp      ← LeviLamina 统一内存分配器
│   │   ├── ModEntry.cpp             ← 模组入口 + 渲染回调
│   │   ├── PacketBridge.h           ← 客户端网络桥接
│   │   ├── PluginApiImpl.cpp        ← IFastRendererAPI 实现
│   │   └── VerificationUI.h         ← N 键控制台 UI
│   ├── gui_defs/                    ← 示例 GUI 定义文件
│   ├── manifest.json                ← LeviLamina 模组清单
│   └── xmake.lua                    ← 构建配置
│
├── FastRenderer-Server/             ← 服务端 DLL（可选，Phase 1.5）
│   ├── src/
│   │   ├── FRPackets.h              ← 6 种自定义 Packet 定义
│   │   ├── ServerPluginApi.h        ← 服务端 IFastRendererAPI 实现
│   │   └── ModEntry.cpp             ← 模组入口 + Packet 注册
│   ├── manifest.json
│   └── xmake.lua
│
├── FastUI-Compiler/                 ← XML 转 JSON 编译器
│   ├── src/main.cpp                 ← 编译器源码（pugixml + nlohmann_json）
│   ├── gui_defs/player_hud/         ← 示例 .ui 文件 + styles.xml + resources.xml
│   ├── templates/value_display.xml  ← 内置模板
│   └── xmake.lua
│
└── docs/                            ← 设计文档与规范
    ├── specs/                       ← 架构设计、FastUI 规范、实施计划
    └── 新手开发指南/                ← 开发者快速上手指南
```

---

## 实施状态

| 模块 | 状态 | 说明 |
|------|------|------|
| **FastRenderer-SDK** | ✅ 完成 | 4 个头文件，FRAPI 导出宏 |
| **FastRenderer-Core** | ✅ 完成 | 8 个 header-only 模块 |
| **DX11Hook** | ✅ 完成 | D3D11 Present Hook + ImGui 初始化 + CJK 字体 |
| **InputBlocker** | ✅ 完成 | GetAsyncKeyState Hook + 键盘白名单 |
| **GuiService / Parser / Renderer** | ✅ 完成 | JSON → GuiNode → ImGui 渲染流水线 |
| **HotReloadService** | ✅ 完成 | 文件轮询 + 自动重载 |
| **KeybindService** | ✅ 完成 | 按键注册、边缘检测、白名单 |
| **ThemeManager** | ✅ 完成 | 6 种主题，延迟应用到 ImGui 就绪 |
| **PluginApiImpl** | ✅ 完成 | IFastRendererAPI 本地实现 |
| **VerificationUI** | ✅ 完成 | N 键控制台，已验证游戏内正常运行 |
| **MemoryOperators** | ✅ 完成 | LeviLamina 内存分配器，DLL 可加载 |
| **PacketBridge（客户端）** | ✅ 完成 | 接收服务端推送注入 GuiService |
| **FastRenderer-Server** | ✅ 代码完成 | 6 种自定义 Packet + ServerPluginApi（需验证编译） |
| **FastUI-Compiler** | ✅ 完成 | XML → JSON 编译，支持 v_for/v_if/模板/样式 |
| Windows 编译验证 | ✅ 通过 | FastRenderer-Client.dll 750KB ✅ |
| 游戏内加载验证 | ✅ 通过 | LeviLamina 加载成功，中文渲染正常 ✅ |
| **DockLayoutService** | ⚠️ 桩实现 | 依赖 ImGui Docking 特性（需 xmake 配置 docking=true） |
| **Plugin 端到端调用测试** | ⏳ 待测 | 需另一个插件链接 FRAPI 验证 |
| **Phase 2: Android** | ❌ 未开始 | FastRenderer-Client-Droid |
| **Phase 3: 高级功能** | ❌ 未开始 | 动画系统、物理渲染、数据绑定 |

---

## FastUI 编译器

FastUI 编译器是一个独立的命令行工具，将 XML 格式的 `.ui` 文件编译为 FR 运行时消费的 JSON：

```bash
# 编译单个 UI
cd FastUI-Compiler
FastUI-Compiler.exe gui_defs/player_hud/ -o dist -t templates

# 批量编译
FastUI-Compiler.exe gui_defs/ -r -t templates -o dist
```

支持特性：模板展开、`v_for` 列表渲染、`v_if` 条件渲染、具名样式、组件默认样式、资源元数据嵌入。

---

## 技术栈

| 组件 | 技术 |
|------|------|
| 模组框架 | LeviLamina (Windows) / LeviLaunchroid (Android) |
| 渲染引擎 | Dear ImGui v1.92+ |
| Hook 框架 | MinHook v1.3+ |
| JSON 解析 | nlohmann_json v3.11+ |
| XML 解析 | pugixml（FastUI-Compiler） |
| 构建工具 | XMake + MSVC |
| 语言标准 | C++20 |

---

## 许可

Copyright © 2024 FastRenderer Team

Licensed under the LGPL-3.0 License.
