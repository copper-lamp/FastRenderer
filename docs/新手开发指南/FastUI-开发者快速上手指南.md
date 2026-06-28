# FastUI 开发者快速上手指南

> 5 分钟学会用 FastUI 写 UI\
> 适配 v1.1（2026-06-28）

***

## 一、最小 UI

创建一个 `layout.ui` 文件：

```xml
<window label="我的第一个面板" width="300" height="200">
    <text value="你好，世界！" color="#FFFFFF"/>
    <button label="确定" width="80" on_click="btn_ok"/>
</window>
```

把它放在 `FastRenderer/gui_defs/` 目录下，游戏内按 N 键打开验证菜单即可看到。

***

## 二、三步写出实用 UI

### 第 1 步：声明资源

`resources.xml`：

```xml
<resources>
    <texture id="icon_health" path="textures/ui/health.png"/>
    <audio id="click" path="sounds/ui/click.ogg" volume="0.5"/>
</resources>
```

### 第 2 步：写布局

`layout.ui`：

```xml
<window label="玩家状态" width="280" height="180" flags="no_collapse">
    <column spacing="6">
        <row spacing="8">
            <image width="20" height="20" tint="#FF4444"/>
            <text ref="health" color="#FF4444" font_weight="bold"/>
        </row>
        <row spacing="8">
            <image width="20" height="20" tint="#4488FF"/>
            <text ref="mana" color="#4488FF" font_weight="bold"/>
        </row>
        <separator/>
        <row spacing="8">
            <button label="恢复" width="80" color="#4CAF50" on_click="heal_player"/>
            <button label="关闭" width="80" on_click="close_panel"/>
        </row>
    </column>
</window>
```

### 第 3 步：写事件处理

`scripts.xml`：

```xml
<scripts>
    <event id="heal_player" target="server">
        <emit service="player_heal">
            <payload name="amount" value="10"/>
        </emit>
    </event>
    <event id="close_panel" target="local">
        <action type="close_gui"/>
    </event>
</scripts>
```

***

## 三、通信模式选择

### 模式 A：纯本地开发（无 FastRenderer-Server）

FRTest-Native 和 FastRenderer-Client-Win 都在 Minecraft 客户端进程中：

```
FRTest-Native ──TCP:12345──▶ 内嵌 TcpServer (Client 进程)
                               └─ dispatchTcpMessage → GuiService
```

- **不需要**启动 FastRenderer-Server
- FRTest-Native 自动连接内嵌 Server
- 按键绑定（F3/F6/C 等）自动通过 TCP 回调
- 双向通信自动走通

### 模式 B：服务器环境（有 FastRenderer-Server）

```
FRTest-Native ──TCP──▶ FastRenderer-Server ──broadcast──▶ Client-Win
(BDS)                     (BDS)                            (MC 客户端)
```

- 内嵌 Server 端口被占时自动回退到 Client 模式
- 双通道共存：内嵌 Server + 外部 Client 同时可用

***

## 四、新特性速览

### 4.1 全屏模态菜单

```xml
<window label="模组菜单" width="480" height="360"
        fullscreen="true" modal="true">
    <text value="FastRenderer 模组菜单" color="#FFFFFF" font_size="18" font_weight="bold"/>
    <row spacing="6">
        <button label="设置" width="100" color="#FF9800" on_click="open_settings"/>
        <button label="关闭" width="100" color="#F44336" on_click="close_menu"/>
    </row>
</window>
```

`fullscreen="true"` 全屏无边框，`modal="true"` 半透明遮罩。按 F6 测试。

### 4.2 F3 调试叠加层

```xml
<overlay x="12" y="12" showBg="true" bgColor="#00000088">
    <text value="FastRenderer v0.1.0" color="#4FC3F7"/>
    <text value="60 FPS" color="#81C784"/>
    <separator/>
    <text value="坐标: 0, 0, 0" color="#FFFFFF"/>
    <text value="TCP: 已连接" color="#FFCC80"/>
</overlay>
```

无窗口，文字直接画在画面上。按 F3 测试。

### 4.3 计算器 + 双向通信测试

按 `C` 键打开计算器。点击按钮 → TCP gui\_event → FRTest-Native 计算 → TCP gui\_register → 更新显示。按下 PING 按钮测 RTT 延迟。

***

## 五、属性速查（v1.1 新增用 ⭐ 标记）

### 按钮增强

```xml
<!-- 完整风格控制 -->
<button label="提交" width="120" height="36"
        color="#4CAF50"        ⭐ bg_hover="#66BB6A" ⭐ bg_active="#388E3C"
        ⭐ text_color="#FFFFFF" ⭐ border_radius="6" ⭐ font_size="16"
        disabled="true" on_click="submit"/>
```

### 文本增强

```xml
<!-- 带阴影、对齐、字号 -->
<text value="标题" color="#FFFFFF"
      ⭐ font_size="18" ⭐ font_weight="bold"
      ⭐ align="center"
      ⭐ shadow="true" ⭐ shadow_color="#00000080" ⭐ shadow_offset="1,1"/>
```

### 输入框增强

```xml
<!-- 只读显示屏、密码框 -->
<input ref="display" readonly="true" align="right" font_size="20"/>
<input placeholder="密码" password="true" max_length="32"/>
```

### 窗口标志

```xml
<!-- 多个标志逗号分隔 -->
<window label="设置" width="400" height="300"
        ⭐ flags="no_collapse, no_resize, no_move"/>
```

| 标志值                  | 效果     |
| -------------------- | ------ |
| `no_collapse`        | 禁止折叠   |
| `no_scrollbar`       | 隐藏滚动条  |
| `no_resize`          | 禁止缩放   |
| `no_move`            | 禁止移动   |
| `no_title_bar`       | 隐藏标题栏  |
| `always_auto_resize` | 自动适应内容 |

***

## 六、常见的模式

### 可滚动面板

```xml
<panel color="#1E1E24CC" padding="8" round="6"
       ⭐ scrollable="true" height="200">
    <text value="第 1 行"/>
    <text value="第 2 行"/>
    <!-- ... 更多内容 -->
</panel>
```

### 可折叠分组（默认展开）

```xml
<group label="高级设置" collapsible="true" ⭐ default_open="true">
    <checkbox label="启用调试模式" default="false"/>
    <slider label="音量" min="0" max="100" default="70"/>
</group>
```

### 自动换行

```xml
<row spacing="4" ⭐ wrap="true">
    <button label="标签1"/><button label="标签2"/>
    <button label="标签3"/><button label="标签4"/>
    <!-- 超过宽度自动换行 -->
</row>
```

### 图片占位

```xml
<!-- 带染色和圆角的色块占位 -->
<image width="32" height="32" tint="#FF4444" round="6"/>
```

***

## 七、C++ 插件注册示例

```cpp
#include <net/TcpClient.h>
#include <nlohmann/json.hpp>

FrTcpClient g_tcpClient;

bool enable() {
    // 连接（内嵌 / 外部 Server 均可）
    g_tcpClient.connect("127.0.0.1", 12345);
    g_tcpClient.startReceiveThread();

    // 注册 GUI
    json gui = { /* ... */ };
    g_tcpClient.send({{"type","gui_register"}, {"pluginId","my_mod"},
                      {"guiId","my_panel"}, {"definition", gui.dump()}});

    // 注册按键
    g_tcpClient.send({{"type","keybind_register"}, {"pluginId","my_mod"},
                      {"bindId","open"}, {"bindName","打开面板"}, {"vkCode", 0x70}});
    return true;
}
```

### 协议消息

| 消息                   | 用途                 |
| -------------------- | ------------------ |
| `gui_register`       | 注册或更新 GUI          |
| `gui_unregister`     | 删除 GUI             |
| `keybind_register`   | 注册按键（按下自动发 TCP 事件） |
| `keybind_unregister` | 删除按键               |
| `gui_event`          | 按钮点按、GUI 事件        |
| `data_exchange`      | 自定义双向数据            |

***

## 八、常见问题

**Q: UI 闪烁一下消失？**\
A: 这是旧版本的 HotReload bug，已修复。升级到最新版即可。

**Q: 内嵌 Server 启动失败？**\
A: 端口 12345 被占用（外部 Server 运行中），会自动尝试 12346\~12350。

**Q: 按键绑定不生效？**\
A: 按键按下会通过 TCP 发送 `gui_event(guiId="keybind", controlId=bindId)`，需要 FRTest-Native 端处理该事件。

**Q: 如何调试 UI？**\
A: 查看日志文件：`FastRenderer_GS.txt`（GuiService）、`FastRenderer_HR.txt`（HotReload）、`FRTest.log`（插件端）。

**Q:** **`font_size`** **不生效？**\
A: 使用 ImGui 的 `SetWindowFontScale` 实现，会缩放整个窗口的字号，不影响其他窗口。

**Q: 如何更新动态文本？**\
A: C++ 端：`DynamicText::values["health"] = "80";`

***

## 九、版本记录

| 版本   | 日期         | 变更                                                                            |
| ---- | ---------- | ----------------------------------------------------------------------------- |
| v1.0 | 2026-06-21 | 初版                                                                            |
| v1.1 | 2026-06-28 | +内嵌 Server 架构，+20+ 新属性，+overlay/fullscreen/modal/image 节点，+计算器测试，+按键绑定 TCP 回调 |

