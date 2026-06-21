# FastUI 开发者快速上手指南

> 5 分钟学会用 FastUI 写 UI

---

## 一、最小 UI

创建一个 `layout.ui` 文件：

```xml
<window label="我的第一个面板" width="300" height="200">
    <text value="你好，世界！" color="#FFFFFF"/>
    <button id="btn_ok" label="确定" width="80"/>
</window>
```

把它放在 `FastRenderer/gui_defs/` 目录下，游戏内按 N 键打开验证菜单即可看到。

---

## 二、三步写出实用 UI

### 第 1 步：声明资源

`resources.xml`：

```xml
<resources>
    <texture id="icon_health" path="textures/ui/health.png"/>
    <texture id="icon_mana" path="textures/ui/mana.png"/>
    <audio id="click" path="sounds/ui/click.ogg" volume="0.5"/>
</resources>
```

### 第 2 步：写布局

`layout.ui`：

```xml
<window label="玩家状态" width="280" height="180" flags="no_collapse">
    <column spacing="6">
        <row spacing="8">
            <image texture="icon_health" width="20" height="20"/>
            <text ref="health" color="#FF4444" weight="bold"/>
        </row>
        <row spacing="8">
            <image texture="icon_mana" width="20" height="20"/>
            <text ref="mana" color="#4488FF" weight="bold"/>
        </row>
        <separator/>
        <row spacing="8">
            <button id="btn_heal" label="恢复" width="80"
                    audio="click" on_click="heal_player"/>
            <button id="btn_close" label="关闭" width="80"
                    on_click="close_panel"/>
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

---

## 三、常见的模式

### 属性面板

```xml
<!-- styles.xml -->
<styles>
    <style id="stat_row">
        <property name="height" value="24"/>
    </style>

    <default type="window">
        <property name="bg_color" value="#1E1E24E0"/>
        <property name="border_radius" value="8"/>
        <property name="padding" value="[12]"/>
    </default>
</styles>
```

```xml
<!-- layout.ui -->
<window label="属性" width="250" height="300">
    <column spacing="2">
        <row v_for="attr in attributes" style="stat_row">
            <text value="{attr.name}" color="#888888" width="80"/>
            <text ref="{attr.ref}" color="#FFD700"/>
        </row>
    </column>
</window>
```

### 确认弹窗

```xml
<window label="确认" width="280" height="130" flags="no_collapse no_resize">
    <column spacing="12">
        <text value="确认要执行此操作吗？" color="#E0E0E0"/>
        <row spacing="8" align="right">
            <button id="btn_confirm" label="确认" width="80" 
                    style="danger_button" on_click="confirm_action"/>
            <button id="btn_cancel" label="取消" width="80" 
                    on_click="close_panel"/>
        </row>
    </column>
</window>
```

### 环形血量 HUD

```xml
<canvas width="100" height="100">
    <!-- 背景环 -->
    <arc cx="50" cy="50" radius="42"
         start_angle="0" end_angle="360"
         color="#333333" filled="false" segments="64"/>
    <!-- 血量环 -->
    <arc cx="50" cy="50" radius="42"
         start_angle="-90"
         end_angle="{(health/max_health)*360-90}"
         color="#FF4444" filled="false" segments="64"/>
    <!-- 血量数字 -->
    <text value="{health}" x="50" y="46"
          color="#FFFFFF" size="22" align="center"/>
</canvas>
```

---

## 四、从服务端插件注册 UI

### C++ 插件

```cpp
#include <FastRendererAPI.h>

// 获取 FastRenderer API
auto* fr = IFastRendererAPI::getInstance();

// 注册 GUI
fr->registerGui("my_plugin", "status_panel", jsonDefinition);

// 注册按键
fr->registerKeybind("my_plugin", "open_status", "状态面板", VK_F6);

// 监听 GUI 事件
fr->onGuiEvent("status_panel", [](const std::string& ctrl,
    const std::string& type, const std::string& val) {
    if (ctrl == "btn_heal" && type == "click") {
        // 执行治疗逻辑
    }
});
```

### JS 插件

```javascript
// 文件桥接方式（当前版本）
const fs = require('fs');

// 写入 GUI 定义
const guiDef = {
    id: "calculator",
    root: {
        type: "window",
        props: { label: "计算器", width: 240, height: 280 },
        children: [
            { type: "text", props: { ref: "display", value: "0" } }
        ]
    }
};
fs.writeFileSync(bridgePath + "/gui_pending.json", JSON.stringify(guiDef));

// 注册按键
fs.writeFileSync(bridgePath + "/keybind_pending.json", JSON.stringify({
    pluginId: "calculator", bindId: "open_calc",
    bindName: "计算器", vk: 0x4B  // K 键
}));
```

---

## 五、速查表

### 组件速查

| 我想做什么 | 怎么写 |
|-----------|--------|
| 一个带标题的窗口 | `<window label="标题" width="400" height="300">` |
| 一行横排按钮 | `<row spacing="8"><button/><button/></row>` |
| 一列竖排文字 | `<column spacing="4"><text/><text/></column>` |
| 显示图片 | `<image texture="icon_id" width="24" height="24"/>` |
| 显示动态数据 | `<text ref="health"/>` |
| 按钮点击发事件 | `<button on_click="event_id">` |
| 按条件显示 | `<row v_if="{health < 20}">` |
| 循环列表 | `<column v_for="item in items">` |
| 自定义绘制 | `<canvas width="200" height="200"><circle/></canvas>` |
| 复用组件 | `<use id="template_id" param1="value"/>` |

### 常见问题

**Q: 如何更新动态文本？**  
A: C++ 端调用 `DynamicText::values["health"] = "80";`

**Q: UI 不显示怎么办？**  
A: 检查 `gui_defs/` 目录路径是否正确，检查 JSON 格式是否合法

**Q: 按钮点击没有反应？**  
A: 检查按钮的 `id` 是否设置，检查 events 的 `target` 属性是否正确

**Q: 贴图不显示？**  
A: 检查 `resources.xml` 中的路径是否相对于正确目录

**Q: 如何调试 UI 布局？**  
A: 在 `layout.ui` 的 `<window>` 上添加 `debug="true"` 属性，会显示辅助边框

---

## 六、完整示例：猜数字游戏

`layout.ui`：

```xml
<window label="猜数字" width="300" height="200" flags="no_resize">
    <column spacing="8" align="center">
        <text value="猜一个 1-100 的数字" color="#888888"/>
        <text ref="hint" color="#FFD700" weight="bold"/>
        <input id="guess_input" ref="guess_value" 
               placeholder="输入数字" width="120" align="center"/>
        <row spacing="8" align="center">
            <button id="btn_guess" label="猜！" width="80"
                    style="success_button" on_click="guess"/>
            <button id="btn_reset" label="重置" width="80"
                    on_click="reset"/>
        </row>
        <text ref="attempts" color="#666666" size="12"/>
    </column>
</window>
```

`scripts.xml`：

```xml
<scripts>
    <event id="guess" target="server">
        <emit service="guess_number">
            <payload name="value" ref="guess_value"/>
        </emit>
    </event>
    <event id="reset" target="server">
        <emit service="reset_game"/>
    </event>
</scripts>
```

服务端插件通过监听 `guess_number` 和 `reset_game` 事件，更新 `hint`/`attempts`/`guess_value` 的 DynamicText，实现完整交互。
