# Architecture Decision Records

> Captured rationale for the choices that shape today's codebase. Each decision is `Accepted`; if a future change supersedes one, add a successor and mark the original `Superseded by …`.

## ADR-001 — Rust on both sides (no Python relay)

- **Context**: Existing Godot MCP implementations (hi-godot/godot-ai, elfensky/godot-mcp) all use Python or Node as the MCP server, talking to a GDScript plugin via HTTP/WebSocket. That introduces a runtime requirement (uv / Python / Node) and surfaces version mismatches.
- **Decision**: GDExtension in Rust (via `godot-rust/gdext`) and MCP server in Rust (via `rmcp`). Shared types in a `core` crate.
- **Consequences**:
  - Single static binary for the server; AI clients don't need Python.
  - Native Godot API access (`EditorInterface`, `ClassDb`, etc.) without GDScript bridging.
  - The crate-pinning game: `godot` and `rmcp` both move fast; we pin hard (`=0.5`, `=1.7`).
  - We give up the ability to ship a quick fix in a `.gd` file — every change needs a recompile.

## ADR-002 — Two processes connected by WebSocket on `127.0.0.1:9500`

- **Context**: A GDExtension cannot itself act as a long-running MCP server because:
  - The editor controls the process lifetime.
  - rmcp's stdio transport must own `stdin`/`stdout`, which the editor needs for its own console.
- **Decision**: Ship two binaries. The MCP server (`godot-mcp-server`) speaks rmcp/stdio to AI clients and connects out to the editor over loopback WebSocket. The GDExtension hosts the WS server.
- **Consequences**:
  - AI clients launch *only* the server binary; the editor is started independently. State (engine version, plugin version) is reported via `godot_ready` on connect.
  - No cross-process auth; the loopback bind makes this acceptable for developer-machine usage.
  - Killing or restarting the editor leaves the server alive — next tool call lazily reconnects via `GodotBridge::ensure_bridge`.
- **Superseded if**: We ever embed an MCP-over-IPC variant directly in the extension. No plans.

## ADR-003 — Single IPC method `tool_call`, all tools wrapped in `ToolCallParams`

- **Context**: The original protocol used `IpcRequest.method` directly as the tool name. The server had to special-case every tool with its own `match` arm in `handle_tool_call` to know what method string to send. Adding a tool required edits on three sides (registry, server router, editor router).
- **Decision**: Server always sends `method: "tool_call"` and embeds `{tool, args}` in `params`. The editor parses `ToolCallParams` and dispatches by `tool`.
- **Consequences**:
  - Adding a new tool now requires edits on *two* sides: the registry schema in the server, and the routing arm + `cmd_*` in the editor. The server's `forward_tool_call` is one function.
  - Backwards-compat path still exists in `IpcWebSocketServer::handle_request` for non-`tool_call` methods, but nothing in production uses it.

## ADR-004 — Tool routing splits into MetaCommands (sync) and SceneCommands (async)

- **Context**: A handful of tools (`ping`, `get_engine_version`, `get_plugin_version`) only need to read `PluginState` — they don't touch the Godot main thread. The other 31 do.
- **Decision**: `MetaCommands::handle_meta_tool` is sync and returns immediately. `SceneCommands::handle_scene_tool` is async and dispatches each call to the main-thread pump via `MainThreadDispatcher::submit`.
- **Consequences**:
  - The hottest IPC paths (`ping`) don't wait for a frame to drain.
  - The `CommandHandler::execute` trait method is effectively dead — both implementers `Err("…should not be called directly")`. Kept on the trait because future handlers (e.g. asset commands) might use it.
  - Adding a new tool group means adding another `if X.can_handle(tool) { … }` branch in `route_tool_call`. Acceptable while we have two groups.

## ADR-005 — Main-thread pump runs from `SceneTree::process_frame`, not `EditorPlugin::process`

- **Context**: The first dispatcher pump lived inside `IEditorPlugin::process(&mut self)`. Every tool call triggered `Gd<T>::bind_mut() failed, already bound; T = McpEditorPlugin`. Root cause: gdext implicitly `bind_mut`s the plugin for the entirety of `process()`. Any `cmd_*` invoking an `EditorInterface` API that synchronously dispatches a Godot signal back to the plugin → second `bind_mut` → panic (gdext issue #338).
- **Decision**: Connect a `Callable::from_fn` to the `SceneTree::process_frame` signal. The callable executes `dispatcher.process_pending()` + `logging::drain_to_console()`. Removing the override of `process(&mut self)` means the plugin is *not* bound while drain runs.
- **Consequences**:
  - The plugin no longer overrides `process`. Anyone adding per-frame behaviour must do it through another Callable, never via the trait method.
  - The pump lives only while the SceneTree exists. Install in `enter_tree`, uninstall in `exit_tree`. Both are guarded with `is_connected` because Godot panics on disconnecting a never-connected signal.
- **See also**: [overview/threading-model.md](../overview/threading-model.md).

## ADR-006 — Worker-thread logs go through an mpsc channel, drained on the main thread

- **Context**: `godot_print!` / `godot_warn!` / `godot_error!` are main-thread-only. Calling them from a tokio worker crashes the editor with `attempted to access binding from different thread than main thread`. The first iteration tried it; the editor would crash on every tool call.
- **Decision**: `logging::log_info` (and warn/error) is callable from any thread. It pushes a `LogRecord` onto a `std::sync::mpsc` and also writes the same line to `eprintln!`. `drain_to_console()` runs from the pump and forwards every queued record to the appropriate native macro.
- **Consequences**:
  - Native Godot output is delayed by up to one frame. Acceptable.
  - Tests can construct log lines without a Godot runtime — they go to stderr only.
  - `log_*_main` variants are reserved for code already on the main thread and want direct macro output; currently unused.

## ADR-007 — Always go through `j2v` / `v2j` when reading or writing Godot Variants

- **Context**: `node.set("position", Variant::from(GString::from(json_str)))` accepts the call silently then writes `(1e-05, 1e-05)` for `Vector2` properties — Godot's near-zero default when string-to-Vector2 fails. Multiple tools silently corrupted scenes before this was discovered.
- **Decision**: All JSON-to-Variant conversion goes through `j2v` in `commands/scene.rs`. `j2v` shape-detects `Vector2/3/4`, `Color`, `Rect2`, `Quaternion`, `Resource` (via `res://` strings and `{"resource_path": …}` objects), and arrays of 2/3/4 numbers. Anything else falls through to `GString` of the JSON text. The inverse `v2j` matches the same shapes so the round-trip is stable.
- **Consequences**:
  - Per-Variant assignment must use `j2v` — no shortcuts. `cmd_set_property` and `cmd_batch_set_property` do; future tools must too.
  - Adding a new Variant type means extending both `v2j` and `j2v` together. `Vector4` is currently *not* recognised in `j2v` (4-key `{x,y,z,w}` goes to `Quaternion`); that's a known asymmetry. Decide explicitly when extending.

## ADR-008 — `resolve_node` accepts root aliases; `get_node_or_null` is never called directly

- **Context**: `root.get_node_or_null("")` and `get_node_or_null(".")` return `None`. `get_node_or_null("/root")` returns `None` if the editor's root has a different name. Yet every reasonable caller will at some point want to refer to "the scene root". `instantiate_scene` failed in initial testing because `parent_path=""` returned None.
- **Decision**: `resolve_node(root, path)` returns `Some(root.clone())` for any of `""`, `"."`, `"/"`, `"/root"`, the root's exact name, or `/root/<root-name>`. Otherwise `root.get_node_or_null(path)`. All scene-tool callers use `resolve_node`.
- **Consequences**:
  - One helper to update if root-form ever changes.
  - `cmd_*` functions are short — node lookup is one match arm.

## ADR-009 — Test counts are asserted, not approximated

- **Context**: When `tool_registry.rs::register_defaults` and the matching gdext routing diverge, the symptom is "Unknown tool" or "handler not yet implemented" at runtime — both look like config problems and are hard to diagnose.
- **Decision**: The test suite hardcodes the expected total tool count and per-state counts (currently 35 total, 34/33 after disabling). Any addition or removal forces a test update, which forces sync between the two sides.
- **Consequences**:
  - You can't add a tool without also bumping the asserted numbers. `cargo test` rejects the mismatch.
  - When changing tool counts in batch, run `cargo test` early and read failure messages — each broken assertion pinpoints the line to update.

## ADR-010 — `Cargo.lock` is committed

- **Context**: This repo ships binary artefacts (`godot-mcp-server.exe`, `godot_mcp_gdext.dll`). For reproducible builds we want exactly one set of transitive versions.
- **Decision**: Commit `Cargo.lock`. `.gitignore` excludes `target/` and addon binaries; everything else builds determinstically off the lockfile.
- **Consequences**:
  - `cargo update` is a deliberate, reviewable commit — not a side effect.
  - CI cache (`Swatinem/rust-cache@v2`) keys off the lockfile for hits.
