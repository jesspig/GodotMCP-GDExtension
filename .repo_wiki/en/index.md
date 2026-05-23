# Godot MCP — Project Knowledge Base

> Rust-only **Model Context Protocol** bridge that lets AI tools drive the Godot 4.6+ editor. Dual-process, three-crate architecture.

## Quick Navigation

| Section | Description |
|---------|-------------|
| [Architecture](overview/architecture.md) | Dual-process architecture, three-crate split, dataflow diagrams |
| [Threading Model](overview/threading-model.md) | **Must read before touching gdext.** tokio↔main-thread split, dispatcher, log pump |
| [Build & Package](reference/build-and-package.md) | `build.py` commands, CI gates, hot-reload tips, file-lock recovery |

## Per-Crate Docs

| Crate | File | Description |
|-------|------|-------------|
| [core](crates/core.md) | `crates/core/src/` | Shared protocol types (IpcRequest, IpcResponse, ToolCallParams, ToolManifest) |
| [server](crates/server.md) | `crates/server/src/` | MCP server binary: rmcp stdio transport, tool registry, WebSocket bridge |
| [gdext](crates/gdext.md) | `crates/gdext/src/` | GDExtension cdylib: editor plugin, WebSocket server, 99 command handlers |

## Module Docs

| Module | Description |
|--------|-------------|
| [Command Routing](modules/command-routing.md) | Full call chain from MCP `call_tool` to `cmd_*`; adding a tool requires both sides |
| [Scene Commands](modules/scene-commands.md) | Node/property/scene tool patterns, `j2v`/`v2j` JSON↔Variant rules, `resolve_node`, `try_load` |
| [IPC Bridge](modules/ipc-bridge.md) | gdext-side `IpcWebSocketServer` + server-side `GodotBridge` WebSocket communication |
| [Dispatcher](modules/dispatcher.md) | `MainThreadDispatcher`: tokio worker → main-thread closure execution |
| [Logging](modules/logging.md) | mpsc log channel + `process_frame` pump (Godot macros panic off main thread) |
| [Editor Plugin](modules/editor-plugin.md) | `McpEditorPlugin` lifecycle; why `process()` is intentionally empty |
| [Dock UI](modules/dock-ui.md) | Right-dock VBox panel, 4 sub-panels, current vs planned state |

## Reference Docs

| Document | Description |
|----------|-------------|
| [Tools Catalog](reference/tools-catalog.md) | All 99 tools with JSON Schema, parameters, return values |
| [Client Configuration](reference/client-config.md) | 12 AI clients × stdio config templates (stdio only) |
| [Client Quirks](reference/client-quirks.md) | Per-client config gotchas quick-reference |
| [Build & Package](reference/build-and-package.md) | `build.py` flags, CI gate order, hot-reload, file-lock recovery |
| [Editor Control](reference/editor-control.md) | 3 server-side `godot_editor_*` tools (open, close, restart) |

## Specification

| Document | Description |
|----------|-------------|
| [IPC Protocol](specification/ipc-protocol.md) | IpcRequest/IpcResponse/IpcNotification/ToolCallParams wire format |
| [Cargo Workspace](specification/workspace.md) | Per-crate Cargo.toml, pinned deps, addon manifest |

## Design

| Document | Description |
|----------|-------------|
| [Design Decisions](design/decisions.md) | ADR-style architecture choice records |
| [Changelog](log.md) | Append-only project change log |

## Stale Doc Warnings

- **`README.md`** says "35 tools" — actual count is **99**. Don't trust that number.
- This index is derived from current source code analysis.
