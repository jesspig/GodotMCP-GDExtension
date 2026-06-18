# 设置、导出、输入与插件工具

15 个用于项目设置、导出、输入映射、插件管理、项目搭建和可视化的工具。

## 项目设置（4 个）

### `get_setting`

**分类**: `editor_tools/settings`  
**描述**: 读取项目设置。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `key` | `string` | 是 | 设置键（如 "application/config/name"） |

### `set_setting`

**分类**: `editor_tools/settings`  
**描述**: 写入项目设置。  
**破坏性**: 是 | **撤销**: 否

### `reset_setting`

**分类**: `editor_tools/settings`  
**描述**: 将项目设置重置为默认值。  
**破坏性**: 是 | **撤销**: 否

### `list_settings`

**分类**: `editor_tools/settings`  
**描述**: 列出匹配前缀的所有项目设置。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|----------|---------|-------------|
| `prefix` | `string` | 否 | "" | 设置键前缀筛选 |
| `max_results` | `int` | 否 | 100 | 最大结果数 |

---

## 导出（4 个）

### `list_export_presets`

**分类**: `editor_tools/export`  
**描述**: 列出导出预设。  
**破坏性**: 否 | **撤销**: 否

### `export_project`

**分类**: `editor_tools/export`  
**描述**: 导出项目。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `preset_name` | `string` | 是 | 导出预设名称 |
| `output_path` | `string` | 是 | 输出文件路径 |

### `validate_export_presets`

**分类**: `editor_tools/export`  
**描述**: 验证导出预设配置。  
**破坏性**: 否 | **撤销**: 否

### `get_export_platforms`

**分类**: `editor_tools/export`  
**描述**: 获取可用的导出平台。  
**破坏性**: 否 | **撤销**: 否

---

## 输入映射（4 个）

### `input_list_actions`

**分类**: `editor_tools/inputmap`  
**描述**: 列出所有输入动作和绑定的事件。  
**破坏性**: 否 | **撤销**: 否

### `add_input_action`

**分类**: `editor_tools/inputmap`  
**描述**: 创建新的输入动作。  
**破坏性**: 否 | **撤销**: 是

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `action_name` | `string` | 是 | 动作名称 |

### `remove_input_action`

**分类**: `editor_tools/inputmap`  
**描述**: 移除输入动作。  
**破坏性**: 是 | **撤销**: 是

### `add_input_event_binding`

**分类**: `editor_tools/inputmap`  
**描述**: 为动作添加按键/按钮绑定。  
**破坏性**: 否 | **撤销**: 是

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `action_name` | `string` | 是 | 动作名称 |
| `event_type` | `string` | 是 | 事件类型（key, mouse_button, joy_button, joy_axis） |
| `device` | `int` | 否 | 设备 ID |
| `keycode` | `string` | 否 | 键码（用于按键事件） |
| `button_index` | `int` | 否 | 按钮索引（用于鼠标/手柄事件） |

---

## 插件管理（2 个）

### `list_plugins`

**分类**: `editor_tools/plugin`  
**描述**: 列出所有插件及其状态。  
**破坏性**: 否 | **撤销**: 否

### `set_plugin_enabled`

**分类**: `editor_tools/plugin`  
**描述**: 启用或禁用插件。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `plugin_name` | `string` | 是 | 插件名称 |
| `enabled` | `bool` | 是 | 启用或禁用 |

---

## 项目搭建（1 个）

### `create_project`

**分类**: `editor_tools/scaffold`  
**描述**: 创建新的 Godot 项目。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `path` | `string` | 是 | 项目目录路径 |
| `project_name` | `string` | 是 | 项目名称 |
| `renderer` | `string` | 否 | 渲染器类型 |

---

## 项目图可视化（1 个）

### `get_project_graph`

**分类**: `editor_tools/visualizer`  
**描述**: 获取项目依赖关系图。  
**破坏性**: 否 | **撤销**: 否