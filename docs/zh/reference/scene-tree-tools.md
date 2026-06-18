# 场景树工具

24 个用于创建、删除和操作场景树节点的工具。

## `add_node`

**分类**: `editor_tools/scene_tree`  
**描述**: 创建新节点作为指定父节点的子节点。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|----------|---------|-------------|
| `parent_path` | `string` | 是 | - | 父节点路径 |
| `class_name` | `string` | 是 | - | 节点类名（如 "Node2D", "Sprite2D"） |
| `node_name` | `string` | 否 | 自动生成 | 新节点名称 |
| `index` | `int` | 否 | -1 | 子节点索引（-1 = 追加） |

### 返回值

```json
{ "success": true, "data": { "node_path": "/root/MyScene/NewNode" } }
```

### 示例

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"add_node","arguments":{"parent_path":"/root","class_name":"Sprite2D","node_name":"PlayerSprite"}}}'
```

---

## `delete_node`

**分类**: `editor_tools/scene_tree`  
**描述**: 删除节点。  
**破坏性**: 是 | **撤销**: 是 | **需要场景**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `node_path` | `string` | 是 | 要删除的节点路径 |

---

## `rename_node`

**分类**: `editor_tools/scene_tree`  
**描述**: 重命名节点。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `node_path` | `string` | 是 | 节点路径 |
| `new_name` | `string` | 是 | 新节点名称 |

---

## `move_node`

**分类**: `editor_tools/scene_tree`  
**描述**: 将节点移动到新的父节点下。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `node_path` | `string` | 是 | 要移动的节点路径 |
| `new_parent_path` | `string` | 是 | 新父节点路径 |
| `index` | `int` | 否 | 兄弟节点中的位置 |

---

## `duplicate_node`

**分类**: `editor_tools/scene_tree`  
**描述**: 复制节点。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `node_path` | `string` | 是 | 要复制的节点路径 |

---

## `copy_node` / `paste_node` / `cut_node`

**分类**: `editor_tools/scene_tree`  
**描述**: 节点的剪贴板操作。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

### 参数

`copy_node` 和 `cut_node`: `node_path`（string，必需）  
`paste_node`: `parent_path`（string，必需）

---

## `get_scene_tree`

**分类**: `editor_tools/scene_tree`  
**描述**: 获取当前场景的节点树结构。  
**破坏性**: 否 | **撤销**: 否 | **需要场景**: 是

### 返回值

包含节点路径、类型和子节点关系的完整层级树结构。

---

## `save_scene`

**分类**: `editor_tools/scene_tree`  
**描述**: 保存当前场景。  
**破坏性**: 否 | **撤销**: 否 | **需要场景**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `path` | `string` | 否 | 保存路径（默认为当前） |

---

## `change_node_type`

**分类**: `editor_tools/scene_tree`  
**描述**: 更改节点类型。  
**破坏性**: 是 | **撤销**: 是 | **需要场景**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `node_path` | `string` | 是 | 节点路径 |
| `new_type` | `string` | 是 | 新类名 |

---

## `attach_script` / `detach_script`

**分类**: `editor_tools/scene_tree`  
**描述**: 附加或分离节点的脚本。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

### 参数

`attach_script`: `node_path`（string，必需）, `script_path`（string，必需）  
`detach_script`: `node_path`（string，必需）

---

## `reparent_node` / `reparent_to_new_node`

**分类**: `editor_tools/scene_tree`  
**描述**: 重新设置父节点操作。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

---

## `group_selected_nodes`

**分类**: `editor_tools/scene_tree`  
**描述**: 将选中的节点分组到新父节点下。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

---

## `make_local`

**分类**: `editor_tools/scene_tree`  
**描述**: 将实例化场景节点转换为本地节点。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

---

## `save_branch_as_scene`

**分类**: `editor_tools/scene_tree`  
**描述**: 将节点分支保存为场景文件。  
**破坏性**: 否 | **撤销**: 否 | **需要场景**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `node_path` | `string` | 是 | 分支根节点 |
| `scene_path` | `string` | 是 | 场景保存路径 |

---

## `instance_child_scene`

**分类**: `editor_tools/scene_tree`  
**描述**: 从 `.tscn` 文件实例化子场景。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

---

## `set_unique_name`

**分类**: `editor_tools/scene_tree`  
**描述**: 设置节点唯一名称（% 前缀）。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

---

## `toggle_editable_children` / `toggle_edit_group` / `toggle_placeholder`

**分类**: `editor_tools/scene_tree`  
**描述**: 切换节点上的编辑器状态标志。  
**破坏性**: 否 | **撤销**: 是 | **需要场景**: 是

### 参数

| 名称 | 类型 | 必需 | 说明 |
|------|------|----------|-------------|
| `node_path` | `string` | 是 | 节点路径 |