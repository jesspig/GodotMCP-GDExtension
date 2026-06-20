# 客户端 quirks

## 通用注意事项

- **MCP Streamable HTTP 是唯一传输协议**。配置 `url` 为 `http://localhost:9600/mcp`。
- Godot 编辑器必须已启动并加载 MCP 插件后客户端才能连接。

## 已知问题

| 问题 | 出现场景 | 原因/解决方法 |
|------|---------|------------|
| `godot_mcp_gdext` 插件加载但连接失败 | 所有客户端 | Godot 编辑器未启动或端口被占用。检查 `GODOT_MCP_HTTP_PORT` 并确保端口 9600 可用 |
| Windows 上的 `python` 存根问题 | 所有 Windows 用户 | `python`/`python3` 是 Microsoft Store 存根——如果你的工作流涉及 Python 脚本，使用 `uv run python` |
| 编辑器进程已被占用 / DLL 覆盖失败 | 重建 dll 时（Windows） | `main.py build` 直接覆盖 DLL 通常可热重载；若 OS 报告文件锁（Windows 特有），关闭编辑器重试 |
| C# Solution 生成冲突 | 打开多个编辑器实例时 | HTTP 端口 9600 被第一个实例占用 |

## 构建后步骤

1. 运行 `uv run python main.py build`（GDExtension 热重载支持直接覆盖 DLL）
2. **如果 Windows 下文件被锁定**：先关闭 Godot 编辑器再构建，然后重新打开编辑器
3. AI 客户端连接 `http://localhost:9600/mcp`

## Godot 编辑器模式限制

| 工具 | 限制 |
|------|------|
| `get_node_property` / `set_node_property` | 仅 `@export` 变量在编辑器模式下可用。非导出成员使用 `PlaceHolderScriptInstance` |
| `validate_script` | GDScript 需要 Editor Settings → Network → Language Server → Enable 为 ON；C# 需要 .NET SDK 在 PATH 中 |
| `rename_node` | 如果目标已打开但不是活动标签，可能返回错误 |
