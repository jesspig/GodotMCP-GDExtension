# Godot MCP — Repo Wiki

> Knowledge base for the GodotMCP project. Updated to match the codebase as of 2026-05-22. All "this is what the code does" claims are verified against current source; speculation and stale Phase-1 talk has been purged.

## What this project is

A Rust-only **Model Context Protocol** bridge that lets AI clients drive the Godot 4.6+ editor. Two processes:

- `godot-mcp-server` — standalone binary; AI client launches it over stdio. Forwards every tool call as a `tool_call` IPC request.
- `godot_mcp_gdext` — `cdylib` loaded by the Godot editor as a GDExtension `EditorPlugin`. Hosts a WebSocket server on `127.0.0.1:9500`, dispatches every call back to the Godot main thread, executes `EditorInterface`/`Node` APIs, and ships results back.

35 tools are exposed today (4 meta + 31 scene). All are end-to-end tested in a live editor.

## How to read this wiki

Start with `overview/` for the mental model, dip into `modules/` for the implementation patterns that bite (threading, command routing, J↔V), use `reference/` for ready-to-copy commands, configs, and the tool catalog. `crates/` is a file-by-file map per crate. `specification/` and `design/` are the stable contracts and recorded decisions.

| Section | When you need it |
|---------|------------------|
| [overview/architecture.md](overview/architecture.md) | Big picture: two processes, three crates, where each layer lives |
| [overview/threading-model.md](overview/threading-model.md) | **Read first if touching gdext.** The tokio↔main-thread split, dispatcher, log pump, and the `bind_mut() failed, already bound` trap |
| [crates/core.md](crates/core.md) | Shared protocol types (`IpcRequest`, `IpcResponse`, `ToolCallParams`, …) |
| [crates/server.md](crates/server.md) | `main` → `handler` → `bridge` → `tool_registry` flow |
| [crates/gdext.md](crates/gdext.md) | Module map for the GDExtension cdylib |
| [modules/command-routing.md](modules/command-routing.md) | How a `call_tool` MCP request becomes a `cmd_*` invocation; must add to **both** sides when you add a tool |
| [modules/scene-commands.md](modules/scene-commands.md) | 31 scene tools, the `j2v`/`v2j` JSON↔Variant rules, `resolve_node`, `try_load` |
| [modules/ipc-bridge.md](modules/ipc-bridge.md) | `IpcWebSocketServer` + `GodotBridge` over WebSocket |
| [modules/dispatcher.md](modules/dispatcher.md) | `MainThreadDispatcher`: tokio worker → main-thread closure execution via oneshot |
| [modules/logging.md](modules/logging.md) | mpsc log channel + `process_frame` pump (because Godot macros panic off main thread) |
| [modules/editor-plugin.md](modules/editor-plugin.md) | `McpEditorPlugin` lifecycle; why `process()` is intentionally empty |
| [modules/dock-ui.md](modules/dock-ui.md) | Right-dock VBox panel, 4 sub-panels, current vs aspirational state |
| [reference/tools-catalog.md](reference/tools-catalog.md) | All 35 tools, JSON schema, args, return shape |
| [reference/client-config.md](reference/client-config.md) | 12 AI clients × stdio/HTTP config templates (only stdio works today) |
| [reference/client-quirks.md](reference/client-quirks.md) | Per-client config oddities cheat sheet |
| [reference/build-and-package.md](reference/build-and-package.md) | `package_addons.py` flags, CI gate order, hot-reload tips, file-lock recovery |
| [specification/ipc-protocol.md](specification/ipc-protocol.md) | Wire format for `IpcRequest`/`IpcResponse`/`IpcNotification`/`ToolCallParams` |
| [specification/workspace.md](specification/workspace.md) | Real per-crate `Cargo.toml`, pinned versions, addon manifest |
| [design/decisions.md](design/decisions.md) | ADR-style record of the choices that shape the codebase |
| [log.md](log.md) | Append-only project changelog |

## Conventions in this wiki

- "Verified" = an actual file line was inspected when the page was written. Pages avoid speculation; if something is aspirational it's marked **Planned**.
- Line/file references use the form `crates/<crate>/src/<file>.rs:<line>` so they survive into editors.
- Mermaid diagrams describe runtime data flow, not call trees.
- Code excerpts are kept short; for the full source, follow the file reference and read directly.
- New ingest? Append an entry to [log.md](log.md). Touched a pattern that's documented here? Update the page in the same change.
