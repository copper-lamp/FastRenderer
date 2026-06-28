# FastRenderer 端到端测试方案

> 版本：v2.1
> 日期：2026-06-28
> 状态：草案

---

## 一、测试目标

验证以下调用路径在**实际游戏环境中**是否正常工作：

| 路径 | 调用方 | 传输方式 |
|------|--------|---------|
| **A-文件桥本地** | 客户端原生 LL3 插件 | 文件桥（写入 gui_pending.json） |
| **B-文件桥跨语言** | JS 脚本插件 (LSE) | 文件桥（写入 gui_pending.json） |
| **C-TCP 桥远程** | 服务端原生 LL3 插件 | TCP 桥（内嵌在 Server 中） |
| **D-Android 远程** | 服务端原生 LL3 插件 | TCP 桥（Android TcpClient 接收） |

---

## 二、整体架构

```
┌──────────────────────────────────────────────────────────┐
│              Minecraft Bedrock 客户端 / Android 设备       │
│                                                          │
│  ┌──────────────┐  写入 JSON    ┌────────────────────┐   │
│  │ FRTest-Native ├─────────────►│  gui_bridge/        │   │
│  │ (原生 LL3)    │              │  gui_pending.json   │   │
│  └──────────────┘              └────────┬───────────┘   │
│                                         │               │
│  ┌──────────────┐  写入 JSON    ┌───────▼───────────┐   │
│  │ FRTest-LSE   ├─────────────►│  文件桥接 (已废弃) │   │
│  │ (JS, LSE)    │              │  → 建议迁移至 TCP │   │
│  └──────────────┘              └───────┬───────────┘   │
│                                        │               │
│                               ┌────────▼──────────┐    │
│                               │  GuiService       │    │
│                               │  KeybindService   │    │
│                               └───────────────────┘    │
│                                                         │
│  ┌──────────────────────────────────────────────────┐   │
│  │  TcpClient ← TCP 桥统一接收 (替代 PacketBridge) │   │
│  └──────────────────────────────────────────────────┘   │
│                           ▲                              │
└───────────────────────────┼──────────────────────────────┘
                            │ TCP 桥 (JSON over TCP)
                            ▼
┌──────────────────────────────────────────────────────────┐
│                    BDS 服务端 (可选)                       │
│  ┌────────────────────────┐  ┌─────────────────────────┐ │
│  │ ServerPlugin           │  │ FastRenderer-Server     │ │
│  │ (服务端原生 LL3)       │─►│ → TcpServer :12345      │ │
│  └────────────────────────┘  │ → 广播/定向 → TcpClient │ │
│                              └─────────────────────────┘ │
└──────────────────────────────────────────────────────────┘
```

---

## 三、测试场景

### ~~场景 A / 场景 B：文件桥 — 已废弃~~

> **注意**：BridgeService 文件桥接协议已被 TCP 桥接协议替代，以下测试场景仅作历史参考。
> 新插件请直接使用 TCP 桥（TcpClient/TcpServer）注册 GUI。
> 原 BridgeService 代码已归档至 `archive/deprecated/BridgeService.h`。

### 场景 C：TCP 桥远程（Win 客户端）

**角色**：FastRenderer-Server（内嵌 TcpServer）+ 服务端测试插件 + Win TcpClient

**测试步骤**：
1. 服务端测试插件调用 `ServerPluginApi::registerGui()`
2. ServerPluginApi 内部通过 TcpServer 遍历所有活跃 TCP 连接，发送 `gui_register` 消息
3. Win 客户端 TcpClient 接收消息，注入 `GuiService`
4. 客户端渲染 GUI，用户点击按钮
5. TcpClient 发送 `gui_event` 消息到 Server
6. Server 端触发插件注册的 `onGuiEvent` 回调

**验证指标**：
- Server 端注册 → Win 客户端 GUI 显示
- 客户端点击 → 服务端事件回调触发
- 日志显示 TCP 消息收发成功

### 场景 D：TCP 桥远程（Android 客户端）

**角色**：FastRenderer-Server（内嵌 TcpServer）+ 服务端测试插件 + Android TcpClient

**测试步骤**：
1. 服务端测试插件调用 `registerGui()`（与场景 C 同样的代码）
2. TcpServer 广播 `gui_register` 消息到所有已连接的 Android 客户端
3. Android 端 TcpClient 接收 → EGLHook → ImGui 渲染
4. 用户触摸点击 ImGui 按钮
5. Android 端发送 `gui_event` 到 Server
6. Server 端触发 `onGuiEvent` 回调

**验证指标**：
- Server 端注册 → Android 手机屏幕显示 GUI
- 触摸操作正常触发 ImGui 控件
- 事件正确回传到 Server 端插件
- 虚拟按键面板正常显示并响应

---

## 四、桥接文件格式（已废弃）

> **注意**：以下文件格式基于已废弃的 BridgeService 文件桥接协议，仅作历史参考。
> 当前使用 TCP 桥接协议，详见 [FR-TCP桥接通信协议.md](FR-TCP桥接通信协议.md)。
> 原代码已归档至 `archive/deprecated/BridgeService.h`。

### gui_pending.json (批量数组格式)

```json
[
    {
        "pluginId": "FRTest-Native",
        "guiId": "bridge_test_gui",
        "definition": "{\"type\":\"window\",\"props\":{\"title\":\"...\"}..."
    },
    {
        "pluginId": "FRTest-Native",
        "guiId": "bridge_dashboard",
        "definition": "..."
    }
]
```

支持 calculator_backend 兼容格式：

```json
[
    {
        "pluginId": "FRTest-LSE",
        "guiId": "lse_dashboard",
        "root": {
            "type": "window",
            "props": { "title": "仪表盘" },
            "children": [...]
        }
    }
]
```

### keybind_pending.json

```json
[
    {
        "pluginId": "FRTest-Native",
        "bindId": "bridge_key_f8",
        "name": "F8 Bridge",
        "vk": 119
    }
]
```

---

## 五、部署说明

| 组件 | 源路径 | 部署路径 |
|------|--------|---------|
| FastRenderer-Client (Win) | `bin/FastRenderer/` | BDS `Mods/FastRenderer/` |
| FRTest-Native | `bin/FRTest-Native/` | BDS `Mods/FRTest-Native/` |
| FRTest-LSE | `FRTest-LSE/` | BDS `plugins/FRTest-LSE/` |
| FastRenderer-Server | `bin/FastRenderer-Server/` | BDS `Mods/FastRenderer-Server/` |
| FastRenderer-Android | `FastRenderer-Client-Droid/build/libs/FastRenderer-Android.so` | LeviLaunchroid `mods/FastRenderer-Android/` |

加载顺序（Win 端）：
```
1. FastRenderer-Client.dll  ← 前置框架（启动 TcpClient）
2. FRTest-Native.dll        ← 测试插件（通过 TCP 桥注册 GUI）
3. FRTest-LSE               ← JS 测试插件（写入桥接文件）
```

加载顺序（Android 端）：
```
1. LeviLaunchroid 启动 → 自动加载 mods/*/lib*.so
2. FastRenderer-Android.so → PLMod_Load → PLMod_Enable
   → hook eglSwapBuffers → 连接 TcpServer → 接收 GUI
```

## 六、日志检查

| 日志文件 | 内容 | 关键信息 |
|---------|------|---------|
| `FastRenderer_Init.txt` | FastRenderer 初始化过程 | TcpClient connected OK |
| `FastRenderer_Bridge.txt` | ~~BridgeService 轮询日志~~ | ~~已废弃~~ |
| `FRTest_Result.txt` | FRTest-Native 测试日志 | `wrote gui_pending.json` |
| `FRTest_LSE.txt` | FRTest-LSE 测试日志 | `Wrote gui_pending.json` |

---

## 七、验证通过标准

| 检查项 | 标准 |
|--------|------|
| ~~场景 A/B~~ (文件桥已废弃) | ~~已由 TCP 桥替代~~ | ~~原代码归档于 archive/deprecated/~~ |
| 场景 C - TCP 桥 (Win) | Win 客户端收到并渲染 Server 推送的 GUI |
| 场景 C - TCP 事件回传 | Server 收到客户端 `gui_event` |
| 场景 D - TCP 桥 (Android) | Android 手机屏幕渲染 Server 推送的 GUI |
| 场景 D - 触摸交互 | 触摸按钮触发 `gui_event` 回传 Server |
| 场景 D - 虚拟按键 | 虚拟按键面板显示并响应触摸 |
