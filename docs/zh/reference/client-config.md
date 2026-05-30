# 客户端配置

GodotMCP 兼容所有支持 MCP Streamable HTTP 传输的 AI 客户端。以下是各客户端的配置方式。

> **务必使用项目级别配置**，不要全局配置。只有安装了 GodotMCP 插件的 Godot 项目才会启动 MCP 服务器，全局配置会导致其他项目一直连接失败。

## 快速参考

| 客户端 | 配置文件 | 顶层键 | 项目级别 |
|--------|----------|--------|----------|
| opencode | `opencode.json` | `mcp` | ✅ |
| Cursor | `.cursor/mcp.json` | `mcpServers` | ✅ |
| VS Code + Copilot | `.vscode/mcp.json` | `servers` | ✅ |
| Claude Code | `.mcp.json` / `claude mcp add` | `mcpServers` | ✅ |
| Trae | `.trae/mcp.json` | `mcpServers` | ✅ |
| Qoder | `.mcp.json` / `qodercli mcp add` | `mcpServers` | ✅ |
| CodeBuddy | `.codebuddy/.mcp.json` | `mcpServers` | ✅ |
| Kilo Code | `kilo.jsonc` / `.kilo/kilo.jsonc` | `mcp` | ✅ |
| Continue | `.continuerc.json` | `mcpServers`（数组） | ✅ |
| Cline | `cline_mcp_settings.json` | `mcpServers` | ✅ |

## opencode

在项目根目录的 `opencode.json` 中添加：

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

在项目根目录创建 `.cursor/mcp.json`：

```json
{
  "mcpServers": {
    "godot-mcp": {
      "url": "http://localhost:9600"
    }
  }
}
```

也支持全局配置 `~/.cursor/mcp.json`，但推荐项目级别。

## VS Code + GitHub Copilot

在项目根目录创建 `.vscode/mcp.json`：

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

> **注意**：VS Code 使用 `servers` 作为顶层键（而非 `mcpServers`），这是与其他客户端最大的区别。工具在 Copilot Chat 的 Agent 模式下可用。

## Claude Code

在项目根目录执行：

```bash
claude mcp add godot-mcp --transport http http://localhost:9600 --scope project
```

这会创建 `.mcp.json` 文件（可提交到版本控制与团队共享）。如不使用 `--scope project`，则默认为 local 范围，仅当前项目可见但不会提交到 git。

## Trae

在项目根目录创建 `.trae/mcp.json`：

```json
{
  "mcpServers": {
    "godot-mcp": {
      "url": "http://localhost:9600"
    }
  }
}
```

也支持通过 Trae 的 MCP 设置面板（Settings → MCP → Manually Add）添加。

## Qoder

使用 CLI 命令（项目级别，会写入 `.mcp.json`）：

```bash
qodercli mcp add godot-mcp -t http http://localhost:9600 -s project
```

或在项目 `.mcp.json` 中手动配置：

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

Qoder 还支持通过 IDE 设置面板（Settings → MCP → + Add）配置。

## CodeBuddy

使用 CLI 命令（项目级别）：

```bash
codebuddy mcp add --scope project --transport http godot-mcp http://localhost:9600
```

或在项目 `.codebuddy/.mcp.json` 中手动配置：

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

在项目根目录的 `kilo.jsonc`（或 `.kilo/kilo.jsonc`）中添加：

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

也支持全局配置 `~/.config/kilo/kilo.jsonc`。

## Continue

在项目根目录创建 `.continuerc.json`：

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

> **注意**：Continue 的 `mcpServers` 是**数组**（而非对象），每个服务器是一个对象条目。

## Cline

编辑 `cline_mcp_settings.json`（可通过 Cline 设置面板访问）：

```json
{
  "mcpServers": {
    "godot-mcp": {
      "url": "http://localhost:9600"
    }
  }
}
```

## 自定义客户端

GodotMCP 提供标准的 Streamable HTTP 端点：

- **端点**: `http://localhost:9600`
- **协议**: JSON-RPC 2.0 + Streamable HTTP
- **端口**: 9600（可通过环境变量 `GODOT_MCP_HTTP_PORT` 修改）

任何支持 MCP Streamable HTTP 传输的客户端均可通过上述端点连接。
