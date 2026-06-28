# Client Setup

## Quick Reference

| Client | Config File | Key/Format |
|--------|-------------|-----------|
| Claude Code | `.mcp.json` | `mcpServers` |
| Cursor | `.cursor/mcp.json` | `mcpServers` |
| VS Code / Copilot | `.vscode/mcp.json` | `servers` |
| Cline | `.cline/mcp.json` | `mcpServers` |
| OpenCode | `.opencode/opencode.json` | `mcp` |
| Codex | `.codex/config.toml` | TOML format |
| Trae / Trae CN | `.trae/mcp.json` | `mcpServers` |
| Qoder | `.qoder/mcp.json` | `mcpServers` |
| CodeBuddy | `.codebuddy/mcp_settings.json` | `mcpServers` |
| Pi | `.pi/settings.json` | `mcp` |
| OpenClaw | `.openclaw/openclaw.json` | `mcpServers` |

All clients use the same command to verify: `curl -X POST http://127.0.0.1:9600/mcp -H "Content-Type: application/json" -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"get_info","arguments":{}}}'`

## Universal Configuration

All clients share the same Streamable HTTP configuration:

```json
{
  "mcpServers": {
    "godot-mcp": {
      "type": "streamable-http",
      "url": "http://127.0.0.1:9600/mcp"
    }
  }
}
```

## Client-Specific Notes

### Project-Level vs Global Config

Always configure MCP at the **project level** (not global/user level). Each Godot project needs its own MCP configuration pointing to the same local server.

### OpenCode

File: `.opencode/opencode.json` in your project root.

```json
{
  "mcp": {
    "godot": {
      "type": "remote",
      "url": "http://127.0.0.1:9600/mcp"
    }
  }
}
```

### Cursor

File: `.cursor/mcp.json` in your project root.

```json
{
  "mcpServers": {
    "godot": {
      "url": "http://127.0.0.1:9600/mcp"
    }
  }
}
```

After editing, restart Cursor. In Cursor's MCP tab, verify the server shows green.

### VS Code / Copilot

File: `.vscode/mcp.json` in your project root.

```json
{
  "servers": {
    "godot": {
      "type": "http",
      "url": "http://127.0.0.1:9600/mcp"
    }
  }
}
```

After editing, reload the VS Code window.

### Claude Code

File: `.mcp.json` in your project root.

```json
{
  "mcpServers": {
    "godot": {
      "type": "http",
      "url": "http://127.0.0.1:9600/mcp"
    }
  }
}
```

### Cline

File: `.cline/mcp.json` in your project root.

```json
{
  "mcpServers": {
    "godot": {
      "type": "http",
      "command": "npx",
      "args": ["-y", "mcp-remote", "http://127.0.0.1:9600/mcp"]
    }
  }
}
```

### Trae / Trae CN

File: `.trae/mcp.json` in your project root.

```json
{
  "mcpServers": {
    "godot": {
      "url": "http://127.0.0.1:9600/mcp"
    }
  }
}
```

### Codex

File: `.codex/config.toml` in your project root.

```toml
[mcp_servers.godot]
enabled = true
url = "http://127.0.0.1:9600/mcp"
transport = "streamable_http"
```

### Qoder

File: `.qoder/mcp.json` in your project root.

```json
{
  "mcpServers": {
    "godot": {
      "transport": "http",
      "url": "http://127.0.0.1:9600/mcp"
    }
  }
}
```

### CodeBuddy

File: `.codebuddy/mcp_settings.json` in your project root.

```json
{
  "mcpServers": {
    "godot": {
      "type": "stdio",
      "command": "npx",
      "args": ["-y", "mcp-remote", "http://127.0.0.1:9600/mcp"]
    }
  }
}
```

### Pi

File: `.pi/settings.json` in your project root.

```json
{
  "mcp": {
    "mcpServers": {
      "godot": {
        "url": "http://127.0.0.1:9600/mcp"
      }
    }
  }
}
```

### OpenClaw

File: `.openclaw/openclaw.json` in your project root.

```json
{
  "mcpServers": {
    "godot": {
      "url": "http://127.0.0.1:9600/mcp"
    }
  }
}
```

## Generating Client Config

Open the **GodotMCP** bottom panel in the editor to auto-generate config for any supported client. Select your client from the dropdown and click **Generate** to see the config, or use `get_info(include_configs=true)` to get client config snippets via the API.

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Connection refused | Godot not running or wrong port | Start Godot editor, verify port |
| 404 Not Found | Wrong URL path | Must be `/mcp` endpoint |
| Server not showing in client | Config in wrong location | Use project-level config, not global |
| Timeout | Firewall or port blocked | Allow port 9600 in firewall |