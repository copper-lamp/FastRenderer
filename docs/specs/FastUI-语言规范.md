# FastUI 语言规范

> 版本：v1.0  
> 日期：2026-06-21

---

## 一、概述

FastUI 是 FastRenderer 框架的声明式 UI 语言。插件开发者通过 XML 风格的标记语言定义界面布局，编译为 JSON 后由 FastRenderer 客户端使用 ImGui 渲染。

### 核心理念

- **XML 风格语法**：类似 HTML，标签清晰、嵌套直观
- **多文件拆分**：布局、资源、样式、事件分文件管理，各司其职
- **声明式绑定**：通过 `ref` 和模板表达式实现数据与 UI 的自动同步
- **可扩展设计**：从简单的按钮面板到复杂 HUD 全覆盖

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

## 二、布局语法（`layout.ui`）

### 2.1 基本规则

- 标签名 = 组件类型
- 属性 = 组件属性
- 子组件 = 嵌套在父标签内
- 自闭合标签用 `/>`，容器标签用 `></xxx>`

### 2.2 组件类型

#### 容器组件

| 标签 | 功能 | 示例 |
|------|------|------|
| `<window>` | 基础窗口（对应 ImGui Begin/End） | `<window label="面板" width="400" height="300">` |
| `<row>` | 水平布局（ImGui SameLine） | `<row spacing="8">` |
| `<column>` | 垂直布局 | `<column spacing="4">` |
| `<group>` | 编组（BeginGroup/EndGroup） | `<group label="设置">` |
| `<tree>` | 可折叠树节点 | `<tree label="高级选项">` |

#### 基础控件

| 标签 | 功能 | 示例 |
|------|------|------|
| `<button>` | 按钮 | `<button id="submit" label="提交" width="100"/>` |
| `<text>` | 文本 | `<text value="欢迎" color="#FFFFFF"/>` |
| `<checkbox>` | 复选框 | `<checkbox id="opt" label="启用" ref="option_enabled"/>` |
| `<slider>` | 滑块 | `<slider id="volume" min="0" max="100" ref="volume"/>` |
| `<input>` | 输入框 | `<input id="name" placeholder="输入名称" ref="player_name"/>` |
| `<separator>` | 分隔线 | `<separator/>` |
| `<image>` | 图片 | `<image texture="icon_health" width="24" height="24"/>` |
| `<combo>` | 下拉选择 | `<combo id="mode" items="[A,B,C]" ref="selected_mode"/>` |

#### 特殊组件

| 标签 | 功能 | 示例 |
|------|------|------|
| `<canvas>` | 自定义画布绘制 | `<canvas width="200" height="200">` |
| `<use>` | 引用模板 | `<use id="icon_button" icon="gear" label="设置"/>` |
| `<tooltip>` | 悬停提示 | `<tooltip value="点击以执行操作"/>` |

### 2.3 通用属性

所有组件共有的属性：

| 属性 | 类型 | 说明 |
|------|------|------|
| `id` | string | 组件唯一标识，用于事件绑定和脚本引用 |
| `width` | int | 宽度（px） |
| `height` | int | 高度（px） |
| `flex` | float | 弹性占比（在 row/column 中分配剩余空间） |
| `style` | string | 引用 styles.xml 中的具名样式 |
| `color` | hex | 文字颜色（如 `#FFFFFF`） |
| `bg_color` | hex | 背景颜色（支持 Alpha：`#1E1E24CC`） |
| `border_radius` | int | 圆角像素 |
| `border_color` | hex | 边框颜色 |
| `border_width` | int | 边框宽度 |
| `padding` | list | 内边距 `[top, right, bottom, left]` 或 `[all]` |
| `margin` | list | 外边距 |
| `visible` | bool | 是否可见 |
| `align` | string | 对齐：`left`, `center`, `right`, `top`, `bottom` |

### 2.4 容器组件特有属性

| 属性 | 适用 | 说明 |
|------|------|------|
| `spacing` | row, column | 子组件间距（px） |
| `wrap` | row | 是否换行 |
| `label` | window, tree, group | 标题/标签文字 |
| `flags` | window | ImGui 窗口标志：`no_collapse`, `no_scrollbar`, `no_resize`, `no_move`, `always_auto_resize` |

### 2.5 示例

```xml
<window label="状态面板" width="300" height="200" flags="no_collapse">
    <column spacing="6">
        <text value="玩家信息" color="#FFD700" size="18" weight="bold"/>
        <separator/>
        <row spacing="8">
            <text value="生命值:" color="#888888"/>
            <text ref="health" color="#FF4444"/>
        </row>
        <row spacing="8">
            <text value="魔力值:" color="#888888"/>
            <text ref="mana" color="#4488FF"/>
        </row>
        <separator/>
        <row spacing="8">
            <button id="btn_heal" label="治疗" width="80"/>
            <button id="btn_buff" label="增益" width="80"/>
            <button id="btn_close" label="关闭" width="80" flex="1"/>
        </row>
    </column>
</window>
```

---

## 三、资源声明（`resources.xml`）

### 3.1 贴图资源

#### 游戏内置贴图

```xml
<texture id="icon_health" path="textures/ui/health.png"/>
<texture id="icon_mana" path="textures/ui/mana.png"/>
```

路径相对于 Minecraft 游戏资源目录。

#### 插件自定义贴图

```xml
<texture id="custom_bg" path="plugins/my_plugin/textures/bg.png"/>
```

路径相对于服务器根目录。

#### 远程贴图

```xml
<texture id="player_avatar" url="https://mc-heads.net/avatar/{player_uuid}"
         cache="3600"/>
```

`cache` 为缓存时间（秒），0 表示不缓存。

#### 程序化生成贴图

```xml
<!-- 渐变条 -->
<texture id="health_bar" type="gradient"
         direction="horizontal"
         colors="#FF0000,#FFFF00,#00FF00"
         width="200" height="16"/>

<!-- 纯色块 -->
<texture id="solid_bg" type="solid"
         color="#1E1E24CC"
         width="64" height="64"/>

<!-- 边框 -->
<texture id="frame" type="border"
         color="#3D85E0"
         width="1"/>
```

### 3.2 音频资源

```xml
<!-- 本地音频文件 -->
<audio id="click" path="sounds/ui/click.ogg"/>
<audio id="hover" path="sounds/ui/hover.wav" volume="0.3"/>

<!-- 程序化音调 -->
<audio id="beep" type="tone" frequency="440" duration="0.1" volume="0.5"/>
```

音频目前支持 `.ogg` 和 `.wav` 格式。

---

## 四、样式系统（`styles.xml`）

### 4.1 具名样式

```xml
<styles>
    <style id="primary_button">
        <property name="bg_color" value="#3D85E0"/>
        <property name="bg_hover" value="#5BA3F0"/>
        <property name="text_color" value="#FFFFFF"/>
        <property name="border_radius" value="6"/>
        <property name="height" value="32"/>
        <property name="padding" value="[8,16]"/>
    </style>

    <style id="success_button">
        <property name="bg_color" value="#27AE60"/>
        <property name="bg_hover" value="#2ECC71"/>
        <property name="text_color" value="#FFFFFF"/>
        <property name="border_radius" value="6"/>
    </style>

    <style id="danger_button">
        <property name="bg_color" value="#E74C3C"/>
        <property name="bg_hover" value="#FF5B4A"/>
        <property name="text_color" value="#FFFFFF"/>
        <property name="border_radius" value="6"/>
    </style>
</styles>
```

### 4.2 组件默认样式

```xml
<styles>
    <default type="button">
        <property name="height" value="32"/>
        <property name="border_radius" value="4"/>
        <property name="bg_color" value="#3D85E0"/>
        <property name="text_color" value="#FFFFFF"/>
    </default>

    <default type="window">
        <property name="bg_color" value="#1E1E24E0"/>
        <property name="border_radius" value="8"/>
        <property name="padding" value="[12]"/>
    </default>

    <default type="text">
        <property name="color" value="#E0E0E0"/>
        <property name="size" value="14"/>
    </default>
</styles>
```

### 4.3 样式属性列表

| 属性 | 适用组件 | 说明 |
|------|---------|------|
| `bg_color` | 所有 | 背景色（支持 Alpha） |
| `bg_hover` | button | 悬停背景色 |
| `bg_active` | button | 点击时背景色 |
| `text_color` | 文字类 | 文字颜色 |
| `border_color` | 所有 | 边框颜色 |
| `border_width` | 所有 | 边框宽度 |
| `border_radius` | 所有 | 圆角（px） |
| `padding` | 容器类 | 内边距 |
| `font_size` | 文字类 | 字号 |
| `font_weight` | 文字类 | 字重：`normal`, `bold` |
| `opacity` | 所有 | 透明度 0.0~1.0 |
| `shadow_color` | 所有 | 阴影颜色 |
| `shadow_offset` | 所有 | 阴影偏移 `[x, y]` |

### 4.4 优先级规则

```
内联属性 > style 引用 > 组件默认样式 > 主题系统
```

---

## 五、事件系统（`scripts.xml`）

### 5.1 事件映射

```xml
<scripts>
    <!-- 本地事件：仅客户端响应，不经过网络 -->
    <event id="play_click" target="local">
        <action type="play_audio" audio="click"/>
    </event>

    <!-- 服务端事件：发送 GuiEventPacket -->
    <event id="submit_form" target="server">
        <emit service="form_submit">
            <payload name="gui" ref="self.parent.window.id"/>
        </emit>
    </event>

    <!-- 混合事件：先本地再服务端 -->
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

### 5.2 布局中绑定事件

```xml
<button id="btn_heal" label="治疗" on_click="submit_form"/>

<!-- 多事件绑定 -->
<button id="btn_act" label="执行">
    <events>
        <click script="mixed_click"/>
        <hover script="play_hover"/>
    </events>
</button>
```

### 5.3 事件类型

| 事件名 | 触发时机 | 适用组件 |
|--------|---------|---------|
| `click` | 鼠标点击 | button, image, checkbox |
| `hover_start` | 鼠标进入 | 所有组件 |
| `hover_end` | 鼠标离开 | 所有组件 |
| `change` | 值变化 | slider, input, checkbox, combo |
| `input` | 输入中 | input |
| `open` | 窗口打开 | window |
| `close` | 窗口关闭 | window |

### 5.4 客户端动作类型

| 动作 | 说明 |
|------|------|
| `play_audio` | 播放音频 |
| `set_state` | 设置本地状态 |
| `close_gui` | 关闭当前窗口 |
| `open_gui` | 打开指定窗口 |
| `set_focus` | 设置输入焦点 |
| `call_function` | 调用客户端注册的 C++ 函数 |

---

## 六、模板机制

### 6.1 模板定义

模板文件放在 `templates/` 目录下：

```xml
<!-- templates/card_button.xml -->
<template id="card_button" params="icon, label, color">
    <button width="44" height="44" bg_color="{color}" 
            border_radius="8">
        <image texture="{icon}" width="28" height="28"/>
        <tooltip value="{label}"/>
    </button>
</template>
```

### 6.2 模板引用

```xml
<use id="card_button" icon="gear" label="设置" color="#3D85E0"/>
<use id="card_button" icon="user" label="玩家" color="#27AE60"/>
```

### 6.3 带布局的模板

```xml
<template id="stat_row" params="name, ref, color">
    <row spacing="6">
        <text value="{name}" color="#888888" width="70"/>
        <text ref="{ref}" color="{color}" weight="bold"/>
    </row>
</template>
```

---

## 七、条件与列表渲染

### 7.1 条件渲染

```xml
<!-- 单条件 -->
<row v_if="{health < 20}">
    <text value="危险!" color="#FF0000"/>
</row>

<!-- 多分支 -->
<image texture="health_high" v_if="{health > 15}"/>
<image texture="health_mid" v_elseif="{health > 8}"/>
<image texture="health_low" v_else=""/>

<!-- 取反 -->
<row v_if="not {is_dead}">
    <text value="存活" color="#00FF00"/>
</row>
```

### 7.2 列表渲染

```xml
<!-- 简单列表 -->
<column v_for="item in inventory" spacing="2">
    <text value="{item.name} x{item.count}"/>
</column>

<!-- 带索引 -->
<column v_for="(buff, idx) in buffs">
    <row bg_color="#FFFFFF{idx % 2 == 0 ? 08 : 04}">
        <text value="{buff.name}" width="100"/>
        <text value="{buff.duration}s" color="#888888"/>
    </row>
</column>

<!-- 空列表处理 -->
<column v_for="item in items">
    <text value="{item.name}"/>
    <empty>
        <text value="(没有物品)" color="#666666"/>
    </empty>
</column>
```

---

## 八、画布系统

### 8.1 基础绘制指令

```xml
<canvas width="200" height="200">
    <!-- 矩形 -->
    <rect x="10" y="10" width="180" height="40" 
          color="#3D85E040" radius="4"/>

    <!-- 圆角矩形 -->
    <rect x="10" y="60" width="180" height="40" 
          color="#27AE60" radius="8" filled="true"/>

    <!-- 圆形 -->
    <circle cx="50" cy="150" radius="30" 
            color="#E74C3C" filled="true"/>

    <!-- 线 -->
    <line x1="100" y1="120" x2="180" y2="180" 
          color="#FFFFFF" width="2"/>

    <!-- 渐变矩形 -->
    <rect_gradient x="10" y="110" width="80" height="20"
                   colors="#FF0000,#FFFF00,#00FF00"
                   direction="horizontal" radius="4"/>

    <!-- 扇形（适合做环形血量） -->
    <arc cx="100" cy="100" radius="50"
         start_angle="-90"
         end_angle="{(health/max_health)*360-90}"
         color="#FF4444" filled="true"
         segments="64"/>

    <!-- 路径 -->
    <path d="M 20 180 Q 60 130 100 180 T 180 180"
          color="#FFD700" width="3" filled="false"/>

    <!-- 文字 -->
    <text value="{health}" x="100" y="96" 
          color="#FFFFFF" size="28" align="center"/>

    <!-- 贴图 -->
    <image texture="gradient_bar" x="0" y="0" 
           width="200" height="20"/>
</canvas>
```

### 8.2 画布属性

| 属性 | 说明 |
|------|------|
| `x`, `y` | 起点坐标 |
| `width`, `height` | 尺寸 |
| `color` | 颜色（hex） |
| `radius` | 圆角 / 半径 |
| `filled` | 是否填充 |
| `width` (线) | 线宽 |
| `segments` | 分段数（弧线平滑度） |

---

## 九、数据绑定

### 9.1 静态绑定

```xml
<text value="固定文字" color="#FFFFFF"/>
```

### 9.2 动态绑定（通过 ref）

```xml
<text ref="player_health" color="#FF4444"/>
```

C++ 端通过 `DynamicText::values["player_health"]` 实时更新。

### 9.3 模板表达式

```xml
<text value="生命值: {health}/{max_health}"/>
<text value="{health < 20 ? '危险' : '安全'}" color="{health < 20 ? '#FF0000' : '#00FF00'}"/>
<text value="{player.name} - 等级 {player.level}"/>
```

### 9.4 表达式语法

| 表达式 | 说明 |
|--------|------|
| `{变量名}` | 引用动态文本变量 |
| `{state.xxx}` | 引用本地状态 |
| `{a + b}` | 算术运算 |
| `{a < b ? x : y}` | 三元条件 |
| `{array.length}` | 属性访问 |
| `{array[idx]}` | 索引访问 |
| `{a == b}` / `{a != b}` | 比较运算 |
| `{a && b}` / `{a \|\| b}` | 逻辑运算 |

---

## 十、动画系统

### 10.1 内置动画

窗口级动画在布局根节点上声明：

```xml
<window label="面板" width="400" height="300"
        enter_animation="fade_in" exit_animation="slide_up">
```

内置动画类型：

| 动画名 | 效果 | 时长 |
|--------|------|------|
| `fade_in` | 淡入 | 0.3s |
| `fade_out` | 淡出 | 0.2s |
| `slide_up` | 上滑进入 | 0.25s |
| `slide_down` | 下滑进入 | 0.25s |
| `scale_in` | 缩放进入 | 0.3s |

### 10.2 自定义动画

```xml
<animation id="pulse" type="custom">
    <keyframe time="0.0" color="#3D85E000"/>
    <keyframe time="0.5" color="#3D85E080"/>
    <keyframe time="1.0" color="#3D85E000"/>
    <loop value="true"/>
</animation>
```

---

## 十一、完整示例

### 计算器 UI

```xml
<window label="计算器" width="240" height="280" flags="no_resize">
    <column spacing="4">
        <input id="display" ref="display" height="36" 
               readonly="true" align="right" font_size="20"/>

        <row spacing="4">
            <button label="7" width="54" height="40" on_click="num_press"/>
            <button label="8" width="54" height="40" on_click="num_press"/>
            <button label="9" width="54" height="40" on_click="num_press"/>
            <button label="÷" width="54" height="40" 
                    bg_color="#3D85E0" on_click="op_press"/>
        </row>

        <row spacing="4">
            <button label="4" width="54" height="40" on_click="num_press"/>
            <button label="5" width="54" height="40" on_click="num_press"/>
            <button label="6" width="54" height="40" on_click="num_press"/>
            <button label="×" width="54" height="40" 
                    bg_color="#3D85E0" on_click="op_press"/>
        </row>

        <row spacing="4">
            <button label="1" width="54" height="40" on_click="num_press"/>
            <button label="2" width="54" height="40" on_click="num_press"/>
            <button label="3" width="54" height="40" on_click="num_press"/>
            <button label="-" width="54" height="40" 
                    bg_color="#3D85E0" on_click="op_press"/>
        </row>

        <row spacing="4">
            <button label="0" width="54" height="40" on_click="num_press"/>
            <button label="." width="54" height="40" on_click="num_press"/>
            <button label="=" width="112" height="40" 
                    bg_color="#27AE60" on_click="calc"/>
        </row>
    </column>
</window>
```

### 背包面板

```xml
<window label="背包" width="400" height="350" flags="no_collapse">
    <column spacing="8">
        <!-- 快捷栏 -->
        <row spacing="2" v_for="(item, idx) in hotbar" flex="1">
            <button id="slot_{idx}" width="40" height="40"
                    bg_color="#FFFFFF" border_radius="2"
                    on_click="select_slot">
                <image texture="{item.icon}" width="32" height="32"/>
                <text value="{item.count}" align="bottom_right" font_size="10"/>
            </button>
        </row>

        <separator/>

        <!-- 物品网格 4x7 -->
        <column spacing="2" v_for="row in [0,1,2,3,4,5,6]">
            <row spacing="2" v_for="col in [0,1,2,3]">
                <button id="slot_{row*4+col+9}" width="40" height="40"
                        bg_color="#FFFFFF" border_radius="2"
                        on_click="select_inv_slot">
                    <image texture="{items[row*4+col].icon}" 
                           width="32" height="32"/>
                </button>
            </row>
        </column>
    </column>
</window>
```

---

## 附录

### A. 文件格式对照

| 文件 | 根元素 | 可读性 | 推荐用途 |
|------|--------|--------|---------|
| `layout.ui` | `<window>/<column>/<row>` | ⭐⭐⭐⭐⭐ | 主布局 |
| `resources.xml` | `<resources>` | ⭐⭐⭐⭐ | 资源声明 |
| `styles.xml` | `<styles>` | ⭐⭐⭐⭐ | 样式定义 |
| `scripts.xml` | `<scripts>` | ⭐⭐⭐ | 事件逻辑 |

### B. 与 ChiyanMap 兼容

FastUI 编译器将上述 XML 格式编译为标准 JSON GUI 定义，与 ChiyanMap 的 `JsonGuiParser`/`JsonGuiRenderer` 完全兼容。现有 JSON GUI 定义无需修改即可运行。
