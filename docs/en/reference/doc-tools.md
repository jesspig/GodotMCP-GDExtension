# Documentation Query Tools

8 tools for querying Godot's ClassDB documentation at runtime.

## `search_docs`

**Category**: `editor_tools/docs`  
**Description**: Search Godot documentation.  
**Destructive**: No | **Undo**: No

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `query` | `string` | Yes | Search query |
| `category` | `string` | No | Filter: "class", "method", "property", "signal", "constant" |

### Returns

```json
{
  "success": true,
  "data": {
    "results": [
      { "name": "Node2D", "type": "class", "description": "Base node for 2D..." },
      { "name": "position", "type": "property", "description": "Position in 2D space..." }
    ]
  }
}
```

---

## `get_class_info`

**Category**: `editor_tools/docs`  
**Description**: Get complete class information.  
**Destructive**: No | **Undo**: No

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `class_name` | `string` | Yes | Class name |

### Returns

```json
{
  "success": true,
  "data": {
    "name": "Sprite2D",
    "inherits": "Node2D",
    "inherits_chain": ["Node2D", "CanvasItem", "Node", "Object"],
    "description": "Sprite2D description from ClassDB...",
    "methods": [...],
    "properties": [...],
    "signals": [...],
    "constants": [...],
    "enums": [...]
  }
}
```

---

## `get_class_list`

**Category**: `editor_tools/docs`  
**Description**: List all Godot classes.  
**Destructive**: No | **Undo**: No

---

## `get_inheritance_chain`

**Category**: `editor_tools/docs`  
**Description**: Get class inheritance chain.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `class_name` | `string` | Yes | Class name |

---

## `get_property_doc`

**Category**: `editor_tools/docs`  
**Description**: Query property documentation.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `class_name` | `string` | Yes | Class name |
| `property_name` | `string` | Yes | Property name |

---

## `get_method_doc`

**Category**: `editor_tools/docs`  
**Description**: Query method documentation.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `class_name` | `string` | Yes | Class name |
| `method_name` | `string` | Yes | Method name |

---

## `get_enum_doc`

**Category**: `editor_tools/docs`  
**Description**: Query enum documentation.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `class_name` | `string` | Yes | Class name |
| `enum_name` | `string` | Yes | Enum name |

---

## `get_best_practices`

**Category**: `editor_tools/docs`  
**Description**: Get best practices advice for a class.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `class_name` | `string` | Yes | Class name |