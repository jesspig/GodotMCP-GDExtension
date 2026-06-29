# 客户端设置

## 快速参考

| 客户端 | 配置文件 | 键/格式 |
|--------|----------|---------|
| Claude Code | `.mcp.json` | `mcpServers` |
| Cursor | `.cursor/mcp.json` | `mcpServers` |
| VS Code / Copilot | `.vscode/mcp.json` | `servers` |
| Cline | `.cline/mcp.json` | `mcpServers` |
| OpenCode | `.opencode/opencode.json` | `mcp` |
| Codex | `.codex/config.toml` | TOML 格式 |
| Trae / Trae CN | `.trae/mcp.json` | `mcpServers` |
| Qoder | `.qoder/mcp.json` | `mcpServers` |
| CodeBuddy | `.codebuddy/mcp_settings.json` | `mcpServers` |
| Pi | `.pi/settings.json` | `mcp` |
| OpenClaw | `.openclaw/openclaw.json` | `mcpServers` |

所有客户端使用相同的验证命令：`curl http://127.0.0.1:9600/mcp`

## 通用配置

所有客户端共享相同的 Streamable HTTP 配置：

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

## 各客户端注意事项

### 项目级配置 vs 全局配置

始终在**项目级别**（而非全局/用户级别）配置 MCP。每个 Godot 项目都需要自己的 MCP 配置，指向同一个本地服务器。

### OpenCode

文件：`.opencode/opencode.json`

```json
{
  "mcp": {
    "mcpServers": {
      "godot-mcp": {
        "type": "streamable-http",
        "url": "http://127.0.0.1:9600/mcp"
      }
    }
  }
}
```

编辑后重新加载 OpenCode 配置。

### Cursor

文件：项目根目录下的 `.cursor/mcp.json`

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

编辑后重启 Cursor。在 Cursor 的 MCP 标签页中确认服务器显示为绿色。

### VS Code / Copilot

文件：项目根目录下的 `.vscode/mcp.json`

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

编辑后重新加载 VS Code 窗口。

### Claude Code

文件：项目根目录下的 `.mcp.json`

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

文件：项目根目录下的 `.cline/mcp.json`

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

### Codex

文件：项目根目录下的 `.codex/config.toml`

```toml
[mcp_servers.godot]
enabled = true
url = "http://127.0.0.1:9600/mcp"
transport = "streamable_http"
```

### Qoder

文件：项目根目录下的 `.qoder/mcp.json`

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

### CodeBuddy

文件：项目根目录下的 `.codebuddy/mcp_settings.json`

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

### Pi

文件：项目根目录下的 `.pi/settings.json`

```json
{
  "mcp": {
    "mcpServers": {
      "godot-mcp": {
        "type": "streamable-http",
        "url": "http://127.0.0.1:9600/mcp"
      }
    }
  }
}
```

### OpenClaw

文件：项目根目录下的 `.openclaw/openclaw.json`

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

## 生成客户端配置

打开编辑器中的 **GodotMCP** 底部面板即可为任一支持的客户端自动生成配置。从下拉菜单中选择客户端，点击 **Generate** 查看配置内容，或通过 `get_info(include_configs=true)` 通过 API 获取配置片段。

## 故障排除

| 症状 | 可能原因 | 解决方法 |
|---------|-------------|-----|
| 连接被拒绝 | Godot 未运行或端口错误 | 启动 Godot 编辑器，验证端口 |
| 404 Not Found | URL 路径错误 | 必须使用 `/mcp` 端点 |
| 客户端未显示服务器 | 配置文件位置错误 | 使用项目级配置，而非全局配置 |
| 超时 | 防火墙或端口被阻止 | 在防火墙中放行端口 9600 |