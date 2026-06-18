# 控件与碰撞工具

5 个用于创建 UI 控件和碰撞形状的工具。

## UI 控件（4 个）

### `create_control`

**分类**: `editor_tools/control`  
**描述**: 创建 UI 控件节点。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

#### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|----------|---------|-------------|
| `parent_path` | `string` | 是 | - | 父节点路径 |
| `control_type` | `string` | 是 | - | 控件类（Button, Label, Panel 等） |
| `node_name` | `string` | 否 | auto | 节点名称 |

### `create_stylebox`

**分类**: `editor_tools/control`  
**描述**: 创建 StyleBox 资源。  
**破坏性**: 否 | **撤销**: 否

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `type` | `string` | 否 | StyleBox 类型（默认: StyleBoxFlat） |
| `bg_color` | `string` | 否 | 背景颜色（十六进制） |
| `border_width` | `int` | 否 | 边框宽度 |
| `corner_radius` | `int` | 否 | 圆角半径 |

### `set_layout_preset`

**分类**: `editor_tools/control`  
**描述**: 设置控件布局预设。  
**破坏性**: 否 | **撤销**: 是 | **需要节点**: 是

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `node_path` | `string` | 是 | 控件节点路径 |
| `preset` | `string` | 是 | 布局预设名称 |

### `set_theme_override`

**分类**: `editor_tools/control`  
**描述**: 设置控件的主题覆盖。  
**破坏性**: 否 | **撤销**: 是 | **需要节点**: 是

#### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `node_path` | `string` | 是 | 控件节点路径 |
| `theme_type` | `string` | 是 | 主题项类型 |
| `theme_property` | `string` | 是 | 主题属性名称 |
| `value` | variant | 是 | 主题值 |

---

## 碰撞形状（1 个）

### `create_collision_shape`

**分类**: `editor_tools/collision`  
**描述**: 创建碰撞形状（2D 或 3D）。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

#### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|----------|---------|-------------|
| `parent_path` | `string` | 是 | - | 父节点路径（Area2D, StaticBody2D 等） |
| `shape_type` | `string` | 否 | "RectangleShape2D" | 形状类型 |
| `node_name` | `string` | 否 | "CollisionShape2D" | 节点名称 |