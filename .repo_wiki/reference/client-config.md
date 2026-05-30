# 客户端配置

> 所有 AI 客户端通过 **MCP Streamable HTTP** 连接，URL 为 `http://localhost:9600/mcp`。

## 通用配置结构

```json
{
  "mcpServers": {
    "godot": {
      "type": "streamable-http",
      "url": "http://localhost:9600/mcp"
    }
  }
}
```

## 示例配置

### opencode

在 `opencode.json` 或 `opencode.jsonc` 中配置：

```json
{
  "mcpServers": {
    "godot": {
      "type": "streamable-http",
      "url": "http://localhost:9600/mcp"
    }
  }
}
```

### Claude Code

在 `claude.json` 或 `CLAUDE.md` 中配置。

### Cursor

**`~/.cursor/mcp.json`**

```json
{
  "mcpServers": {
    "godot": {
      "type": "streamable-http",
      "url": "http://localhost:9600/mcp"
    }
  }
}
```

### JetBrains

在 IDE 中的 MCP 配置面板设置 Streamable HTTP URL。

## 注意事项

- **Godot 编辑器必须先启动并加载插件**，客户端才能连接
- **端口**：默认 9600，可通过 `GODOT_MCP_HTTP_PORT` 环境变量覆盖
- **重启编辑器/客户端**: 修改配置后必须重启使用 MCP 的编辑器
- **重建后**: 如果重建了 `godot_mcp_gdext.dll`，需要关闭并重新打开 Godot 编辑器
