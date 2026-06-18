# Scene Tree Tools

24 tools for creating, deleting, and manipulating scene tree nodes.

## `add_node`

**Category**: `editor_tools/scene_tree`  
**Description**: Create a new node as a child of the specified parent.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `parent_path` | `string` | Yes | - | Parent node path |
| `class_name` | `string` | Yes | - | Node class name (e.g., "Node2D", "Sprite2D") |
| `node_name` | `string` | No | auto-generated | Name for the new node |
| `index` | `int` | No | -1 | Child index (-1 = append) |

### Returns

```json
{ "success": true, "data": { "node_path": "/root/MyScene/NewNode" } }
```

### Example

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"add_node","arguments":{"parent_path":"/root","class_name":"Sprite2D","node_name":"PlayerSprite"}}}'
```

---

## `delete_node`

**Category**: `editor_tools/scene_tree`  
**Description**: Delete a node.  
**Destructive**: Yes | **Undo**: Yes | **Needs Scene**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Path to the node to delete |

---

## `rename_node`

**Category**: `editor_tools/scene_tree`  
**Description**: Rename a node.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Path to the node |
| `new_name` | `string` | Yes | New node name |

---

## `move_node`

**Category**: `editor_tools/scene_tree`  
**Description**: Move a node to a new parent.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Path to the node to move |
| `new_parent_path` | `string` | Yes | New parent path |
| `index` | `int` | No | Position among siblings |

---

## `duplicate_node`

**Category**: `editor_tools/scene_tree`  
**Description**: Duplicate a node.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Path to the node to duplicate |

---

## `copy_node` / `paste_node` / `cut_node`

**Category**: `editor_tools/scene_tree`  
**Description**: Clipboard operations for nodes.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

### Parameters

`copy_node` and `cut_node`: `node_path` (string, required)  
`paste_node`: `parent_path` (string, required)

---

## `get_scene_tree`

**Category**: `editor_tools/scene_tree`  
**Description**: Get the current scene node tree structure.  
**Destructive**: No | **Undo**: No | **Needs Scene**: Yes

### Returns

Complete hierarchical tree of nodes with paths, types, and child relationships.

---

## `save_scene`

**Category**: `editor_tools/scene_tree`  
**Description**: Save the current scene.  
**Destructive**: No | **Undo**: No | **Needs Scene**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | No | Save path (defaults to current) |

---

## `change_node_type`

**Category**: `editor_tools/scene_tree`  
**Description**: Change a node's type.  
**Destructive**: Yes | **Undo**: Yes | **Needs Scene**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Path to the node |
| `new_type` | `string` | Yes | New class name |

---

## `attach_script` / `detach_script`

**Category**: `editor_tools/scene_tree`  
**Description**: Attach or detach a script from a node.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

### Parameters

`attach_script`: `node_path` (string, required), `script_path` (string, required)  
`detach_script`: `node_path` (string, required)

---

## `reparent_node` / `reparent_to_new_node`

**Category**: `editor_tools/scene_tree`  
**Description**: Reparent operations.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

---

## `group_selected_nodes`

**Category**: `editor_tools/scene_tree`  
**Description**: Group selected nodes under a new parent.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

---

## `make_local`

**Category**: `editor_tools/scene_tree`  
**Description**: Make an instanced scene node local.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

---

## `save_branch_as_scene`

**Category**: `editor_tools/scene_tree`  
**Description**: Save a node branch as a scene file.  
**Destructive**: No | **Undo**: No | **Needs Scene**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Root of the branch |
| `scene_path` | `string` | Yes | Path to save the scene |

---

## `instance_child_scene`

**Category**: `editor_tools/scene_tree`  
**Description**: Instance a child scene from a `.tscn` file.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

---

## `set_unique_name`

**Category**: `editor_tools/scene_tree`  
**Description**: Set node unique name (% prefix).  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

---

## `toggle_editable_children` / `toggle_edit_group` / `toggle_placeholder`

**Category**: `editor_tools/scene_tree`  
**Description**: Toggle editor state flags on nodes.  
**Destructive**: No | **Undo**: Yes | **Needs Scene**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Path to the node |