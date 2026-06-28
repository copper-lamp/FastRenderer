# FastRenderer GUI 属性覆盖分析

> 分析当前 JsonGuiRenderer 的实际实现与理想 GUI 系统之间的差距，列出需要补充的属性。

---

## 一、当前实现状态

### ✅ 已实现的节点类型（13 种）

| 节点类型 | 支持的属性 |
|---------|-----------|
| `window` | title, width, height, **fullscreen**, **modal** |
| `row` | spacing |
| `column` | spacing |
| `button` | text, color, width, height, **disabled**, onClick |
| `text` | value, color, ref |
| `separator` | (无) |
| `checkbox` | label, default, onChange |
| `slider` | label, min, max, default, onChange |
| `input` | placeholder, default |
| `combo` | label, options, default |
| `group` | label, **collapsible** |
| `spacer` | width, height |
| `panel` | color, padding, round |
| `progress` | value, max, label, width, height |
| **`overlay`** (新增) | **x, y, showBg, bgColor** |

### ❌ 规范中提及但尚未实现的节点

| 节点类型 | 缺失程度 | 说明 |
|---------|---------|------|
| `image` | **完全缺失** | 纹理/图片渲染 |
| `canvas` | **完全缺失** | 自定义绘制（矩形、圆、线、弧） |
| `tooltip` | **完全缺失** | 悬停提示 |

### ⚠️ 已实现但属性覆盖不全

| 节点 | 已有属性 | 缺少的属性 |
|------|---------|-----------|
| `window` | title, width, height, fullscreen, modal | flags(no_collapse/no_scrollbar...), bg_color, border_radius, enter/exit_animation |
| `button` | text, color, width, height, disabled | bg_hover, bg_active, border_radius, font_size, icon |
| `text` | value, color, ref | font_size, font_weight, align, shadow |
| `input` | placeholder, default | **readonly**, width, align, font_size, max_length, password_mode |
| `panel` | color, padding, round | border_color, border_width |
| `group` | label, collapsible | default_open, indent |

---

## 二、需要补充的属性分级清单

### P0 — 核心体验（缺少就无法做出像样 UI）

| 属性 | 适用节点 | 实现难度 | 说明 |
|------|---------|---------|------|
| `readonly` | input | ★☆☆ | 只读输入框，禁用编辑 |
| `font_size` | text, button, input | ★☆☆ | 字号控制 |
| `font_weight` | text | ★☆☆ | 字重：normal / bold |
| `align` | text, button | ★★☆ | 水平对齐：left / center / right |
| `disabled` | checkbox, slider, combo | ★☆☆ | 禁用状态，扩展到更多控件 |
| `image` 节点 | 独立节点 | ★★☆ | 显示纹理/贴图/图标 |

### P1 — 视觉丰富度（让 UI 更好看）

| 属性 | 适用节点 | 实现难度 | 说明 |
|------|---------|---------|------|
| `bg_hover` | button | ★☆☆ | 悬停背景色 |
| `bg_active` | button | ★☆☆ | 按下背景色 |
| `window.flags` | window | ★★☆ | ImGui 窗口标志位映射 |
| `border_radius` | button, panel, window | ★★☆ | 圆角（需要 ImGui PushStyleVar） |
| `border_color` | panel, window | ★☆☆ | 边框颜色 |
| `border_width` | panel, window | ★☆☆ | 边框宽度 |
| `tooltip` 节点 | 独立节点 | ★★☆ | 悬停提示 |
| `opacity` | 所有节点 | ★★☆ | 透明度（整体 PushStyleVar） |

### P2 — 高级功能（锦上添花）

| 属性 | 适用节点 | 实现难度 | 说明 |
|------|---------|---------|------|
| `default_open` | group(collapsible) | ★☆☆ | 可折叠组默认展开 |
| `indent` | group | ★☆☆ | 缩进级别 |
| `max_length` | input | ★☆☆ | 输入最大长度 |
| `password` | input | ★☆☆ | 密码模式（显示 ***） |
| `wrap` | row | ★☆☆ | 子项换行 |
| `scrollable` | child/panel | ★☆☆ | 可滚动区域 |
| `color` (直接值) | checkbox label | ★☆☆ | 复选框文字颜色 |
| `shadow` / `shadow_offset` | text | ★★☆ | 文字阴影 |

### P3 — 未来扩展（匹配 FastUI 规范）

| 属性 | 说明 | 依赖 |
|------|------|------|
| `canvas` 节点 | 矩形/圆/线/弧绘制 | 新节点类型 |
| `v_if` / `v_for` | 条件/列表渲染 | 表达式引擎 |
| `enter_animation` / `exit_animation` | 窗口入场/出场动画 | 动画系统 |
| 样式系统 `<style>` | 具名样式引用 | 样式解析 |
| 模板系统 `<use>` | 组件模板 | 模板解析 |

---

## 三、推荐优先实现（P0 + P1 共 14 项）

### 3.1 `readonly`（input）

```cpp
if (node.type == "input") {
    bool readonly = JsonGuiParser::getPropBool(node, "readonly", false);
    ImGuiInputTextFlags flags = readonly ? ImGuiInputTextFlags_ReadOnly : 0;
    ImGui::InputText(inputId.c_str(), buf, sizeof(buf), flags);
}
```

### 3.2 `font_size` + `font_weight`（text）

```cpp
if (node.type == "text") {
    int fontSize = JsonGuiParser::getPropInt(node, "font_size", 0);
    bool bold = JsonGuiParser::getProp(node, "font_weight", "") == "bold";
    if (fontSize > 0 || bold) {
        ImGui::PushFont(/* select appropriate font */);
    }
    // render text
    if (fontSize > 0 || bold) ImGui::PopFont();
}
```

难点：ImGui 需要提前加载不同字号的字体，或者在渲染时临时 PushFont。

### 3.3 `align`（text, button）

```cpp
if (node.type == "text") {
    std::string align = JsonGuiParser::getProp(node, "align", "left");
    float availWidth = ImGui::GetContentRegionAvail().x;
    if (align == "center") {
        float tw = ImGui::CalcTextSize(value.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - tw) * 0.5f);
    } else if (align == "right") {
        float tw = ImGui::CalcTextSize(value.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availWidth - tw);
    }
    ImGui::TextUnformatted(value.c_str());
}
```

### 3.4 `disabled` 扩展到 checkbox, slider, combo

```cpp
// 每个控件增加：
bool disabled = JsonGuiParser::getPropBool(node, "disabled", false);
if (disabled) ImGui::BeginDisabled();
// ... render control ...
if (disabled) ImGui::EndDisabled();
```

### 3.5 `image` 节点

```cpp
} else if (node.type == "image") {
    std::string textureId = JsonGuiParser::getProp(node, "texture", "");
    float w = JsonGuiParser::getPropFloat(node, "width", 0);
    float h = JsonGuiParser::getPropFloat(node, "height", 0);
    // Lookup texture from TextureManager
    ImTextureID tex = TextureManager::get(textureId);
    if (tex) {
        ImGui::Image(tex, ImVec2(w, h));
    }
}
```

但需要实现 TextureManager 来管理纹理资源。如果暂时不需要运行时纹理管理，可以实现一个简易版本。

### 3.6 `bg_hover` / `bg_active`（button）

```cpp
if (!bgHover.empty()) {
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, parseColor(bgHover));
}
if (!bgActive.empty()) {
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, parseColor(bgActive));
}
```

### 3.7 `window.flags`

定义 ImGui 窗口标志的字符串映射：

| 字符串 | ImGui 标志 |
|--------|-----------|
| `no_collapse` | ImGuiWindowFlags_NoCollapse |
| `no_scrollbar` | ImGuiWindowFlags_NoScrollbar |
| `no_resize` | ImGuiWindowFlags_NoResize |
| `no_move` | ImGuiWindowFlags_NoMove |
| `no_title_bar` | ImGuiWindowFlags_NoTitleBar |
| `always_auto_resize` | ImGuiWindowFlags_AlwaysAutoResize |
| `no_scroll_with_mouse` | ImGuiWindowFlags_NoScrollWithMouse |

```cpp
if (node.type == "window") {
    std::string flagsStr = JsonGuiParser::getProp(node, "flags", "");
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    if (flagsStr.find("no_collapse") != std::string::npos) flags |= ImGuiWindowFlags_NoCollapse;
    if (flagsStr.find("no_scrollbar") != std::string::npos) flags |= ImGuiWindowFlags_NoScrollbar;
    // ...
}
```

---

## 四、实现路径建议

### Phase 1（立即可做，低风险）
1. `readonly` on input
2. `disabled` on checkbox, slider, combo
3. `align` on text
4. `bg_hover` / `bg_active` on button
5. `window.flags` string parsing
6. `default_open` on collapsible group

### Phase 2（中等复杂度）
7. `font_size` / `font_weight` on text（需要字体管理）
8. `image` 节点（需要 TextureManager）
9. `tooltip` 节点
10. `border_radius` / `border_color` / `border_width` on panel/window

### Phase 3（高级功能）
11. `opacity` 全局透明度
12. `max_length` / `password` on input
13. `wrap` on row
14. `scrollable` on panel

---

## 五、现有属性速查表

| 属性 | 按钮 | 文本 | 滑条 | 输入框 | 复选框 | 下拉框 | 面板 | 窗口 | 叠加层 |
|------|:----:|:----:|:----:|:-----:|:-----:|:-----:|:----:|:----:|:-----:|
| text/label | ✅ | ✅ | ✅ | | ✅ | ✅ | | | |
| value | | ✅ | | | | | | | |
| color | ✅ | ✅ | | | | | ✅ | | ✅ |
| width | ✅ | | | ✅ | | | | ✅ | |
| height | ✅ | | | ✅ | | | | ✅ | |
| ref | | ✅ | | | | | | | |
| default | | | ✅ | ✅ | ✅ | ✅ | | | |
| disabled | ✅ | | | | ⬜ | ⬜ | | | |
| onClick/onChange | ✅ | | ✅ | | ✅ | | | | |
| placeholder | | | | ✅ | | | | | |
| collapsible | | | | | | | | | |
| padding | | | | | | | ✅ | | |
| round | | | | | | | ✅ | | |
| fullscreen | | | | | | | | ✅ | |
| modal | | | | | | | | ✅ | |
| x, y, showBg, bgColor | | | | | | | | | ✅ |
| ⬜=缺失 | | | | | | | | | |
