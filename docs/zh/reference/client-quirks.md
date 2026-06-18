# 客户端注意事项

## 通用

- Godot 编辑器必须已启动并加载 MCP 插件后客户端才能连接
- MCP Streamable HTTP 是唯一传输协议，配置 `url` 为 `http://localhost:9600/mcp`（或 `http://127.0.0.1:9600/mcp`）

### 已知问题

| 问题 | 原因/解决方法 |
|------|------------|
| 插件加载但连接失败 | Godot 编辑器未启动或端口被占用。检查 `GODOT_MCP_HTTP_PORT` 并确保端口 9600 可用 |
| Windows 上 `python` 卡死 | Microsoft Store 存根问题——使用 `py -3` |
| DLL 被锁定 | 关闭 Godot 编辑器或禁用插件后重建 |
| C# Solution 冲突 | 打开多个编辑器实例时 HTTP 端口 9600 被第一个实例占用 |

## Godot 编辑器模式限制

| 工具 | 限制 |
|------|------|
| `get_node_property` / `set_node_property` | 仅 `@export` 变量在编辑器模式下可用 |
| `validate_gd_script` / `validate_csharp_script` | 需要 Editor Settings → Network → Language Server → Enable 为 ON |
| `csharp_build` | 不能和编辑器同时运行（编辑器持有程序集文件锁） |
| `rename_scene` | 如果目标已打开但不是活动标签，返回错误 |
