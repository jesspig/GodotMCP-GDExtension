# `crates/gdext` — GDExtension CDyLib

> Native plugin loaded into the Godot editor. Contains all 99 tool command implementations.

```mermaid
flowchart LR
    subgraph Godot["Godot Editor Process"]
        subgraph DLL["godot_mcp_gdext.dll"]
            EP["editor_plugin.rs<br/>McpEditorPlugin"]
            IP["ipc/ws_server.rs<br/>IpcWebSocketServer"]
            DISP["dispatcher.rs<br/>MainThreadDispatcher"]
            LOG["logging.rs"]
            CMDS["commands/ (*.rs)"]
            LSP["lsp/"]
            DOCK["dock/"]
        end
    end
    
    subgraph Server["godot-mcp-server.exe"]
        WS["WebSocket Client"]
    end
    
    Server <-->|tool_call IPC| IP
    IP --> DISP
    DISP --> CMDS
    CMDS -.->|Godot API| EP
    LOG -.->|process_frame pump| EP
    EP -.->|Godot signals| DISP
    EP -.->|Godot signals| LOG
    
    DOCK ---|EditorInterface.get_editor_main_screen().add_control_to_dock| EP
    LSP ---|EditorInterface| EP
```

## Files

### `lib.rs`

- `#[gdextension]` entry point
- No init logic — all initialization in `EditorPlugin`

### `editor_plugin.rs`

See [Editor Plugin](../modules/editor-plugin.md) page. Key points:

- `McpEditorPlugin::_enter_tree()`: initialize statics, start tokio runtime, create WebSocket server, connect `process_frame` signal
- `McpEditorPlugin::_exit_tree()`: close WebSocket, stop tokio runtime, clean statics
- `McpEditorPlugin::_process()`: **intentionally empty** (avoids `bind_mut` deadlock)

## Directory Map

| Directory | Content | Tool Count |
|-----------|---------|------------|
| `commands/meta.rs` | ping, version queries | 4 |
| `commands/node.rs` | Node CRUD, scene tree, group mgmt | 17 |
| `commands/property.rs` | 2D property get/set | 19 |
| `commands/property_3d.rs` | 3D property get/set | 6 |
| `commands/scene.rs` | Scene file + editor tab ops | 15 |
| `commands/collision.rs` | Collision shape addition | 2 |
| `commands/find.rs` | Node search | 4 |
| `commands/script_helpers.rs` | call_method, get/set_variable | 3 |
| `commands/project_settings.rs` | Project settings R/W | 3 |
| `commands/script_gd.rs` | GDScript file ops | 5 |
| `commands/script_cs.rs` | C# file ops + Solution gen | 6 |
| `commands/search.rs` | File search & replace | 3 |
| `commands/undo.rs` | Undo/redo | 2 |
| `commands/mod.rs` | CommandHandler trait + shared utils | - |
| `ipc/ws_server.rs` | WebSocket server, routing | - |
| `ipc/plugin_state.rs` | PluginState static | - |
| `lsp/` | GDScript LSP validation | - |
| `dock/` | Editor right-dock | - |

## Tool Registration (vs server side)

`commands/mod.rs` has `create_registry()` (8 groups for tool name discovery):

```
MetaCommands        → ["ping", "get_engine_version", "get_plugin_version", "get_server_version"]
NodeCommands        → 11 node ops + 6 convenience
PropertyCommands    → 19 2D properties
Property3dCommands  → 6 3D properties
CollisionCommands   → 2
FindCommands        → 4
ScriptGdCommands    → 5
ScriptCsCommands    → 6
```

**Note**: `create_registry()` has 8 groups while `route_tool_call` has 13 routing branches — the latter includes `SceneCommands`, `ScriptHelpersCommands`, `SearchCommands`, `UndoCommands`, `ProjectSettingsCommands` that don't appear in `create_registry()`. **Both must stay in sync.**

## Shared Utilities (`commands/mod.rs`)

| Function | Description |
|----------|-------------|
| `j2v(v: &Value) -> Variant` | JSON → Godot Variant (handles Vector2/3/4, Color, Rect2, Quaternion, Resource) |
| `v2j(v: &Variant) -> Value` | Godot Variant → JSON |
| `resolve_node(root: &Node, path: &str) -> Option<Gd<Node>>` | Node path resolver |
| `pipe(result: Value) -> Result<Value, String>` | `json!({"error":"..."})` → `Err` |
| `try_load<T: GodotClass>(path: &str) -> Result<Gd<T>, String>` | Safe resource loading |
| `get_current_scene_root() -> Gd<Node>` | Get current edited scene root node |
| `with_editor_interface(f)` | EditorInterface accessor macro |
| `make_path(value: &str) -> String` | Path normalization (adds `res://` prefix) |
