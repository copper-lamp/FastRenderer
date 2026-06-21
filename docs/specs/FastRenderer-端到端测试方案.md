# FastRenderer 端到端测试方案

> 版本：v2.0
> 日期：2026-06-21
> 状态：草案

---

## 一、测试目标

验证以下调用路径在**实际游戏环境中**是否正常工作：

| 路径 | 调用方 | 传输方式 |
|------|--------|---------|
| **A-文件桥本地** | 客户端原生 LL3 插件 | 文件桥（写入 gui_pending.json） |
| **B-文件桥跨语言** | JS 脚本插件 (LSE) | 文件桥（写入 gui_pending.json） |
| **C-服务端远程** | 服务端原生 LL3 插件 | Minecraft 自定义 Packet |

---

## 二、整体架构

```
┌──────────────────────────────────────────────────────────┐
│                   Minecraft Bedrock 客户端                 │
│                                                          │
│  ┌──────────────┐  写入 JSON    ┌────────────────────┐   │
│  │ FRTest-Native ├─────────────►│  gui_bridge/        │   │
│  │ (原生 LL3)    │              │  gui_pending.json   │   │
│  └──────────────┘              └────────┬───────────┘   │
│                                         │               │
│  ┌──────────────┐  写入 JSON    ┌───────▼───────────┐   │
│  │ FRTest-LSE   ├─────────────►│  BridgeService     │   │
│  │ (JS, LSE)    │              │  (200ms 轮询)      │   │
│  └──────────────┘              └───────┬───────────┘   │
│                                        │               │
│                               ┌────────▼──────────┐    │
│                               │  GuiService       │    │
│                               │  KeybindService   │    │
│                               └───────────────────┘    │
│                                                         │
│  ┌──────────────────────────────────────────────────┐   │
│  │  PacketBridge ← 接收 Minecraft 自定义 Packet     │   │
│  └──────────────────────────────────────────────────┘   │
│                           ▲                              │
└───────────────────────────┼──────────────────────────────┘
                            │ Minecraft 自定义 Packet
                            ▼
┌──────────────────────────────────────────────────────────┐
│                    BDS 服务端 (可选)                       │
│  ┌────────────────────────┐  ┌─────────────────────────┐ │
│  │ ServerPlugin           │  │ FastRenderer-Server     │ │
│  │ (服务端原生 LL3)       │─►│ → Packet → Client       │ │
│  └────────────────────────┘  └─────────────────────────┘ │
└──────────────────────────────────────────────────────────┘
```

---

## 三、测试场景

### 场景 A：文件桥 — 原生插件

**角色**：FRTest-Native（原生 LL3 插件）

**测试步骤**：
1. FRTest-Native 在 `load()` 阶段写入 `gui_pending.json`（批量数组格式）
2. BridgeService 轮询读取，解析 JSON 数组
3. 调用 `GuiService::registerGui()` 注册所有 GUI
4. 注册按键绑定（写入 `keybind_pending.json`）
5. 验证 GUI 在游戏内正确渲染
6. 验证按键绑定生效

**验证指标**：
- BridgeService 日志显示 `batch: N entries`（N ≥ 6）
- `GuiService` 包含对应数量的定义
- 游戏内可见对应窗口

### 场景 B：文件桥 — JS 插件

**角色**：FRTest-LSE（LegacyScriptEngine nodejs 插件）

**测试步骤**：
1. FRTest-LSE 在 `plugins/` 目录加载后写入 `gui_pending.json`
2. BridgeService 轮询读取并注册
3. 验证 JS 插件注册的 GUI 在游戏内显示

**验证指标**：
- FRTest_LSE.txt 日志显示写入成功
- BridgeService 日志显示 `batch: N entries`
- 游戏内可见 "FRTest LSE" 相关窗口

### 场景 C：服务端远程

**角色**：FastRenderer-Server + 服务端测试插件

**测试步骤**：
1. 服务端测试插件调用 `ServerPluginApi::registerGui()`
2. 发送 `GuiRegistrationPacket` 到客户端
3. 客户端 `PacketBridge` 接收并注入 `GuiService`
4. 客户端渲染 GUI

**验证指标**：
- 服务端注册 → 客户端 GUI 显示
- 客户端点击 → 服务端事件回调触发

---

## 四、桥接文件格式

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
| FastRenderer-Client | `bin/FastRenderer/` | BDS `Mods/FastRenderer/` |
| FRTest-Native | `bin/FRTest-Native/` | BDS `Mods/FRTest-Native/` |
| FRTest-LSE | `FRTest-LSE/` | BDS `plugins/FRTest-LSE/` |

加载顺序：
```
1. FastRenderer-Client.dll  ← 前置框架（启动 BridgeService）
2. FRTest-Native.dll        ← 测试插件（写入桥接文件）
3. FRTest-LSE               ← JS 测试插件（写入桥接文件）
```

---

## 六、日志检查

| 日志文件 | 内容 | 关键信息 |
|---------|------|---------|
| `FastRenderer_Init.txt` | FastRenderer 初始化过程 | BridgeService init OK |
| `FastRenderer_Bridge.txt` | BridgeService 轮询日志 | `batch: N entries` |
| `FRTest_Result.txt` | FRTest-Native 测试日志 | `wrote gui_pending.json` |
| `FRTest_LSE.txt` | FRTest-LSE 测试日志 | `Wrote gui_pending.json` |

---

## 七、验证通过标准

| 检查项 | 标准 |
|--------|------|
| 场景 A - BridgeService 启动 | 日志显示 `bridgeLoop: started` |
| 场景 A - 批量数组注册 | 日志显示 `batch: 6 entries` |
| 场景 A - 注册成功 | GuiService 包含 ≥6 个定义 |
| 场景 A - GUI 可见 | 游戏内显示注册的窗口 |
| 场景 B - JS 写入 | FRTest_LSE.txt 显示写入成功 |
| 场景 B - BridgeService 处理 | 日志显示 `batch: N entries` |
| 场景 C - 服务端推送 | 客户端收到并渲染 |
