# Runtime Tools

13 tools for controlling the game runtime and bridging to the game process.

## Runtime Lifecycle (6)

### `run_project`

**Category**: `runtime_tools/lifecycle`  
**Description**: Run the project.  
**Destructive**: No | **Undo**: No

### `run_current_scene`

**Category**: `runtime_tools/lifecycle`  
**Description**: Run the current scene.  
**Destructive**: No | **Undo**: No

### `run_specific_scene`

**Category**: `runtime_tools/lifecycle`  
**Description**: Run a specific scene file.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `scene_path` | `string` | Yes | Path to the scene file |

### `stop_project`

**Category**: `runtime_tools/lifecycle`  
**Description**: Stop the running project.  
**Destructive**: No | **Undo**: No

### `pause_project`

**Category**: `runtime_tools/lifecycle`  
**Description**: Pause or resume the running project.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `paused` | `bool` | No | true = pause, false = resume |

### `set_movie_maker`

**Category**: `runtime_tools/lifecycle`  
**Description**: Toggle Movie Maker mode.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `enabled` | `bool` | Yes | Enable or disable movie maker |

---

## Runtime Bridge (7)

### `wait_for_bridge`

**Category**: `runtime_tools/bridge`  
**Description**: Wait for the runtime bridge connection.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `timeout_ms` | `int` | No | 5000 | Max wait time in ms |

### `get_game_scene_tree`

**Category**: `runtime_tools/bridge`  
**Description**: Get the game runtime scene tree.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `timeout_ms` | `int` | No | 5000 | Bridge timeout |

### `get_game_node_property`

**Category**: `runtime_tools/bridge`  
**Description**: Get a node property from the running game.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Node path in the game |
| `property` | `string` | Yes | Property name |
| `timeout_ms` | `int` | No | Bridge timeout (default: 5000) |

### `set_game_node_property`

**Category**: `runtime_tools/bridge`  
**Description**: Set a node property in the running game.  
**Destructive**: Yes | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Node path in the game |
| `property` | `string` | Yes | Property name |
| `value` | variant | Yes | New value |
| `timeout_ms` | `int` | No | Bridge timeout (default: 5000) |

### `call_method_in_game`

**Category**: `runtime_tools/bridge`  
**Description**: Call a method in the running game.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `node_path` | `string` | Yes | Node path in the game |
| `method` | `string` | Yes | Method name |
| `args` | `array` | No | Method arguments |
| `timeout_ms` | `int` | No | Bridge timeout (default: 5000) |

### `capture_game_screenshot`

**Category**: `runtime_tools/bridge`  
**Description**: Capture a screenshot from the running game.  
**Destructive**: No | **Undo**: No

### `simulate_game_input`

**Category**: `runtime_tools/bridge`  
**Description**: Simulate input in the running game.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `action` | `string` | Yes | Input action name |
| `pressed` | `bool` | Yes | Key pressed state |
| `timeout_ms` | `int` | No | Bridge timeout (default: 5000) |