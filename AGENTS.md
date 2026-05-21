# GodotMCP — Agent Instructions

## Current state

Phase 1 steps 1–5 complete. See `.repo_wiki/` for full planning (`index.md` → architecture/tools/ipc-bridge/phases).

| Milestone | Status |
|-----------|--------|
| Cargo workspace + `crates/core` protocol types | Done |
| GDExtension skeleton (EditorPlugin + WS server on `:9500`) | Done |
| MCP Server (rmcp stdio + 4 tools) | Done |
| WebSocket IPC bridge end-to-end (ping/pong) | Done |
| Dock UI panel | Not started |
| 48-tool full set (only 4 stubs exist) | Not started |
| Streamable HTTP transport (`transports/` dir empty) | Not started |
| rust-toolchain.toml | Missing |

Empty directories exist but are not `mod`-declared: `crates/gdext/src/commands/`, `crates/server/src/transports/`.

No tests exist yet. `Cargo.lock` is `.gitignore`-d but should be committed for binary crates.

## Build & verify

```bash
python package_addons.py              # One-shot: build both crates + copy libs + zip addons.zip
cargo check --workspace               # Fast type-check everything
cargo build -p godot-mcp-gdext        # Build GDExtension shared library (debug)
cargo build -p godot-mcp-server       # Build MCP server binary (debug)
```

Release builds use `--release`. There is no `Makefile`, `Justfile`, or test suite.

## Architecture

- **Two processes**: `godot-mcp-server` (standalone binary) ↔ WebSocket `ws://127.0.0.1:9500` ↔ `godot_mcp_gdext` (cdylib inside Godot Editor)
- **MCP transport**: stdio only for now (AI clients spawn server as subprocess). Streamable HTTP on `:8900/mcp` is planned but not implemented.
- **IPC protocol**: JSON-RPC 2.0 over WebSocket. Types: `crates/core/src/protocol.rs`
- **Target engine**: Godot **4.6+** (gdext v0.5, entry symbol `gdext_rust_init`)

## Critical conventions

- **GDExtension EditorPlugin** must register at `InitLevel::Editor`. `InitLevel::Scene` will not work.
- **tokio runtime** inside GDExtension runs on a separate thread. DO NOT block Godot's main thread. Use `builder().worker_threads(2)`.
- **plugin.cfg** `script=""` is intentional — all logic is in the native GDExtension.
- **Tool hot-switching** (planned, not implemented): per-tool CheckBox in Dock → IPC `tool_list_updated` → Server rebuilds `#[tool_router]`. Granularity is per-tool, not per-category.

## Key dependencies

| Crate | Version | Feature flags |
|-------|---------|--------------|
| `godot` | `=0.5` | `default` |
| `rmcp` | `=1.7` | `server`, `macros`, `schemars`, `transport-io`, `transport-streamable-http-server` |
| `tokio` | `1` | `full` |
| `tokio-tungstenite` | `0.24` | — |
| `axum` | `0.8` | — |
| `clap` | `4` | `derive` |

## Client config reference

Full config templates in `.repo_wiki/guide/client-config.md`. Key differences worth remembering:

- **GitHub Copilot**: key is `servers` (not `mcpServers`), each entry needs `type: "stdio"` or `type: "http"`
- **Antigravity**: HTTP URL field is `serverUrl` (not `url`)
- **Gemini CLI / Qwen Code**: HTTP URL field is `httpUrl` (`url` = SSE)
- **Codex**: TOML format, not JSON
- **CodeBuddy**: needs explicit `type` field