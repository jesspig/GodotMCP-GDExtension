# Signal & Group Tools

8 tools for managing node signals and groups.

## Signal Management (4)

### `connect_signal`

**Category**: `node_tools/signal`  
**Description**: Connect a node signal to a method.  
**Destructive**: No | **Undo**: No | **Needs Node**: Yes

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Node path |
| `signal_name` | `string` | Yes | Signal name |
| `target_path` | `string` | Yes | Target node path |
| `method_name` | `string` | Yes | Target method name |
| `flags` | `int` | No | Connect flags |

### `disconnect_signal`

**Category**: `node_tools/signal`  
**Description**: Disconnect a node signal.  
**Destructive**: No | **Undo**: No | **Needs Node**: Yes

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Node path |
| `signal_name` | `string` | Yes | Signal name |
| `target_path` | `string` | Yes | Target node path |

### `list_signals`

**Category**: `node_tools/signal`  
**Description**: List all signals of a node.  
**Destructive**: No | **Undo**: No | **Needs Node**: Yes

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Node path |

### `get_signal_connections`

**Category**: `node_tools/signal`  
**Description**: Get signal connection list.  
**Destructive**: No | **Undo**: No | **Needs Node**: Yes

---

## Node Groups (4)

### `add_to_group`

**Category**: `node_tools/group`  
**Description**: Add a node to a group.  
**Destructive**: No | **Undo**: Yes | **Needs Node**: Yes

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Node path |
| `group_name` | `string` | Yes | Group name |

### `remove_from_group`

**Category**: `node_tools/group`  
**Description**: Remove a node from a group.  
**Destructive**: No | **Undo**: Yes | **Needs Node**: Yes

### `get_node_groups`

**Category**: `node_tools/group`  
**Description**: Get the groups a node belongs to.  
**Destructive**: No | **Undo**: No | **Needs Node**: Yes

### `get_nodes_in_group`

**Category**: `node_tools/group`  
**Description**: Get all nodes in a group.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `group_name` | `string` | Yes | Group name |