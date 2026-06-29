# 元工具

9 个用于工具发现、内省和编排的元工具。这些是 `tools/list` 返回的唯一工具（`is_meta() == true`），其余 155 个工具通过发现链按需加载。

## `get_info`

**分类**: `meta_tools`
**描述**: 获取编辑器连接状态和运行时环境信息。
**破坏性**: 否 | **撤销**: 否 | **元工具**: 是

### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `include_configs` | `boolean` | 否 | `false` | 若为 true，则一并返回客户端配置片段 |

### 返回值

```json
{
  "success": true,
  "data": {
    "connection": { "status": "ok" },
    "engine": { "version": "4.6", "hash": "abc123" },
    "plugin": { "version": "0.2.2", "builtin_tools": 164, "custom_tools": 0 },
    "project": { "name": "My Game", "path": "C:/Projects/MyGame/", "main_scene": "res://main.tscn" },
    "editor": { "current_scene": "res://main.tscn", "is_playing": false, "open_scenes": ["res://main.tscn"] },
    "bridge": { "server_status": 1, "server_port": 9601, "server_listening": true, "instance_count": 0 }
  }
}
```

### 示例

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"get_info","arguments":{}}}'
```

---

## `get_tools`

**分类**: `meta_tools`
**描述**: 按名称获取工具的完整 schema，或列出所有已注册工具。
**破坏性**: 否 | **撤销**: 否 | **元工具**: 是

### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `name` | `string` | 否 | — | 工具名称（如 `add_node`），提供时返回该工具的完整元数据（参数、类型、描述、使用示例） |

### 返回值

无 `name` 时返回工具简要列表（id, name, description, category）；有 `name` 时返回该工具的完整 schema（id, name, description, parameters, required, return_value, category_path, usage_example）。

### 示例

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"get_tools","arguments":{"name":"add_node"}}}'
```

---

## `get_categories`

**分类**: `meta_tools`
**描述**: 列出工具分类树。
**破坏性**: 否 | **撤销**: 否 | **元工具**: 是

### 参数

无。

### 返回值

嵌套的分类树结构，按 category() 返回值中的 `/` 分割自动建树（如 `editor_tools/scene_tree`）。

---

## `find_tool`

**分类**: `meta_tools`
**描述**: 全文搜索工具。支持精确名称匹配、前缀匹配、分词搜索和全文描述搜索，结果按相关性（精确 > 前缀 > 分词 > 全文）和使用频率排序。
**破坏性**: 否 | **撤销**: 否 | **元工具**: 是

### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `query` | `string` | 是 | — | 搜索查询，支持名称、关键字或部分文本 |
| `category` | `string` | 否 | — | 按分类筛选（如 `meta_tools`、`node_tools`） |
| `limit` | `integer` | 否 | `20` | 最大结果数 |

### 返回值

```json
{
  "query": "add_node",
  "count": 1,
  "results": [
    { "name": "add_node", "category": "editor_tools/scene_tree", "brief": "添加节点到场景", "usage_frequency": 42 }
  ]
}
```

---

## `call_tool`

**分类**: `meta_tools`
**描述**: 按名称调用任意已注册工具。AI 客户端应优先使用原生工具调用，仅在直接调用不可用时才使用此回退。
**破坏性**: 否 | **撤销**: 否 | **元工具**: 是

### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `tool_name` | `string` | 是 | — | 要调用的工具名称 |
| `arguments` | `object` | 否 | `{}` | 工具参数键值对 |

### 返回值

对应工具的执行结果。

---

## `undo`

**分类**: `meta_tools`
**描述**: 撤销上一个可逆工具操作。通过重放其反向参数来逆转上一次工具调用，已撤销操作移至重做栈。
**破坏性**: 否 | **撤销**: — | **元工具**: 是

### 参数

无。

### 返回值

```json
{
  "undone": "add_node",
  "description": "添加 Node2D 子节点",
  "remaining_undos": 3,
  "redo_available": true
}
```

---

## `redo`

**分类**: `meta_tools`
**描述**: 重做上一个已撤销的操作。通过重放其正向参数重新应用最后一次撤销的操作。
**破坏性**: 否 | **撤销**: — | **元工具**: 是

### 参数

无。

### 返回值

```json
{
  "redone": "add_node",
  "description": "添加 Node2D 子节点",
  "remaining_redos": 2,
  "undo_available": true
}
```

---

## `get_undo_history`

**分类**: `meta_tools`
**描述**: 返回撤销/重做栈内容，不修改栈状态。用于在执行撤销前查看哪些操作可逆。
**破坏性**: 否 | **撤销**: 否 | **元工具**: 是

### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `max_items` | `integer` | 否 | `50` | 每个栈返回的最大条目数 |

### 返回值

```json
{
  "undo_count": 3,
  "redo_count": 1,
  "max_steps": 100,
  "can_undo": true,
  "can_redo": true,
  "undo_stack": [
    { "tool_name": "add_node", "description": "添加 Node2D 子节点", "timestamp": 1234567890 }
  ],
  "redo_stack": [
    { "tool_name": "delete_node", "description": "删除 Node2D 子节点", "timestamp": 1234567880 }
  ]
}
```

---

## `execute_workflow`

**分类**: `meta_tools`
**描述**: 执行多步骤工作流。接受 workflow_json（对象）或 workflow_yaml（字符串）。支持 `${vars.*}` 变量替换和 `${steps.<id>.result.<path>}` 步骤结果链式引用。设置 `dry_run=true` 可仅验证不执行。
**破坏性**: 否 | **撤销**: 否 | **元工具**: 是

### 参数

| 名称 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `workflow_json` | `object` | 否 | — | JSON 格式工作流定义（stages、steps、vars、timeout） |
| `workflow_yaml` | `string` | 否 | — | YAML 格式工作流定义 |
| `dry_run` | `boolean` | 否 | `false` | 仅验证工作流结构，不实际执行 |
