# Input Map 工具

> 4 个工具，管理 Godot InputMap（动作 + 事件绑定）。位于 `extensions/src/built_in/tools/editor_tools/inputmap/`，分类 `editor_tools/inputmap`。

## 工具列表

| 工具 | 描述 | 必需参数 |
|------|------|---------|
| `input_list_actions` | 列出所有输入动作及其绑定事件 | 无 |
| `add_input_action` | 创建新动作（指定 deadzone） | `action` |
| `add_input_event_binding` | 为已有动作追加单个事件绑定 | `action`, `event_type` |
| `remove_input_action` | 移除动作（含其全部绑定） | `action` |

## `input_list_actions`

**无参数**。schema 为空 object（`input_list_actions.hpp:23-28`）。

返回 `data`：

```json
{
  "count": 35,
  "actions": [
    {
      "name": "ui_accept",
      "deadzone": 0.2,
      "event_count": 2,
      "events": [
        {
          "type": "InputEventKey",
          "as_text": "Enter",
          "matched_action": true,
          "properties": {
            " keycode ": 4194309,
            "ctrl_pressed": false,
            "physical_keycode": 0
          }
        }
      ]
    }
  ]
}
```

- `type`：Godot 类名（`InputEventKey` / `InputEventMouseButton` / `InputEventJoypadButton` / `InputEventJoypadMotion`），来自 `event->get_class()`（`input_list_actions.hpp:53`）
- `properties`：遍历 `get_property_list()` 导出全部属性值（`input_list_actions.hpp:60-67`）

## `add_input_action`

**参数**（`add_input_action.hpp:22-46`）：

| 参数 | 类型 | 必需 | 默认 | 说明 |
|------|------|------|------|------|
| `action` | string | 是 | — | 动作名 |
| `deadzone` | number | 否 | **0.2** | 死区值 0.0–1.0 |

已存在则返回错误码 `ALREADY_EXISTS`。返回 `{ action, deadzone, created: true }`。

## `add_input_event_binding`

**参数**（`add_input_event_binding.hpp:26-78`）：

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `action` | string | 是 | 目标动作名（必须已存在） |
| `event_type` | string | 是 | 事件类型（见下表） |
| `keycode` | int 或 string | 否 | `key` 类型：整数值或 `"KEY_W"` 字符串（自动解析） |
| `button_index` | int | 否 | `mb`/`jb` 类型：按钮索引 |
| `axis` | int | 否 | `ja` 类型：轴索引 |
| `axis_sign` | number | 否 | `ja` 类型：轴方向（1.0 或 -1.0） |
| `modifiers` | object | 否 | `key` 类型：`{ ctrl, shift, alt, meta }` 布尔值 |

**`event_type` 取值**（`add_input_event_binding.hpp:114-170`）：

| 值 | 别名 | 创建的 InputEvent | 附加字段 |
|----|------|-------------------|---------|
| `key` | — | `InputEventKey` | `keycode`、`modifiers` |
| `mb` | `mouse_button` | `InputEventMouseButton` | `button_index`（默认 1） |
| `jb` | `joy_button` | `InputEventJoypadButton` | `button_index`（默认 0） |
| `ja` | `joy_axis` | `InputEventJoypadMotion` | `axis`（默认 0）、`axis_sign`（默认 1.0） |

`keycode` 支持两种形式（`add_input_event_binding.hpp:117-129`）：
- 整数：直接传 `87`（W 键）
- 字符串：传 `"KEY_W"`，代码会截取 `KEY_` 后部分、capitalize、去下划线后转 `Key.W` 枚举值

动作不存在返回 `NOT_FOUND`；类型未知返回 `UNKNOWN_TYPE`。返回 `{ action, event_type, added: true }`。

## `remove_input_action`

**参数**（`remove_input_action.hpp:22-38`）：

| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `action` | string | 是 | 要删除的动作名 |

不存在返回 `NOT_FOUND`。返回 `{ action, removed: true }`。
