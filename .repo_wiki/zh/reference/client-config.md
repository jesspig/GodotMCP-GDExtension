# 客户端配置

> `godot-mcp-server` 只支持 **stdio 传输**。所有 AI 客户端必须通过 stdio 配置。

## 通用配置结构

所有客户端都需要两样东西：
1. `command` — 指向 `godot-mcp-server.exe` 的路径
2. `env.GODOT_PATH` — Godot 编辑器可执行文件的路径

```json
{
  "mcpServers": {
    "godot-mcp": {
      "command": "/absolute/path/to/godot-mcp-server",
      "args": [],
      "env": {
        "GODOT_PATH": "/absolute/path/to/godot/Godot_v4.6-stable_win64.exe"
      }
    }
  }
}
```

## 示例配置

### VS Code (Roo Code / Continue)

**`~/.config/Code/User/globalStorage/rooveterinaryinc.roo-cline/settings/cline_customMCPServers.json`**

```json
{
  "command": "C:\\Tools\\godot-mcp-server.exe",
  "env": {
    "GODOT_PATH": "C:\\Godot\\Godot_v4.6-stable_win64.exe"
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
    "godot-mcp": {
      "command": "C:/Tools/godot-mcp-server.exe",
      "env": {
        "GODOT_PATH": "C:/Godot/Godot_v4.6-stable_win64.exe"
      }
    }
  }
}
```

### JetBrains

在 IDE 中的 MCP 配置面板设置。

## 注意事项

- **Windows 路径**: 使用前向斜杠 (`/`) 或双反斜杠 (`\\`)
- `GODOT_PATH` **不能**从 shell profile 继承——stdio 服务器不继承终端环境变量
- **重启编辑器/客户端**: 修改配置后必须重启使用 MCP 的编辑器
- **重启后重建**: 如果重建了 `godot-mcp-server.exe`，必须重启使用 MCP 的编辑器进程
