# FastRenderer GUI 属性完全规范

> 版本：v1.1  
> 覆盖范围：所有节点类型 + 完整属性集  
> 对应渲染器：`JsonGuiRenderer.h`

---

## 一、节点类型总览（16 种）

| # | 节点类型 | 分类 | 说明 | 实现状态 |
|---|---------|------|------|---------|
| 1 | `window` | 容器 | 顶层窗口 | ✅ |
| 2 | `row` | 容器 | 水平布局（SameLine） | ✅ |
| 3 | `column` | 容器 | 垂直布局 | ✅ |
| 4 | `group` | 容器 | 编组/可折叠分组 | ✅ |
| 5 | `panel` | 容器 | 带背景的子窗口 | ✅ |
| 6 | `button` | 控件 | 按钮 | ✅ |
| 7 | `text` | 控件 | 文本标签 | ✅ |
| 8 | `separator` | 控件 | 分隔线 | ✅ |
| 9 | `checkbox` | 控件 | 复选框 | ✅ |
| 10 | `slider` | 控件 | 滑块 | ✅ |
| 11 | `input` | 控件 | 文本输入框 | ✅ |
| 12 | `combo` | 控件 | 下拉选择 | ✅ |
| 13 | `spacer` | 控件 | 空白占位 | ✅ |
| 14 | `progress` | 控件 | 进度条 | ✅ |
| 15 | `overlay` | 特殊 | F3 调试文字叠加 | ✅ |
| 16 | `image` | 控件 | 图片/纹理 | ⬜ 待实现 |

---

## 二、完整属性表

### 2.1 容器节点属性

#### `window`（窗口）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `title` | string | `"窗口"` | 窗口标题（也是 ImGui 唯一 ID） |
| `width` | int | `400` | 窗口宽度（px） |
| `height` | int | `300` | 窗口高度（px） |
| `fullscreen` | bool | `false` | 全屏无边框模式 |
| `modal` | bool | `false` | 模态遮罩模式（半透明黑色背景） |
| `flags` | string | `""` | ImGui 窗口标志，逗号分隔。
  可选值：`no_collapse`, `no_scrollbar`, `no_resize`, `no_move`,
  `no_title_bar`, `always_auto_resize`, `no_scroll_with_mouse`,
  `no_saved_settings` |
| `bg_color` | hex | — | 窗口背景色（覆盖 ImGui 主题） |
| `border` | bool | `true` | 是否显示窗口边框 |
| `border_radius` | float | `0` | 窗口圆角（全屏模式下自动=0） |

#### `row`（水平行）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `spacing` | float | `6.0` | 子控件间距（px） |
| `wrap` | bool | `false` | 超出宽度自动换行 |

#### `column`（垂直列）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `spacing` | float | `6.0` | 子控件垂直间距（px） |

#### `group`（编组）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `label` | string | `""` | 组标题 |
| `collapsible` | bool | `false` | 是否可折叠 |
| `default_open` | bool | `false` | 可折叠时默认展开状态（`collapsible=true` 时才生效） |
| `indent` | float | `0` | 缩进级别（px） |

#### `panel`（面板）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `color` | hex | `"#1E1E24CC"` | 面板背景色 |
| `padding` | float | `8.0` | 内边距（px） |
| `round` | float | `6.0` | 圆角（px） |
| `border_color` | hex | — | 边框颜色 |
| `border_width` | float | `0` | 边框宽度（px） |
| `scrollable` | bool | `false` | 是否可滚动（内容超出时显示滚动条） |

---

### 2.2 基础控件属性

#### `button`（按钮）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `text` | string | `"按钮"` | 按钮文字 |
| `color` | hex | — | 按钮背景色（映射到 ImGuiCol_Button） |
| `bg_hover` | hex | — | 悬停背景色（ImGuiCol_ButtonHovered） |
| `bg_active` | hex | — | 按下背景色（ImGuiCol_ButtonActive） |
| `text_color` | hex | — | 文字颜色（ImGuiCol_Text） |
| `width` | float | `0` | 按钮宽度（0=自动） |
| `height` | float | `0` | 按钮高度（0=自动） |
| `disabled` | bool | `false` | 禁用状态 |
| `border_radius` | float | `0` | 圆角（PushStyleVar FrameRounding） |
| `font_size` | int | `0` | 字号（0=默认） |
| `onClick` | string | `""` | 点击事件名 |

#### `text`（文本）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `value` | string | `""` | 文本内容 |
| `color` | hex | `"#E0E0E0"` | 文字颜色 |
| `ref` | string | `""` | 动态文本引用键（优先级高于 value） |
| `font_size` | int | `0` | 字号（0=默认） |
| `font_weight` | string | `"normal"` | 字重：`normal`, `bold` |
| `align` | string | `"left"` | 水平对齐：`left`, `center`, `right` |
| `shadow` | bool | `false` | 是否显示文字阴影 |
| `shadow_color` | hex | `"#00000080"` | 阴影颜色 |
| `shadow_offset` | list | `[1,1]` | 阴影偏移 `[dx, dy]` |

#### `separator`（分隔线）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| （无属性） | | | |

#### `checkbox`（复选框）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `label` | string | `""` | 复选框标签文字 |
| `default` | bool | `false` | 默认选中状态 |
| `disabled` | bool | `false` | 禁用状态 |
| `color` | hex | — | 标签文字颜色 |
| `onChange` | string | `""` | 值变化事件名 |

#### `slider`（滑块）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `label` | string | `""` | 标签文字 |
| `min` | float | `0.0` | 最小值 |
| `max` | float | `100.0` | 最大值 |
| `default` | float | `min` | 默认值 |
| `disabled` | bool | `false` | 禁用状态 |
| `onChange` | string | `""` | 值变化事件名 |

#### `input`（输入框）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `placeholder` | string | `""` | 占位提示文字 |
| `default` | string | `""` | 默认值 |
| `width` | float | `0` | 宽度（0=自动填充） |
| `readonly` | bool | `false` | 只读模式 |
| `password` | bool | `false` | 密码模式（显示 ***） |
| `max_length` | int | `255` | 最大字符数 |
| `align` | string | `"left"` | 文字对齐：`left`, `center`, `right` |
| `font_size` | int | `0` | 字号 |

#### `combo`（下拉选择）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `label` | string | `""` | 标签文字 |
| `options` | string | `"选项1,选项2,选项3"` | 选项列表（逗号分隔） |
| `default` | int | `0` | 默认选中索引 |
| `disabled` | bool | `false` | 禁用状态 |

#### `spacer`（空白占位）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `width` | float | `0` | 宽度（px） |
| `height` | float | `8.0` | 高度（px） |

#### `progress`（进度条）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `value` | float | `0.5` | 当前值 |
| `max` | float | `1.0` | 最大值 |
| `label` | string | `""` | 叠加文字 |
| `width` | float | `0` | 宽度（0=自动填充） |
| `height` | float | `0` | 高度 |

#### `image`（图片）— 新节点

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `texture` | string | `""` | 纹理 ID |
| `width` | float | `0` | 显示宽度 |
| `height` | float | `0` | 显示高度 |
| `tint` | hex | — | 染色颜色 |
| `round` | float | `0` | 圆角 |

#### `overlay`（调试叠加）

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `x` | float | `10.0` | 起始 X 位置 |
| `y` | float | `10.0` | 起始 Y 位置 |
| `showBg` | bool | `true` | 是否显示半透明背景 |
| `bgColor` | hex | `"#00000088"` | 背景色 |

子节点仅支持：`text`（彩色文字）、`separator`（分割线）

---

## 三、通用属性（所有节点共有）

以下属性可应用于任意节点类型：

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `visible` | bool | `true` | 是否可见 |
| `opacity` | float | `1.0` | 整体透明度（0.0~1.0） |
| `margin` | list | `[0,0,0,0]` | 外边距 `[top, right, bottom, left]` |
| `id` | string | `""` | 组件标识（用于事件绑定） |

---

## 四、渲染器实现指南

### 4.1 `image` 节点实现

需要 TextureManager 支持。当前可先实现一个简化版本：

```cpp
} else if (node.type == "image") {
    // Simplified placeholder: colored rect with text
    float w = JsonGuiParser::getPropFloat(node, "width", 32);
    float h = JsonGuiParser::getPropFloat(node, "height", 32);
    ImVec4 tint = parseColor(JsonGuiParser::getProp(node, "tint", ""), ImVec4(1,1,1,1));
    ImGui::InvisibleButton(("##img" + path).c_str(), ImVec2(w, h));
    // Draw placeholder rect
    auto dl = ImGui::GetWindowDrawList();
    ImVec2 pMin = ImGui::GetItemRectMin();
    ImVec2 pMax = ImGui::GetItemRectMax();
    dl->AddRectFilled(pMin, pMax, IM_COL32(60,60,80,200), 4.0f);
    dl->AddText(ImVec2(pMin.x + 4, pMin.y + (h-ImGui::GetFontSize())*0.5f),
                IM_COL32(128,128,128,255), "img");
}
```

### 4.2 `align` 对齐实现

```cpp
// 在 text / input 中
std::string align = JsonGuiParser::getProp(node, "align", "left");
float availWidth = ImGui::GetContentRegionAvail().x;
if (align == "center") {
    float tw = ImGui::CalcTextSize(value.c_str()).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - tw) * 0.5f);
} else if (align == "right") {
    float tw = ImGui::CalcTextSize(value.c_str()).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availWidth - tw);
}
```

### 4.3 `font_size` 实现（简化）

由于 ImGui 需要预先加载不同字号字体，简化方案是临时 Push 缩放：

```cpp
// 临时字号：利用 ImGui::SetWindowFontScale
if (fontSize > 0) {
    float scale = fontSize / ImGui::GetFontSize();
    ImGui::SetWindowFontScale(scale);
}
// render ...
if (fontSize > 0) ImGui::SetWindowFontScale(1.0f);
```

### 4.4 `shadow` 文字阴影实现

```cpp
auto dl = ImGui::GetWindowDrawList();
ImVec2 pos = ImGui::GetCursorScreenPos();
ImVec4 textCol = parseColor(color, ImVec4(1,1,1,1));
ImVec4 shadowCol = parseColor(shadowColor, ImVec4(0,0,0,0.5f));
float sx = shadowOffset.size() > 0 ? shadowOffset[0] : 1.0f;
float sy = shadowOffset.size() > 1 ? shadowOffset[1] : 1.0f;
// Shadow
dl->AddText(ImVec2(pos.x + sx, pos.y + sy),
    IM_COL32(0,0,0,(int)(shadowCol.w*255)), value.c_str());
// Foreground
dl->AddText(pos,
    IM_COL32((int)(textCol.x*255),(int)(textCol.y*255),(int)(textCol.z*255),(int)(textCol.w*255)),
    value.c_str());
```

---

## 五、使用示例

### 5.1 带完整属性的窗口

```json
{
  "type": "window",
  "props": {
    "title": "设置",
    "width": 400,
    "height": 300,
    "flags": "no_collapse, no_resize"
  },
  "children": [
    {
      "type": "text",
      "props": {
        "value": "游戏设置",
        "font_size": 18,
        "font_weight": "bold",
        "color": "#FFFFFF",
        "align": "center",
        "shadow": true
      }
    }
  ]
}
```

### 5.2 只读输入框（计算器显示屏）

```json
{
  "type": "input",
  "props": {
    "default": "0",
    "readonly": true,
    "align": "right",
    "font_size": 20,
    "height": 36
  }
}
```

### 5.3 带悬停效果的按钮

```json
{
  "type": "button",
  "props": {
    "text": "提交",
    "color": "#4CAF50",
    "bg_hover": "#66BB6A",
    "bg_active": "#388E3C",
    "border_radius": 6,
    "width": 120,
    "height": 36,
    "onClick": "submit_form"
  }
}
```

### 5.4 禁用控件

```json
{
  "type": "checkbox",
  "props": {
    "label": "启用高级模式",
    "disabled": true,
    "default": false
  }
}
```

### 5.5 可滚动面板

```json
{
  "type": "panel",
  "props": {
    "color": "#1E1E24CC",
    "scrollable": true,
    "height": 200
  },
  "children": [
    { "type": "text", "props": { "value": "第1行" } },
    { "type": "text", "props": { "value": "第2行" } }
  ]
}
```

---

## 六、版本记录

| 版本 | 日期 | 变更 |
|------|------|------|
| v1.0 | 2026-06-21 | 初版 FastUI 语言规范 |
| v1.1 | 2026-06-28 | 完整属性规范：新增 readonly, align, font_size, font_weight, bg_hover, bg_active, shadow, disabled(扩展), window.flags, default_open, image 节点, scrollable, border_*, password, max_length, opacity, wrap |
