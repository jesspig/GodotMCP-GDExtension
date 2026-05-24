# Client Configuration

> `godot-mcp-server` uses **stdio transport only**. All AI clients must be configured via stdio.

## Universal Config Structure

Every client needs two things:
1. `command` — path to `godot-mcp-server.exe`
2. `env.GODOT_PATH` — path to Godot editor executable

```json
{
  "mcpServers": {
    "godot-mcp": {
      "command": "/absolute/path/to/godot-mcp-server",
      "args": [],
      "env": {
        "GODOT_PATH": "/absolute/path/to/godot/Godot_v4.6-stable_win64.exe"
      }
    }
  }
}
```

## Example Configurations

### VS Code (Roo Code / Continue)

**`~/.config/Code/User/globalStorage/rooveterinaryinc.roo-cline/settings/cline_customMCPServers.json`**

```json
{
  "command": "C:\\Tools\\godot-mcp-server.exe",
  "env": {
    "GODOT_PATH": "C:\\Godot\\Godot_v4.6-stable_win64.exe"
  }
}
```

### Claude Code

Configure in `claude.json` or `CLAUDE.md`.

### Cursor

**`~/.cursor/mcp.json`**

```json
{
  "mcpServers": {
    "godot-mcp": {
      "command": "C:/Tools/godot-mcp-server.exe",
      "env": {
        "GODOT_PATH": "C:/Godot/Godot_v4.6-stable_win64.exe"
      }
    }
  }
}
```

### JetBrains

Configure in IDE MCP settings panel.

## Notes

- **Windows paths**: use forward slashes (`/`) or double backslashes (`\\`)
- `GODOT_PATH` **cannot** be inherited from shell profile — stdio servers don't inherit terminal env
- **Restart editor/client**: must restart the MCP-using editor after config changes
- **Restart after rebuild**: if `godot-mcp-server.exe` is rebuilt, restart the MCP-using editor process
