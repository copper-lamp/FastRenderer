# FastRenderer 实施计划（修订版）

> 基于架构设计文档和 FastUI 语言规范
> 日期：2026-06-28
> 版本：v2.2 — TCP 桥统一通信，Android 移植规划

---

## 总体策略

采用 **客户端优先、自底向上** 的渐进式开发策略：

```
Phase 1 (MVP):    FastRenderer-Core + FastRenderer-Client-Win  →  客户端独立可运行
   ↓
Phase 2:          FastRenderer-Client-Droid (Android)
   ↓
Phase 3:          高级功能与生态
```

**当前状态：Phase 1 MVP 核心功能已完成。**

---

## 项目结构（已实现）

```
FastRenderer/
├── FastRenderer-SDK/include/         ← 共享头文件（插件开发者引用）
│   ├── FastRendererAPI.h             ← 统一 API 定义（文档用途）
│   ├── IPlatform.h                   ← 平台抽象接口
│   ├── GuiDefinition.h               ← GUI 定义类型
│   └── PacketTypes.h                 ← Packet 类型定义
│
├── FastRenderer-Core/                ← 共享核心逻辑（头文件 only）
│   ├── gui/
│   │   ├── JsonGuiParser.h
│   │   ├── JsonGuiRenderer.h
│   │   └── GuiService.h
│   ├── input/
│   │   └── KeybindService.h
│   ├── layout/
│   │   └── DockLayoutService.h
│   ├── theme/
│   │   └── ThemeManager.h
│   └── hotload/
│       └── HotReloadService.h
│
├── FastRenderer-Client-Win/          ← Windows 客户端 DLL (主体)
│   ├── src/
│   │   ├── WinPlatform.cpp           ← IPlatform 实现
│   │   ├── DX11Hook.h
│   │   ├── TcpClient.h               ← TCP 客户端（Win/Android 共用）
│   │   ├── ... (BridgeService.h 已废弃，移至 archive/deprecated/)
│   │   ├── ImGuiManager.h
│   │   ├── InputBlocker.h
│   │   └── ModEntry.cpp
│   ├── gui_defs/                     ← 示例 GUI 定义
│   ├── manifest.json
│   └── xmake.lua
│
├── FastRenderer-Server/              ← 服务端 DLL (可选，内嵌 TCP Server)
│   ├── src/
│   │   ├── TcpServer.h               ← TCP 服务器（替代 FRPackets）
│   │   ├── ServerPluginApi.h
│   │   └── ModEntry.cpp
│   ├── manifest.json
│   └── xmake.lua
│
├── FastRenderer-Client-Droid/        ← Android 客户端 SO (Phase 2)
│
└── docs/
```

---

## Phase 1 实施状态

| 任务 | 状态 | 备注 |
|------|------|------|
| 项目骨架 + 构建系统 | ✅ | FastRenderer-Core / Client-Win / Server |
| IPlatform + WinPlatform | ✅ | D3D11 Hook + ImGui 渲染 |
| JsonGuiParser / JsonGuiRenderer | ✅ | 从 ChiyanMap 移植 |
| GuiService | ✅ | 注册/去重/生命周期 |
| KeybindService | ✅ | 按键注册 + 边缘检测 |
| ThemeManager | ✅ | 6 种主题 |
| DockLayoutService | ✅ | DockSpace 布局 |
| HotReloadService | ✅ | gui_defs/ 文件监控 |
| PacketBridge | → | **已移除**，替换为 **TcpClient + TcpServer** 统一 TCP 桥 |
| PluginApiImpl | ✅ | 接口定义保留，实现已迁移至 BridgeService（BridgeService 已废弃，现使用 TCP 桥） |
| BridgeService | → | **已废弃**，由 TcpClient/TcpServer TCP 桥替代，原代码归档于 `archive/deprecated/` |
| TcpClient + TcpServer | ✅ | TCP 桥统一协议，替代 MC 自定义 Packet |
| E2E 测试 | ⏳ | 需实机验证 |

### 架构变更说明

项目在开发过程中经历了架构变更：
- **v1.0**：插件通过 `IFastRendererAPI::getInstance()` 同进程直调
- **v1.1**：插件通过文件桥（BridgeService）写入 JSON 文件注册 GUI，支持任意语言
- **v2.0**：统一 TCP 桥接协议，替代 MC 自定义 Packet，Win/Android 统一通信

---

## Phase 1 剩余任务

| 任务 | 优先级 | 说明 |
|------|--------|------|
| 实机集成测试 | 高 | 部署到 BDS Mods 目录验证全链路 |
| 错误恢复 | 中 | TCP 断线重连、文件桥目录损坏恢复 |
| 桥接协议文档 | 中 | 已产出：TCP 桥接协议规范 + Android 适配方案 |

---

## Phase 2: Android 移植

| 序号 | 任务 | 说明 | 预估耗时 |
|------|------|------|---------|
| 2.1 | NDK CMake 项目搭建 + ImGui/nlohmann_json 集成 | 编译通过 | 0.5 天 |
| 2.2 | PL_REGISTER_MOD 插件入口 | 验证生命周期 load/enable | 0.5 天 |
| 2.3 | GlossHook hook eglSwapBuffers + ImGui OpenGLES3 渲染 | 屏幕出现 ImGui 窗口 | 1 天 |
| 2.4 | PreloaderInput 触摸事件 → ImGui 鼠标事件映射 | 能点击按钮、拖拽 | 0.5 天 |
| 2.5 | TcpClient 实现 + identify 消息 + 消息路由 | 接入 Server TCP 桥 | 0.5 天 |
| 2.6 | 虚拟按键面板（替代物理键盘） | 屏幕底部浮动工具栏 | 0.5 天 |
| 2.7 | 软键盘支持 + GUI 事件回传 Server | 完整双向通信 | 0.5 天 |
| 2.8 | 端到端集成测试 | 完整链路验证 | 0.5 天 |

### 文件产出

| 实施任务 | 文件路径 |
|---------|---------|
| NDK 配置 | `FastRenderer-Client-Droid/CMakeLists.txt` |
| 插件入口 | `FastRenderer-Client-Droid/src/ModEntry.cpp` |
| EGL Hook | `FastRenderer-Client-Droid/src/EGLHook.h` |
| ImGui 渲染 | `FastRenderer-Client-Droid/src/ImGuiBackend.h` |
| 触摸输入 | `FastRenderer-Client-Droid/src/TouchInput.h` |
| TCP 客户端 | `FastRenderer-Client-Droid/src/TcpClient.h`（与 Win 端共用） |
| 插件清单 | `FastRenderer-Client-Droid/manifest.json` |

---

## Phase 3: 高级功能

---

## 文件产出对应

| 实施任务 | 文件路径 |
|---------|---------|
| IPlatform | `FastRenderer-SDK/include/IPlatform.h` |
| GuiDefinition | `FastRenderer-SDK/include/GuiDefinition.h` |
| FastRendererAPI | `FastRenderer-SDK/include/FastRendererAPI.h` |
| ModEntry | `FastRenderer-Client-Win/src/ModEntry.cpp` |
| DX11Hook | `FastRenderer-Client-Win/src/DX11Hook.h` |
| ~~BridgeService~~ (已废弃) | `archive/deprecated/BridgeService.h` |
| ImGuiManager | `FastRenderer-Client-Win/src/ImGuiManager.h` |
| WinPlatform | `FastRenderer-Client-Win/src/WinPlatform.cpp` |
| InputBlocker | `FastRenderer-Client-Win/src/InputBlocker.h` |
| JsonGuiParser | `FastRenderer-Core/src/gui/JsonGuiParser.h` |
| JsonGuiRenderer | `FastRenderer-Core/src/gui/JsonGuiRenderer.h` |
| GuiService | `FastRenderer-Core/src/gui/GuiService.h` |
| DynamicText | `FastRenderer-Core/src/gui/DynamicText.h` |
| KeybindService | `FastRenderer-Core/src/input/KeybindService.h` |
| ThemeManager | `FastRenderer-Core/src/theme/ThemeManager.h` |
| DockLayoutService | `FastRenderer-Core/src/layout/DockLayoutService.h` |
| HotReloadService | `FastRenderer-Core/src/hotload/HotReloadService.h` |
| xmake config | `FastRenderer-Client-Win/xmake.lua` |
| manifest | `FastRenderer-Client-Win/manifest.json` |

---

## 里程碑

| 里程碑 | 条件 | 状态 |
|--------|------|------|
| M1: FR Client 可加载 | Step 1 完成 | ✅ |
| M2: ImGui 渲染成功 | Step 2 完成 | ✅ |
| M3: JSON GUI 渲染成功 | Step 3 完成 | ✅ |
| M4: 文件桥接通信 | ~~BridgeService 实现~~ (已废弃，由 TCP 桥替代) | ✅ |
| M5: 端到端验证 | 实机验证通过 | ⏳ |
| M6: Android 移植 | Phase 2 完成 | ⏳ |
