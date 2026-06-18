# Workspace & Debugger Tools

13 tools for controlling the editor workspace, debugger, console, and breakpoints.

## Viewport Capture

### `capture_viewport`

**Category**: `editor_tools/workspace`  
**Description**: Capture the editor viewport as a PNG.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `path` | `string` | No | temp | Save path for the screenshot |

#### Returns

```json
{ "success": true, "data": { "path": "res://.godot/mcp/screenshot_1234.png" } }
```

### `capture_game_viewport`

**Category**: `editor_tools/workspace`  
**Description**: Capture the game viewport while running.  
**Destructive**: No | **Undo**: No

---

## Console

### `clear_console`

**Category**: `editor_tools/workspace`  
**Description**: Clear the editor console output.  
**Destructive**: No | **Undo**: No

### `get_console_output`

**Category**: `editor_tools/workspace`  
**Description**: Get console output with optional filtering.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Default | Description |
|------|------|----------|---------|-------------|
| `filter` | `string` | No | - | Filter: "error", "warning", "all" |
| `max_lines` | `int` | No | 100 | Max lines to return |

---

## Debugger Control

### `get_debugger_state`

**Category**: `editor_tools/workspace`  
**Description**: Get debugger state and error list.  
**Destructive**: No | **Undo**: No

### `get_performance_monitors`

**Category**: `editor_tools/workspace`  
**Description**: Get performance monitor data.  
**Destructive**: No | **Undo**: No

#### Returns

```json
{
  "success": true,
  "data": {
    "fps": 60,
    "memory": { "static": 52428800, "dynamic": 10485760 },
    "objects": { "total": 1500, "nodes": 200 },
    "physics": { "fps": 60 },
    "render": { "draw_calls": 100 }
  }
}
```

### `get_stack_trace`

**Category**: `editor_tools/workspace`  
**Description**: Get current stack trace.  
**Destructive**: No | **Undo**: No

### `get_locals`

**Category**: `editor_tools/workspace`  
**Description**: Get local variables at breakpoint.  
**Destructive**: No | **Undo**: No

### `debugger_control`

**Category**: `editor_tools/workspace`  
**Description**: Control the debugger.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `action` | `string` | Yes | Action: "break", "continue", "step_into", "step_over", "step_out" |

---

## Breakpoints

### `list_breakpoints`

**Category**: `editor_tools/workspace`  
**Description**: List all breakpoints.  
**Destructive**: No | **Undo**: No

### `set_breakpoint`

**Category**: `editor_tools/workspace`  
**Description**: Set a breakpoint.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `path` | `string` | Yes | Script path |
| `line` | `int` | Yes | Line number |

### `remove_breakpoint`

**Category**: `editor_tools/workspace`  
**Description**: Remove a breakpoint.  
**Destructive**: No | **Undo**: No

---

## Workspace Switching

### `set_workspace`

**Category**: `editor_tools/workspace`  
**Description**: Switch the editor workspace.  
**Destructive**: No | **Undo**: No

#### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `workspace` | `string` | Yes | Workspace: "2D", "3D", "Script", "AssetLib" |