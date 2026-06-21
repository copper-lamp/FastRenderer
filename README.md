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
- **跨平台** — Windows (D3D11) 优先，Android (OpenGL ES 3.0) 规划中
- **服务端可选增强** — 当服务端也安装了 FR 时，服务端插件可推送 UI 到客户端

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
xmake f --target_type=client -c
xmake
```

成功编译后，`build/windows/x64/release/FastRenderer-Client.dll` 即为输出文件。

### 安装

1. 将 `FastRenderer-Client.dll` 复制到 `Minecraft/mods/FastRenderer/` 目录
2. 将 `FastRenderer-Client-Win/gui_defs/` 目录下的 JSON 文件复制到 `Minecraft/mods/FastRenderer/gui_defs/`
3. 启动游戏，FastRenderer 自动加载

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

bool MyPlugin::enable() {
    auto* fr = IFastRendererAPI::getInstance();
    if (!fr) {
        getSelf().getLogger().error("FastRenderer not found!");
        return false;
    }

    fr->registerGui("my_plugin", "status_panel", jsonContent);
    fr->registerKeybind("my_plugin", "open_panel", "状态面板", VK_F6);

    fr->onGuiEvent("status_panel", [](const std::string& ctrl,
        const std::string& type, const std::string& val) {
        // 处理按钮点击等事件
    });

    return true;
}
```

---

## 项目结构

```
FastRenderer/
├── FastRenderer-SDK/include/       ← 共享头文件（插件开发者引用）
│   ├── FastRendererAPI.h           ← IFastRendererAPI 接口
│   ├── GuiDefinition.h             ← GuiNode/GuiDefinition 类型
│   ├── IPlatform.h                 ← 跨平台抽象接口
│   └── PacketTypes.h               ← 网络 Packet 类型定义
│
├── FastRenderer-Core/src/          ← 共享核心逻辑（头文件 only）
│   ├── gui/                        ← JSON 解析、渲染、动态文本
│   ├── input/                      ← 按键注册与边缘检测
│   ├── theme/                      ← 6 种主题系统
│   ├── layout/                     ← DockSpace 布局
│   └── hotload/                    ← 文件热加载
│
├── FastRenderer-Client-Win/        ← Windows 客户端 DLL
│   ├── src/                        ← DX11Hook / InputBlocker / WinPlatform
│   ├── gui_defs/                   ← 示例 GUI 定义文件
│   ├── manifest.json               ← LeviLamina 模组清单
│   └── xmake.lua                   ← 构建配置
│
└── docs/                           ← 设计文档与规范
    ├── specs/                      ← 架构设计、FastUI 规范、实施计划
    └── 新手开发指南/               ← 开发者快速上手指南
```

---

## FastUI 语言

FastRenderer 提供了一套 XML 风格的声明式 UI 语言 **FastUI**，编译为 JSON 后由渲染引擎执行。详见：

- [FastUI 语言规范](docs/specs/FastUI-语言规范.md)
- [开发者快速上手指南](docs/新手开发指南/FastUI-开发者快速上手指南.md)

---

## 架构设计

设计文档包括：

- [架构设计文档](docs/specs/FastRenderer-架构设计文档.md) — 分层模块化架构、跨平台设计、通信协议
- [实施计划](docs/specs/FastRenderer-实施计划.md) — Phase 1-3 开发路线图

---

## 技术栈

| 组件 | 技术 |
|------|------|
| 模组框架 | LeviLamina (Windows) / LeviLaunchroid (Android) |
| 渲染引擎 | Dear ImGui v1.92+ |
| Hook 框架 | MinHook v1.3+ |
| JSON 解析 | nlohmann_json v3.11+ |
| 构建工具 | XMake + MSVC |
| 语言标准 | C++20 |

---

## License

Copyright © 2024 FastRenderer Team

Licensed under the LGPL-3.0 License.
