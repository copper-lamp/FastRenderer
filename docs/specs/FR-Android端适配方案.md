# FastRenderer Android 端适配方案

> 版本：v1.1
> 日期：2026-06-28
> 状态：草案

---

## 一、概述

本文档描述如何将 FastRenderer 框架移植到 Android 平台，使其以 LeviLaunchroid 原生 `.io` 插件的形式运行在 Minecraft Bedrock Android 版本中。

### 适配目标

- 在 Android 手机的 Minecraft 游戏画面上渲染 ImGui 界面
- 通过 TCP 桥接接收 BDS Server 推送的 GUI 定义
- 通过 TCP 桥接上报 GUI 交互事件
- 复用现有的 FastRenderer-Core 核心逻辑（GuiService、JsonGuiParser、JsonGuiRenderer）
- **零改动**服务端插件代码

### 核心差异

| 维度 | Windows | Android |
|---|---|---|
| 模组框架 | LeviLamina (Client) | LeviLaunchroid Preloader |
| 模组文件 | `.dll` (x64) | `.so` (arm64-v8a) |
| 渲染 API | DirectX 11 | OpenGL ES 3.0 |
| 输入来源 | WndProc + GetAsyncKeyState | 触摸事件 (PreloaderInput) |
| 通信 | LeviLamina 网络包 | TCP Socket |
| 构建 | xmake + MSVC | NDK + CMake |

---

## 二、Android 项目结构

```
FastRenderer-Client-Droid/
├── CMakeLists.txt                # NDK CMake 构建
├── manifest.json                 # LeviLaunchroid 插件清单
│
├── src/
│   ├── ModEntry.cpp              # PL_REGISTER_MOD 插件入口
│   ├── EGLHook.h                 # GlossHook hook eglSwapBuffers
│   ├── ImGuiBackend.h            # ImGui OpenGL ES 初始化与渲染
│   ├── TouchInput.h              # PreloaderInput 触摸→ImGui 桥接
│   ├── TcpClient.h               # TCP 客户端（与 Win 端可复用）
│   ├── ui/
│   │   ├── FloatingTrigger.h     # 悬浮触发器（可拖动图标）
│   │   ├── FRAndroidMenu.h       # 主菜单（按键标签页+设置标签页）
│   │   ├── FloatingActionBar.h   # 浮动快捷栏（只显示勾选按键）
│   │   └── FRMenuStore.h         # 状态管理（按键列表、配置持久化）
│   └── VerificationUI.h          # 验证 UI
│
├── libs/
│   └── arm64/
│       └── libGlossHook.a        # 预编译 hook 库（来自 preloader-android）
│
├── res/
│   ├── fonts/
│   │   └── NotoSansSC-Regular.ttf  # 中文字体
│   └── icons/
│       └── icon.png
│
└── build/
    ├── CMakePresets.json
    └── build.sh
```

---

## 三、插件生命周期 (LeviLaunchroid)

### 3.1 导出符号

依据 LeviLaunchroid 的 `PL_REGISTER_MOD` 宏，插件需导出 4 个符号：

```cpp
#include <pl/cpp/mod/RegisterHelper.hpp>

class FRAndroidMod {
public:
    bool load();      // dlopen 后立即调用
    bool enable();    // 所有插件加载完成，游戏启动前
    bool disable();   // 游戏退出时
    bool unload();    // 插件卸载时
};

PL_REGISTER_MOD(FRAndroidMod, frAndroidInstance)
```

### 3.2 load() — 初始化

```cpp
bool load() {
    // 1. 保存 JavaVM 指针（用于后续 JNI 调用）
    // 2. 记录插件根目录（用于配置文件路径）
    // 3. 初始化日志系统
    return true;
}
```

### 3.3 enable() — 注入和启动

```cpp
bool enable() {
    // 1. 注册 GlossHook：hook eglSwapBuffers
    // 2. 注册 PreloaderInput 触摸回调
    // 3. 初始化 ImGui 上下文（延迟到首次 eglSwapBuffers hook 执行）
    // 4. 连接 TCP 服务器
    // 5. 启动 TCP 接收线程
    return true;
}
```

### 3.4 disable() / unload() — 清理

```cpp
bool disable() {
    // 1. 断开 TCP 连接
    // 2. 反注册 hook
    // 3. 清理 ImGui 资源
    return true;
}
```

---

## 四、渲染注入（EGL Hook）

### 4.1 原理

Minecraft Bedrock Android 使用 OpenGL ES 渲染管线，每帧最终调用 `eglSwapBuffers()` 交换前后缓冲区。Hook 这个函数即可在每帧渲染前插入 ImGui。

### 4.2 使用 GlossHook

```cpp
#include <pl/memory/Hook.h>

// 函数签名
using EglSwapBuffers_t = EGLBoolean (*)(EGLDisplay dpy, EGLSurface surface);
EglSwapBuffers_t oEglSwapBuffers = nullptr;

EGLBoolean hkEglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_imguiInitialized) {
        InitImGuiFromGLContext(dpy, surface);  // 首次调用时初始化
    }

    if (g_imguiInitialized) {
        RenderImGui();  // 插入 ImGui 渲染
    }

    return oEglSwapBuffers(dpy, surface);  // 调用原始函数
}

void HookEglSwapBuffers() {
    // 查找 libEGL.so 中 eglSwapBuffers 的地址
    void* target = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    // 使用 GlossHook 创建 hook
    pl_hook((PLFuncPtr)target, (PLFuncPtr)hkEglSwapBuffers,
            (PLFuncPtr*)&oEglSwapBuffers, PL_HOOK_PRIORITY_DEFAULT);
}
```

### 4.3 ImGui OpenGL ES 初始化

```cpp
void InitImGuiFromGLContext(EGLDisplay dpy, EGLSurface surface) {
    // 1. 创建 ImGui 上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;  // 不写入 .ini 文件

    // 2. 初始化 OpenGL3 后端 (ES 3.0)
    ImGui_ImplOpenGL3_Init("#version 300 es");

    // 3. 加载中文字体
    // 字体文件嵌入在 .so 的 assets 中，或从插件资源目录加载
    std::string fontPath = g_modRootPath + "/resources/NotoSansSC-Regular.ttf";
    io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f, nullptr,
        io.Fonts->GetGlyphRangesChineseFull());

    // 4. 获取屏幕尺寸
    JNIEnv* env = GetJniEnv();
    jclass activityClass = env->FindClass("org/levimc/launcher/core/minecraft/MinecraftActivity");
    // 通过 JNI 获取屏幕宽度和高度
    g_screenWidth = ...;   // 实际渲染 surface 的宽度
    g_screenHeight = ...;  // 实际渲染 surface 的高度

    g_imguiInitialized = true;
}
```

### 4.4 每帧渲染

```cpp
void RenderImGui() {
    // 1. 保存 MC 的 GL 状态
    glUseProgram(0);          // 清除着色器
    glActiveTexture(GL_TEXTURE0);

    // 2. ImGui 帧开始
    ImGui_ImplOpenGL3_NewFrame();

    // ImGui Android 平台后端处理（输入事件已在 TouchInput 中注入）
    // Android 后端需要设置显示尺寸和当前帧的时间
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)g_screenWidth, (float)g_screenHeight);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    io.DeltaTime = g_frameTime;

    ImGui::NewFrame();

    // 3. 渲染 FastRenderer GUI
    // 复用 Core 层逻辑
    GuiService::renderAll();

    // 4. 渲染虚拟按键面板（替代物理键盘）
    renderVirtualKeybindPanel();

    // 5. ImGui 帧结束 + 渲染
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // 6. MC 的 GL 状态通常不需要恢复，ImGui 渲染后 GPU 管线状态会被下一帧覆盖
}
```

### 4.5 GL 状态保存与恢复

Minecraft 对 GL 状态敏感，如果渲染后发现画面异常，需在 ImGui 渲染前后保存/恢复状态：

```cpp
#include <GLES3/gl3.h>

void RenderImGui_Safe() {
    // 保存 MC GL 状态
    GLint last_program, last_texture, last_active_texture;
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);

    GLboolean last_blend = glIsEnabled(GL_BLEND);
    GLboolean last_cull = glIsEnabled(GL_CULL_FACE);
    GLboolean last_depth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_scissor = glIsEnabled(GL_SCISSOR_TEST);

    // --- ImGui 渲染 ---
    ImGui_ImplOpenGL3_NewFrame();
    // ... (同上)
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // 恢复 MC GL 状态
    glUseProgram(last_program);
    glActiveTexture(last_active_texture);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    if (last_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_cull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_depth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_scissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
}
```

---

## 五、触摸输入适配

### 5.1 PreloaderInput 接口

LeviLaunchroid 提供 `PreloaderInput_Interface`，插件可以在 enable() 中获取并注册回调：

```cpp
#include <pl/c/PreloaderInput.h>

// 注册触摸回调
auto* input = GetPreloaderInput();
if (input) {
    input->RegisterTouchCallback([](int action, float x, float y, int pointerId) -> bool {
        return handleTouchEvent(action, x, y, pointerId);
    });

    input->RegisterKeyEventCallback([](int keyCode, int action) -> bool {
        return handleKeyEvent(keyCode, action);
    });
}
```

### 5.2 触摸事件 → ImGui IO 映射

```cpp
// 触摸事件回调
// action: 0=ACTION_DOWN, 1=ACTION_UP, 2=ACTION_MOVE
// x, y: 触摸坐标（归一化或像素坐标）
// pointerId: 触摸点 ID（多指触控支持）

bool handleTouchEvent(int action, float x, float y, int pointerId) {
    if (!g_imguiInitialized) return false;

    ImGuiIO& io = ImGui::GetIO();

    switch (action) {
        case 0:  // ACTION_DOWN
            io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
            io.AddMouseButtonEvent(0, true);   // 左键按下
            io.AddMousePosEvent(x, y);          // 设置鼠标位置
            break;

        case 1:  // ACTION_UP
            io.AddMouseButtonEvent(0, false);  // 左键释放
            break;

        case 2:  // ACTION_MOVE
            io.AddMousePosEvent(x, y);          // 更新鼠标位置
            break;
    }

    // 如果 ImGui 正在交互（如拖动滑块），消费该事件
    if (io.WantCaptureMouse) {
        return true;  // 阻止事件穿透到游戏
    }

    return false;  // 事件传递到游戏
}
```

### 5.3 软键盘支持

Android 没有物理键盘，ImGui 的文本输入需要调用系统软键盘：

```cpp
// 当 ImGui 输入框获得焦点时
if (io.WantTextInput && !g_keyboardShown) {
    g_keyboardShown = true;
    GetPreloaderInput()->ShowKeyboard();  // 弹出系统键盘
}

// 当 ImGui 输入框失去焦点
if (!io.WantTextInput && g_keyboardShown) {
    g_keyboardShown = false;
    GetPreloaderInput()->HideKeyboard();  // 隐藏键盘
}

// 键盘输入通过 key event 回调传入
bool handleKeyEvent(int keyCode, int action) {
    if (!g_imguiInitialized) return false;
    ImGuiIO& io = ImGui::GetIO();

    if (action == 0) {  // ACTION_DOWN
        io.AddKeyEvent(mapAndroidKeyToImGui(keyCode), true);
    } else if (action == 1) {  // ACTION_UP
        io.AddKeyEvent(mapAndroidKeyToImGui(keyCode), false);
    }

    return io.WantCaptureKeyboard;
}
```

---

## 六、Android 专属悬浮菜单

### 6.1 设计思路

Android 用户没有键盘，不能按 F6/F7 来触发按键绑定。但 Server 插件注册的按键**不会自动变成屏幕按钮**——用户通过悬浮菜单自主控制：

```
模组注册按键 ─→ 存入 FRMenuStore（待选列表）
                   │
         ┌─────────┴──────────┐
         ▼                    ▼
   用户打开悬浮菜单      用户忽略（不处理）
   ├─ 看到所有可用按键     → 按键存在但不可见
   ├─ 点击某个按键 → 立即执行
   ├─ 收藏某个按键 → 加到浮动快捷栏
   └─ 配置桥接 IP、连接状态等
```

### 6.2 UI 组件层次

```
┌────────────────────────────────────────────┐
│                游戏画面                      │
│                                            │
│                                            │
│                                            │
│                  [⚙]                       │  ← 悬浮触发器
│                                            │   (可拖动，半透明)
│                                            │
│         (点击 ⚙ 后展开菜单)                  │
│                                            │
│  ┌──────────────────────────────────────┐  │
│  │ FR 安卓菜单                [收起]    │  │
│  ├──────────────────────────────────────┤  │
│  │ [按键] 标签页                        │  │
│  │                                      │  │
│  │  ☐ 打开商店   shop/open_shop    ▶   │  │  ← 点击 ▶ 立即执行
│  │  ☐ 背包       bag/open_bag      ▶   │  │  ← 勾选 ☐ 显示到快捷栏
│  │  ☐ 菜单       menu/toggle       ▶   │  │
│  │  ☐ 地图       map/show          ▶   │  │
│  │  + 暂无更多按键                     │  │
│  ├──────────────────────────────────────┤  │
│  │ [设置] 标签页                        │  │
│  │                                      │  │
│  │ 桥接地址: [192.168.1.100:12345 ]    │  │
│  │                  [连接] [断开]      │  │
│  │ 状态: ● 已连接                       │  │
│  └──────────────────────────────────────┘  │
│                                            │
│  ┌─────────────────────────────────────┐   │
│  │   [打开商店]  [背包]  [菜单]       │   │  ← 浮动快捷栏
│  │   (用户勾选的按键才会出现)           │   │  (可拖拽，可开关)
│  └─────────────────────────────────────┘   │
└────────────────────────────────────────────┘
```

### 6.3 组件定义

| 组件 | 文件 | 说明 |
|------|------|------|
| **FloatingTrigger** | `ui/FloatingTrigger.h` | 半透明悬浮图标，可拖动，点击展开/收起菜单 |
| **FRAndroidMenu** | `ui/FRAndroidMenu.h` | 主菜单窗口，含"按键"和"设置"两个标签页 |
| **FloatingActionBar** | `ui/FloatingActionBar.h` | 底部快捷栏，只显示用户勾选的按键 |
| **FRMenuStore** | `ui/FRMenuStore.h` | 状态管理：可用按键列表、用户收藏、配置 |

### 6.4 FloatingTrigger — 悬浮触发器

- 位置：默认屏幕右侧中间，可拖动到任意位置
- 大小：48×48dp
- 外观：半透明圆形齿轮图标 ⚙
- 交互：
  - 点击：打开/关闭 FRAndroidMenu
  - 长按 + 拖动：改变位置（位置存到 config 持久化）
  - 双击：快速切换 FloatingActionBar 显隐

```cpp
// ui/FloatingTrigger.h
class FloatingTrigger {
public:
    void render();           // 每帧渲染
    bool isOpen() const;     // 菜单是否展开
    void toggle();           // 展开/收起

private:
    ImVec2 m_pos;            // 当前位置（可拖动）
    bool m_open = false;     
    bool m_dragging = false;
    bool m_actionBarVisible = true;
};
```

### 6.5 FRAndroidMenu — 主菜单

菜单分为两个标签页，用 ImGui TabBar 实现：

#### 标签页 1：按键 (Keybinds)

列出所有通过 TCP `keybind_register` 注册的按键绑定（数据来自 `FRMenuStore`）。

每条按键显示：
- **复选框** ☐：勾选后该按键出现在底部 FloatingActionBar
- **名称**：键位名称（如"打开商店"）
- **插件标识**：小字显示 `shop/open_shop`
- **▶ 按钮**：点击立即执行该按键（`KeybindService::execute(bindId)`）

特殊区域：
- 如果当前没有注册任何按键，显示 "暂无按键绑定，请连接到服务器"

#### 标签页 2：设置 (Settings)

| 控件 | 说明 |
|------|------|
| 桥接地址 | 文本输入框，默认 `127.0.0.1:12345` |
| 连接/断开按钮 | 手动控制 TCP 连接 |
| 状态指示灯 | ● 绿色已连接 / ● 红色未连接 |
| 自动重连开关 | 开关，默认开启 |
| 版本信息 | 显示当前 FR Android 插件版本 |
| 恢复默认 | 重置所有配置到默认值 |

### 6.6 FloatingActionBar — 浮动快捷栏

- 位置：屏幕底部（可配置上边/下边）
- 内容：只显示用户在菜单中**勾选**的按键
- 交互：点击按钮直接触发 `KeybindService::execute()`
- 显隐：通过 FloatingTrigger 双击切换，或菜单中设置开关
- 空状态：没有勾选任何按键时不显示

```cpp
// ui/FloatingActionBar.h
class FloatingActionBar {
public:
    void render(const std::vector<MenuItem>& pinnedItems);
    bool isVisible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }
};
```

### 6.7 FRMenuStore — 状态管理

```cpp
// ui/FRMenuStore.h
struct MenuItem {
    std::string pluginId;
    std::string bindId;
    std::string name;
    int vkCode;              // 来自 Server 注册，Android 端忽略
    bool pinned = false;     // 用户是否勾选（显示到快捷栏）
};

class FRMenuStore {
public:
    static FRMenuStore& getInstance();

    // 由 TCP dispatchTcpMessage 调用
    void addOrUpdateItem(const std::string& pluginId,
        const std::string& bindId, const std::string& name, int vkCode);

    void removeItem(const std::string& pluginId, const std::string& bindId);

    // 用户操作
    void togglePin(const std::string& bindId);  // 勾选/取消
    const std::vector<MenuItem>& getAllItems() const;
    std::vector<MenuItem> getPinnedItems() const;  // 只返回勾选的

    // 配置持久化
    void saveConfig(const std::string& host, uint16_t port);
    void loadConfig(std::string& host, uint16_t& port);

private:
    std::vector<MenuItem> m_items;
    std::mutex m_mutex;
};
```

### 6.8 渲染流程

在 `EGLHook.cpp` 的 `RenderImGui()` 中按顺序渲染所有层：

```cpp
void RenderImGui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_screenWidth, g_screenHeight);
    io.DeltaTime = g_frameTime;
    ImGui::NewFrame();

    // 1. 渲染 Server 下发的 GUI（原有的 GuiService）
    GuiService::renderAll();

    // 2. 渲染浮动快捷栏（用户勾选的按键）
    if (g_floatingActionBar.isVisible()) {
        g_floatingActionBar.render(
            FRMenuStore::getInstance().getPinnedItems());
    }

    // 3. 渲染悬浮触发器（始终在最上层）
    g_floatingTrigger.render();

    // 4. 渲染主菜单（只有展开时才渲染）
    if (g_floatingTrigger.isOpen()) {
        g_frMenu.render();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
```

### 6.9 交互序列

```
用户操作                       系统行为
──────────────────────────────────────────────────────
1. 用户进入游戏                FR 自动开始渲染，只有 ⚙ 图标
                               TCP 自动连接（如果有配置）

2. 服务器插件注册按键            TCP 推送 keybind_register
                               FRMenuStore::addOrUpdateItem()
                               菜单中"按键"列表更新
                               快捷栏不变（用户未勾选）

3. 用户点击 ⚙                  展开 FRAndroidMenu
                               看到"按键"列表中有新注册的项

4. 用户勾选"打开商店"           FRMenuStore::togglePin("open_shop")
                               快捷栏自动出现"打开商店"按钮

5. 用户关闭菜单                 快捷栏保留勾选的按钮
                               ⚙ 继续悬浮在屏幕上

6. 用户点击快捷栏"打开商店"      KeybindService::execute("open_shop")
                               触发绑定功能

7. 用户进入设置页               看到桥接地址、连接状态
                               修改地址 → 点击连接 → TCP 重连
```

### 6.10 配置持久化

保存在插件 `data/config.json`：

```json
{
    "trigger_x": 700,
    "trigger_y": 400,
    "action_bar_visible": true,
    "action_bar_position": "bottom",
    "server_host": "play.example.com",
    "server_port": 12345,
    "auto_reconnect": true,
    "pinned_binds": ["open_shop", "open_bag"]
}
```

- `trigger_x/y`：FloatingTrigger 拖动后的位置
- `pinned_binds`：用户勾选了哪些按键（按 bindId 存储）
- 服务器更新插件注册的键位时，已勾选的按键**不会自动取消**

### 6.11 新文件清单

```
FastRenderer-Client-Droid/src/
├── ui/
│   ├── FloatingTrigger.h         ← 悬浮触发器（可拖动图标）
│   ├── FRAndroidMenu.h           ← 主菜单（按键标签页+设置标签页）
│   ├── FloatingActionBar.h       ← 浮动快捷栏（只显示勾选按键）
│   └── FRMenuStore.h             ← 状态管理（按键列表、配置持久化）
```

### 6.12 与旧方案的对比

| 对比项 | 旧方案（自动生成按钮） | 新方案（悬浮菜单） |
|--------|----------------------|-------------------|
| Server 注册按键后 | 自动全部显示在底部 | 只存入列表，**不自动显示** |
| 用户控制权 | 无 | 自主选择哪些按键显示 |
| 桥接配置 | 无界面，改配置文件 | 菜单内直接输入 |
| 菜单入口 | 无 | ⚙ 悬浮图标 |
| 额外功能 | 无 | 连接状态显示、版本信息 |
| 实现复杂度 | 低 | 中等 |

---

## 七、网络通信（TCP 桥）

### 7.1 TCP 客户端

Android 端使用标准 POSIX Socket API 建立 TCP 连接。实现与 Win 端完全相同的 `TcpClient.h`，可建立共享头文件：

```cpp
// TcpClient.h — Win/Android 共用

class FrTcpClient {
public:
    bool connect(const std::string& host, uint16_t port);
    void disconnect();
    bool send(const std::string& jsonMessage);
    void setMessageCallback(std::function<void(const std::string&)> onMessage);
    void startReceiveThread();
    void stop();

private:
    int m_sock = -1;
    std::thread m_recvThread;
    std::atomic<bool> m_running{false};
    std::function<void(const std::string&)> m_onMessage;
};

// receive thread
void FrTcpClient::receiveLoop() {
    std::string buffer;
    char buf[4096];
    while (m_running) {
        int ret = recv(m_sock, buf, sizeof(buf), 0);
        if (ret <= 0) { /* 断开处理 */ break; }
        buffer.append(buf, ret);

        // 按 \n 分割消息
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string msg = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            if (!msg.empty() && m_onMessage) {
                m_onMessage(msg);
            }
        }
    }
}
```

### 7.2 配置地址

在插件的 `config/fr_android.json` 中保存 BDS 地址：

```json
{
    "server_host": "192.168.1.100",
    "server_port": 12345,
    "auto_reconnect": true
}
```

首次启动时配置生成默认文件，用户可通过 ImGui 界面修改并测试连接。

### 7.3 消息处理

```cpp
// TCP 消息回调
void onTcpMessage(const std::string& jsonStr) {
    auto j = nlohmann::json::parse(jsonStr);
    std::string type = j["type"];

    if (type == "gui_register") {
        GuiService::registerGui(
            j["pluginId"].get<std::string>(),
            j["guiId"].get<std::string>(),
            j["definition"].get<std::string>()
        );
    } else if (type == "gui_unregister") {
        GuiService::unregisterGui(
            j["pluginId"].get<std::string>(),
            j["guiId"].get<std::string>()
        );
    } else if (type == "keybind_register") {
        KeybindService::registerKeybind(
            j["pluginId"].get<std::string>(),
            j["bindId"].get<std::string>(),
            j["bindName"].get<std::string>(),
            j["vkCode"].get<int>(),
            nullptr
        );
    } else if (type == "keybind_unregister") {
        KeybindService::unregisterKeybind(
            j["pluginId"].get<std::string>(),
            j["bindId"].get<std::string>()
        );
    } else if (type == "data_exchange") {
        // 触发 channel 回调
        // ...
    }
}
```

---

## 八、代码复用对照

| FastRenderer 组件 | 文件位置 | Android 复用方式 |
|---|---|---|
| **FastRenderer-Core** | `FastRenderer-Core/src/` | **完全复用** — 直接编译进 .so |
| ├─ GuiService | `gui/GuiService.h` | 直接 include，无需修改 |
| ├─ JsonGuiParser | `gui/JsonGuiParser.h` | 直接 include，无需修改 |
| ├─ JsonGuiRenderer | `gui/JsonGuiRenderer.h` | 直接 include，无需修改 |
| ├─ KeybindService | `input/KeybindService.h` | 直接 include，缺少的 VK 码需适配 |
| ├─ ThemeManager | `theme/ThemeManager.h` | 直接 include，无需修改 |
| ├─ HotReloadService | `hotload/HotReloadService.h` | 可复用（文件监控需适配 Android 文件系统） |
| └─ DockLayoutService | `layout/DockLayoutService.h` | 直接 include，无需修改 |
| **FastRenderer-SDK** | `FastRenderer-SDK/include/` | **完全复用** |
| ├─ FastRendererAPI.h | — | 接口定义，纯头文件 |
| ├─ GuiDefinition.h | — | 数据结构定义 |
| └─ IPlatform.h | — | 接口定义，Android 需实现 |
| **FastRenderer-Client-Win** | 适配层代码 | **需重写** |
| ├─ DX11Hook.h | → | 替换为 EGLHook.h |
| ├─ BridgeService.h | → | 替换为 TcpClient.h |
| ├─ InputBlocker.h | → | Android 不需要，或通过触摸事件消费替代 |
| └─ ModEntry.cpp | → | 替换为 PL_REGISTER_MOD 入口 |
| **FastRenderer-Server** | `FastRenderer-Server/src/` | **加 TCP Server，Server 端 LL 插件不改** |

---

## 九、构建环境

### 9.1 NDK CMake 配置

```cmake
cmake_minimum_required(VERSION 3.22)
project(FastRenderer-Android)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 只构建 arm64-v8a
set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)
set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK_HOME})
set(CMAKE_ANDROID_STL_TYPE c++_shared)

# ImGui
add_subdirectory(deps/imgui)

# nlohmann_json
find_package(nlohmann_json REQUIRED)

# GlossHook (LeviLaunchroid 预编译库)
add_library(GlossHook STATIC IMPORTED)
set_target_properties(GlossHook PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libs/arm64/libGlossHook.a
)

# 核心模块
add_library(FastRenderer_Core INTERFACE)
target_include_directories(FastRenderer_Core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../FastRenderer-Core/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../FastRenderer-SDK/include
)

# 插件 SO
add_library(FastRenderer-Android SHARED
    src/ModEntry.cpp
    src/EGLHook.cpp
    src/ImGuiBackend.cpp
    src/TouchInput.cpp
    src/TcpClient.cpp
)

target_link_libraries(FastRenderer-Android
    FastRenderer_Core
    imgui::imgui
    nlohmann_json::nlohmann_json
    GlossHook
    EGL
    GLESv3
)
```

### 9.2 依赖清单

| 依赖 | 来源 | 说明 |
|---|---|---|
| Dear ImGui | v1.92+ (docking 分支) | GUI 渲染库，需启用 `IMGUI_IMPL_OPENGL3` |
| nlohmann_json | v3.11+ | JSON 解析 |
| GlossHook | preloader-android 预编译库 | ARM64 hook 引擎 |
| OpenGL ES 3.0 | 系统库 | Android 内置 |
| LeviLaunchroid SDK | preloader-android 头文件 | `PL_REGISTER_MOD`, `PreloaderInput` 等 |

### 9.3 manifest.json

```json
{
    "type": "preload-native",
    "name": "FastRenderer-Android",
    "entry": "libFastRenderer-Android.so",
    "author": "FastRenderer Team",
    "version": "0.1.0",
    "description": "Minecraft Bedrock Android 渲染框架插件",
    "minecraft_versions": ["1.21.0*"]
}
```

---

## 十、实施路线

### Phase 1：基础框架 (1-2 天)

| 步骤 | 内容 | 产出 |
|---|---|---|
| 1.1 | 搭建 NDK CMake 项目，集成 ImGui 和 nlohmann_json | 编译通过 |
| 1.2 | 实现 PL_REGISTER_MOD 入口，验证生命周期 `load/enable` | 插件被 LeviLaunchroid 加载 |
| 1.3 | GlossHook hook `eglSwapBuffers` | 每帧 hook 被调用 |
| 1.4 | ImGui OpenGL3 后端初始化 + 渲染 demo 窗口 | 屏幕上出现 ImGui 窗口 |

### Phase 2：TCP 通信 (1 天)

| 步骤 | 内容 | 产出 |
|---|---|---|
| 2.1 | 实现 TcpClient (Socket 连接 + 按行读取 + 自动重连) | 能连接到 TCP Server |
| 2.2 | 实现 identify 消息发送 + 消息路由 | 客户端被 Server 识别 |
| 2.3 | 接入 GuiService：TCP 消息 → 注册 GUI | Server 下发的 GUI 在手机上渲染 |

### Phase 3：悬浮菜单 (1.5 天)

| 步骤 | 内容 | 产出 |
|---|---|---|
| 3.1 | 实现 FRMenuStore（按键列表管理、配置持久化） | 按键增删查改 + config.json 读写 |
| 3.2 | 实现 FloatingTrigger（可拖动 ICON、点击展开/收起） | ⚙ 悬浮图标可拖动 |
| 3.3 | 实现 FRAndroidMenu（按键标签页 + 设置标签页） | 菜单两个标签页完整功能 |
| 3.4 | 实现 FloatingActionBar（只显示勾选按键） | 底部快捷栏随勾选更新 |
| 3.5 | 集成：TCP 消息 → FRMenuStore → 菜单/快捷栏联动 | 端到端可用 |
| 3.6 | 手势识别（双击开关快捷栏、长按拖动） | 交互完善 |

### Phase 4：优化与集成 (1 天)

| 步骤 | 内容 | 产出 |
|---|---|---|
| 4.1 | ThemeManager 适配（Android 无系统字体，字体等资源的路径适配） | 主题正常显示 |
| 4.2 | GL 状态保存/恢复稳定 | 不干扰游戏渲染 |
| 4.3 | 性能优化 | 稳定 30+ FPS |
| 4.4 | 端到端测试（Server 插件 → Android 渲染） | 完整链路验证 |

---

## 十一、已知风险

1. **eglSwapBuffers Hook 时机的稳定性**：Minecraft 在加载过程和异常退出时可能调用 eglSwapBuffers 的时机不确定，需要在 hook 函数内做严格的空指针和初始化状态检查

2. **GL 上下文状态冲突**：ImGui 渲染时可能修改 MC 的 GL 状态（着色器、混合模式、视口等），即使保存/恢复也不能 100% 保证兼容所有 MC 版本

3. **ImGui 的 Android 平台后端**：Dear ImGui 官方提供 `imgui_impl_android.h`，但它是为原生 Android 窗口设计的。我们需要在 MC 的现有 GL 上下文上运行，不能直接复用，需要自己管理输入事件和显示尺寸

4. **多点触控冲突**：ImGui 需要消费触摸事件时（用户点击 GUI 按钮），游戏不应再响应这次触摸。需要正确地判断 `io.WantCaptureMouse`

5. **中文输入问题**：Android 的软键盘与 ImGui 的文本输入框集成需要额外处理。通常方案是：
   - 点击 ImGui 输入框时通过 JNI 弹出系统键盘
   - 键盘输入通过 `PreloaderInput` 的回调传入
   - 需要与 LeviLaunchroid 的 `ShowKeyboard()`/`HideKeyboard()` 配合

6. **LeviLaunchroid 版本兼容性**：preloader-android 的 API（如 `PL_REGISTER_MOD`、`PreloaderInput`）可能随版本变化，需要锁定兼容的版本号
7. **悬浮菜单 Z-order**：ImGui 在 MC GL 上下文中渲染，需要确保悬浮触发器始终在 GuiService 渲染的窗口之上。通过在 ImGui 渲染循环中按顺序绘制（先 GuiService → 后悬浮菜单）解决
8. **触摸事件抢断**：悬浮菜单打开时，触摸事件会被 ImGui 消费，不会穿透到 MC。关闭菜单后触摸事件恢复正常。需在 `TouchInput.h` 中正确处理 `io.WantCaptureMouse` 状态