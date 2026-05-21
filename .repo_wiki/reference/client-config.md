# Client Configuration

> Connect AI MCP clients to `godot-mcp-server`. Today only the **stdio** transport works; the HTTP rows are listed for completeness (planned) but the server has no HTTP listener yet.

## Quickest path

Drop this into your MCP-client config (most clients use `mcpServers`):

```json
{
  "mcpServers": {
    "godot-mcp": {
      "command": "C:\\path\\to\\target\\debug\\godot-mcp-server.exe",
      "args": ["--godot-port", "9500"]
    }
  }
}
```

On Linux/macOS: drop the `.exe` and adjust the path. The server takes stdio on its own — no `--transport` flag exists today, the implicit transport is stdio.

After (re)building the server, **restart the MCP client** — it keeps the old binary handle alive otherwise.

## Supported clients and where their config lives

| # | Client | Default transport | Config path | Notes |
|---|--------|-------------------|-------------|-------|
| 1 | Claude Code | stdio | `~/.claude/mcp.json` | Standard `mcpServers`. |
| 2 | Codex | HTTP | `~/.codex/config.toml` | TOML, not JSON. `[mcp_servers."godot-mcp"]`. |
| 3 | Gemini CLI | HTTP | `~/.gemini/settings.json` or `<project>/.gemini/settings.json` | HTTP URL key is **`httpUrl`** (not `url`). |
| 4 | OpenCode | stdio | `~/.config/opencode/config.json` or project-level `opencode.json` | Standard `mcpServers`. |
| 5 | Cursor | HTTP | `<project>/.cursor/mcp.json` | Standard `mcpServers`. |
| 6 | GitHub Copilot | HTTP | `<project>/.vscode/mcp.json` | Top-level key is **`servers`** (not `mcpServers`). Each entry needs `"type": "stdio"` or `"type": "http"`. |
| 7 | Qwen Code | HTTP | `~/.qwen/settings.json` or `<project>/.qwen/settings.json` | HTTP URL key is `httpUrl`. |
| 8 | Trae | HTTP | `<project>/.trae/mcp.json` | Standard `mcpServers`. |
| 9 | Trae CN | HTTP | `<project>/.trae/mcp.json` (same file as Trae) | Identical to Trae. |
| 10 | Qoder | HTTP | Win `%APPDATA%/Qoder/mcp-settings.json` · mac `~/Library/Application Support/Qoder/` · Linux `~/.config/qoder/` | Standard `mcpServers`. |
| 11 | Antigravity | HTTP | `~/.gemini/antigravity/mcp_config.json` | HTTP URL key is **`serverUrl`**. |
| 12 | CodeBuddy | HTTP | `~/.codebuddy/.mcp.json` or `<project>/.mcp.json` | Each entry needs explicit `"type"` (`"stdio"`/`"sse"`/`"http"`). |

Pitfalls compressed in [reference/client-quirks.md](client-quirks.md).

## Templates

### stdio (Claude Code, OpenCode, generic)

```json
{
  "mcpServers": {
    "godot-mcp": {
      "command": "/path/to/godot-mcp-server",
      "args": ["--godot-port", "9500"]
    }
  }
}
```

### stdio — GitHub Copilot (`servers` key + `type`)

```json
{
  "servers": {
    "godot-mcp": {
      "type": "stdio",
      "command": "/path/to/godot-mcp-server",
      "args": ["--godot-port", "9500"]
    }
  }
}
```

### stdio — CodeBuddy (explicit `type`)

```json
{
  "mcpServers": {
    "godot-mcp": {
      "type": "stdio",
      "command": "/path/to/godot-mcp-server",
      "args": ["--godot-port", "9500"]
    }
  }
}
```

### stdio — Codex (TOML)

```toml
[mcp_servers."godot-mcp"]
type    = "stdio"
command = "/path/to/godot-mcp-server"
args    = ["--godot-port", "9500"]
enabled = true
```

### HTTP — generic (Cursor, Trae, Qoder, CodeBuddy)  ⚠ planned, not implemented

```json
{
  "mcpServers": {
    "godot-mcp": {
      "url": "http://127.0.0.1:8900/mcp"
    }
  }
}
```

### HTTP — Gemini CLI / Qwen Code (`httpUrl`)  ⚠ planned

```json
{
  "mcpServers": {
    "godot-mcp": {
      "httpUrl": "http://127.0.0.1:8900/mcp"
    }
  }
}
```

### HTTP — Antigravity (`serverUrl`)  ⚠ planned

```json
{
  "mcpServers": {
    "godot-mcp": {
      "serverUrl": "http://127.0.0.1:8900/mcp"
    }
  }
}
```

### HTTP — GitHub Copilot (`servers` + `type: "http"`)  ⚠ planned

```json
{
  "servers": {
    "godot-mcp": {
      "type": "http",
      "url": "http://127.0.0.1:8900/mcp"
    }
  }
}
```

## Sanity check after configuring

1. Start (or restart) the Godot editor; ensure the plugin loads (Output panel shows `[Godot MCP] Plugin loaded!`).
2. In the AI client, call the `ping` tool. Expected `"pong"`.
3. If the response is `"Godot 编辑器未连接"` then the server can't reach `ws://127.0.0.1:9500` — check the editor is open, the plugin is enabled, no firewall is blocking loopback.
4. Then call `get_open_scenes`. If the list is empty, call `open_scene` with a `res://...tscn` path — most tools need an open scene to operate on.
