# Client Setup

## Quick Reference

| Client | Config File | Key |
|--------|------------|-----|
| opencode | `.opencode/opencode.json` | `mcpServers` |
| Cursor | `.cursor/mcp.json` | `mcpServers` |
| VS Code / Copilot | `.vscode/mcp.json` | `servers` |
| Claude Code | `CLAUDE.md` | `mcpServers` |
| Trae | `.trae/mcp.json` | `mcpServers` |
| Cline | `.vscode/cline_mcp_settings.json` | `mcpServers` |
| Roo Code | `.vscode/roocode_settings.json` | `mcpServers` |
| Continue | `~/.continue/config.json` | `experimental.mcpServers` |
| Windsurf | `.windsurf/models.json` | `mcpServers` |

All clients use the same command to verify: `curl http://127.0.0.1:9600/mcp`

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

### opencode

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

### Cursor

File: `.cursor/mcp.json` in your project root.

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

After editing, restart Cursor. In Cursor's MCP tab, verify the server shows green.

### VS Code / Copilot

File: `.vscode/mcp.json` in your project root.

```json
{
  "servers": {
    "godot-mcp": {
      "type": "streamable-http",
      "url": "http://127.0.0.1:9600/mcp"
    }
  }
}
```

After editing, reload the VS Code window.

### Claude Code

Add to your `CLAUDE.md` file:

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

### Cline

File: `.vscode/cline_mcp_settings.json` in your project root.

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

### Roo Code

File: `.vscode/roocode_settings.json` in your project root.

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

### Continue

File: `~/.continue/config.json`

```json
{
  "experimental": {
    "mcpServers": {
      "godot-mcp": {
        "type": "streamable-http",
        "url": "http://127.0.0.1:9600/mcp"
      }
    }
  }
}
```

### Windsurf

File: `.windsurf/models.json` in your project root.

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

## Generating Client Config

Use the `generate_client_config` meta tool to auto-generate the config for your client:

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"generate_client_config","arguments":{"client_type":"cursor"}}}'
```

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Connection refused | Godot not running or wrong port | Start Godot editor, verify port |
| 404 Not Found | Wrong URL path | Must be `/mcp` endpoint |
| Server not showing in client | Config in wrong location | Use project-level config, not global |
| Timeout | Firewall or port blocked | Allow port 9600 in firewall |