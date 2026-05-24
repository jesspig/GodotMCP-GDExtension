# 编辑器控制

> 3 个服务器端工具（`godot_editor_*`）在 `handler.rs` 中处理，从不发送到 gdext。

```mermaid
flowchart TD
    CLIENT["AI 客户端"] -->|call_tool("godot_editor_open")| SERVER["godot-mcp-server"]
    SERVER -->|handler.rs 拦截| OPEN["编辑器控制"]
    
    subgraph OPEN[Editor Control Handlers]
        O["godot_editor_open"]
        C["godot_editor_close"]
        R["godot_editor_restart"]
    end
    
    O -->|"taskkill /F /IM godot.exe"| OS["操作系统"]
    O -->|"启动新进程"| GODOT["Godot Editor"]
    C -->|"taskkill /F /IM godot.exe"| OS
    R -->|"kill + 500ms 等待 + respawn"| OS
```

## `godot_editor_open`

- `project_path` 从 CWD 解析（默认 `./godot/`）
- 解析 `GODOT_PATH` 环境变量——必需
- 启动 Godot 编辑器进程
- 等待 WebSocket 连接（最多约 60 秒）
- 返回 `{"status": "ok"}`

## `godot_editor_close`

- Windows: `taskkill /F /IM godot*.exe`（通配以匹配各种 Godot 可执行文件名）
- Unix: `pkill -f Godot`
- 返回 `{"status": "ok"}`

**注意**: 响应直接从 server 进程发送——编辑器被终止时 WebSocket 连接关闭，gdext 无法返回任何内容。

## `godot_editor_restart`

1. 调用 `close`（kill 编辑器进程）
2. 等待 500ms（确保进程完全终止）
3. 调用 `open`（使用 `project_path` 参数）

## 环境变量

| 变量 | 必需 | 说明 |
|------|------|------|
| `GODOT_PATH` | 是 | Godot 可执行文件的完整路径 |

## 项目路径解析

- 默认：`./godot/`（相对 CWD）
- 用户可以传递绝对路径或相对路径
- 路径解析发生在服务器进程中，使用 `std::env::current_dir()` 作为基础目录
