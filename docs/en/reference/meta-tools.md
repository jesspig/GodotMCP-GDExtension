# Meta Tools

8 tools for tool discovery, introspection, and configuration.

## `get_info`

**Category**: `meta_tools`  
**Description**: Get editor connection status and runtime environment info.  
**Destructive**: No | **Undo**: No | **Meta**: Yes

### Parameters

None.

### Returns

```json
{
  "success": true,
  "data": {
    "connection": { "status": "ok" },
    "engine": { "version": "4.6", "major": 4, "minor": 6 },
    "plugin": { "version": "0.2.2-dev1", "mode": "editor" },
    "project": { "name": "My Game", "path": "res://" },
    "editor": { "scene_open": true, "node_count": 5 }
  }
}
```

### Example

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"get_info","arguments":{}}}'
```

---

## `get_tools`

**Category**: `meta_tools`  
**Description**: List all registered tools.  
**Destructive**: No | **Undo**: No | **Meta**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `category` | `string` | No | Filter by category path |
| `query` | `string` | No | Search query filter |

### Returns

Array of tool metadata (name, category, brief, input_schema).

### Example

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"get_tools","arguments":{"category":"editor_tools/scene_tree"}}}'
```

---

## `get_categories`

**Category**: `meta_tools`  
**Description**: List tool category tree.  
**Destructive**: No | **Undo**: No | **Meta**: Yes

### Parameters

None.

### Returns

Nested category tree structure.

---

## `get_tool_detail`

**Category**: `meta_tools`  
**Description**: Get full metadata for a specific tool.  
**Destructive**: No | **Undo**: No | **Meta**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `name` | `string` | Yes | Tool name |

### Returns

Full tool metadata including input_schema and description.

---

## `find_tool`

**Category**: `meta_tools`  
**Description**: Full-text tool search.  
**Destructive**: No | **Undo**: No | **Meta**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `query` | `string` | Yes | Search query |
| `category` | `string` | No | Filter by category |
| `limit` | `int` | No | Max results (default: 20) |

---

## `call_tool`

**Category**: `meta_tools`  
**Description**: Call any tool by name.  
**Destructive**: No | **Undo**: No | **Meta**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `name` | `string` | Yes | Tool name to call |
| `arguments` | `object` | Yes | Tool arguments |

---

## `generate_client_config`

**Category**: `meta_tools`  
**Description**: Generate MCP client configuration files.  
**Destructive**: No | **Undo**: No | **Meta**: Yes

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `client_type` | `string` | No | Client type (opencode, cursor, cline, etc.) |
| `output_path` | `string` | No | Custom output path |