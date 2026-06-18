# Control & Collision Tools

5 tools for creating UI controls and collision shapes.

## UI Controls (4)

### `create_control`

**Category**: `editor_tools/control`  
**Description**: Create a UI control node.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

#### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `parent_path` | `string` | Yes | - | Parent node path |
| `control_type` | `string` | Yes | - | Control class (Button, Label, Panel, etc.) |
| `node_name` | `string` | No | auto | Node name |

### `create_stylebox`

**Category**: `editor_tools/control`  
**Description**: Create a StyleBox resource.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `type` | `string` | No | StyleBox type (default: StyleBoxFlat) |
| `bg_color` | `string` | No | Background color (hex) |
| `border_width` | `int` | No | Border width |
| `corner_radius` | `int` | No | Corner radius |

### `set_layout_preset`

**Category**: `editor_tools/control`  
**Description**: Set a control layout preset.  
**Destructive**: No | **Undo**: Yes | **Needs Node**: Yes

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Control node path |
| `preset` | `string` | Yes | Layout preset name |

### `set_theme_override`

**Category**: `editor_tools/control`  
**Description**: Set a theme override on a control.  
**Destructive**: No | **Undo**: Yes | **Needs Node**: Yes

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Control node path |
| `theme_type` | `string` | Yes | Theme item type |
| `theme_property` | `string` | Yes | Theme property name |
| `value` | variant | Yes | Theme value |

---

## Collision Shapes (1)

### `create_collision_shape`

**Category**: `editor_tools/collision`  
**Description**: Create a collision shape (2D or 3D).  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

#### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `parent_path` | `string` | Yes | - | Parent node path (Area2D, StaticBody2D, etc.) |
| `shape_type` | `string` | No | "RectangleShape2D" | Shape type |
| `node_name` | `string` | No | "CollisionShape2D" | Node name |