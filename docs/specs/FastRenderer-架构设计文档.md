# FastRenderer 前置渲染框架 — 架构设计文档

> 版本：v1.1
> 日期：2026-06-21
> 状态：草稿

---

## 一、项目定位

**FastRenderer** 是一个基于 LeviLamina 生态的 Minecraft Bedrock **客户端前置渲染框架插件**。它不直接提供游戏功能，而是为其他客户端插件（C++ / JS / 其他语言）提供统一的 **GUI 渲染能力**，让其他插件可以专注于业务逻辑，无需关心底层渲染和跨平台适配。

FastRenderer-Client 是**独立可运行的主体**，不依赖服务端组件。服务端 FR 组件为可选增强，仅在服务端也安装了 FR 时，允许服务端插件向客户端推送 UI。

### 核心目标

- **前置框架**：客户端插件通过文件桥 (BridgeService) 或 PacketBridge 注册 GUI
- **多语言支持**：C++ 原生、LegacyScriptEngine (JS)、任何能写 JSON 文件的语言均可调用
- **独立运行**：FR Client 不依赖服务端，单机/局域网/联服均可使用
- **声明式 GUI**：JSON 定义界面，插件只需声明布局
- **热加载**：GUI 定义文件修改即时生效，无需重启
- **服务端可选增强**：当服务端也有 FR 时，服务端插件可推送 UI 到客户端

---

## 二、整体架构

### 2.1 分层模块化架构

```
┌─────────────────────────────────────────────────────────────────┐
│                    Minecraft Bedrock 客户端                       │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │              FastRenderer-Client (.dll / .so)            │    │
│  │                                                         │    │
│  │  ┌─────────────────────────────────────────────────┐    │    │
│  │  │  BridgeService (文件桥接入口)                    │    │    │
│  │  │  轮询 gui_bridge/ 目录，任意语言插件通过         │    │    │
│  │  │  JSON 文件注册 GUI / Keybind                    │    │    │
│  │  └─────────────────────────────────────────────────┘    │    │
│  │                                                         │    │
│  │  ┌─────────────────────────────────────────────────┐    │    │
│  │  │  共享核心层 (Core)                               │    │    │
│  │  │  JsonGuiParser / JsonGuiRenderer                │    │    │
│  │  │  GuiService / KeybindService / ThemeManager     │    │    │
│  │  │  DockLayoutService / HotReloadService            │    │    │
│  │  └─────────────────────────────────────────────────┘    │    │
│  │                                                         │    │
│  │  ┌─────────────────────────────────────────────────┐    │    │
│  │  │  平台适配层 (Platform)                           │    │    │
│  │  │  WinPlatform (Win) / AndroidPlatform (Droid)    │    │    │
│  │  │  DX11 Hook / eglSwapBuffers Hook                │    │    │
│  │  └─────────────────────────────────────────────────┘    │    │
│  │                                                         │    │
│  │  ┌─────────────────────────────────────────────────┐    │    │
│  │  │  网络层 (Network) ← 可选，仅服务端有FR时激活     │    │    │
│  │  │  PacketBridge → 静默降级                        │    │    │
│  │  └─────────────────────────────────────────────────┘    │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  其他客户端插件 (通过文件桥注册)                         │    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘

                    ─── 可选：如果服务端也有 FR ───
                                   │
                          Minecraft 自定义 Packet
                                   │
                                   ▼
                    ┌─────────────────────────────┐
                    │  FastRenderer-Server.dll    │ ← 可选组件
                    │  (仅当 BDS 安装了它才存在)   │
                    │  服务端插件 → PluginApi →   │
                    │  Packet → 客户端             │
                    └─────────────────────────────┘
```

### 2.2 模块职责

| 层次 | 模块 | 职责 | 平台相关性 |
|------|------|------|-----------|
| **客户端通信** | BridgeService | 轮询 `gui_bridge/` 目录 JSON 文件，统一注册到 GuiService / KeybindService | 通用 |
| **客户端核心** | GuiService | GUI 定义注册/反注册、生命周期管理、去重 | 通用 |
| | JsonGuiParser | JSON → 控件节点树 (GuiNode) | 通用 |
| | JsonGuiRenderer | 控件节点树 → ImGui 控件渲染 | 通用 |
| | KeybindService | 按键注册、边缘检测、白名单管理 | 通用 |
| | ThemeManager | 6 种主题的 ImGui 颜色/样式管理 | 通用 |
| | DockLayoutService | DockSpace 布局持久化 (JSON 文件) | 通用 |
| | HotReloadService | gui_defs/ 目录文件监控、自动重载 | 通用 |
| **客户端平台** | IPlatform (接口) | 渲染/输入/网络/文件系统的抽象 | 接口通用 |
| | WinPlatform | D3D11 Present Hook, ImGui Win32+DX11, GetAsyncKeyState | Windows |
| | AndroidPlatform | eglSwapBuffers Hook, ImGui Android+GLES3, Touch映射 | Android |
| **客户端网络** | PacketBridge | 与服务端 FR 通信，无服务端时静默不工作 | Windows |
| **服务端可选** | Server PluginApi | 供服务端插件调用，通过 Packet 推送 UI 到客户端 | 通用 |
| | PacketHandler | Packet 收发 + 路由分发 | 通用 |

---

## 三、网络协议设计

### 3.1 自定义 Packet 定义

所有通信使用 Minecraft 自定义数据包，Payload 为 JSON 格式。

| Packet 名称 | 方向 | 用途 | 关键 Payload 字段 |
|---|---|---|---|
| `GuiRegistrationPacket` | Server→Client | 注册/更新 GUI 定义 | `pluginId`, `guiId`, `jsonContent`, `version` |
| `GuiUnregistrationPacket` | Server→Client | 卸载 GUI | `pluginId`, `guiId` |
| `KeybindRegistrationPacket` | Server→Client | 注册按键绑定 | `pluginId`, `bindId`, `bindName`, `vkCode` |
| `KeybindUnregistrationPacket` | Server→Client | 卸载按键 | `pluginId`, `bindId` |
| `GuiEventPacket` | Client→Server | GUI 交互事件上报 | `guiId`, `controlId`, `eventType`, `value` |
| `DataExchangePacket` | 双向 | 通用数据交换 | `channel`, `data` |

### 3.2 Payload 格式示例

```
GuiRegistrationPacket:
{
    "pluginId": "my_plugin",
    "guiId": "main_panel",
    "version": 1,
    "jsonContent": {
        "type": "window",
        "props": {
            "label": "控制面板",
            "width": 400,
            "height": 300
        },
        "children": [
            { "type": "button", "props": { "label": "执行", "id": "btn_exec" } },
            { "type": "text", "props": { "value": "状态: 就绪", "id": "txt_status" } }
        ]
    }
}

GuiEventPacket:
{
    "guiId": "main_panel",
    "controlId": "btn_exec",
    "eventType": "click",
    "value": ""
}
```

### 3.3 Packet ID 定义

```cpp
enum class FastPacketId : uint64 {
    GuiRegistration = hash("fastrenderer:gui_reg"),
    GuiUnregistration = hash("fastrenderer:gui_unreg"),
    KeybindRegistration = hash("fastrenderer:keybind_reg"),
    KeybindUnregistration = hash("fastrenderer:keybind_unreg"),
    GuiEvent = hash("fastrenderer:gui_event"),
    DataExchange = hash("fastrenderer:data_exchange"),
};
```

使用 Meyer's Singleton 管理 Packet 注册，避免 MSVC DLL inline 变量跨 TU 问题。

---

## 四、JSON GUI 定义规范

### 4.1 控件节点结构

```cpp
struct GuiNode {
    std::string type;                              // 控件类型
    std::map<std::string, std::string> props;      // 属性键值对
    std::vector<GuiNode> children;                 // 子节点
};

struct GuiDefinition {
    GuiNode root;                                  // 根节点
    std::string id;                                // 唯一标识
    std::string pluginId;                          // 来源插件
    int version;                                   // 版本号（去重用）
};
```

### 4.2 支持的控件类型

| 类型 | 对应 ImGui 控件 | 优先级 |
|------|----------------|--------|
| `window` | Begin/End | P0 |
| `column` | 垂直布局 | P0 |
| `row` | 水平 SameLine | P0 |
| `button` | Button | P0 |
| `text` | Text | P0 |
| `checkbox` | Checkbox | P0 |
| `slider` | SliderFloat | P0 |
| `separator` | Separator | P0 |
| `input` | InputText | P1 |
| `combo` | Combo | P1 |
| `canvas` | 自定义绘制指令 | P1 |
| `group` | BeginGroup/EndGroup | P1 |
| `tree` | TreeNode/TreePop | P1 |
| `table` | BeginTable/EndTable | P2 |
| `image` | Image (需纹理加载) | P2 |

### 4.3 DynamicText 机制

JSON GUI 中支持动态文本绑定，C++ 端可实时更新值：

```json
{
    "type": "text",
    "props": {
        "ref": "player_health",
        "prefix": "生命值: "
    }
}
```

```cpp
// C++ 端更新
DynamicText::values["player_health"] = std::to_string(player->getHealth());
```

---

## 五、平台抽象层

### 5.1 IPlatform 接口

```cpp
class IPlatform {
public:
    virtual ~IPlatform() = default;

    // 生命周期
    virtual bool init() = 0;
    virtual void shutdown() = 0;

    // 渲染
    virtual void initImGui() = 0;
    virtual void newFrame() = 0;
    virtual void renderDrawData() = 0;
    virtual void shutdownImGui() = 0;
    virtual float getFrameTime() = 0;
    virtual Vec2 getDisplaySize() = 0;

    // 输入
    virtual bool isKeyDown(int vk) = 0;
    virtual Vec2 getMousePos() = 0;
    virtual void setCursorVisible(bool visible) = 0;
    virtual void setInputBlocked(bool blocked) = 0;

    // 文件系统
    virtual std::filesystem::path getDataDir() = 0;
    virtual std::filesystem::path getGuiDefsDir() = 0;
    virtual std::filesystem::path getConfigDir() = 0;

    // 字体
    virtual bool loadFont(const std::string& name, const std::string& path,
        float size) = 0;
};
```

### 5.2 平台差异对照表

| 维度 | Windows | Android |
|------|---------|---------|
| 模组框架 | LeviLamina (Client) | LeviLaunchroid Preloader |
| 模组文件 | `.dll` (x64) | `.so` (arm64-v8a) |
| 渲染 API | DirectX 11 | OpenGL ES 3.0 |
| ImGui 后端 | ImGui_ImplWin32 + ImGui_ImplDX11 | ImGui_ImplAndroid + ImGui_ImplOpenGL3 |
| Hook 点 | IDXGISwapChain::Present (vtable[8]) | eglSwapBuffers |
| 输入来源 | WndProc + GetAsyncKeyState | Android Touch Events (JNI) |
| 字体 | msyh.ttc (系统) | TTF 嵌入 .so 或 assets |
| 构建系统 | XMake + MSVC | Android NDK + CMake |

---

## 六、插件通信方式

FastRenderer 不要求插件链接其 DLL 或调用 getInstance()。插件通过**文件桥接协议**与 FR 通信：

### 6.1 桥接目录

```
Mods/FastRenderer/gui_bridge/
├── gui_pending.json     ← 注册 GUI（JSON 数组或单个对象）
├── gui_unreg.json       ← 反注册 GUI
└── keybind_pending.json ← 注册按键绑定
```

### 6.2 文件桥接格式

任何语言（C++ / JS / Python / Lua）只需写入 JSON 文件到桥接目录：

```json
[
    {
        "pluginId": "my_plugin",
        "guiId": "main_panel",
        "definition": "{\"type\":\"window\",\"props\":{\"title\":\"面板\"}...}"
    }
]
```

或使用 calculator_backend 兼容的格式：

```json
[
    {
        "pluginId": "my_plugin",
        "guiId": "main_panel",
        "root": { "type": "window", "props": { "title": "面板" }, "children": [...] }
    }
]
```

BridgeService 每 200ms 轮询桥接目录，读取后自动调用 GuiService::registerGui() 注册到渲染管线。

### 6.3 三种调用路径

```
场景A: 客户端原生插件 → 文件桥
  [客户端插件] → 写入 gui_pending.json → [BridgeService]
                                          → GuiService / KeybindService

场景B: 服务端原生插件 → Packet → 客户端渲染
  [服务端插件] → ServerPluginApi → sendBroadcast(Packet)
                → Minecraft Network → [FastRenderer-Client::PacketBridge]
                → GuiService / KeybindService

场景C: 任意语言脚本插件 → 文件桥
  [JS/Python/Lua 插件] → 写入 gui_pending.json → [BridgeService]
                        → GuiService / KeybindService
```

---

## 七、文件热加载机制

```
gui_defs/ 目录
    │
    ├── plugin_a/
    │   ├── main_panel.json
    │   └── settings.json
    │
    ├── plugin_b/
    │   └── overlay.json
    │
    └── (每个插件独立子目录，防止冲突)
```

- **监控线程**：每 500ms 扫描 `gui_defs/` 目录
- **变更检测**：`std::filesystem::last_write_time` 轮询
- **热加载流程**：检测变更 → 标记脏 → 下一帧重新解析 + 渲染
- **内存注入优先级**：BridgeService 和网络 Packet 注入的定义优先于文件定义

---

## 八、渲染流水线

```
Minecraft 游戏帧循环
    │
    ▼
DX11::Present (Win) / eglSwapBuffers (Android)
    │
    ├─ Hook 拦截
    │   │
    │   ├─ ImGuiManager::newFrame()
    │   │   ├─ IPlatform::newFrame()       ← 平台相关
    │   │   ├─ ImGui::NewFrame()
    │   │   │
    │   │   ├─ HotReloadService::poll()    ← 检查文件变更
    │   │   ├─ GuiService::renderAll()     ← 渲染所有活跃 GUI
    │   │   │   ├─ JsonGuiRenderer::render()
    │   │   │   ├─ DockLayoutService::apply()
    │   │   │   └─ ThemeManager::apply()
    │   │   │
    │   │   └─ KeybindService::poll()      ← 按键检测
    │   │
    │   └─ ImGui::Render()
    │   └─ IPlatform::renderDrawData()
    │
    ▼
原始 Present / eglSwapBuffers 继续
```

---

## 九、项目目录结构

```
FastRenderer/
├── FastRenderer-SDK/               ← 共享头文件（插件开发者引用）
│   ├── include/
│   │   ├── FastRendererAPI.h       ← 统一 API 定义（文档用途）
│   │   ├── IPlatform.h             ← 平台抽象接口
│   │   ├── GuiDefinition.h         ← GUI 定义类型
│   │   ├── PacketTypes.h           ← Packet 类型
│   │   └── EventTypes.h            ← 事件类型
│   └── CMakeLists.txt
│
├── FastRenderer-Core/              ← 共享核心逻辑（双端复用）
│   ├── src/
│   │   ├── gui/
│   │   │   ├── JsonGuiParser.h
│   │   │   ├── JsonGuiRenderer.h
│   │   │   ├── GuiService.h
│   │   │   └── DynamicText.h
│   │   ├── input/
│   │   │   └── KeybindService.h
│   │   ├── layout/
│   │   │   └── DockLayoutService.h
│   │   ├── theme/
│   │   │   └── ThemeManager.h
│   │   ├── hotload/
│   │   │   └── HotReloadService.h
│   │   └── util/
│   │       ├── Logger.h
│   │       └── Config.h
│   └── CMakeLists.txt
│
├── FastRenderer-Client-Win/        ← Windows 客户端 DLL
│   ├── src/
│   │   ├── WinPlatform.cpp         ← IPlatform 实现
│   │   ├── DX11Hook.h
│   │   ├── BridgeService.h         ← 文件桥接服务
│   │   ├── ImGuiManager.h
│   │   ├── InputBlocker.h
│   │   └── ModEntry.cpp
│   ├── gui_defs/                   ← 示例 GUI 定义
│   │   └── example.json
│   ├── manifest.json
│   └── xmake.lua
│
├── FastRenderer-Client-Droid/      ← Android 客户端 SO (Phase 2)
│   ├── src/
│   │   ├── AndroidPlatform.cpp     ← IPlatform 实现
│   │   ├── EGLHook.h
│   │   ├── ImGuiManager.h
│   │   ├── TouchMapper.h
│   │   └── JniPacketBridge.h
│   ├── manifest.json
│   └── CMakeLists.txt
│
├── FastRenderer-Server/            ← 服务端（双平台复用）
│   ├── src/
│   │   ├── FRPackets.h            ← 服务端 Packet 定义
│   │   ├── ServerPluginApi.h      ← 服务端 PluginApi 实现
│   │   └── ModEntry.cpp
│   ├── manifest.json
│   └── xmake.lua
│
├── docs/
│   ├── specs/
│   │   ├── FastRenderer-架构设计文档.md
│   │   ├── FastRenderer-实施计划.md
│   │   ├── FastRenderer-端到端测试方案.md
│   │   └── FastUI-语言规范.md
│   ├── 网络传输架构技术文档.md
│   └── 最小验证设计.md
│
└── README.md
```

---

## 十、Phase 开发路线图

### Phase 1: 基础设施（MVP）

| 序号 | 任务 | 预估状态 |
|------|------|---------|
| 1.1 | 创建 FastRenderer-Core 项目骨架 | ✅ |
| 1.2 | 实现 IPlatform 接口 + WinPlatform 适配 (D3D11+ImGui) | ✅ |
| 1.3 | 移植 JsonGuiParser (从 ChiyanMap) | ✅ |
| 1.4 | 移植 JsonGuiRenderer (从 ChiyanMap) | ✅ |
| 1.5 | 实现 HotReloadService (文件热加载) | ✅ |
| 1.6 | 实现 KeybindService (按键注册+边缘检测) | ✅ |
| 1.7 | 实现 ThemeManager (6 种主题) | ✅ |
| 1.8 | 实现 DockLayoutService (DockSpace 布局) | ✅ |
| 1.9 | 创建 FastRenderer-Server + PluginApi | ✅ |
| 1.10 | 创建 FastRenderer-Client-Win (DLL 入口) | ✅ |
| 1.11 | 网络 Packet 定义 + 收发 | ✅ |
| 1.12 | 实现 BridgeService (文件桥接) | ✅ |
| 1.13 | 构建示例 + E2E 验证 | ⏳ |

### Phase 2: Android 移植

| 序号 | 任务 |
|------|------|
| 2.1 | FastRenderer-Client-Droid AndroidPlatform 适配 |
| 2.2 | EGLHook + GLES3 ImGui 适配 |
| 2.3 | TouchMapper 触摸输入适配 |
| 2.4 | JniPacketBridge 网络桥接 |
| 2.5 | 双端集成测试 |
| 2.6 | 错误恢复机制（超时、重试、损坏恢复） |

### Phase 3: 高级功能

| 序号 | 任务 |
|------|------|
| 3.1 | 扩展控件库 (table, tree, image) |
| 3.2 | DynamicText 系统完善 |
| 3.3 | GUI 事件回调系统增强（双向数据绑定） |
| 3.4 | 桥接协议版本化（支持向后兼容） |
| 3.5 | 性能优化（图元数量控制、惰性渲染） |
| 3.6 | 网络传输加密/压缩 |

---

## 十一、已知限制与风险

1. **MSVC DLL inline 变量**：所有跨 TU 共享状态必须使用 Meyer's Singleton
2. **Android 网络**：JNI 调用 Java 层 Packet 发送可能有性能开销
3. **Android 渲染**：eglSwapBuffers Hook 的时机和稳定性需验证
4. **GUI 复杂度**：JSON 定义的 GUI 适合中等复杂度的界面，极复杂 UI 建议 C++ 原生
5. **版本兼容**：LeviLamina 和 LeviLaunchroid 的 Minecraft 版本需同步更新
6. **文件桥竞争**：多个插件同时写入桥接文件可能存在竞态，建议使用批量数组格式

---

## 十二、技术依赖

| 依赖 | 版本 | 用途 | 平台 |
|------|------|------|------|
| LeviLamina | latest (client) | 模组框架 + Packet API | Windows |
| LeviLaunchroid | latest | 模组框架 | Android |
| Dear ImGui | v1.92+ | GUI 渲染 | 双端 |
| nlohmann_json | v3.11+ | JSON 解析 | 双端 |
| MinHook | v1.3+ | API Hook | Windows |
| OpenGL ES 3.0 | 系统 | 渲染 API | Android |
