# 客户端配置

> 所有 AI 客户端通过 **MCP Streamable HTTP** 连接，URL 为 `http://127.0.0.1:9600/mcp`。

## 自动生成配置

调用元工具 `generate_client_config` 即可获取 11 种 AI 客户端的预生成配置（`client_registry.hpp:151-163`）：

| 客户端 | key |
|--------|-----|
| Claude Desktop | `claude_desktop` |
| Cursor | `cursor` |
| Windsurf | `windsurf` |
| VSCode | `vscode` |
| Cline | `cline` |
| opencode | `opencode` |
| Roo Code | `roo_code` |
| Continue | `continue` |
| Zed | `zed` |
| JetBrains | `jetbrains` |
| Generic | `generic` |

## 手动配置

### opencode

`.opencode/opencode.json`（项目实际使用格式）：

```json
{
    "mcp": {
        "godot-mcp": {
            "type": "remote",
            "url": "http://127.0.0.1:9600/mcp",
            "oauth": false
        }
    }
}
```

### Cursor

`~/.cursor/mcp.json`：

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

### Claude Desktop / VSCode / 其他

参考 `generate_client_config` 输出的对应格式。各客户端的配置 key 和结构略有不同（如 `"mcpServers"` vs `"mcp"`、`"streamable-http"` vs `"remote"`）。

## 注意事项

- **Godot 编辑器必须先启动并加载插件**，客户端才能连接
- **端口**：默认 9600，可通过 `GODOT_MCP_HTTP_PORT` 环境变量覆盖（也可在 `project.godot` 中设置 `godot_mcp/http_port`）
- **重建后**: 如果重建了 `godot_mcp_gdext.dll`，需要关闭并重新打开 Godot 编辑器
