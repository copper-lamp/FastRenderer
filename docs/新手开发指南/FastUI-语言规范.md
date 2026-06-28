# FastUI 语言规范

> 版本：v1.1  
> 日期：2026-06-28  
> 覆盖：16 种节点类型，80+ 属性  
> 渲染器：`JsonGuiRenderer.h`

---

## 一、概述

FastUI 是 FastRenderer 框架的声明式 UI 语言。插件开发者通过 XML 风格的标记语言定义界面布局，编译为 JSON 后由 FastRenderer 客户端使用 ImGui 渲染。

### 核心理念

- **XML 风格语法**：类似 HTML，标签清晰、嵌套直观
- **多文件拆分**：布局、资源、样式、事件分文件管理
- **声明式绑定**：通过 `ref` 和模板表达式实现数据与 UI 的自动同步
- **可扩展设计**：支持全屏菜单、F3 调试叠加、计算器等 16 种节点类型

### 文件组织

```
my_ui_pack/
├── layout.ui           ← 布局定义（唯一必需）
├── resources.xml       ← 资源声明（贴图/音频）
├── styles.xml          ← 样式定义（可选）
├── scripts.xml         ← 事件脚本（可选）
└── templates/          ← 组件模板（可选）
    └── *.xml
```

---

## 二、通信架构

### 2.1 生产模式（有 FastRenderer-Server）

```
FRTest-Native ──TCP──▶ FastRenderer-Server ──broadcast──▶ FastRenderer-Client
(BDS 端)                                                      (MC 客户端)
```

### 2.2 纯本地开发模式（无 Server）

```
FRTest-Native ──TCP──▶ 内嵌 TcpServer (Client 进程中) ──dispatch──▶ GuiService
(同进程)                端口 12345 / 自动回退 12346~12350
```

双通道共存：
- **通道A**：内嵌 TCP Server — FRTest-Native 直连，无需外部 Server
- **通道B**：TCP Client — 连接外部 FastRenderer-Server

---

## 三、节点类型完整列表（16 种）

### 3.1 容器节点

| 标签 | 说明 | 特有属性 |
|------|------|---------|
| `<window>` | 顶层窗口 | title, width, height, fullscreen, modal, flags, bg_color, border, border_radius |
| `<row>` | 水平布局（SameLine） | spacing, wrap |
| `<column>` | 垂直布局 | spacing |
| `<group>` | 编组/可折叠 | label, collapsible, default_open, indent |
| `<panel>` | 带背景子窗口 | color, padding, round, border_color, border_width, scrollable |

### 3.2 基础控件

| 标签 | 说明 | 特有属性 |
|------|------|---------|
| `<button>` | 按钮 | text, color, bg_hover, bg_active, text_color, width, height, disabled, border_radius, font_size, onClick |
| `<text>` | 文本标签 | value, color, ref, font_size, font_weight, align, shadow, shadow_color, shadow_offset |
| `<separator>` | 分隔线 | （无属性） |
| `<checkbox>` | 复选框 | label, default, disabled, color, onChange |
| `<slider>` | 滑块 | label, min, max, default, disabled, onChange |
| `<input>` | 输入框 | placeholder, default, width, readonly, password, max_length, align, font_size |
| `<combo>` | 下拉选择 | label, options, default, disabled |
| `<spacer>` | 空白占位 | width, height |
| `<progress>` | 进度条 | value, max, label, width, height |
| `<image>` | 图片占位 | texture（预留）, width, height, tint, round |

### 3.3 特殊节点

| 标签 | 说明 | 特有属性 |
|------|------|---------|
| `<overlay>` | F3 调试文字叠加（无窗口） | x, y, showBg, bgColor |
| `<canvas>` | 自定义画布（预留） | width, height |

---

## 四、完整属性参考

### 4.1 `window` 窗口

```xml
<window label="标题" width="400" height="300" flags="no_collapse, no_resize"
        fullscreen="true" modal="true" bg_color="#1E1E24E0"
        border="true" border_radius="8">
```

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `label` / `title` | string | `"窗口"` | 窗口标题 |
| `width` | int | `400` | 宽度（px） |
| `height` | int | `300` | 高度（px） |
| `fullscreen` | bool | `false` | 全屏无边框 |
| `modal` | bool | `false` | 半透明黑色遮罩 |
| `flags` | string | `""` | 逗号分隔的标志：
  `no_collapse`, `no_scrollbar`, `no_resize`, `no_move`,
  `no_title_bar`, `always_auto_resize`, `no_saved_settings` |
| `bg_color` | hex | — | 窗口背景色 |
| `border` | bool | `true` | 显示边框 |
| `border_radius` | float | `0` | 圆角 |

### 4.2 `button` 按钮

```xml
<button label="提交" width="120" height="36" color="#4CAF50"
        bg_hover="#66BB6A" bg_active="#388E3C" text_color="#FFFFFF"
        border_radius="6" font_size="16" disabled="false"
        on_click="submit_form"/>
```

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `label` / `text` | string | `"按钮"` | 按钮文字 |
| `color` | hex | — | 背景色 |
| `bg_hover` | hex | — | 悬停色 |
| `bg_active` | hex | — | 按下色 |
| `text_color` | hex | — | 文字色 |
| `width` | float | `0` | 宽度（0=自动） |
| `height` | float | `0` | 高度 |
| `disabled` | bool | `false` | 禁用 |
| `border_radius` | float | `0` | 圆角 |
| `font_size` | int | `0` | 字号（0=默认） |
| `on_click` / `onClick` | string | `""` | 点击事件名 |

### 4.3 `text` 文本

```xml
<text value="标题" color="#FFFFFF" font_size="18" font_weight="bold"
      align="center" shadow="true" shadow_color="#00000080"
      shadow_offset="1,1" ref="player_name"/>
```

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `value` | string | `""` | 内容 |
| `color` | hex | `"#E0E0E0"` | 颜色 |
| `ref` | string | `""` | 动态绑定键（优先级 > value） |
| `font_size` | int | `0` | 字号 |
| `font_weight` | string | `"normal"` | `normal` / `bold` |
| `align` | string | `"left"` | `left` / `center` / `right` |
| `shadow` | bool | `false` | 文字阴影 |
| `shadow_color` | hex | `"#00000080"` | 阴影颜色 |
| `shadow_offset` | string | `"1,1"` | 阴影偏移 `dx,dy` |

### 4.4 `input` 输入框

```xml
<input placeholder="输入名称" default="默认值" width="200"
       readonly="true" password="false" max_length="255"
       align="right" font_size="16"/>
```

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `placeholder` | string | `""` | 占位提示 |
| `default` | string | `""` | 默认值 |
| `width` | float | `0` | 宽度 |
| `readonly` | bool | `false` | 只读 |
| `password` | bool | `false` | 密码模式 |
| `max_length` | int | `255` | 最大字符数 |
| `align` | string | `"left"` | 对齐 |
| `font_size` | int | `0` | 字号 |

### 4.5 其他控件属性速查

| 节点 | 属性 |
|------|------|
| **checkbox** | label, default, **disabled**, **color**, onChange |
| **slider** | label, min, max, default, **disabled**, onChange |
| **combo** | label, options, default, **disabled** |
| **group** | label, collapsible, **default_open**, **indent** |
| **panel** | color, padding, round, **border_color**, **border_width**, **scrollable** |
| **row** | spacing, **wrap** |
| **progress** | value, max, label, width, height |
| **spacer** | width, height |
| **image** | width, height, tint, round |
| **overlay** | x, y, showBg, bgColor |

> **加粗** = v1.1 新增属性

---

## 五、资源声明（`resources.xml`）

### 5.1 贴图资源

```xml
<texture id="icon_health" path="textures/ui/health.png"/>
<texture id="custom_bg" path="plugins/my_plugin/textures/bg.png"/>

<!-- 远程贴图 -->
<texture id="player_avatar" url="https://mc-heads.net/avatar/{player_uuid}"
         cache="3600"/>

<!-- 程序化贴图 -->
<texture id="health_bar" type="gradient"
         direction="horizontal" colors="#FF0000,#00FF00"
         width="200" height="16"/>
<texture id="solid_bg" type="solid" color="#1E1E24CC" width="64" height="64"/>
```

### 5.2 音频资源

```xml
<audio id="click" path="sounds/ui/click.ogg"/>
<audio id="hover" path="sounds/ui/hover.wav" volume="0.3"/>
<audio id="beep" type="tone" frequency="440" duration="0.1" volume="0.5"/>
```

---

## 六、事件系统（`scripts.xml`）

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

    <event id="mixed_click" target="mixed">
        <sequence>
            <action type="play_audio" audio="click"/>
            <action type="set_state" key="click_count"
                    value="{state.click_count + 1}"/>
            <emit service="button_click">
                <payload name="id" ref="self.id"/>
            </emit>
        </sequence>
    </event>
</scripts>
```

### 事件类型

| 事件名 | 触发时机 | 适用组件 |
|--------|---------|---------|
| `click` | 鼠标点击 | button, image, checkbox |
| `hover_start` | 鼠标进入 | 所有组件 |
| `hover_end` | 鼠标离开 | 所有组件 |
| `change` | 值变化 | slider, input, checkbox, combo |
| `open` / `close` | 窗口开关 | window |

---

## 七、数据绑定

```xml
<!-- 静态 -->
<text value="固定文字" color="#FFFFFF"/>

<!-- 动态 ref 绑定 -->
<text ref="player_health" color="#FF4444"/>

<!-- 模板表达式 -->
<text value="生命值: {health}/{max_health}"/>
<text value="{health < 20 ? '危险' : '安全'}"
      color="{health < 20 ? '#FF0000' : '#00FF00'}"/>
```

C++ 端更新：`DynamicText::values["player_health"] = "80";`

---

## 八、C++ 插件端注册（FRTest-Native 模式）

在 LeviLamina 插件中通过 TCP 注册：

```cpp
#include <net/TcpClient.h>
#include <nlohmann/json.hpp>

FrTcpClient g_tcpClient;

// 连接内嵌 Server（纯本地）或外部 Server
g_tcpClient.connect("127.0.0.1", 12345);
g_tcpClient.enableAutoReconnect(3000, 5);
g_tcpClient.startReceiveThread();

// 注册 GUI
json def = {/* GUI 定义 JSON */};
json msg = {
    {"type", "gui_register"},
    {"pluginId", "my_plugin"},
    {"guiId", "my_gui"},
    {"definition", def.dump()}
};
g_tcpClient.send(msg.dump());

// 注册按键绑定
json kb = {
    {"type", "keybind_register"},
    {"pluginId", "my_plugin"},
    {"bindId", "open_menu"},
    {"bindName", "打开菜单"},
    {"vkCode", 0x72}  // F3
};
g_tcpClient.send(kb.dump());
```

### 协议消息类型

| type | 方向 | 说明 |
|------|------|------|
| `identify` | → Server | 标识客户端身份 |
| `gui_register` | → Server | 注册 GUI 定义 |
| `gui_unregister` | → Server | 注销 GUI |
| `keybind_register` | → Server | 注册按键绑定 |
| `keybind_unregister` | → Server | 注销按键绑定 |
| `gui_event` | 双向 | GUI 交互事件 |
| `data_exchange` | 双向 | 自定义数据交换 |
| `sync_request` | → Server | 请求重放缓存的注册 |

---

## 九、完整示例：计算器 UI

```xml
<window label="计算器" width="340" height="420" flags="no_resize">
    <column spacing="4">
        <!-- 显示屏（只读） -->
        <input id="display" ref="display" height="36"
               readonly="true" align="right" font_size="20"/>

        <!-- 按钮行 1: 7 8 9 + -->
        <row spacing="4">
            <button label="7" width="48" height="44" on_click="num_press"/>
            <button label="8" width="48" height="44" on_click="num_press"/>
            <button label="9" width="48" height="44" on_click="num_press"/>
            <button label="+" width="48" height="44"
                    color="#FF9800" on_click="op_press"/>
        </row>

        <!-- 按钮行 2: 4 5 6 - -->
        <row spacing="4">
            <button label="4" width="48" height="44" on_click="num_press"/>
            <button label="5" width="48" height="44" on_click="num_press"/>
            <button label="6" width="48" height="44" on_click="num_press"/>
            <button label="-" width="48" height="44"
                    color="#FF9800" on_click="op_press"/>
        </row>

        <!-- 按钮行 3: 1 2 3 * -->
        <row spacing="4">
            <button label="1" width="48" height="44" on_click="num_press"/>
            <button label="2" width="48" height="44" on_click="num_press"/>
            <button label="3" width="48" height="44" on_click="num_press"/>
            <button label="×" width="48" height="44"
                    color="#FF9800" on_click="op_press"/>
        </row>

        <!-- 按钮行 4: 0 . = / -->
        <row spacing="4">
            <button label="0" width="48" height="44" on_click="num_press"/>
            <button label="." width="48" height="44" on_click="num_press"/>
            <button label="=" width="100" height="44"
                    color="#4CAF50" on_click="calc_eq"/>
            <button label="÷" width="48" height="44"
                    color="#FF9800" on_click="op_press"/>
        </row>
    </column>
</window>
```

---

## 十、版本记录

| 版本 | 日期 | 变更 |
|------|------|------|
| v1.0 | 2026-06-21 | 初版 |
| v1.1 | 2026-06-28 | +内嵌 Server 架构，+readonly/disabled扩展/align/font_size/font_weight/bg_hover/bg_active/text_color/border_radius/window.flags/default_open/indent/border_color/border_width/scrollable/image节点/shadow/password/wrap/overlay/fullscreen/modal |
