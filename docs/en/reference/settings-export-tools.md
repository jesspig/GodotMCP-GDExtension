# Settings, Export, Input & Plugin Tools

15 tools for project settings, export, input mapping, plugin management, scaffolding, and visualization.

## Project Settings (4)

### `get_setting`

**Category**: `editor_tools/settings`  
**Description**: Read a project setting.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `key` | `string` | Yes | Setting key (e.g., "application/config/name") |

### `set_setting`

**Category**: `editor_tools/settings`  
**Description**: Write a project setting.  
**Destructive**: Yes | **Undo**: No

### `reset_setting`

**Category**: `editor_tools/settings`  
**Description**: Reset a project setting to default.  
**Destructive**: Yes | **Undo**: No

### `list_settings`

**Category**: `editor_tools/settings`  
**Description**: List all project settings matching a prefix.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `prefix` | `string` | No | "" | Setting key prefix filter |
| `max_results` | `int` | No | 100 | Max results |

---

## Export (4)

### `list_export_presets`

**Category**: `editor_tools/export`  
**Description**: List export presets.  
**Destructive**: No | **Undo**: No

### `export_project`

**Category**: `editor_tools/export`  
**Description**: Export the project.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `preset_name` | `string` | Yes | Export preset name |
| `output_path` | `string` | Yes | Output file path |

### `validate_export_presets`

**Category**: `editor_tools/export`  
**Description**: Validate export preset configuration.  
**Destructive**: No | **Undo**: No

### `get_export_platforms`

**Category**: `editor_tools/export`  
**Description**: Get available export platforms.  
**Destructive**: No | **Undo**: No

---

## Input Mapping (4)

### `input_list_actions`

**Category**: `editor_tools/inputmap`  
**Description**: List all input actions and bound events.  
**Destructive**: No | **Undo**: No

### `add_input_action`

**Category**: `editor_tools/inputmap`  
**Description**: Create a new input action.  
**Destructive**: No | **Undo**: Yes

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `action_name` | `string` | Yes | Action name |

### `remove_input_action`

**Category**: `editor_tools/inputmap`  
**Description**: Remove an input action.  
**Destructive**: Yes | **Undo**: Yes

### `add_input_event_binding`

**Category**: `editor_tools/inputmap`  
**Description**: Add a key/button binding to an action.  
**Destructive**: No | **Undo**: Yes

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `action_name` | `string` | Yes | Action name |
| `event_type` | `string` | Yes | Event type (key, mouse_button, joy_button, joy_axis) |
| `device` | `int` | No | Device ID |
| `keycode` | `string` | No | Key code (for key events) |
| `button_index` | `int` | No | Button index (for mouse/joy events) |

---

## Plugin Management (2)

### `list_plugins`

**Category**: `editor_tools/plugin`  
**Description**: List all plugins and their status.  
**Destructive**: No | **Undo**: No

### `set_plugin_enabled`

**Category**: `editor_tools/plugin`  
**Description**: Enable or disable a plugin.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `plugin_name` | `string` | Yes | Plugin name |
| `enabled` | `bool` | Yes | Enable or disable |

---

## Project Scaffolding (1)

### `create_project`

**Category**: `editor_tools/scaffold`  
**Description**: Create a new Godot project.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Project directory path |
| `project_name` | `string` | Yes | Project name |
| `renderer` | `string` | No | Renderer type |

---

## Project Graph Visualization (1)

### `get_project_graph`

**Category**: `editor_tools/visualizer`  
**Description**: Get the project dependency graph.  
**Destructive**: No | **Undo**: No