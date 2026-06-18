# Client Configuration

GodotMCP is compatible with all AI clients that support the MCP Streamable HTTP transport. Below are the configuration instructions for each client.

> **Always use project-level configuration**, not global. Only Godot projects with the GodotMCP plugin installed will start the MCP server; a global config will cause connection failures in other projects.

## Quick Reference

| Client | Config File | Top-level Key | Project Level |
|--------|-------------|---------------|---------------|
| opencode | `opencode.json` | `mcp` | ✅ |
| Cursor | `.cursor/mcp.json` | `mcpServers` | ✅ |
| VS Code + Copilot | `.vscode/mcp.json` | `servers` | ✅ |
| Claude Code | `.mcp.json` / `claude mcp add` | `mcpServers` | ✅ |
| Trae | `.trae/mcp.json` | `mcpServers` | ✅ |
| Qoder | `.qoder/mcp.json` | `mcpServers` | ✅ |
| CodeBuddy | `.codebuddy/mcp_settings.json` | `mcpServers` | ✅ |
| Codex | `.codex/config.toml` | `mcp_servers` (TOML) | ✅ |
| Pi | `.pi/settings.json` | `mcp` | ✅ |
| OpenClaw | `.openclaw/openclaw.json` | `mcpServers` | ✅ |
| Cline | `.cline/mcp.json` | `mcpServers` | ✅ |

## opencode

Add to `opencode.json` in your project root:

```json
{
  "$schema": "https://opencode.ai/config.json",
  "mcp": {
    "godot-mcp": {
      "type": "remote",
      "url": "http://localhost:9600/mcp"
    }
  }
}
```

## Cursor

Create `.cursor/mcp.json` in your project root:

```json
{
  "mcpServers": {
    "godot-mcp": {
      "url": "http://localhost:9600/mcp"
    }
  }
}
```

Global config at `~/.cursor/mcp.json` is also supported, but project level is recommended.

## VS Code + GitHub Copilot

Create `.vscode/mcp.json` in your project root:

```json
{
  "servers": {
    "godot-mcp": {
      "type": "http",
      "url": "http://localhost:9600/mcp"
    }
  }
}
```

> **Note**: VS Code uses `servers` as the top-level key (not `mcpServers`). Tools are available in Copilot Chat Agent mode.

## Claude Code

Run in your project root:

```bash
claude mcp add godot-mcp --transport http http://localhost:9600/mcp --scope project
```

This creates a `.mcp.json` file (can be committed to version control). Without `--scope project`, it defaults to local scope (visible to the project but not committed to git).

## Trae

Create `.trae/mcp.json` in your project root:

```json
{
  "mcpServers": {
    "godot-mcp": {
      "url": "http://localhost:9600/mcp"
    }
  }
}
```

Also configurable via Trae's MCP settings panel (Settings → MCP → Manually Add).

## Qoder

Using CLI (project level, writes to `.mcp.json`):

```bash
qodercli mcp add godot-mcp -t http http://localhost:9600/mcp -s project
```

Or manually in `.mcp.json`:

```json
{
  "mcpServers": {
    "godot-mcp": {
      "type": "http",
      "url": "http://localhost:9600/mcp"
    }
  }
}
```

Qoder also supports configuration via IDE settings panel (Settings → MCP → + Add).

## CodeBuddy

Using CLI (project level):

```bash
codebuddy mcp add --scope project --transport http godot-mcp http://localhost:9600/mcp
```

Or manually in `.codebuddy/.mcp.json`:

```json
{
  "mcpServers": {
    "godot-mcp": {
      "type": "http",
      "url": "http://localhost:9600/mcp"
    }
  }
}
```

## Codex

Create `.codex/config.toml` in your project root:

```toml
[mcp_servers.godot]
enabled = true
url = "http://localhost:9600/mcp"
transport = "streamable_http"
```

## Pi

Add to `.pi/settings.json` in your project root:

```json
{
  "mcp": {
    "mcpServers": {
      "godot": {
        "url": "http://localhost:9600/mcp"
      }
    }
  }
}
```

## OpenClaw

Create `.openclaw/openclaw.json` in your project root:

```json
{
  "mcpServers": {
    "godot": {
      "url": "http://localhost:9600/mcp"
    }
  }
}
```

## Cline

Create `.cline/mcp.json` in your project root:

```json
{
  "mcpServers": {
    "godot": {
      "type": "http",
      "url": "http://localhost:9600/mcp"
    }
  }
}
```

## Custom Client

GodotMCP provides a standard Streamable HTTP endpoint:

- **Endpoint**: `http://localhost:9600/mcp`
- **Protocol**: JSON-RPC 2.0 + Streamable HTTP
- **Port**: 9600 (configurable via `GODOT_MCP_HTTP_PORT` environment variable)

Any client that supports the MCP Streamable HTTP transport can connect via the `/mcp` endpoint.
