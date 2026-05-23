# Editor Control

> 3 server-side tools (`godot_editor_*`) handled in `handler.rs`, never reaching gdext.

```mermaid
flowchart TD
    CLIENT["AI Client"] -->|call_tool("godot_editor_open")| SERVER["godot-mcp-server"]
    SERVER -->|handler.rs intercepts| OPEN["Editor Control"]
    
    subgraph OPEN[Editor Control Handlers]
        O["godot_editor_open"]
        C["godot_editor_close"]
        R["godot_editor_restart"]
    end
    
    O -->|"taskkill /F /IM godot.exe"| OS["OS"]
    O -->|"spawn new process"| GODOT["Godot Editor"]
    C -->|"taskkill /F /IM godot.exe"| OS
    R -->|"kill + 500ms wait + respawn"| OS
```

## `godot_editor_open`

- `project_path` resolved from CWD (default `./godot/`)
- Resolves `GODOT_PATH` env var — required
- Spawns Godot editor process
- Waits for WebSocket connection (~60s max)
- Returns `{"status": "ok"}`

## `godot_editor_close`

- Windows: `taskkill /F /IM godot*.exe` (wildcard to match various Godot executables)
- Unix: `pkill -f Godot`
- Returns `{"status": "ok"}`

**Note**: Response sent directly from server process — editor killed = WebSocket drops, gdext can't respond.

## `godot_editor_restart`

1. Call `close` (kill editor process)
2. Wait 500ms (ensure process fully terminated)
3. Call `open` (using `project_path` param)

## Environment Variable

| Var | Required | Description |
|-----|----------|-------------|
| `GODOT_PATH` | Yes | Full path to Godot executable |

## Project Path Resolution

- Default: `./godot/` (relative to CWD)
- Users can pass absolute or relative paths
- Path resolution happens in the server process using `std::env::current_dir()` as base
