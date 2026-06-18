# Animation Tools

10 tools for creating and managing animations.

## `create_animation_player`

**Category**: `editor_tools/animation`  
**Description**: Create an AnimationPlayer node.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `parent_path` | `string` | Yes | - | Parent node path |
| `node_name` | `string` | No | "AnimationPlayer" | Node name |
| `autoplay` | `string` | No | - | Autoplay animation name |

---

## `create_animation_clip`

**Category**: `editor_tools/animation`  
**Description**: Create an animation clip.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `player_path` | `string` | Yes | - | AnimationPlayer path |
| `animation_name` | `string` | Yes | - | Animation name |
| `length` | `float` | No | 1.0 | Duration in seconds |

---

## `add_animation_track`

**Category**: `editor_tools/animation`  
**Description**: Add an animation track.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `player_path` | `string` | Yes | AnimationPlayer path |
| `animation_name` | `string` | Yes | Animation name |
| `track_type` | `string` | Yes | Track type (property, position, rotation, scale, method) |
| `node_path` | `string` | Yes | Target node path |
| `property` | `string` | Yes | Property name |

---

## `set_keyframe`

**Category**: `editor_tools/animation`  
**Description**: Set a keyframe value.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `player_path` | `string` | Yes | AnimationPlayer path |
| `animation_name` | `string` | Yes | Animation name |
| `track_idx` | `int` | Yes | Track index |
| `time` | `float` | Yes | Time in seconds |
| `value` | variant | Yes | Keyframe value |

---

## `get_animation_info`

**Category**: `editor_tools/animation`  
**Description**: Get animation info.  
**Destructive**: No | **Undo**: No | **Needs Scene**: Yes

---

## `create_animation_tree` / `get_animation_tree_info`

**Category**: `editor_tools/animation`  
**Description**: Create or query AnimationTree nodes.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

---

## `add_animation_node`

**Category**: `editor_tools/animation`  
**Description**: Add an animation node to the AnimationTree.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

---

## `add_transition`

**Category**: `editor_tools/animation`  
**Description**: Add a transition between animation nodes.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

---

## `set_transition_condition`

**Category**: `editor_tools/animation`  
**Description**: Set a transition condition value.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes