# FastRenderer 前置渲染框架 — 架构设计文档

> 版本：v1.0  
> 日期：2026-06-21  
> 状态：草稿

---

## 一、项目定位

**FastRenderer** 是一个基于 LeviLamina 生态的 Minecraft Bedrock **客户端前置渲染框架插件**。它不直接提供游戏功能，而是为其他客户端插件（C++ / JS）提供统一的 **GUI 渲染能力** 和 **API 接口**，让其他插件可以专注于业务逻辑，无需关心底层渲染和跨平台适配。

FastRenderer-Client 是**独立可运行的主体**，不依赖服务端组件。服务端 FR 组件为可选增强，仅在服务端也安装了 FR 时，允许服务端插件向客户端推送 UI。

### 核心目标

- **前置框架**：客户端插件通过 `IFastRendererAPI::getInstance()` 同进程调用 FR API
- **独立运行**：FR Client 不依赖服务端，单机/局域网/联服均可使用
- **跨平台**：同时支持 Windows (D3D11) 和 Android (OpenGL ES 3.0)
- **声明式 GUI**：JSON / XML 定义界面，插件只需声明布局
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
│  │  │  Local PluginApi (同进程直调)                    │    │    │
│  │  │  ┌────────────┐ ┌──────────┐ ┌──────────────┐  │    │    │
│  │  │  │ IGuiApi    │ │IKeybind  │ │ IDataChannel │  │    │    │
│  │  │  │            │ │Api       │ │              │  │    │    │
│  │  │  └────────────┘ └──────────┘ └──────────────┘  │    │    │
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
│  │  其他客户端插件 (通过 IFastRendererAPI::getInstance())  │    │
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
| **客户端 API** | Local PluginApi | 同进程 `IFastRendererAPI::getInstance()` 供其他客户端插件调用 | 通用 |
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

    // 网络
    virtual void sendPacket(std::string_view channel, std::string_view data) = 0;
    virtual void onPacket(std::string_view channel,
        std::function<void(std::string_view)> callback) = 0;

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
| 网络发送 | ll::network::Packet | JNI → Minecraft Java Packet |
| 字体 | msyh.ttc (系统) | TTF 嵌入 .so 或 assets |
| 构建系统 | XMake + MSVC | Android NDK + CMake |

---

## 六、PluginApi 设计

### 6.1 统一 API 接口

客户端和服务端共享同一个 API 接口定义，但实现方式不同：

- **客户端**：`IFastRendererAPI` 在客户端 DLL 中实现，其他客户端插件通过 `getInstance()` 同进程直调
- **服务端**：`IFastRendererAPI` 在服务端 DLL 中实现，通过网络 Packet 转发到客户端

```cpp
// FastRenderer-SDK/include/FastRendererAPI.h

class IFastRendererAPI {
public:
    virtual ~IFastRendererAPI() = default;

    // 获取实例（由 FR Client/Server DLL 提供实现）
    static IFastRendererAPI* getInstance();

    // GUI 注册（本地直接生效 / 服务端则发送 Packet）
    virtual bool registerGui(const std::string& pluginId,
        const std::string& guiId, const nlohmann::json& definition) = 0;
    virtual bool unregisterGui(const std::string& pluginId,
        const std::string& guiId) = 0;

    // 按键注册
    virtual bool registerKeybind(const std::string& pluginId,
        const std::string& bindId, const std::string& name, int vkCode) = 0;
    virtual bool unregisterKeybind(const std::string& pluginId,
        const std::string& bindId) = 0;

    // 数据通道（本地模式：同进程发布/订阅）
    virtual void publish(const std::string& channel,
        const std::string& data) = 0;
    virtual void subscribe(const std::string& channel,
        std::function<void(const std::string&)> callback) = 0;

    // GUI 事件回调
    virtual void onGuiEvent(const std::string& guiId,
        std::function<void(const std::string& controlId,
            const std::string& eventType, const std::string& value)> callback) = 0;

    // 网络层状态查询
    virtual bool isServerConnected() = 0;
};
```

### 6.2 客户端插件使用方式

```cpp
#include <FastRendererAPI.h>

bool MyPlugin::enable() {
    // 检查 FR 是否已加载
    auto* fr = IFastRendererAPI::getInstance();
    if (!fr) {
        getSelf().getLogger().error("FastRenderer not found! Disabling.");
        return false;
    }

    // 注册 GUI（本地，零网络开销）
    fr->registerGui("my_plugin", "status_panel", jsonDefinition);

    // 注册按键
    fr->registerKeybind("my_plugin", "open_panel", "状态面板", VK_F6);

    // 监听 GUI 事件
    fr->onGuiEvent("status_panel", [](const std::string& ctrl,
        const std::string& type, const std::string& val) {
        // 处理按钮点击等事件
    });

    return true;
}
```

### 6.3 各场景行为

| 场景 | FR Client | FR Server | 客户端插件 | 服务端插件 |
|------|-----------|-----------|-----------|-----------|
| 单机 | ✅ 全功能 | 不存在 | ✅ 正常使用 | N/A |
| 连服但服务器无 FR | ✅ 全功能 | 不存在 | ✅ 正常使用 | 不适用 |
| 连服且服务器有 FR | ✅ 全功能 | ✅ 全功能 | ✅ + 服务端推送 | ✅ 推送 UI 到客户端 |
| 客户端无 FR | 不存在 | 可有可无 | ❌ 加载报错 | 服务端 FR 功能无效 |

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
- **内存注入优先级**：网络 Packet 注入的定义优先于文件定义

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
├── FastRenderer-SDK/               ← 共享头文件（双端插件开发者引用）
│   ├── include/
│   │   ├── FastRendererAPI.h       ← 统一 API 入口
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
│   │   ├── ImGuiManager.h
│   │   └── InputBlocker.h
│   ├── gui_defs/                   ← 示例 GUI 定义
│   │   └── example.json
│   ├── manifest.json
│   └── xmake.lua
│
├── FastRenderer-Client-Droid/      ← Android 客户端 SO
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
│   │   ├── PluginApi.h
│   │   ├── PacketHandler.h
│   │   └── PluginDispatcher.h
│   ├── manifest.json
│   └── xmake.lua
│
├── docs/
│   ├── specs/
│   │   └── FastRenderer-架构设计文档.md
│   ├── 前端渲染验证系统技术文档.md
│   └── 网络传输架构技术文档.md
│
└── README.md
```

---

## 十、Phase 开发路线图

### Phase 1: 基础设施（MVP）

| 序号 | 任务 | 预估状态 |
|------|------|---------|
| 1.1 | 创建 FastRenderer-Core 项目骨架 | 📋 |
| 1.2 | 实现 IPlatform 接口 + WinPlatform 适配 (D3D11+ImGui) | 📋 |
| 1.3 | 移植 JsonGuiParser (从 ChiyanMap) | 📋 |
| 1.4 | 移植 JsonGuiRenderer (从 ChiyanMap) | 📋 |
| 1.5 | 实现 HotReloadService (文件热加载) | 📋 |
| 1.6 | 实现 KeybindService (按键注册+边缘检测) | 📋 |
| 1.7 | 实现 ThemeManager (6 种主题) | 📋 |
| 1.8 | 实现 DockLayoutService (DockSpace 布局) | 📋 |
| 1.9 | 创建 FastRenderer-Server + PluginApi | 📋 |
| 1.10 | 创建 FastRenderer-Client-Win (DLL 入口) | 📋 |
| 1.11 | 网络 Packet 定义 + 收发 | 📋 |
| 1.12 | 构建示例 + 验证链路 | 📋 |

### Phase 2: 完善与 Android

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
| 3.3 | GUI 事件回调系统 |
| 3.4 | 插件依赖管理 |
| 3.5 | 性能优化（图元数量控制） |

---

## 十一、已知限制与风险

1. **MSVC DLL inline 变量**：所有跨 TU 共享状态必须使用 Meyer's Singleton
2. **Android 网络**：JNI 调用 Java 层 Packet 发送可能有性能开销
3. **Android 渲染**：eglSwapBuffers Hook 的时机和稳定性需验证
4. **GUI 复杂度**：JSON 定义的 GUI 适合中等复杂度的界面，极复杂 UI 建议 C++ 原生
5. **版本兼容**：LeviLamina 和 LeviLaunchroid 的 Minecraft 版本需同步更新

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
