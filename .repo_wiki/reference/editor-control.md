# 编辑器控制（服务器端）

> 4 个服务器端工具在 `handler.py`（Python）中处理，从不发送到 gdext。

```mermaid
flowchart TD
    CLIENT["AI 客户端"] -->|call_tool("godot_editor_open")| SERVER["godot-mcp-server"]
    SERVER -->|handler.py 拦截| OPEN["编辑器控制"]
    
    subgraph OPEN[Editor Control (Python)]
        V["get_server_version"]
        O["godot_editor_open"]
        C["godot_editor_close"]
        R["godot_editor_restart"]
    end
    
    V -->|"直接返回版本号"| CLIENT
    O -->|"subprocess.Popen"| GODOT["Godot Editor"]
    C -->|"taskkill / pkill"| OS["操作系统"]
    R -->|"kill + 500ms + respawn"| OS
```

## 实现位置

所有 4 个服务器端工具在 `server/src/godot_mcp_server/` 中实现：

| 文件 | 内容 |
|------|------|
| `handler.py` | 工具调用分发逻辑 |
| `editor_ctl.py` | 编辑器进程管理（open/close/restart）和版本号 |

## `get_server_version`

- 直接返回 `editor_ctl.py` 中硬编码的 `SERVER_VERSION`（如 `"0.1.4"`）
- 与 `Cargo.toml` 版本手动同步
- 不涉及 WebSocket 或 gdext

## `godot_editor_open`

```python
# 简化流程
godot_path = resolve_godot_path(override)  # GODOT_PATH 环境变量
project_path = resolve_project_path(args)   # 默认 "example/"
result = subprocess.Popen([godot_path, "--editor", "--path", project_path])
return {"status": "opened", "pid": result.pid, ...}
```

- `project_path` 从 args 读取（默认 `example/`，相对于 CWD）
- 解析 `GODOT_PATH` 环境变量——必需
- 启动 Godot 编辑器进程（不等待连接）

## `godot_editor_close`

- Windows: `taskkill /F /IM <exe_name>`（使用实际的 exe 文件名）
- Unix: `pkill -x <exe_name>`
- 返回 `{"status": "closed"}` 或 `"not_running"`

## `godot_editor_restart`

1. 调用 `kill_process_by_name`（杀编辑器进程）
2. 调用 `_disconnect()`（清理桥接状态）
3. 等待 500ms
4. 调用 `_open_editor()` 重新启动

## 环境变量

| 变量 | 必需 | 说明 |
|------|------|------|
| `GODOT_PATH` | 是 | Godot 可执行文件的完整路径（必须在 MCP 客户端 `env` 中设置） |

## `GODOT_PATH` 配置

stdio 服务器不继承 shell 环境变量，因此 `GODOT_PATH` 必须在 MCP 客户端配置的 `env` 字段中显式设置：

```json
{
  "env": {
    "GODOT_PATH": "C:/Program Files/Godot/Godot_v4.6-stable_win64.exe"
  }
}
