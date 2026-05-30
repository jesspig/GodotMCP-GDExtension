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
| Qoder | `.mcp.json` / `qodercli mcp add` | `mcpServers` | ✅ |
| CodeBuddy | `.codebuddy/.mcp.json` | `mcpServers` | ✅ |
| Kilo Code | `kilo.jsonc` / `.kilo/kilo.jsonc` | `mcp` | ✅ |
| Continue | `.continuerc.json` | `mcpServers` (array) | ✅ |
| Cline | `cline_mcp_settings.json` | `mcpServers` | ✅ |

## opencode

Add to `opencode.json` in your project root:

```json
{
  "$schema": "https://opencode.ai/config.json",
  "mcp": {
    "godot-mcp": {
      "type": "remote",
      "url": "http://localhost:9600"
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
      "url": "http://localhost:9600"
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
      "url": "http://localhost:9600"
    }
  }
}
```

> **Note**: VS Code uses `servers` as the top-level key (not `mcpServers`). Tools are available in Copilot Chat Agent mode.

## Claude Code

Run in your project root:

```bash
claude mcp add godot-mcp --transport http http://localhost:9600 --scope project
```

This creates a `.mcp.json` file (can be committed to version control). Without `--scope project`, it defaults to local scope (visible to the project but not committed to git).

## Trae

Create `.trae/mcp.json` in your project root:

```json
{
  "mcpServers": {
    "godot-mcp": {
      "url": "http://localhost:9600"
    }
  }
}
```

Also configurable via Trae's MCP settings panel (Settings → MCP → Manually Add).

## Qoder

Using CLI (project level, writes to `.mcp.json`):

```bash
qodercli mcp add godot-mcp -t http http://localhost:9600 -s project
```

Or manually in `.mcp.json`:

```json
{
  "mcpServers": {
    "godot-mcp": {
      "type": "http",
      "url": "http://localhost:9600"
    }
  }
}
```

Qoder also supports configuration via IDE settings panel (Settings → MCP → + Add).

## CodeBuddy

Using CLI (project level):

```bash
codebuddy mcp add --scope project --transport http godot-mcp http://localhost:9600
```

Or manually in `.codebuddy/.mcp.json`:

```json
{
  "mcpServers": {
    "godot-mcp": {
      "type": "http",
      "url": "http://localhost:9600"
    }
  }
}
```

## Kilo Code

Add to `kilo.jsonc` (or `.kilo/kilo.jsonc`) in your project root:

```json
{
  "mcp": {
    "godot-mcp": {
      "type": "remote",
      "url": "http://localhost:9600"
    }
  }
}
```

Global config `~/.config/kilo/kilo.jsonc` is also supported.

## Continue

Create `.continuerc.json` in your project root:

```json
{
  "mcpServers": [
    {
      "name": "godot-mcp",
      "type": "streamable-http",
      "url": "http://localhost:9600"
    }
  ]
}
```

> **Note**: Continue's `mcpServers` is an **array** (not an object), each server is a separate entry.

## Cline

Edit `cline_mcp_settings.json` (accessible via Cline settings panel):

```json
{
  "mcpServers": {
    "godot-mcp": {
      "url": "http://localhost:9600"
    }
  }
}
```

## Custom Client

GodotMCP provides a standard Streamable HTTP endpoint:

- **Endpoint**: `http://localhost:9600`
- **Protocol**: JSON-RPC 2.0 + Streamable HTTP
- **Port**: 9600 (configurable via `GODOT_MCP_HTTP_PORT` environment variable)

Any client that supports the MCP Streamable HTTP transport can connect via this endpoint.
