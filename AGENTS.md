# GodotMCP — Agent Instructions

## State of the repo

This is a clean-slate project. No code has been written yet. All planning is in `.repo_wiki/` — **read it first** (`index.md` → architecture/tools/ipc-bridge/phases).

## Build artifacts

| Artifact | Crate | Cargo target |
|----------|-------|-------------|
| `godot-mcp-server` | `crates/server` | `[[bin]]` |
| `godot_mcp_gdext.{dll,so,dylib}` | `crates/gdext` | `crate-type = ["cdylib"]` |

Both built from a Cargo workspace (`Cargo.toml` not yet created). The GDExtension is a **shared library** loaded by Godot, not a standalone binary.

## Architecture highlights

- **Two processes**: `godot-mcp-server` (standalone binary) ↔ WebSocket `ws://127.0.0.1:9500` ↔ `godot_mcp_gdext` (cdylib inside Godot Editor process)
- **MCP transport**: stdio (AI clients spawn server as subprocess) + Streamable HTTP on `:8900/mcp`
- **IPC protocol**: JSON-RPC 2.0 over WebSocket. Types defined in `crates/core/src/protocol.rs`
- **UI**: EditorPlugin in the GDExtension adds a Dock panel via `add_control_to_dock(DockSlot::RIGHT_UL, ...)`
- **Target engine**: Godot **4.6+** (gdext v0.5 default API level)

## Critical conventions

- **GDExtension EditorPlugin** must be registered at `InitLevel::Editor`. Registering at `InitLevel::Scene` will not work.
- **tokio runtime** inside GDExtension runs on a separate thread. DO NOT block Godot's main thread. Use `builder().worker_threads(2)`.
- **tool hot-switching**: Dock panel individual tool CheckBox → IPC notification `tool_list_updated` → Server rebuilds `#[tool_router]`. Granularity is per-tool, not per-category.
- **plugin.cfg** `script=""` is intentional (all logic is in the native GDExtension, not GDScript/C#).

## Supported MCP clients (12)

| Client | Default | Config path | Key / Notes |
|--------|---------|-------------|-------------|
| Claude Code | stdio | `~/.claude/mcp.json` | `mcpServers` |
| Codex | HTTP | `~/.codex/config.toml` | TOML format, `[mcp_servers."godot-mcp"]` |
| Gemini CLI | HTTP | `~/.gemini/settings.json` | `mcpServers`, HTTP uses `httpUrl` |
| OpenCode | stdio | `~/.config/opencode/config.json` | `mcpServers` |
| Cursor | HTTP | `.cursor/mcp.json` (project root) | `mcpServers` |
| GitHub Copilot | HTTP | `.vscode/mcp.json` (project root) | **`servers`** (not `mcpServers`), must have `type` |
| Qwen Code | HTTP | `~/.qwen/settings.json` | `mcpServers`, HTTP uses `httpUrl` |
| Trae | HTTP | `.trae/mcp.json` (project root) | `mcpServers` |
| Trae CN | HTTP | `.trae/mcp.json` (project root, same as Trae) | `mcpServers` |
| Qoder | HTTP | Platform-specific (see wiki) | `mcpServers` |
| Antigravity | HTTP | `~/.gemini/antigravity/mcp_config.json` | HTTP uses **`serverUrl`** (not `url`) |
| CodeBuddy | HTTP | `~/.codebuddy/.mcp.json` | `mcpServers`, requires `type` field |

### Client key differences

- GitHub Copilot: key is `servers`, each entry needs `type: "stdio"` or `type: "http"`
- Antigravity: HTTP URL field is `serverUrl`
- Gemini CLI / Qwen Code: HTTP URL field is `httpUrl` (`url` = SSE)
- Codex: TOML format, not JSON
- Trae CN: identical config to Trae (`.trae/mcp.json`)
- CodeBuddy: needs explicit `type` field (`"stdio"` / `"sse"` / `"http"`)

## Implementation order (Phase 1)

1. `Cargo.toml` workspace + `crates/core` (protocol types, unit tests for serde round-trip)
2. `rust-toolchain.toml` (edition = "2024")
3. `crates/gdext` — skeleton `#[gdextension]` + `#[class(tool, editor_plugin)]` + empty dock
4. `crates/server` — CLI + stdio transport + `ping` tool
5. WebSocket IPC bridge (both sides)
6. End-to-end `ping → pong` test

## Verify commands

```bash
cargo check --workspace
cargo build --release -p godot-mcp-server
cargo build --release -p godot-mcp-gdext
cargo test -p godot-mcp-core
```

## Key dependencies (lock these versions)

| Crate | Version | Feature flags |
|-------|---------|--------------|
| `godot` | `=0.5` | `codegen` |
| `rmcp` | `=1.7` | `server`, `macros`, `schemars`, `transport-io`, `transport-streamable-http-server` |
| `tokio` | `1` | `full` |
| `tokio-tungstenite` | `0.24` | — |
| `axum` | `0.8` | — |
| `clap` | `4` | `derive` |
