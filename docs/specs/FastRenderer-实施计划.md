# FastRenderer 实施计划（修订版）

> 基于架构设计文档和 FastUI 语言规范  
> 日期：2026-06-21  
> 版本：v2.0 — 客户端独立运行，服务端可选

---

## 总体策略

采用 **客户端优先、自底向上** 的渐进式开发策略：

```
Phase 1 (MVP):    FastRenderer-Core + FastRenderer-Client-Win  →  客户端独立可运行
   ↓
Phase 1.5:        FastRenderer-Server (可选服务端组件)
   ↓
Phase 2:          FastRenderer-Client-Droid (Android)
   ↓
Phase 3:          FastUI 编译器 + 高级功能
```

**当前目标：Phase 1 MVP — FastRenderer-Client 在 Windows 上独立运行，其他插件可通过 API 调用。**

---

## 项目结构

```
FastRenderer/
├── FastRenderer-SDK/include/         ← 共享头文件（插件开发者引用）
│   ├── FastRendererAPI.h             ← IFastRendererAPI 接口
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
│   │   ├── ImGuiManager.h
│   │   ├── InputBlocker.h
│   │   └── ModEntry.cpp              ← LeviLamina Mod 入口
│   ├── gui_defs/                     ← 示例 GUI 定义
│   ├── manifest.json
│   └── xmake.lua
│
├── FastRenderer-Server/              ← 服务端 DLL (可选)
│   ├── src/
│   ├── manifest.json
│   └── xmake.lua
│
├── FastRenderer-Client-Droid/        ← Android 客户端 SO (Phase 2)
│
└── docs/
```

---

## Phase 1 任务分解

### Step 1: 项目骨架（高优先级）

| 任务 | 产出 | 依赖 |
|------|------|------|
| 1.1 创建 FastRenderer-SDK 共享头文件 | `include/` 目录 + 4 个头文件 | 无 |
| 1.2 创建 FastRenderer-Core 目录结构 | `gui/`, `input/`, `layout/`, `theme/`, `hotload/` 子目录 | 无 |
| 1.3 创建 FastRenderer-Client-Win xmake 项目 | `xmake.lua` + LeviLamina/ImGui/MinHook 依赖 | 1.1 |
| 1.4 实现 ModEntry + manifest.json | FR Client DLL 入口，验证可加载 | 1.3 |
| 1.5 验证：LeviLamina 可加载 FastRenderer.dll | 游戏内日志确认加载成功 | 1.4 |

### Step 2: IPlatform + WinPlatform 平台适配

| 任务 | 产出 | 依赖 |
|------|------|------|
| 2.1 实现 `IPlatform.h` 接口定义 | `FastRenderer-SDK/include/IPlatform.h` | 1.1 |
| 2.2 实现 `DX11Hook` Present Hook | 拦截 IDXGISwapChain::Present | 1.3 |
| 2.3 实现 `ImGuiManager` (Win32+DX11) | ImGui 初始化/NewFrame/Render/Shutdown | 2.2 |
| 2.4 实现 `InputBlocker` | GetAsyncKeyState Hook + 白名单 | 2.2 |
| 2.5 实现 `WinPlatform` (组合以上) | IPlatform 完整实现 | 2.2-2.4 |
| 2.6 验证：游戏内显示 ImGui 测试窗口 | ImGui Demo Window 可见 | 2.5 |

### Step 3: FastUI 运行时核心（从 ChiyanMap 移植）

| 任务 | 产出 | 依赖 |
|------|------|------|
| 3.1 移植 `JsonGuiParser` | JSON → GuiNode 树 | 1.2 |
| 3.2 移植 `JsonGuiRenderer` | GuiNode 树 → ImGui 控件渲染 | 3.1 |
| 3.3 实现 `GuiService` | GUI 定义注册/去重/生命周期 | 3.2 |
| 3.4 实现 `DynamicText` | `values` 映射 + `ref` 属性解析 | 3.2 |
| 3.5 验证：硬编码 JSON GUI 渲染成功 | JSON 定义 → ImGui 渲染显示 | 2.6, 3.2 |

### Step 4: 热加载 + 按键 + 主题 + DockSpace

| 任务 | 产出 | 依赖 |
|------|------|------|
| 4.1 实现 `HotReloadService` | gui_defs/ 文件监控 + 自动重载 | 3.2 |
| 4.2 实现 `KeybindService` | 按键注册/边缘检测/白名单 | 2.4 |
| 4.3 移植 `ThemeManager` (6 种主题) | ImGui 颜色/样式管理 | 2.3 |
| 4.4 实现 `DockLayoutService` | DockSpace 布局持久化 | 2.3 |
| 4.5 实现 `IFastRendererAPI` 实现类 | Local PluginApi 供其他插件调用 | 3.3, 4.1-4.4 |
| 4.6 验证：完整本地功能演示 | 全部功能可用 | 4.5 |

### Step 5: 示例与集成测试

| 任务 | 产出 | 依赖 |
|------|------|------|
| 5.1 编写示例 GUI 定义 | `gui_defs/example.json` | 3.3 |
| 5.2 编写示例客户端 C++ 插件 | 调用 IFastRendererAPI 注册 UI | 4.5 |
| 5.3 部署测试 | 放入 Minecraft Mods 目录验证 | 5.1-5.2 |

### Step 6: 服务端可选组件（Phase 1.5）

| 任务 | 产出 | 依赖 |
|------|------|------|
| 6.1 FastRenderer-Server xmake 项目 | 可编译的服务端 DLL | 1.1 |
| 6.2 服务端 PluginApi 实现 | 通过 Packet 推送 UI | 4.5 |
| 6.3 客户端 PacketBridge | 接收服务端 Packet 并注入 GuiService | 3.3 |
| 6.4 端到端验证 | 服务端插件注册 → 客户端显示 | 6.2, 6.3 |

---

## 实施顺序图

```
Step 1 (骨架)
    │
    ▼
Step 2 (平台适配) → Step 3 (UI 核心) → Step 4 (集成) → Step 5 (测试)
                                                               │
                                                               ▼
                                                     Step 6 (服务端可选)
```

---

## 文件产出对应

| 实施任务 | 文件路径 |
|---------|---------|
| IPlatform | `FastRenderer-SDK/include/IPlatform.h` |
| GuiDefinition | `FastRenderer-SDK/include/GuiDefinition.h` |
| FastRendererAPI | `FastRenderer-SDK/include/FastRendererAPI.h` |
| ModEntry | `FastRenderer-Client-Win/src/ModEntry.cpp` |
| DX11Hook | `FastRenderer-Client-Win/src/DX11Hook.h` |
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
| PluginApi impl | `FastRenderer-Client-Win/src/PluginApiImpl.h` |
| xmake config | `FastRenderer-Client-Win/xmake.lua` |
| manifest | `FastRenderer-Client-Win/manifest.json` |

---

## 里程碑

| 里程碑 | 条件 | 预计时间 |
|--------|------|---------|
| M1: FR Client 可加载 | Step 1 完成 | Day 1 |
| M2: ImGui 渲染成功 | Step 2 完成 | Day 4 |
| M3: JSON GUI 渲染成功 | Step 3 完成 | Day 6 |
| M4: 本地全功能可用 | Step 4 完成 | Day 9 |
| M5: MVP 完整可用的客户端 FR | Step 5 完成 | Day 10 |
| M6: 服务端可选组件 | Step 6 完成 | Day 13 |
