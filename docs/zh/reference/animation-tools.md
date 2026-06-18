# 动画工具

10 个用于创建和管理动画的工具。

## `create_animation_player`

**分类**: `editor_tools/animation`  
**描述**: 创建 AnimationPlayer 节点。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|----------|---------|-------------|
| `parent_path` | `string` | 是 | - | 父节点路径 |
| `node_name` | `string` | 否 | "AnimationPlayer" | 节点名称 |
| `autoplay` | `string` | 否 | - | 自动播放的动画名称 |

---

## `create_animation_clip`

**分类**: `editor_tools/animation`  
**描述**: 创建动画剪辑。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|----------|---------|-------------|
| `player_path` | `string` | 是 | - | AnimationPlayer 路径 |
| `animation_name` | `string` | 是 | - | 动画名称 |
| `length` | `float` | 否 | 1.0 | 持续时间（秒） |

---

## `add_animation_track`

**分类**: `editor_tools/animation`  
**描述**: 添加动画轨道。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `player_path` | `string` | 是 | AnimationPlayer 路径 |
| `animation_name` | `string` | 是 | 动画名称 |
| `track_type` | `string` | 是 | 轨道类型（property, position, rotation, scale, method） |
| `node_path` | `string` | 是 | 目标节点路径 |
| `property` | `string` | 是 | 属性名称 |

---

## `set_keyframe`

**分类**: `editor_tools/animation`  
**描述**: 设置关键帧值。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `player_path` | `string` | 是 | AnimationPlayer 路径 |
| `animation_name` | `string` | 是 | 动画名称 |
| `track_idx` | `int` | 是 | 轨道索引 |
| `time` | `float` | 是 | 时间（秒） |
| `value` | variant | 是 | 关键帧值 |

---

## `get_animation_info`

**分类**: `editor_tools/animation`  
**描述**: 获取动画信息。  
**破坏性**: 否 | **撤销**: 否 | **需要场景**: 是

---

## `create_animation_tree` / `get_animation_tree_info`

**分类**: `editor_tools/animation`  
**描述**: 创建或查询 AnimationTree 节点。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

---

## `add_animation_node`

**分类**: `editor_tools/animation`  
**描述**: 向 AnimationTree 添加动画节点。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

---

## `add_transition`

**分类**: `editor_tools/animation`  
**描述**: 添加动画节点间的过渡。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

---

## `set_transition_condition`

**分类**: `editor_tools/animation`  
**描述**: 设置过渡条件值。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是