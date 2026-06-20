# 客户端配置

> 所有 AI 客户端通过 **MCP Streamable HTTP** 连接，URL 为 `http://127.0.0.1:9600/mcp`。

## 自动生成配置

通过 Godot 底部面板（`McpDock`）或 `generate_client_config` 工具，可一键生成 11 个客户端的项目级配置。所有配置文件均为**项目级路径**，避免污染全局 MCP 配置。

配置生成通过 `client_config_registry.hpp`（声明式描述符 + 策略模式）实现：

| 客户端 | 配置文件路径 | 格式 | generator |
|--------|-------------|:----:|-----------|
| Claude Code | `.mcp.json` | JSON | `generate_claude_code_config` |
| Cursor | `.cursor/mcp.json` | JSON | `generate_cursor_config` |
| VS Code Copilot | `.vscode/mcp.json` | JSON | `generate_vscode_config` |
| Cline | `.cline/mcp.json` | JSON | `generate_cline_config` |
| OpenCode | `.opencode/opencode.json` | JSON | `generate_opencode_config` |
| Codex | `.codex/config.toml` | TOML | `generate_codex_toml` |
| Trae / Trae CN | `.trae/mcp.json` | JSON | `generate_trae_config` |
| Qoder | `.qoder/mcp.json` | JSON | `generate_qoder_config` |
| CodeBuddy | `.codebuddy/mcp_settings.json` | JSON | `generate_codebuddy_config` |
| Pi | `.pi/settings.json` | JSON | `generate_pi_config` |
| OpenClaw | `.openclaw/openclaw.json` | JSON | `generate_openclaw_config` |

## 手动配置

### OpenCode

`.opencode/opencode.json`（项目级）：

```json
{
    "mcp": {
        "godot-mcp": {
            "type": "remote",
            "url": "http://127.0.0.1:9600/mcp"
        }
    }
}
```

### Cursor

`.cursor/mcp.json`（项目级）：

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

`.mcp.json`（项目级）：

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

## 注意事项

- **Godot 编辑器必须先启动并加载插件**，客户端才能连接
- **端口**：默认 9600，可通过 `GODOT_MCP_HTTP_PORT` 环境变量覆盖（也可在 `project.godot` 中设置 `godot_mcp/http_port`）
- **热重载**: `.gdextension` 设 `reloadable = true`（Godot 4.2+），`main.py build` 可直接覆盖 DLL，编辑器自动重载。Windows 下因 OS Loader 锁定 DLL 可能失败（视系统版本和配置而异），此时关闭编辑器重试。
