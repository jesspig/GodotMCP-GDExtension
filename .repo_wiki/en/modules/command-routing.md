# Command Routing

## Full Call Chain

```
AI Client call_tool("get_node_position", {"node_path": "Player"})
  │
  ▼
godot-mcp-server / handler.rs
  ├─ match "godot_editor_*" → server-side (not forwarded)
  ├─ else → forward_tool_call(name, args)
  │
  ▼ (WebSocket :9500)
godot_mcp_gdext / ws_server.rs → route_tool_call()
  │
  ├─ can_handle("get_node_position") → PropertyCommands → no
  ├─ can_handle("get_node_position") → NodeCommands → no
  ├─ can_handle("get_node_position") → SceneCommands → no
  ├─ ... (13 groups checked in order)
  ├─ can_handle("get_node_position") → NodeConvenience → yes!
  │   │
  │   ▼
  │   dispatcher.submit(move || cmd_get_node_position(args)).await
  │   │
  │   ▼
  │   (main thread process_frame pump)
  │   cmd_get_node_position() → EditorInterface → Node API
  │   │
  │   ▼ (return value)
  │   WebSocket IPC response
  │
  ▼
godot-mcp-server → JSON-RPC response
  │
  ▼
AI Client {"result": {"x": 100, "y": 200}}
```

## Routing Chain (`ws_server.rs`)

```rust
// Check order (13 groups)
MetaCommands::can_handle(name)
    || NodeCommands::can_handle(name)
    || PropertyCommands::can_handle(name)
    || CollisionCommands::can_handle(name)
    || FindCommands::can_handle(name)
    || ScriptHelpersCommands::can_handle(name)
    || ProjectSettingsCommands::can_handle(name)
    || SceneCommands::can_handle(name)
    || ScriptGdCommands::can_handle(name)
    || ScriptCsCommands::can_handle(name)
    || SearchCommands::can_handle(name)
    || UndoCommands::can_handle(name)
    || Property3dCommands::can_handle(name)
    || NodeConvenience::can_handle(name)
    || SceneInfo::can_handle(name)
```

**Note**: `NodeConvenience` (4 tools) and `SceneInfo` (1 tool) are extra routing branches in `route_tool_call`, not separate `CommandHandler`s.

## Adding a Tool: Checklist

### Server Side (`crates/server/`)

| File | Change |
|------|--------|
| `src/tool_registry.rs` | Add tool's JSON Schema in `register_defaults()` |
| `src/handler.rs` | If server-side tool (`godot_editor_*`), add handler branch |
| Tests | Update `total == 99` assertion in `tool_registry.rs` |
| Tests | Update `total == 99` assertion in `handler.rs` |

### GDExt Side (`crates/gdext/`)

| File | Change |
|------|--------|
| `src/commands/xx.rs` | Implement `cmd_your_tool()` + `YourToolHandler::can_handle()` |
| `src/commands/mod.rs` | Add `YourToolHandler` to `create_registry()` return list (if new group) |
| `src/ipc/ws_server.rs` | Add `YourToolHandler::can_handle()` branch in `route_tool_call()` chain |

### Existing Group vs New Group

- **Existing group**: just add `can_handle` and `handle` methods; register route if not yet registered
- **New group**: must register in both `create_registry()` and `route_tool_call()`

## Tests

- `handler.rs` asserts `total == 99` (server-side count)
- `tool_registry.rs` asserts `total == 99` (registry count)
- Offline tests (no Godot): can't test real tool calls, but can test schema registration and tool list query

## Notes

- Both counts **must be updated** when adding/removing tools, otherwise tests fail
- `create_registry()` and `route_tool_call()` groups **must stay in sync**, otherwise gdext returns "unknown tool" errors
