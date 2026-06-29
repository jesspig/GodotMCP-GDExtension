# Meta Tools

9 tools for tool discovery, introspection, and configuration.

## `get_info`

**Category**: `meta_tools`
**Description**: Return connection status and engine/plugin/project/editor information.
**Destructive**: No | **Undo**: No | **Meta**: Yes

### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `include_configs` | boolean | No | false | If true, include client configuration snippets |

### Returns

Dictionary with connection, engine, plugin, project, editor, and bridge status. When `include_configs` is true, also returns `client_configs`.

### Example

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"get_info","arguments":{}}}'
```

---

## `get_tools`

**Category**: `meta_tools`
**Description**: Get a tool's full schema by name, or list all tools.
**Destructive**: No | **Undo**: No | **Meta**: Yes

### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `name` | string | No | "" | Tool name (e.g. add_node, get_game_scene_tree). Returns full detail for that tool. |

### Returns

Without `name`: dictionary with `tools` array and `count`. With `name`: full tool schema including parameters, return value, and usage example.

### Example

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"get_tools","arguments":{}}}'
```

---

## `get_categories`

**Category**: `meta_tools`
**Description**: List tool categories as a tree, supporting path drilling and depth control.
**Destructive**: No | **Undo**: No | **Meta**: Yes

### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `path` | string | No | "" | Category path, empty starts from root. E.g. node_tools/property/Node/CanvasItem |
| `max_depth` | integer | No | 3 | Maximum depth (default 3, -1 for unlimited) |

### Returns

Dictionary with `categories` array containing nested category tree nodes.

---

## `find_tool`

**Category**: `meta_tools`
**Description**: Search for tools by name, keyword, or description.
**Destructive**: No | **Undo**: No | **Meta**: Yes

### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `query` | string | Yes | — | Search query — supports name, keyword, or partial text |
| `category` | string | No | "" | Optional category filter (e.g. meta_tools, node_tools, editor_tools) |
| `limit` | integer | No | 20 | Maximum results to return |

### Returns

Dictionary with `query`, `results` array, and `count`.

---

## `call_tool`

**Category**: `meta_tools`
**Description**: Fallback to call any registered tool (not recommended for direct use).
**Destructive**: Depends | **Undo**: Depends | **Meta**: Yes

### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `tool_name` | string | Yes | — | Name of the tool to call |
| `arguments` | object | No | {} | Tool arguments (optional key-value pairs) |

---

## `undo`

**Category**: `meta_tools`
**Description**: Undo the last reversible tool operation.
**Destructive**: No | **Undo**: No | **Meta**: Yes

### Parameters

None.

### Returns

Dictionary with `undone` (tool name), `description`, `remaining_undos`, `redo_available`.

### Example

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"undo","arguments":{}}}'
```

---

## `redo`

**Category**: `meta_tools`
**Description**: Redo the last undone operation.
**Destructive**: No | **Undo**: No | **Meta**: Yes

### Parameters

None.

### Returns

Dictionary with `redone` (tool name), `description`, `remaining_redos`, `undo_available`.

### Example

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"redo","arguments":{}}}'
```

---

## `get_undo_history`

**Category**: `meta_tools`
**Description**: Return undo/redo stack contents.
**Destructive**: No | **Undo**: No | **Meta**: Yes

### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `max_items` | integer | No | 50 | Maximum items per stack |

### Returns

Dictionary with `undo_stack`, `redo_stack`, `undo_count`, `redo_count`, `max_steps`, `can_undo`, `can_redo`.

---

## `execute_workflow`

**Category**: `meta_tools`
**Description**: Execute a multi-step workflow.
**Destructive**: Depends | **Undo**: Depends | **Meta**: Yes

### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `workflow_json` | object | No | — | Workflow definition as JSON object (stages, steps, vars, timeout) |
| `workflow_yaml` | string | No | — | Workflow definition as YAML string |
| `dry_run` | boolean | No | false | Validate workflow without executing |

### Returns

On success: workflow execution results. On dry run: `valid` flag and `total_steps` count.
