# Resource & Fallback Tools

8 tools for managing resources and generic property access.

## Resource Operations (6)

### `save_resource`

**Category**: `node_tools/general`  
**Description**: Save a resource to file.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Resource path |
| `resource_path` | `string` | Yes | Source resource path |

### `load_resource`

**Category**: `node_tools/general`  
**Description**: Load a resource from path.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Path to load |

### `new_resource`

**Category**: `node_tools/general`  
**Description**: Create a new resource.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `type` | `string` | Yes | Resource class name |

### `duplicate_resource`

**Category**: `node_tools/general`  
**Description**: Duplicate a resource.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Resource path to duplicate |

### `clear_resource`

**Category**: `node_tools/general`  
**Description**: Clear a node resource reference.  
**Destructive**: Yes | **Undo**: Yes | **Needs Node**: Yes

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Node path |
| `property` | `string` | Yes | Resource property name |

### `get_resource_info`

**Category**: `node_tools/general`  
**Description**: Get resource information.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Resource path |

---

## Property Fallback (2)

### `get_node_property`

**Category**: `node_tools/fallback`  
**Description**: Get any node property value.  
**Destructive**: No | **Undo**: No | **Needs Node**: Yes

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Node path |
| `property` | `string` | Yes | Property name |

### `set_node_property`

**Category**: `node_tools/fallback`  
**Description**: Set any node property value.  
**Destructive**: Yes | **Undo**: Yes | **Needs Node**: Yes

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Node path |
| `property` | `string` | Yes | Property name |
| `value` | variant | Yes | New value (any type) |