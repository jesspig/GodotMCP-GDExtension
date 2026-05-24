# 客户端 quirks

## 通用注意事项

- **stdio 是唯一传输协议**。确保配置的是 `command` 而非 `url`。
- 所有客户端都需要 `GODOT_PATH` 环境变量手动设置。

## 已知问题

| 问题 | 出现场景 | 原因/解决方法 |
|------|---------|------------|
| `godot-mcp-server` 启动但超时 | 所有客户端 | Godot 编辑器未重启或 `GODOT_PATH` 路径错误。检查路径并使用 `godot_editor_open` 工具 |
| Windows 上的 `python` 存根问题 | 所有 Windows 用户 | `python`/`python3` 是 Microsoft Store 存根——如果你的工作流涉及 Python 脚本，使用 `py -3` |
| 编辑器进程已被占用 | 重建 dll 时 | 关闭 Godot 编辑器或禁用插件后重建。CMake 自动杀掉 server 进程 |
| C# Solution 生成冲突 | 打开多个编辑器实例时 | WebSocket 端口 9500 已被第一个实例占用 |

## 构建后步骤

1. 如果在开发 dll，**关闭 Godot 编辑器**（编辑器持有 `godot_mcp_gdext.dll` 文件锁）
2. 运行 `py -3 build.py`
3. **重启 MCP 客户端**（重启 editor）
4. 使用 `godot_editor_open` 工具启动 Godot

## Godot 编辑器模式限制

| 工具 | 限制 |
|------|------|
| `get_variable` / `set_variable` | 仅 `@export` 变量在编辑器模式下可用。非导出成员使用 `PlaceHolderScriptInstance` |
| `validate_gdscript` | 需要 Editor Settings → Network → Language Server → Enable 为 ON |
| `csharp_build` | 不能和编辑器同时运行（编辑器持有程序集文件锁） |
| `rename_scene` | 如果目标已打开但不是活动标签，返回错误 |
| `add_circle_collision` / `add_rectangle_collision` | 检测现存的 `CollisionShape2D`；响应中的 `mode` 字段指示操作路径 |
