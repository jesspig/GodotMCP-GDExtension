# Godot MCP

[![Version](https://img.shields.io/badge/version-0.1.1-blue?logo=github)](https://github.com/jessp/godot-mcp)
[![Rust](https://img.shields.io/badge/Rust-2024%20edition-orange?logo=rust)](https://www.rust-lang.org)
[![Godot](https://img.shields.io/badge/Godot-4.6%2B-478cbf?logo=godot%20engine)](https://godotengine.org)
[![MCP](https://badge.mcpx.dev/?type=plugin&plugin_id=github.com/jessp/godot-mcp&logo=true)](https://modelcontextprotocol.io)
[![License](https://img.shields.io/badge/License-MIT-green?logo=open-source-initiative)](License)

> Model Context Protocol bridge that lets AI assistants control the Godot Engine editor.

*[中文文档](README.zh-CN.md)*

```mermaid
graph LR
    subgraph AI["AI Client"]
        Client["Claude Code / OpenCode /<br/>Cursor / Copilot / …"]
    end

    subgraph ServerProc["godot-mcp-server"]
        Stdio["rmcp stdio Transport"]
        Handler["GodotMcpHandler"]
        Registry["ToolRegistry<br/>(99 tool schemas)"]
        Bridge["GodotBridge<br/>(WS client)"]
    end

    subgraph GodotProc["Godot Editor"]
        WS["IpcWebSocketServer"]
        Dispatcher["MainThreadDispatcher"]
        Commands["13 handler groups<br/>96 gdext commands"]
        Editor["EditorInterface /<br/>SceneTree / Node API"]
    end

    Client -->|stdio JSON-RPC| Stdio
    Stdio --> Handler
    Handler --> Registry
    Handler --> Bridge
    Bridge <-->|ws://127.0.0.1:9500<br/>tool_call| WS
    WS --> Dispatcher
    Dispatcher --> Commands
    Commands --> Editor
```

Godot MCP exposes the Godot 4.6+ editor to AI tools through **99 commands** — create nodes, modify properties, manage scenes, inspect the scene tree, edit GDScript/C# files, and more.

## Features

- **99 Editor Commands** — Scene/node manipulation, properties, search, undo/redo, collision shapes, GDScript/C# script management, LSP validation, file search/replace, project settings, multi-scene operations
- **Dual-Process Architecture** — stdio MCP server + in-editor GDExtension plugin, connected via local WebSocket
- **Thread-Safe Design** — Async tokio runtime paired with a main-thread dispatcher for safe Godot API access
- **12 AI Client Support** — Claude Code, OpenCode, Cursor, GitHub Copilot, Codex, Trae, and more (stdio transport)
- **Cross-Platform** — Windows, macOS, and Linux
- **50 Offline Tests** — Protocol round-trips and tool registry correctness (no Godot needed)

## How It Works

```
AI Assistant ──► godot-mcp-server ──► godot_mcp_gdext
   (stdio)       (Rust binary)    ws://127.0.0.1:9500   (GDExtension plugin)
```

1. Your AI client launches `godot-mcp-server` and speaks to it over stdio (MCP protocol).
2. The server forwards tool calls to the Godot editor plugin via WebSocket on `localhost:9500`.
3. The plugin dispatches each call to the Godot main thread, executes editor APIs safely, and returns results.
4. The server relays results back to the AI client as MCP responses.

## Installation

### Prerequisites

- [Godot 4.6+](https://godotengine.org/download)
- [Rust](https://rustup.rs) (channel 1.83.0, pinned in `rust-toolchain.toml`)
- [Python 3](https://www.python.org) (for the build script)

### Build

```bash
git clone https://github.com/jessp/godot-mcp.git
cd godot-mcp
py -3 build.py
```

This produces:
- `addons.zip` — extract into any Godot project to install the editor plugin
- `target/debug/godot-mcp-server.exe` (Windows) or `godot-mcp-server` (Unix)

> **On Windows**, always use `py -3` instead of `python` — the Microsoft Store stubs hang silently.

### Install the Plugin in Godot

1. Extract `addons.zip` into your Godot project root.
2. Open the project in Godot.
3. Go to **Project → Project Settings → Plugins** and enable **Godot MCP**.
4. You should see `[Godot MCP] Plugin loaded!` in the Output panel.

### Configure Your AI Client

Add this to your MCP client config:

```json
{
  "mcpServers": {
    "godot-mcp": {
      "command": "/path/to/godot-mcp-server",
      "args": ["--godot-port", "9500"],
      "env": {
        "GODOT_PATH": "/path/to/Godot_v4.6-stable_win64.exe"
      }
    }
  }
}
```

`GODOT_PATH` is required for editor control tools (`godot_editor_open/close/restart`). Stdio servers don't inherit shell env, so it must be in the `env` block.

### Client Config Locations

| Client | Config Path |
|--------|-------------|
| Claude Code | `~/.claude/mcp.json` |
| OpenCode | `~/.config/opencode/config.json` |
| Cursor | `<project>/.cursor/mcp.json` |
| GitHub Copilot | `<project>/.vscode/mcp.json` |
| Trae / Trae CN | `<project>/.trae/mcp.json` |
| Codex | `~/.codex/config.toml` |

> After rebuilding the server, restart your MCP client — it keeps the old binary handle alive.

## Usage

1. **Start the Godot editor** with the plugin enabled — the WebSocket server automatically starts on port 9500.
2. **Connect your AI client** using the config above.
3. **Call any tool** from your AI assistant.

### Quick Examples

```
# Check the connection
"ping the godot editor"

# Create a scene and populate it
"open scene res://main.tscn"
"create a Node2D called Player under the root"

# Inspect and modify
"get the scene tree"
"set the Player's position to x=100, y=200"
"attach the script res://player.gd to the Player node"
```

### Available Tools (99 total)

| Category | Count | Tools |
|----------|-------|-------|
| Meta | 4 | `ping`, `get_engine_version`, `get_plugin_version`, `get_server_version` |
| Node CRUD | 17 | `create/delete/rename/move/duplicate` node, `reset_parent`, `set_as_root`, `attach/detach_script`, group mgmt, `set_node_unique_name`, `get_node_path/info`, `get_scene_tree`, `scene_to_branch`/`branch_to_scene` |
| 2D Properties | 19 | `get/set_property`, `get_property_list`, `batch_set_property`, `get/set_node_position/rotation/scale/modulate/visible/text/z_index`, `call_method` |
| 3D Properties | 6 | `get/set_node_position_3d/rotation_3d/scale_3d` |
| Collision | 2 | `add_circle_collision`, `add_rectangle_collision` |
| Scene Files | 15 | `create/open/save/save_as/close/reload/rename/delete` scene, `save_all`, `is_scene_dirty`, `mark_scene_unsaved`, `instantiate_scene`, `get_open_scenes/roots`, `set_node_transform_2d/3d` |
| GDScript | 5 | `create/edit/read/list_gdscript`, `validate_gdscript` |
| C# | 6 | `csharp_create_solution`, `create/edit/read/list_csharp_script`, `csharp_build` |
| Search | 3 | `find_in_file`, `search_project`, `find_and_replace` |
| Script Helpers | 3 | `get_script_variables`, `get_variable`, `set_variable` |
| Project Settings | 3 | `get/set_project_setting`, `set_main_scene` |
| Undo/Redo | 2 | `undo`, `redo` |
| Node Search | 4 | `find_nodes_by_name/type/group/script` |
| Editor Control | 3 | `godot_editor_open/close/restart` (server-side) |
| Convenience | 4 | `get/set_node_texture`, `get/set_node_collision_layer/mask` |

See the [Tool Catalog](.repo_wiki/en/reference/tools-catalog.md) for detailed argument shapes and return values.

## Development

### Project Structure

```
crates/
├── core/          Shared protocol types (IpcRequest, IpcResponse, ToolCallParams)
├── server/        MCP server binary — rmcp stdio transport, tool registry, WS client
└── gdext/         GDExtension cdylib — editor plugin, WS server, 99 command handlers
```

### CI Gates

Run these in order before pushing:

```bash
cargo fmt --check --all                       # Formatting
cargo clippy --workspace -- -D warnings       # Linting (strict)
cmake -B build -S .                           # Configure CMake
cmake --build build --config Debug            # Build gdext + server
cargo test --workspace                        # Tests (50, all offline, no Godot)
```

### Build Flags

```bash
py -3 build.py                                # Debug + addons.zip
py -3 build.py --release                      # Release + addons.zip
py -3 build.py --clean                        # cargo clean + wipe addons/bin/
py -3 build.py --no-zip                       # Skip zip (fast iteration)
py -3 build.py --no-server                    # DLL only (editor changes)
```

### File Lock Tips

- **Server binary locked by MCP client** → `build.py` auto-kills it via `taskkill`/`pkill`; manual builds: `taskkill /F /IM godot-mcp-server.exe` first.
- **DLL locked by Godot editor** → close editor or disable plugin before rebuilding.

### Key Constraints

- **Pinned deps**: `godot = "=0.5"`, `rmcp = "=1.7"`. Don't bump without testing.
- **Rust channel**: `1.83.0` (pinned in `rust-toolchain.toml`).
- **`Cargo.lock`** is committed (binary crate).
- **Version** auto-syncs from `Cargo.toml` → `plugin.cfg` via CMake. Bump cargo version only.
- **Adding a tool** requires registering the schema in `tool_registry.rs` AND adding a routing arm in `ws_server.rs::route_tool_call`. Update both `total == 99` assertions in tests.

## Documentation

- [Architecture Overview](.repo_wiki/en/overview/architecture.md) — Two-process, three-crate design
- [Threading Model](.repo_wiki/en/overview/threading-model.md) — tokio ↔ main-thread split and dispatcher pattern
- [Tool Catalog](.repo_wiki/en/reference/tools-catalog.md) — All 99 tools with args and return shapes
- [Client Configuration](.repo_wiki/en/reference/client-config.md) — 12 AI client config templates
- [Build & Package](.repo_wiki/en/reference/build-and-package.md) — Build flags, CI gates, gotchas
- [IPC Protocol](.repo_wiki/en/specification/ipc-protocol.md) — Wire format specification
- [Design Decisions](.repo_wiki/en/design/decisions.md) — Recorded architectural choices
