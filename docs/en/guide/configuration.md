# Configuration

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `GODOT_MCP_HTTP_PORT` | `9600` | HTTP server port |
| `GODOT_MCP_HTTP_HOST` | `127.0.0.1` | HTTP server bind address |
| `GODOT_MCP_BRIDGE_PORT` | `9601` | Runtime bridge TCP port |

Set these in your system environment or launch Godot with them:

```bash
# Windows (Command Prompt)
set GODOT_MCP_HTTP_PORT=9600
godot --editor

# Windows (PowerShell)
$env:GODOT_MCP_HTTP_PORT = 9600
godot --editor

# Linux/macOS
GODOT_MCP_HTTP_PORT=9600 godot --editor
```

## ProjectSettings

The HTTP port can also be configured via ProjectSettings:

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `godot_mcp/http_port` | int | `9600` | HTTP server port (overrides env var) |

To change it:

1. Go to **Project > Project Settings**
2. Search for `godot_mcp`
3. Set the desired port value
4. Restart the editor

## Port Conflict Resolution

If port 9600 is already in use:

1. Check what's using it: `netstat -ano | findstr :9600`
2. Set a different port via env var or ProjectSettings
3. Update your AI client's MCP configuration to match
4. Restart Godot

## Aggregated Project Settings Tools

GodotMCP provides 10 aggregated settings tools (5 get/set pairs) that bundle related ProjectSettings properties:

| Category | Get Tool | Set Tool | Properties Covered |
|----------|----------|----------|-------------------|
| Display | `get_display_settings` | `set_display_settings` | Window size, mode, vsync, MSAA, scaling |
| Project Info | `get_project_info_settings` | `set_project_info_settings` | Name, description, version, company |
| Physics | `get_physics_settings` | `set_physics_settings` | FPS, gravity, damping, layers |
| Rendering | `get_rendering_settings` | `set_rendering_settings` | Renderer, quality, shadows, reflections |
| Layer Names | `get_layer_names_settings` | `set_layer_names_settings` | 2D/3D physics and render layer names |

These tools save you from making 20+ individual `set_setting` calls for common configuration tasks.