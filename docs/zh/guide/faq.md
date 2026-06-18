# 常见问题

## 连接问题

### 问：AI 客户端提示"连接被拒绝"

请确认 Godot 正在运行（编辑器，而不仅仅是项目）。检查端口：

```bash
curl http://127.0.0.1:9600/mcp -X POST -H "Content-Type: application/json" -d "{}"
```

如果失败，请检查：
- Godot 已打开且插件已启用
- 端口 9600 未被其他应用程序占用
- 防火墙未阻止该端口

### 问：端口 9600 已被占用

通过 `GODOT_MCP_HTTP_PORT` 环境变量或项目设置中的 `godot_mcp/http_port` 设置不同端口。同时更新 AI 客户端的配置以匹配。

## 构建问题

### 问：Windows 构建卡死或耗时过长

尝试使用 `clang-cl` 加 `ninja`：

```bash
uv run python main.py build --compiler clang-cl --generator ninja
```

### 问：CMake 配置时出现 SSL 错误

构建系统会在 SSL 错误时自动以 `CMAKE_TLS_VERIFY=0` 重试。如果仍然失败，请检查网络连接或代理设置。

### 问：构建时提示"DLL 被锁定"

Godot 编辑器持有插件 DLL 的锁。重新构建前请先关闭 Godot。

## 运行时问题

### 问：工具返回"需要场景"错误

某些工具需要打开一个场景。在调用依赖场景的工具之前，先创建或打开一个场景。

### 问：C# 工具导致冲突

GDExtension 中的 C# 解决方案生成可能与 Godot 自身的 C# 项目管理冲突。请参阅 API 参考了解正确的 C# 工具注册方法。

### 问：set_node_property 对某些值不起作用

值必须与目标属性的类型匹配。兜底工具接受所有类型，但编辑器可能会拒绝类型不兼容的值。先使用 `get_node_property` 检查期望的类型。

## 一般问题

### 问：如何更新插件？

从 Releases 下载最新的 `addons.zip`，解压覆盖已有的 `addons/godot_mcp/` 目录。重启 Godot。

### 问：能否在 C# Godot 项目中使用？

可以。该插件是一个 GDExtension（C++），可与 Godot 中的 C# 并行工作。SDK 还支持通过 `McpToolRegistry` 注册 C# 自定义工具。

### 问：是否支持 Godot 4.5 或更早版本？

不支持。该插件需要 Godot 4.6+（具体来说是 GDExtension API 版本 4.6）。