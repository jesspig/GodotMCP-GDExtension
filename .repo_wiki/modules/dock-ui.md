# Module: `dock-ui`

> Right-dock VBoxContainer with 4 stacked sub-panels. Files: `crates/gdext/src/dock/*.rs`.

## Slot and entry point

`McpEditorPlugin::enter_tree` calls `dock::main_dock::create_dock(broadcast_tx)` to get a `Gd<VBoxContainer>`, then `self.base_mut().add_control_to_dock(DockSlot::RIGHT_UL, &dock)`. `DockSlot::RIGHT_UL` is the same slot the editor's FileSystem panel uses by default.

`create_dock` (`dock/main_dock.rs`) assembles four children in order:

1. `status_bar::create_status_bar()`
2. `integration::create_panel()`
3. `tool_manager::ToolManager::new(broadcast_tx).create_panel()`
4. `settings::create_panel()`

The `tokio::sync::broadcast::Sender<String>` is threaded all the way down to `tool_manager` so toggle events can broadcast `tool_list_updated` notifications to every WS client without going through the dispatcher.

## Panel-by-panel reality

### `status_bar.rs`

- Green 12×12 `ColorRect` indicator.
- `Label` reading `"Status: Running"`.
- Two text rows: `ws://127.0.0.1:9500` and `http://127.0.0.1:8900/mcp`.

**Hardcoded.** The status label doesn't actually reflect the WS server state; the HTTP URL is aspirational (no HTTP transport implemented). When the server crashes the indicator stays green.

### `tool_manager.rs`

- Header `Label`: `"Tools (4/4)"`.
- 4 hardcoded `CheckBox` rows for `ping`, `get_engine_version`, `get_plugin_version`, `get_server_version`.
- Each CheckBox connects its `"toggled"` signal to a `Callable::from_fn` that calls `Self::on_tool_toggled(name, pressed, &tx)`, which builds and sends a `tool_list_updated` notification through `broadcast_tx`.

**Stale.** The list and header say "Tools (4/4)" but the project actually exposes 35 tools. Adding a tool only touches the server registry and the gdext routing layer; this dock list does **not** pick them up automatically. Anyone wanting per-tool toggles in the UI must rewrite `ToolManager::create_panel` to enumerate tools dynamically (e.g. read `MetaCommands::tool_names()` + `SceneCommands::tool_names()` via the `CommandHandler` trait).

The toggle plumbing itself works: the server's `ToolRegistry::update_from_notification` correctly flips the right tool's `enabled` bit when the notification arrives. So if you do hook a real CheckBox up to a real tool name, end-to-end toggle works today.

Also note: the source has UTF-8 mojibake in a few Chinese labels (`"状?"` / `"版本?"` patterns) — leftover from an earlier mis-encoded edit. Cosmetic only; the matched tool names are ASCII and route correctly.

### `integration.rs`

- Header `Label`: `"Client Integration"`.
- 12 `Button` rows, one per supported client (Claude Code, Codex, Gemini CLI, OpenCode, Cursor, GitHub Copilot, Qwen Code, Trae, Trae CN, Qoder, Antigravity, CodeBuddy).
- **Buttons do nothing.** Skeleton only. There is no `connect("pressed", …)` and no config-file writer.

The intent is each button writes the matching MCP config (path and shape per [reference/client-quirks.md](../reference/client-quirks.md)) but the writer logic was never implemented.

### `settings.rs`

- Header `Label`: `"Advanced Settings"`.
- Two `LineEdit`s for "WS Port" (`9500`) and "HTTP Port" (`8900`), both `set_editable(false)`.
- Some buttons (start/stop/reload) — also wired to nothing.

**Read-only skeleton.** Ports are hardcoded in `editor_plugin.rs:enter_tree` (`IpcWebSocketServer::new(9500, …)`); changing the LineEdit value has no effect. Making it real would require: writable inputs, validation, a "Restart" action that calls `shutdown.notify_one()` + rebuilds the server with the new port.

## Why the dock exists in this half-finished state

Phase 2a delivered the status + skeleton dock as scaffolding so the plugin had a visible presence in the editor. Phase 2b focused on the 35 tools instead of fleshing out the UI. The `broadcast_tx` plumbing is in place precisely so a future agent can wire the dynamic tool list, real client-config writers, and live port changes without refactoring.

If you reach for this module:

- Resist hand-coding tool lists. Pull from a central source so future tool additions auto-appear. The trait `CommandHandler::tool_names()` is the natural API; combine the lists from `create_registry()`.
- Don't mix UI updates with tokio. Dock signal callbacks run on the main thread and may call `EditorInterface` directly; for log output prefer `logging::log_*_main` to land in the Output panel.
- Disconnect every `Callable` in `exit_tree` — Godot complains about leaked signal connections on plugin reload.
