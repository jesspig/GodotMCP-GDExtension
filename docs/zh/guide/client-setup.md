# 客户端设置

## 快速参考

| 客户端 | 配置文件 | 键 |
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

添加到 `CLAUDE.md` 文件：

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

文件：项目根目录下的 `.vscode/cline_mcp_settings.json`

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

文件：项目根目录下的 `.vscode/roocode_settings.json`

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

文件：`~/.continue/config.json`

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

文件：项目根目录下的 `.windsurf/models.json`

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

使用 `generate_client_config` 元工具自动生成客户端的配置：

```bash
curl -X POST http://localhost:9600/mcp \
  -H "Content-Type: application/json" \
  -d '{"method":"tools/call","params":{"name":"generate_client_config","arguments":{"client_type":"cursor"}}}'
```

## 故障排除

| 症状 | 可能原因 | 解决方法 |
|---------|-------------|-----|
| 连接被拒绝 | Godot 未运行或端口错误 | 启动 Godot 编辑器，验证端口 |
| 404 Not Found | URL 路径错误 | 必须使用 `/mcp` 端点 |
| 客户端未显示服务器 | 配置文件位置错误 | 使用项目级配置，而非全局配置 |
| 超时 | 防火墙或端口被阻止 | 在防火墙中放行端口 9600 |