# FastRenderer TCP 桥接通信协议

> 版本：v1.0
> 日期：2026-06-28
> 状态：草案

---

## 一、概述

TCP 桥接协议是 FastRenderer 的统一跨进程通信协议，用于替代原有的 Minecraft 自定义网络包。该协议解决以下问题：

- **统一通信通道**：Win 客户端和 Android 客户端使用完全相同的协议
- **降低耦合**：不再强依赖 LeviLamina 的 Packet 接口
- **跨平台支持**：从 Windows 扩展到 Android 不需要额外的协议适配层

### 协议设计原则

- **JSON over TCP**：载荷全部使用 JSON 文本，方便调试和扩展
- **双向通信**：Server 和 Client 均可主动发送消息
- **广播 + 定向**：支持发给所有客户端和指定玩家
- **无锁异步**：使用独立线程收发，不阻塞主线程

---

## 二、网络架构

### 2.1 拓扑结构

```
┌──────────────────────────────────────────────────┐
│ BDS 服务器 (LeviLamina)                           │
│                                                  │
│  ┌────────────────────────────────────┐          │
│  │ FR-Server (LeviLamina mod)         │          │
│  │                                    │          │
│  │  ┌──────────────┐  ┌────────────┐ │          │
│  │  │ ServerPlugin │  │ TCP Server │ │          │
│  │  │ Api          │  │ :12345     │ │          │
│  │  └──────┬───────┘  └─────┬──────┘ │          │
│  └─────────┼────────────────┼────────┘          │
└────────────┼────────────────┼───────────────────┘
             │                │
             │ 同进程         │ TCP (JSON)
             │ C++ 直接调用   │
             │                │
             ▼                ▼
     ┌──────────────┐  ┌──────────────┐
     │ 其他 LL 插件  │  │  TCP 客户端   │
     │ (同进程)      │  │              │
     └──────────────┘  │ Win / Android│
                        └──────────────┘
```

### 2.2 通信模式

| 模式 | 方向 | 说明 |
|---|---|---|
| **同进程调用** | Server 插件 → FR-Server API | 直接 C++ 函数调用，不走 TCP |
| **Server → Client** | FR-Server → TCP 客户端 | 广播或定向推送 GUI 定义、数据等 |
| **Client → Server** | TCP 客户端 → FR-Server | 上报 GUI 事件、数据交换 |
| **Client → Client** | (暂不支持) | 如需跨客户端通信需经 Server 中转 |

### 2.3 连接生命周期

```
Client 端                          Server 端
  │                                   │
  │  1. TCP 连接 :12345               │
  │  ──────────────────────────────→  │  accept()
  │                                   │
  │  2. 发送身份标识                   │
  │  {"type":"identify",              │
  │   "player":"Steve",               │  ← 记录 player→conn 映射
  │   "platform":"win"}               │
  │  ──────────────────────────────→  │
  │                                   │
  │  3. 正常通信（双向）               │
  │  ← 服务器推送 GUI 定义 ───────── │
  │  ─ 客户端上报事件 ──────────────→ │
  │  ← 服务器推送数据交换 ────────── │
  │  ─ 客户端数据交换 ─────────────→ │
  │                                   │
  │  4. 断开或心跳超时                 │
  │  ────────── TCP 断开 ──────────→ │  清理 player→conn 映射
```

---

## 三、消息协议

### 3.1 消息格式

所有消息均为 **一行 JSON**（以 `\n` 作为分隔符，便于按行读取）：

```json
{"type":"<message_type>", ...payload...}
```

### 3.2 消息类型列表

| 类型 (type) | 方向 | 用途 | 关键字段 |
|---|---|---|---|
| `identify` | Client→Server | 客户端身份标识 | `player`, `platform` |
| `gui_register` | Server→Client | 注册/更新 GUI 定义 | `pluginId`, `guiId`, `definition`, `targetPlayer` (可选) |
| `gui_unregister` | Server→Client | 卸载 GUI | `pluginId`, `guiId`, `targetPlayer` (可选) |
| `keybind_register` | Server→Client | 注册按键绑定 | `pluginId`, `bindId`, `bindName`, `vkCode`, `targetPlayer` (可选) |
| `keybind_unregister` | Server→Client | 卸载按键绑定 | `pluginId`, `bindId`, `targetPlayer` (可选) |
| `gui_event` | Client→Server | GUI 交互事件上报 | `guiId`, `controlId`, `eventType`, `value` |
| `data_exchange` | 双向 | 通用数据交换 | `channel`, `data`, `targetPlayer` (可选) |
| `pong` | 双向 | 心跳回复 | — |

### 3.3 消息详情

#### 3.3.1 identify — 身份标识

```
方向: Client → Server（连接后第一条消息）
用途: 告知 Server 当前客户端的玩家名和平台类型，用于消息定向投递
```

```json
{
    "type": "identify",
    "player": "Steve",
    "platform": "win"
}
```

- `player`: 游戏中的玩家名，用于 Server 插件定向推送
- `platform`: `"win"` 或 `"android"`，用于后续平台特定功能

如果不发送 identify，Server 认为该连接为"匿名客户端"，只能接收广播消息。

#### 3.3.2 gui_register — GUI 注册

```
方向: Server → Client
用途: 向指定客户端（或广播所有客户端）注册一个 GUI 定义
```

```json
{
    "type": "gui_register",
    "pluginId": "shop",
    "guiId": "main_panel",
    "definition": "{\"root\":{\"type\":\"window\",\"props\":{\"title\":\"商店\"},\"children\":[...]}}",
    "version": 1,
    "targetPlayer": ""
}
```

- `definition`: 完整的 GUI JSON 定义（字符串），与 BridgeService 的 `gui_pending.json` 格式一致
- `targetPlayer`: 空字符串表示广播给所有客户端；指定玩家名则只发送给对应客户端
- `version`: 用于客户端去重，相同 `pluginId` + `guiId` + `version` 不重复处理

#### 3.3.3 gui_unregister — GUI 卸载

```
方向: Server → Client
用途: 卸载指定 GUI
```

```json
{
    "type": "gui_unregister",
    "pluginId": "shop",
    "guiId": "main_panel",
    "targetPlayer": ""
}
```

#### 3.3.4 keybind_register — 按键绑定注册

```
方向: Server → Client
用途: 注册一个按键绑定
```

```json
{
    "type": "keybind_register",
    "pluginId": "shop",
    "bindId": "open_shop",
    "bindName": "打开商店",
    "vkCode": 79,
    "targetPlayer": ""
}
```

- `vkCode`: 虚拟键码（Win 端用 Windows VK_ 常量，Android 端映射为手势 ID 或虚拟按钮索引）

#### 3.3.5 keybind_unregister — 按键绑定卸载

```
方向: Server → Client
用途: 卸载指定按键绑定
```

```json
{
    "type": "keybind_unregister",
    "pluginId": "shop",
    "bindId": "open_shop",
    "targetPlayer": ""
}
```

#### 3.3.6 gui_event — GUI 事件上报

```
方向: Client → Server
用途: 客户端用户在 GUI 上操作时，上报事件到 Server
```

```json
{
    "type": "gui_event",
    "guiId": "main_panel",
    "controlId": "buy_btn",
    "eventType": "click",
    "value": ""
}
```

- `eventType`: `"click"`（按钮点击）、`"change"`（滑块/复选框变化）、`"input"`（文本输入）
- `value`: 事件伴随值（滑块数值、勾选状态等）

Server 端收到后转发给对应注册了 `onGuiEvent` 的插件。

#### 3.3.7 data_exchange — 通用数据交换

```
方向: 双向
用途: 任意自定义数据通道，不限于 GUI
```

```json
{
    "type": "data_exchange",
    "channel": "player_info",
    "data": "{\"health\":20,\"hunger\":18}",
    "targetPlayer": ""
}
```

- `channel`: 数据通道名，Server 插件通过 `subscribe(channel)` 监听
- `data`: 任意 JSON 字符串

从 **Client → Server** 时，`targetPlayer` 固定为空（客户端不必指定目标）。
从 **Server → Client** 时，`targetPlayer` 控制是广播还是定向。

### 3.4 心跳机制

- Server 每 30 秒检测 TCP 连接存活状态
- 客户端可选择性发送 `{"type":"pong"}` 保持连接
- Server 在 60 秒内未收到任何消息即主动断开连接

---

## 四、通信流程

### 4.1 Server 端注册 GUI

```
Server 端 LL 插件调用 IFastRendererAPI::registerGui()
  │
  ▼
FR-Server::ServerFRPluginApi::registerGui()
  │
  ├─ (可选) 如果指定了 targetPlayer：
  │   查找 player→conn 映射
  │   向对应 TCP 连接发送 gui_register 消息
  │
  └─ (默认) 如果 targetPlayer 为空：
      遍历所有活跃 TCP 连接
      向每个连接发送 gui_register 消息
```

### 4.2 Client 端接收 GUI

```
TCP 接收线程收到消息
  │
  ├─ 解析 JSON
  │
  ├─ type == "gui_register"
  │   → GuiService::registerGui(pluginId, guiId, definition)
  │   → 下一帧 ImGui 渲染
  │
  ├─ type == "gui_unregister"
  │   → GuiService::unregisterGui(pluginId, guiId)
  │
  ├─ type == "keybind_register"
  │   → KeybindService::registerKeybind(...)
  │
  ├─ type == "keybind_unregister"
  │   → KeybindService::unregisterKeybind(...)
  │
  └─ type == "data_exchange"
      → 调用本地注册的 channel 回调
```

### 4.3 Client 端上报事件

```
用户在 ImGui 界面上点击按钮
  │
  ▼
JsonGuiRenderer::fireEvent("buy_btn")
  │
  ▼
(本地回调执行完成)
  │
  ▼
TCP 发送线程：
  {"type":"gui_event","guiId":"main_panel",
   "controlId":"buy_btn","eventType":"click","value":""}
  │
  ▼
Server 端收到
  │
  ▼
FR-Server::dispatchGuiEvent()
  → 调用插件注册的 onGuiEvent 回调
```

---

## 五、数据流对照

### 5.1 与原有 MC 自定义 Packet 的映射

| 原 MC Packet | 方向 | 现 TCP 消息 |
|---|---|---|
| `GuiRegistrationPacket` | Server→Client | `gui_register` |
| `GuiUnregistrationPacket` | Server→Client | `gui_unregister` |
| `KeybindRegistrationPacket` | Server→Client | `keybind_register` |
| `KeybindUnregistrationPacket` | Server→Client | `keybind_unregister` |
| `GuiEventPacket` | Client→Server | `gui_event` |
| `DataExchangePacket` | 双向 | `data_exchange` |

### 5.2 语义一致性

Server 端插件调用方式不变：

```cpp
// 改造前（发送 MC 网络包）
FRAPI->registerGui("shop", "main", definition);

// 改造后（发送 TCP 消息）
// 接口不变，底层实现从 sendBroadcast() 改为 TCP Server 广播
FRAPI->registerGui("shop", "main", definition);
```

**Server 端插件不需要修改任何代码**，`IFastRendererAPI` 接口不变，只是底层实现从 MC 网络包改为 TCP。

---

## 六、错误处理

### 6.1 连接断开

- Client 端检测到 TCP 断开后，自动进入**重连循环**（3 秒间隔，最多重试 5 次）
- 重连成功后自动重新发送 `identify` 消息
- Server 端检测到连接断开，清理该客户端的 player→conn 映射

### 6.2 消息格式错误

- 接收方解析 JSON 失败时，忽略该消息并记录日志（不中断连接）
- 未知 `type` 的消息同样忽略

### 6.3 粘包处理

- 使用 `\n` 作为消息分隔符
- TCP 接收缓冲区按行读取，每行是一条完整的 JSON 消息
- 不完整的行缓存在接收缓冲区中，等待后续数据到达

---

## 七、性能考虑

| 指标 | 预期 |
|---|---|
| 单条消息延迟 | < 1ms（同机回环） / < 10ms（局域网） |
| Server 并发连接数 | 支持 50+ 客户端 |
| 消息吞吐量 | 每秒 1000+ 条 JSON 消息（单条 < 4KB） |
| 内存占用 | 每个连接约 64KB 接收缓冲区 |

---

## 八、安全

- TCP 端口默认绑定 `127.0.0.1`（仅本机访问），改为 `0.0.0.0` 允许外部连接
- 当前版本**不支持传输加密**（运行在可信网络环境中）
- 未来版本可增加 TLS 或简单的 AES 加密

---

## 九、接口变更清单

### 9.1 删除的文件

| 文件 | 原因 |
|---|---|
| `FastRenderer-Server/src/FRPackets.h` | 6 个 MC 自定义 Packet 类不再需要 |
| `FastRenderer-Client-Win/src/PacketBridge.h` | MC 自定义包接收端不再需要 |

### 9.2 新增的文件

| 文件 | 说明 |
|---|---|
| `FastRenderer-Server/src/TcpServer.h` | TCP Server 实现，管理客户端连接 + 消息广播/定向 |
| `FastRenderer-Client-Win/src/TcpClient.h` | TCP 客户端，连接 Server + 收发消息 + 自动重连 |
| `FastRenderer-Client-Droid/src/TcpClient.h` | Android TCP 客户端（与 Win 端完全相同，可复用） |

### 9.3 修改的文件

| 文件 | 变化 |
|---|---|
| `FastRenderer-Server/src/ServerPluginApi.h` | `registerGui()` 等方法的实现从 `sendBroadcast()` 改为 TCP Server 广播 |
| `FastRenderer-Server/src/ModEntry.cpp` | 移除 6 个 Packet 注册，新增 TCP Server 启动 |
| `FastRenderer-Client-Win/src/ModEntry.cpp` | 移除 PacketBridge 初始化，新增 TCP Client 连接 |
| `FastRenderer-Client-Win/xmake.lua` | 不再需要 LeviLamina packet 相关依赖 |