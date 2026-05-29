# Godot MCP

[![Version](https://img.shields.io/badge/version-0.1.5--dev3-blue?logo=github)](https://github.com/jessp/godot-mcp)
[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B)](https://isocpp.org)
[![Python](https://img.shields.io/badge/Python-3.13%2B-3776AB?logo=python)](https://python.org)
[![Godot](https://img.shields.io/badge/Godot-4.6%2B-478cbf?logo=godot%20engine)](https://godotengine.org)
[![MCP](https://badge.mcpx.dev/?type=plugin&plugin_id=github.com/jessp/godot-mcp&logo=true)](https://modelcontextprotocol.io)
[![License](https://img.shields.io/badge/License-MIT-green?logo=open-source-initiative)](License)

> Model Context Protocol bridge that lets AI assistants control the Godot Engine editor.

*[中文文档](README-zh.md)*

```mermaid
graph LR
    subgraph AI["AI Client"]
        Client["Claude Code / OpenCode /<br/>Cursor / Copilot / …"]
    end

    subgraph ServerProc["godot-mcp-server"]
        Stdio["MCP stdio Transport"]
        Handler["GodotMcpHandler"]
        Registry["ToolRegistry<br/>(125 tool schemas)"]
        Bridge["GodotBridge<br/>(WS client)"]
    end

    subgraph GodotProc["Godot Editor"]
        WS["WsServer<br/>(:9500)"]
        HTTP["HttpServer + McpHandler<br/>(:9600, MCP Streamable HTTP)"]
        Registry2["HandlerRegistry<br/>(115 active commands)"]
        Editor["EditorInterface /<br/>SceneTree / Node API"]
    end

    Client -->|stdio JSON-RPC| Stdio
    Stdio --> Handler
    Handler --> Registry
    Handler --> Bridge
    Bridge <-->|ws://127.0.0.1:9500<br/>tool_call IPC| WS
    Client -->|HTTP POST /mcp<br/>JSON-RPC 2.0| HTTP
    WS --> Registry2
    HTTP --> Registry2
    Registry2 --> Editor
```

Godot MCP exposes the Godot 4.6+ editor to AI tools through **125 commands** — create nodes, modify properties, manage scenes, inspect the scene tree, edit GDScript/C# files, and more.

## Features

- **125 Editor Commands** — Scene/node manipulation, properties, search, undo/redo, collision shapes, GDScript/C# script management, LSP validation, file search/replace, project settings, multi-scene operations
- **Dual Transport** — Legacy stdio/WebSocket path via Python server, and direct MCP Streamable HTTP (`:9600`) into the GDExtension
- **Dual-Process Architecture** — Python/Cython MCP server + C++ GDExtension plugin (godot-cpp 10.0.0-rc1)
- **Pure Main-Thread C++** — No worker threads, no tokio, no locks. Everything runs on Godot's main thread via `process_frame`
- **12 AI Client Support** — Claude Code, OpenCode, Cursor, GitHub Copilot, Codex, Trae, and more (stdio transport)
- **Cross-Platform** — Windows, macOS, and Linux
- **36 Offline Tests** — Protocol round-trips, tool registry correctness, handler dispatch, and editor control tests (no Godot needed)

## How It Works

### Path A: Legacy stdio (via Python server)

```
AI Assistant ──► godot-mcp-server ──► godot_mcp_gdext.dll
   (stdio)      (Python/Cython)  ws://127.0.0.1:9500   (C++ GDExtension)
```

1. Your AI client launches `godot-mcp-server` and speaks to it over stdio (MCP protocol).
2. The server forwards tool calls to the Godot editor plugin via WebSocket on `localhost:9500`.
3. The plugin dispatches each call to the Godot main thread via `EditorPlugin::_on_process_frame()`, executes editor APIs safely, and returns results.

### Path B: MCP Streamable HTTP (direct)

```
AI Assistant ──► godot_mcp_gdext.dll
   (HTTP POST /mcp, :9600)     (C++ GDExtension, JSON-RPC 2.0)
```

AI clients can connect directly to the GDExtension's HTTP server on `localhost:9600`, bypassing the Python server entirely. Supports SSE for server-initiated events.

## Installation

### Prerequisites

- [Godot 4.6+](https://godotengine.org/download)
- [CMake 3.22+](https://cmake.org/download)
- [Python 3.13+](https://www.python.org)
- [Visual Studio 2022](https://visualstudio.microsoft.com) (Windows) with C++ toolchain, or equivalent on macOS/Linux

### Build

```bash
git clone https://github.com/jessp/godot-mcp.git
cd godot-mcp
py -3 build.py
```

This produces:
- `build/addons.zip` — extract into any Godot project to install the editor plugin
- `build/godot-mcp-server.exe` (Windows) or `build/godot-mcp-server` (Unix)

> **On Windows**, always use `py -3` instead of `python` — the Microsoft Store stubs hang silently.

### Install the Plugin in Godot

1. Extract `build/addons.zip` into your Godot project root.
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
      "args": [],
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

1. **Start the Godot editor** with the plugin enabled — the servers automatically start on ports 9500 (WebSocket) and 9600 (HTTP).
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

### Available Tools (125 total)

| Category | Count | Tools |
|----------|-------|-------|
| Meta | 4 | `ping`, `get_engine_version`, `get_plugin_version`, `get_server_version` |
| Node: Read | 4 | `get_scene_tree`, `get_node_path`, `get_property`, `get_property_list` |
| Node: Write | 13 | `create/delete/rename/duplicate/move` node, `set_property`, `reset_parent`, `set_as_root`, `batch_set_property`, `attach/detach_script`, `add/remove_node_from_group` |
| 2D Properties | 21 | `get/set_node_position/rotation/scale`, `get/set_node_visible/modulate/z_index/text`, `get/set_node_collision_layer/mask`, `get/set_node_texture`, `set_node_unique_name` |
| 3D Properties | 6 | `get/set_node_position_3d/rotation_3d/scale_3d` |
| Collision | 2 | `add_circle_collision`, `add_rectangle_collision` |
| Node Search | 4 | `find_nodes_by_name/type/group/script` |
| Script Helpers | 3 | `call_method`, `get_variable`, `set_variable` |
| Project Settings | 7 | `get/set_project_setting`, `set_main_scene`, `list/add/remove_autoload`, `list_scenes` |
| Scene: File | 6 | `create/delete/rename_scene`, `branch_to_scene`, `scene_to_branch`, `instantiate_scene` |
| Scene: Editor Tabs | 9 | `open/close/save/save_as/save_all/reload_scene`, `get_open_scenes/roots`, `mark_scene_unsaved` |
| GDScript | 5 | `create/edit/read/list_gdscript`, `validate_gdscript` |
| C# | 6 | `csharp_create_solution`, `create/edit/read/list_csharp_script`, `csharp_build` |
| Search | 3 | `find_in_file`, `search_project`, `find_and_replace` |
| Editor Control (server) | 3 | `godot_editor_open/close/restart` |
| Editor Control (gdext) | 6 | `play_current_scene`, `play_main_scene`, `stop_scene`, `is_scene_playing`, `refresh_filesystem`, `get_editor_info` |
| Undo/Redo | 2 | `undo`, `redo` |
| Node Convenience | 4 | `set_node_transform_2d/3d`, `get_node_info`, `get_script_variables` |
| Scene Info | 1 | `is_scene_dirty` |
| Display Settings | 2 | `get/set_display_settings` |
| Project Info | 2 | `get/set_project_info` |
| Physics Settings | 2 | `get/set_physics_settings` |
| Rendering Settings | 2 | `get/set_rendering_settings` |
| Layer Names | 2 | `get/set_layer_names` |
| Plugin Management | 2 | `list_plugins`, `set_plugin_enabled` |
| Input Map | 4 | `list/add/remove_input_action`, `set_input_action_events` |

See the [Tool Catalog](.repo_wiki/reference/tools-catalog.md) for detailed argument shapes and return values.

## Development

### Project Structure

```
server/                     Python MCP server (Cython --embed → .exe)
├── src/godot_mcp_server/
│   ├── protocol.py         IPC protocol models
│   ├── registry.py         Tool schema authority (125 tools)
│   ├── handler.py          GodotMcpHandler — tool call dispatch
│   ├── bridge.py           GodotBridge — WebSocket client
│   └── editor_ctl.py       Editor process control
├── tests/                  36 offline tests
└── entry.py                Cython entry point

extensions/gdext/           C++ GDExtension plugin (godot-cpp 10.0.0-rc1)
├── CMakeLists.txt
└── src/
    ├── register_types.cpp  GDExtension entry (symbol: gdext_rust_init)
    ├── editor_plugin.cpp   EditorPlugin — dual-server poll via _on_process_frame()
    ├── ipc/
    │   ├── ws_server.cpp   WebSocket server :9500 (legacy IPC)
    │   └── http_server.cpp MCP Streamable HTTP server :9600
    ├── mcp/
    │   └── mcp_handler.cpp MCP JSON-RPC 2.0 session management
    ├── lsp/client.cpp      Godot LSP client (GDScript validation)
    ├── protocol/ipc_types.hpp
    └── commands/           17 handler files (16 active groups)
        ├── handler_registry.cpp/hpp
        ├── cmd_utils.cpp/hpp
        └── cmd_<group>.cpp
```

### CI Gates

```bash
cmake -B build -S .                           # Configure CMake
cmake --build build --config Debug            # Build gdext + server
pytest                                         # 36 tests (offline, no Godot)
ruff check server/src                         # Lint
mypy server/src --strict                      # Type check
```

### Build Flags

```bash
py -3 build.py                                # Debug + addons.zip
py -3 build.py --release                      # Release + addons.zip
py -3 build.py --clean                        # Clear CMake cache (keeps _deps/)
py -3 build.py --no-zip                       # Skip zip (fast iteration)
py -3 build.py --no-server                    # DLL only (editor changes)
```

### File Lock Tips

- **Server binary locked by MCP client** → `build.py` auto-kills it via `taskkill`/`pkill`; manual builds: `taskkill /F /IM godot-mcp-server.exe` first.
- **DLL locked by Godot editor** → close editor or disable plugin before rebuilding.

### Key Constraints

- **Pinned deps**: `godot-cpp` at `10.0.0-rc1` (FetchContent). Don't bump without testing.
- **Python**: `>=3.13`, dependency management via `uv`.
- **`godot_mcp.gdextension`**: entry symbol `gdext_rust_init`, `compatibility_minimum = "4.6"`.
- **Version** maintained in `CMakeLists.txt` (`set(PROJECT_VERSION "...")`). Only change there — `plugin.cfg` is generated by CMake.
- **Adding a tool** requires registering the schema in `server/src/godot_mcp_server/registry.py` AND adding a `register_<group>()` function in `extensions/gdext/src/commands/handler_registry.cpp`.

## Documentation

- [Architecture Overview](.repo_wiki/overview/architecture.md) — Dual-transport architecture, Python server + C++ GDExtension
- [Threading Model](.repo_wiki/overview/threading-model.md) — Pure main-thread, dual-server poll
- [Tool Catalog](.repo_wiki/reference/tools-catalog.md) — All 125 tools with args and return shapes
- [IPC Protocol](.repo_wiki/specification/ipc-protocol.md) — WebSocket IPC + MCP Streamable HTTP wire format
- [Client Configuration](.repo_wiki/reference/client-config.md) — AI client config templates
- [Build & Package](.repo_wiki/reference/build-and-package.md) — Build flags, CI gates, gotchas
- [Design Decisions](.repo_wiki/design/decisions.md) — Recorded architectural choices
