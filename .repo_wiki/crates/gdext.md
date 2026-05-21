# `godot-mcp-gdext`

> `cdylib` loaded by the Godot editor as a GDExtension `EditorPlugin`. Hosts the WebSocket server, runs every tool, and owns the right-dock UI.

## Module map

```
crates/gdext/src/
├── lib.rs              # #[gdextension] entry; declares all submodules
├── editor_plugin.rs    # McpEditorPlugin (enter_tree, exit_tree, pump install/uninstall)
├── dispatcher.rs       # MainThreadDispatcher (closure queue between tokio workers and main thread)
├── logging.rs          # Cross-thread log channel + drain_to_console
├── ipc/
│   ├── mod.rs          # pub mod plugin_state; pub mod ws_server;
│   ├── plugin_state.rs # PluginState { engine_version, plugin_version } (immutable after init)
│   └── ws_server.rs    # IpcWebSocketServer, handle_connection, route_tool_call
├── commands/
│   ├── mod.rs          # CommandHandler trait, create_registry()
│   ├── meta.rs         # MetaCommands (ping, get_engine_version, get_plugin_version)
│   └── scene.rs        # SceneCommands (31 cmd_* functions + j2v/v2j helpers)
└── dock/
    ├── mod.rs
    ├── main_dock.rs    # create_dock() — VBoxContainer with 4 sub-panels
    ├── status_bar.rs
    ├── tool_manager.rs # Tool toggle CheckBoxes (currently lists only 4 tools — see modules/dock-ui.md)
    ├── integration.rs  # 12-client list (skeleton, buttons disabled)
    └── settings.rs     # WS/HTTP port LineEdits (read-only skeleton)
```

`lib.rs` registers `GodotMcpExtension` with `min_level() -> InitLevel::Editor`. `InitLevel::Scene` would never give us editor-only APIs.

## Module deep-dives

| Topic | Page |
|-------|------|
| Plugin lifecycle, pump install/uninstall | [modules/editor-plugin.md](../modules/editor-plugin.md) |
| `MainThreadDispatcher` internals | [modules/dispatcher.md](../modules/dispatcher.md) |
| Cross-thread logging channel | [modules/logging.md](../modules/logging.md) |
| WebSocket server and request routing | [modules/ipc-bridge.md](../modules/ipc-bridge.md) |
| `tool_call` routing protocol, `MetaCommands` | [modules/command-routing.md](../modules/command-routing.md) |
| 31 scene tools, J↔V helpers, `resolve_node` | [modules/scene-commands.md](../modules/scene-commands.md) |
| Right-dock UI panel and known stale text | [modules/dock-ui.md](../modules/dock-ui.md) |
| Why everything goes through a main-thread pump | [overview/threading-model.md](../overview/threading-model.md) |

## How a tool call enters and leaves this crate

```
WebSocket frame in
        ↓
IpcWebSocketServer::handle_connection (tokio task)
        ↓
parse IpcRequest, dispatch by method
        ↓
route_tool_call(tool, args, state, dispatcher, registry_tools)   ← tokio worker
        ↓
log_info(tool, "called args=…")  ── push to mpsc channel
        ↓
MetaCommands::handle_meta_tool   (sync, no Godot APIs) ──── reply
        OR
SceneCommands::handle_scene_tool (async)
        ↓
dispatcher.submit(move || cmd_create_node(&args)).await
        ↓                                  (await on oneshot)
        │                            ┌────── main thread ──────┐
        │                            │ pump Callable fires     │
        │                            │ dispatcher.process_pending()
        │                            │   runs cmd_create_node  │
        │                            │   returns Value          │
        │                            │ logging::drain_to_console()
        │                            │   forwards LogRecords to godot_print!
        │                            └─────────────────────────┘
        ↓
serde_json::Value back on tokio side
        ↓
log_info(tool, "ok result=…") or log_error(tool, "failed: …")
        ↓
wrap in IpcResponse, write to WS
```

## Dependencies that matter

- `godot = "=0.5"` (godot-rust gdext). Pinned hard — minor versions break trait shapes and API names regularly.
- `tokio = "1"` with `features = ["full"]`.
- `tokio-tungstenite = "0.24"`, `futures-util = "0.3"`.
- `godot-mcp-core = { path = "../core" }`.

Build artefact: `target/{debug,release}/godot_mcp_gdext.{dll,so,dylib}`. The `.gdextension` manifest in `addons/godot_mcp/` points at `res://addons/godot_mcp/bin/godot_mcp_gdext.<ext>`; `package_addons.py` copies the freshly built lib into that bin folder.

## Tests

12 tests in `dispatcher` and `commands::meta`. The dispatcher tests are tokio-runtime tests that exercise the queue from worker threads. The meta tests verify the synchronous tool-name matching and version-string formatting. No test loads a real Godot runtime; `cargo test --workspace` succeeds with no editor installed.
