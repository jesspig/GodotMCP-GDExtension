# Roadmap

> Phases ordered by user-facing value × engineering risk. Each phase is a coherent slice that ships independently. Detailed plans in the per-phase files; this page is the index + sequencing rationale.

## Sequencing

```
Phase 1 — Foundations                ✅ Shipped  ── workspace, EditorPlugin, WS :9500, 4 meta tools
   │
   ▼
┌──────────────────────────────────────────────────────────────┐
│  Phase 2a — MVP Integration         ✅ Shipped  ── merged   │
│              `5c68d32a` (feature/mvp → develop)             │
│  Dispatcher, ToolRegistry, CommandHandler routing, Dock UI  │
│  skeleton                                                    │
└──────────────────────────────────────────────────────────────┘
   │
   ▼
┌──────────────────────────────────────────────────────────────┐
│  Phase 2b — Scene Management      ✅ Fully shipped ──        │
│              `12fb1431` (feature/scene_manager → develop)    │
│               tagged `v0.1.0` at `d1ee1fb3`                 │
│  ✅ Scene/scene-file (16+22) + Script (5+6+3+lsp)            │
│  ✅ Editor control (6+3) + Project/Input/Plugin (7+10+4+2)  │
└──────────────────────────────────────────────────────────────┘
   │
   ▼
NOW   ── 125 tools, 17 handler groups, LSP validation, C# build, stdio only
   │
   ▼
Phase 3 — Dock UI polish              (1 week)   ── unblocks self-service tool toggling + client config
   │
   ▼
Phase 4 — HTTP transport              (3-5 days) ── unblocks Cursor / Copilot / Codex / Antigravity et al.
   │
   ▼
Phase 5 — Tool group expansion        (open)     ── runtime / asset / project / script / debug
   │
   ▼
Phase 6 — Resilience                  (1 week)   ── once we have real-world usage, heartbeat + multi-client matter
```

Phases 1 and 2 (a+b) are fully shipped: 125 tools, 17 handler groups, cross-thread logging, LSP validation, C# build. Phases 3 and 4 are independent and can be parallelised; both build on top of today's foundations without touching the dispatcher / IPC layer. Phase 5 depends on Phase 4 only if we want the new tools accessible to HTTP-only clients. Phase 6 is best done after Phase 5 because the failure modes only show up under volume.

## Phase 1 — Foundations (✅ Shipped)

**Goal**: biprocess skeleton with end-to-end connectivity. Merged as `ad9c2eb5`..`e15e5183`.

- Cargo workspace (`crates/core`, `crates/server`, `crates/gdext`) + `IpcRequest`/`IpcResponse`/`ToolManifest` types
- GDExtension `EditorPlugin` + `PluginState` + WebSocket server on `127.0.0.1:9500`
- MCP server binary (`godot-mcp-server`) with `rmcp` stdio transport + `GodotBridge` (id→oneshot) + `GodotMcpHandler`
- 4 meta tools: `ping`, `get_engine_version`, `get_plugin_version`, `get_server_version` + lazy-connect
- `package_addons.py` packaging script, CI pipeline (fmt + clippy + build + test), `rust-toolchain.toml`
- `version.workspace = true` unification, `Cargo.lock` committed

## Phase 2a — MVP Integration (✅ Shipped)

**Goal**: structured dispatch, thread-safe Godot calls, and Dock UI skeleton. Merged as `5c68d32a`.

- `ToolCallParams` protocol, `MainThreadDispatcher` (worker→main-thread oneshot queue), `ToolRegistry` with enable/disable
- `CommandHandler` trait + `MetaCommands`/`SceneCommands` modules + `route_tool_call` dispatcher
- `broadcast_tx` notification channel + dynamic `list_tools` + tool state validation in bridge
- Dock UI: 4-panel `VBoxContainer` (status bar with green dot + Stop button, tool manager with 4 checkboxes, 12-client integration skeleton, settings with read-only ports)
- Editor plugin lifecycle integration with dispatcher

## Phase 2b — Scene Management (✅ Fully shipped)

**Goal**: full scene-tree manipulation + script editing + editor control + project config. Merged as `12fb1431`; tagged `v0.1.0`.

| # | Sub-phase | Tools | Status |
|---|-----------|-------|--------|
| 2b.1 | Scene Management | `get_scene_tree`, `create_node`, `delete_node`, `modify_node_property`, `get_node_properties`, `move_node`, `duplicate_node`, `rename_node`, `set_node_script`, `find_nodes` (10) | ✅ Shipped |
| 2b.2 | Script Management | GDScript (5) + C# (6) + Search (3) + LSP validation | ✅ Shipped |
| 2b.3 | Editor Control | `play_current_scene`, `play_main_scene`, `stop_scene`, `is_scene_playing`, `refresh_filesystem`, `get_editor_info` (6) + server-side `godot_editor_*` (3) | ✅ Shipped |
| 2b.4 | Project Management | `project_settings.rs` (7) + `project_settings_ext.rs` (10) + `input_map.rs` (4) + `plugin_management.rs` (2) | ✅ Shipped |
| 2b.5 | Server registry sync | 125 tools visible in `list_tools` | ✅ Shipped |
| 2b.6 | e2e tests | 5 representative tools (mock WS server + real server process) | ⏳ Not started |
| 2b.7 | Documentation sync | parameter and response examples per tool | ⏳ Not started |

Sub-page: [`phase-2b.md`](phase-2b.md) — 仅含待办项（2b.6 e2e 测试 + 2b.7 文档同步）。

Shipped so far:
- Cross-thread logging: `log_info`/`log_warn`/`log_error` → `mpsc` channel → `drain_to_console()` via pump; eprintln! mirror
- Main-thread pump: `Callable::from_fn` on `SceneTree::process_frame` (not `EditorPlugin::process`) — solves `bind_mut` re-entrancy (gdext issue #338)
- 125 tools across 17 gdext CommandHandler groups + 3 server-side editor control tools
- Full GDScript toolchain: create, read, edit, validate (via LSP TCP), list
- Full C# toolchain: create solution (Rust-level generation), create script, read, edit, build (`dotnet build`)
- Search: `find_in_file`, `search_project`, `find_and_replace`
- Editor control: play/stop/refresh + server-side open/close/restart
- Project settings: get/set settings, autoloads, main scene, scene listing, display/physics/rendering/layer aggregate settings
- Input map: list/add/set events/remove actions
- Plugin management: list/set enabled
- Collision helpers: circle + rectangle collision shapes
- 3D property helpers: position, rotation, scale
- Undo/redo support with property-level granularity
- `j2v`/`v2j` JSON↔Variant helpers (Vector2/3/4, Color, Rect2, Quaternion, Resource); `resolve_node` for root aliases
- `lsp/` module for GDScript validation via short-lived TCP connections
- Server registry at 125 tools; `package_addons.py` rewritten with flags
- Wiki restructure (`.repo_wiki/`), bilingual README, AGENTS.md, License

Still to do: 2b.6 (e2e tests), 2b.7 (docs).

## Phase 3 — Dock UI polish

**Goal**: turn the four panels from skeleton into something a non-developer can use.

| Gap (visible in the current code) | Target |
|-----------------------------------|--------|
| `dock/tool_manager.rs` hardcodes `("ping", …, true)` × 4 and the header `"Tools (4/4)"` | Enumerate live from `create_registry()` (125 tools in 17 groups); header reflects `enabled/total` |
| `dock/integration.rs` builds 12 `Button` rows but never `.connect("pressed", …)` | Each button writes the matching MCP config file (see `.repo_wiki/reference/client-config.md`) and toasts success/failure |
| `dock/settings.rs` LineEdits are `set_editable(false)` and read-only `9500` / `8900` | Writable inputs + Apply button that triggers `shutdown.notify_one()` and restarts `IpcWebSocketServer` on the new port |
| `dock/status_bar.rs` ColorRect is always green | Reflect actual server state via the existing `broadcast_tx` (or a new state-changed notification) |
| Mojibake in `tool_manager.rs` tooltip strings | Fix encoding when rewriting the panel |

Sub-pages: [`phase-3-dock-ui.md`](phase-3-dock-ui.md).

## Phase 4 — HTTP transport

**Goal**: let HTTP-only MCP clients (Cursor, Copilot, Codex, Antigravity, …) talk to the server.

Current state: `Cargo.toml` already enables `rmcp` features `transport-streamable-http-server` and depends on `axum = "0.8"`, but no `transports/` module exists in `crates/server/src/`. `main.rs` only owns stdio.

| Target | Notes |
|--------|-------|
| CLI flag `--transport stdio\|http\|all` (default `stdio` for compat) | clap subcommand or `enum` arg |
| HTTP listener on `127.0.0.1:8900/mcp` using `rmcp::transport::streamable_http_server` + axum | Reuse the same `GodotMcpHandler` instance behind `Arc` |
| Dock `settings.rs` HTTP port becomes editable | Pair with Phase 3 |
| Existing client-config templates ([`reference/client-config.md`](../../.repo_wiki/reference/client-config.md)) updated to remove the "⚠ planned" markers | One-line cleanup |

Sub-page: [`phase-4-http-transport.md`](phase-4-http-transport.md).

## Phase 5 — Tool group expansion

**Goal**: cover the editor surface beyond scene/node mutation.

Today's 125 tools cover scene management, node/property CRUD, GDScript/C# editing, project settings, editor control, input map, plugin management, and search. The next batches expand into new domains:

| Group | Indicative tools | Why |
|-------|------------------|-----|
| `asset_*` (6-8) | search assets, import, generate previews, write `.tres`, scan filesystem | AI agents need to *create* resources, not just attach existing ones |
| `script_*` (5-7) | read/write `.gd`/`.cs` body via `ScriptEditor`, get diagnostics, run static checks | Closes the loop after `attach_script` |
| `project_*` (4-6) | read/write `project.godot` sections, add autoloads, set input map | Project-level config without leaving the agent |
| `editor_*` (4-6) | play/stop scene, screenshot viewport, inspect selection, undo/redo | Lets the agent observe + drive the editor |
| `debug_*` (4-6, **off by default**) | runtime values via remote debugger, frame stepping | Per-tool toggle in the dock keeps this opt-in |

Each new group means: schema in `crates/server/src/tool_registry.rs::register_defaults`, a new `CommandHandler` impl in `crates/gdext/src/commands/<group>.rs`, new arm in `IpcWebSocketServer::route_tool_call`, bump test counts. Process documented in [`.repo_wiki/modules/command-routing.md`](../../.repo_wiki/modules/command-routing.md).

Sub-page: [`phase-5-tool-expansion.md`](phase-5-tool-expansion.md).

## Phase 6 — Resilience

**Goal**: survive real-world use (long sessions, flaky clients, multi-client setups).

| Gap | Target |
|-----|--------|
| No heartbeat — half-open WS connections only surface on the next call | 10s ping/pong from the bridge, reconnect on miss |
| Bridge has no timeout — a hung `cmd_*` blocks the AI client forever | `tokio::time::timeout` per `bridge.call` (default 30s, configurable) |
| Only one MCP client at a time is tested | Per-connection state in `IpcWebSocketServer`; broadcast goes to all, responses only to the originator (already true today, but no test covers concurrent clients) |
| `eprintln!`-only audit | Optional `--log-file <path>` writing the same `[Godot MCP]` lines as JSONL for later inspection |
| No graceful shutdown signal handling in the server | SIGINT/CTRL_C → drop bridge, finish in-flight rmcp calls, exit |

Sub-page: [`phase-6-resilience.md`](phase-6-resilience.md).

## Out of scope (deliberately)

These are not on the roadmap; record here so we don't relitigate.

- **GDScript-side companion plugin**. The whole point of the native GDExtension is to avoid GDScript glue.
- **Embedding the MCP server inside the editor process**. The two-process design exists because rmcp's stdio transport needs to own `stdin`/`stdout`. See [`.repo_wiki/design/decisions.md#adr-002`](../../.repo_wiki/design/decisions.md).
- **Authentication / TLS**. Loopback-only; if we ever expose over a real network this changes.
- **Cross-version Godot support (4.5, 4.7)**. `compatibility_minimum = "4.6"`. Touching this means re-vetting every `EditorInterface` call.
- **Replacing `MainThreadDispatcher` with `Engine::singleton().get_main_loop().call_thread_safe`**. Looked at, decided not worth it — see ADR-005 in the wiki.

## Open questions

Things that block planning until resolved. Track in [`open-questions.md`](open-questions.md). Today's list:

- Are we OK shipping the HTTP listener bound to `127.0.0.1` only, or should it also accept LAN connections (introduces auth question)?
- Per-tool enable bit lives in the **server** registry. If the dock toggles a tool while no bridge is connected, the toggle is dropped. Worth fixing or accepted?
- For Phase 5 `debug_*`: do we need the Godot remote debugger protocol or is post-mortem inspection enough?
