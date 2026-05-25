# Phase 3 — Dock UI Polish

> **Status**: ⏳ Not started
> **Estimate**: 1 week
> **Depends on**: nothing (today's foundations are enough)

## Why this phase exists

The right-dock UI shipped as scaffolding during Phase 2a. It's structurally correct (4 sub-panels in a `VBoxContainer`, `broadcast_tx` plumbed through, signal handlers registered) but the contents are stub data. Every panel has visible gaps a user notices in the first minute.

This phase makes the dock match the tools-catalog reality (125 tools, real status, working client-config writers, editable settings).

## Concrete gaps to close

Reference: [`.repo_wiki/modules/dock-ui.md`](../../.repo_wiki/modules/dock-ui.md) lists today's state per panel.

### 3.1  `tool_manager.rs` — dynamic tool list

**Today**: header reads `"Tools (4/4)"`; the body builds 4 hardcoded CheckBox rows (`ping`, `get_engine_version`, `get_plugin_version`, `get_server_version`). The plumbing (toggle → `broadcast_tool_list_updated` → server `update_from_notification`) works end-to-end *for those 4 tools*. Actual registered tools: 125 in 17 handler groups.

**Target**:

- Enumerate live tool names from `crate::commands::create_registry()`. Each `Box<dyn CommandHandler>` already exposes `tool_names() -> &[&str]` and `group_name() -> &str`.
- Render one collapsible section per group (Meta, Scene). Inside each section: one `CheckBox` per tool, plus a "Toggle all" header CheckBox.
- Header updates live: `"Tools (<enabled>/<total>)"`.
- Tooltip text comes from the registry. **Important**: the descriptions live on the *server* side (`tool_registry.rs::register_defaults`). Options:
  - **Option A** (simple): hardcode tooltips on the gdext side too. Doubles the source of truth.
  - **Option B** (better): add a `list_tools` IPC call (`method: "list_tools"`) returning `Vec<ToolInfo>`. The dock fetches once on startup. This also enables Phase 5 to grow tools without UI changes. **Recommended.**

**Pitfall**: the dock is created from `enter_tree` *after* the WS server is spawned but possibly before any client connects. If we go with Option B, the dock must be able to call its own gdext-side handler without needing a WS client; the natural place is to bypass the WS and ask the registry directly via a function exported from `commands::mod`. That keeps the dock decoupled from network state.

**Pitfall**: re-encoding. The existing `tool_manager.rs` has UTF-8 mojibake in the tooltip strings (`"状?"`, `"版本?"`). When rewriting, write in proper UTF-8; the source file is otherwise fine.

### 3.2  `integration.rs` — make the 12 buttons do something

**Today**: 12 `Button` rows, no signal connections, no logic. The data needed to actually write configs is in the wiki ([`.repo_wiki/reference/client-config.md`](../../.repo_wiki/reference/client-config.md) and [`reference/client-quirks.md`](../../.repo_wiki/reference/client-quirks.md)).

**Target**:

- Per-button `connect("pressed", Callable::from_fn(...))` invoking a `write_config(client_id)` function.
- A `ClientDef` struct per client: `{ id, name, config_path_fn, write_fn(server_path, port) -> Result<(), Error> }`.
- Detect whether the project has a "project-local" config preference (Cursor, Trae, OpenCode) vs user-global (Claude Code, Codex, Antigravity). For project-local: use `ProjectSettings::globalize_path("res://")` as the project root.
- Surface success/failure via the editor toaster (`EditorInterface::singleton().get_editor_toaster()`).
- Use `serde_json::to_string_pretty` for JSON, `toml::to_string` for Codex.

**Pitfall**: the `serde_json` already in `crates/gdext/Cargo.toml` covers JSON output. **TOML support** requires adding `toml = "0.8"` to the gdext crate. Add it deliberately and document in the workspace spec.

**Pitfall**: editing existing config files. Users may have other MCP servers in `mcpServers`. **Do not overwrite the whole file** — read, merge our entry, write back. For Codex's TOML this is bit harder; consider using `toml_edit` to preserve comments.

**Pitfall**: the server binary path. The dock has no idea where `godot-mcp-server.exe` was built. Options:
  - **Option A**: ask the user via a `FileDialog`.
  - **Option B**: read `cargo metadata` output. Slow, requires shell.
  - **Option C**: bundle the binary inside the addon `bin/` folder and reference it via `res://addons/godot_mcp/bin/godot-mcp-server.exe`. Already kind of what `package_addons.py` does for the dll. **Recommended** — extend the packager to copy the server binary too.

### 3.3  `settings.rs` — editable ports + Apply

**Today**: WS Port `9500` and HTTP Port `8900` LineEdits are `set_editable(false)`. No "Apply", no validation.

**Target**:

- `set_editable(true)`, validate `parse::<u16>()` on change, red border on invalid.
- "Apply" button → calls `McpEditorPlugin::restart_servers(ws_port, http_port)`. That method:
  - Triggers the existing `shutdown.notify_one()` to gracefully stop the current `IpcWebSocketServer`.
  - Awaits a small grace period (200 ms, like `exit_tree`).
  - Constructs a new `IpcWebSocketServer` with the new port, spawns it.
  - Updates the status bar text.
- Persist the chosen ports across editor restarts. Use Godot's `EditorSettings::set_setting("godot_mcp/ws_port", port)`.

**Pitfall**: HTTP port has no listener yet — Phase 4. Until then, set the HTTP input read-only and label it "(planned)".

**Pitfall**: changing the WS port at runtime breaks the *currently-connected* MCP server. The toaster should warn "client must reconnect". The server's lazy reconnect (`GodotBridge::ensure_bridge`) handles it on the next call.

### 3.4  `status_bar.rs` — actual status

**Today**: hardcoded green dot + "Status: Running" label.

**Target** (minimum):

- Two state values fed from the plugin: `ws_listening: bool`, `client_connected: bool`.
- Map to colours: gray (not listening), yellow (listening, no client), green (client connected).
- Source the state from `IpcWebSocketServer`. Easiest: have the server send a `client_count_changed` notification through `broadcast_tx` on connect/disconnect; the dock subscribes via `broadcast_rx`.
- Once Phase 4 ships, add a third state for HTTP.

**Pitfall**: broadcast subscribers don't auto-reconnect if the broadcast sender is replaced (e.g. on a server restart in 3.3). Resubscribe on every `restart_servers`.

## Tests to add

- Unit-level: rendering helpers (the function that takes `(enabled, total, tools)` and produces the strings/labels) should be testable without a Godot runtime.
- A new integration scenario: launch a fake MCP server connection (already feasible from the bridge tests), assert the dock receives a `client_count_changed` event.

## Out of scope for Phase 3

- Per-tool documentation viewer / search.
- Drag-and-drop reordering of tools (Godot dock CheckBox doesn't support it natively).
- Dark/light theme awareness — Godot already handles editor theming for our controls.

## Done means

- [ ] Dock shows all 125 tools enumerated dynamically.
- [ ] All 12 client integration buttons write a valid config and toast the result.
- [ ] Both ports are editable; Apply restarts the affected server.
- [ ] Status bar reflects connect/disconnect within one frame.
- [ ] `cargo fmt && cargo clippy -D warnings && cargo build && cargo test` all green.
- [ ] [`.repo_wiki/modules/dock-ui.md`](../../.repo_wiki/modules/dock-ui.md) updated to remove the "skeleton" disclaimers; log entry in [`.repo_wiki/log.md`](../../.repo_wiki/log.md).
- [ ] This file deleted (Phase 3 is done; the plan removed the gap).
