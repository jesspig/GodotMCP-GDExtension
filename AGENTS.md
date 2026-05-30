# GodotMCP — 智能体指令

MCP 服务器，通过 C++ GDExtension 将 Godot 4.6+ 编辑器暴露给 AI 工具。

## 架构

```
AI 客户端 ── Streamable HTTP :9600 ──► godot_mcp_gdext.dll/.so/.dylib（C++ GDExtension）
                                            │
                                            ├── McpEditorPlugin (EditorPlugin)
                                            ├── HttpServer (TCPServer + 手动 HTTP/1.1)
                                            ├── McpHandler (JSON-RPC 2.0 + SSE)
                                            └── HandlerRegistry (CommandFn 函数指针)
```

- **纯主线程**——无 worker 线程、无锁。`EditorPlugin::_on_process_frame()` 驱动 `HttpServer::poll()`。
- `McpHandler` 管理 MCP 会话/SSE，`HandlerRegistry` 路由工具调用到 `CommandFn`。
- `LspClient` 通过 `StreamPeerTCP` 连接 Godot 内置 LSP 服务器（默认端口 6005）做 GDScript 验证。
- C++17，godot-cpp 10.0.0-rc1（FetchContent 固定）。

## 构建

```bash
py -3 build.py                        # debug + addons.zip
py -3 build.py --release              # release + addons.zip
py -3 build.py --clean                # 清空 CMake 缓存（保留 _deps/）
py -3 build.py --no-zip               # 跳过打包（快速迭代）
cmake --build build --target deep-clean  # 同时删除 _deps/（FetchContent 缓存）
```

**Windows 关键**：必须用 `py -3` 而非 `python`——Microsoft Store 的 python 路由桩会静默卡死。

**build.py 特性**：

- `--clean` 仅清除 CMake 缓存，保留 `_deps/`（godot-cpp FetchContent 缓存），避免重新下载。
- 检测到 `MSB4019`/`VCTargetsPath`/`CMAKE_C_COMPILER` 等 stale cache 错误时，自动清空缓存重试。
- 通过 `vswhere` 自动检测 Visual Studio 安装路径设置 `VCTargetsPath`（解决 .NET SDK preview 干扰）。

**CI 门禁**（Ubuntu）：`cmake -B build -S . && cmake --build build --config Debug`。
**Release**：push tag `v*` → 跨平台构建（Ubuntu/MacOS/Windows），产出 `addons.zip`（含三平台 gdext 库）。

**注意事项**：

- Godot 编辑器加载插件时 DLL 被锁定。重建前需关闭编辑器或在项目设置中禁用插件。
- 版本在根 `CMakeLists.txt` 设置：`set(PROJECT_VERSION "...")`。仅在此处更改——`plugin.cfg` 由 CMake 自动生成。
- `.gdextension` 由 CMake 生成到 `example/addons/godot_mcp/`。入口符号：`gdext_rust_init`（兼容遗留名——不要修改 `.gdextension` 文件），`compatibility_minimum = "4.6"`，`reloadable = true`。

## 工具注册约定

122 个已注册工具分布在 16 组中（另有 6 个 C# 工具已实现但暂未注册）。注册流程：

1. 在 `extensions/gdext/src/commands/` 中创建 `cmd_<group>.cpp` → 实现 `register_<group>(HandlerRegistry &)` 自由函数 → 在 `handler_registry.cpp` 中添加声明并在 `register_all_tools()` 中调用。

## 端口配置

| 端口 | 用途 | 环境变量 |
|------|------|----------|
| 9600 | MCP Streamable HTTP | `GODOT_MCP_HTTP_PORT` |
| 6005 | Godot LSP（内置，仅用于验证） | — |

## 注册新工具

16 个处理器组在 `handler_registry.cpp:register_all_tools()` 中注册。步骤：

1. 在 `extensions/gdext/src/commands/` 创建 `cmd_<group>.cpp`
2. 实现 `register_<group>(HandlerRegistry &)` 自由函数
3. 在 `handler_registry.cpp` 添加前向声明，在 `register_all_tools()` 中调用

工具 schemas 从 `res://addons/godot_mcp/tool_schemas.json` 加载（由 `tools/tool_schemas.json` 复制到构建输出）。

## C++ GDExtension API 注意事项

- 所有处理器接收 `Dictionary args` 并返回 `Dictionary`。失败时返回包含 `"error"` 字符串键的 Dictionary。
- 场景文件操作必须使用 `EditorInterface`——编辑器看不到直接的 `.tscn` 文件写入。
- 写入文件后，调用 `EditorInterface::get_singleton()->get_resource_filesystem()->update_file()` 让编辑器感知变更。
- 创建子目录：`DirAccess::open("res://")` → `make_dir_recursive()`。
- `resolve_node()` 在 `cmd_utils.hpp` 中——接受 `""`、`"."`、`"/"`、`"/root"`、根节点名称或 `"RootName/Child"`。
- `.gdextension` 文件由 CMake 生成输出到 `example/addons/godot_mcp/`。`entry_symbol = "gdext_rust_init"`，`compatibility_minimum = "4.6"`，`reloadable = true`。
- 工具限制见 [注意事项](/reference/client-quirks) 和 [常见问题](/reference/faq)。

## 项目规范

AI 客户端（opencode、claude code 等）通过 Streamable HTTP 连接：

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

## 文档

完整的项目文档位于 `docs/` 目录（Rspress 构建）：

| 路径 | 内容 |
|------|------|
| [指南：快速开始](docs/guide/getting-started.md) | 安装、配置、基本使用 |
| [指南：架构概览](docs/guide/architecture.md) | 单进程 GDExtension 架构 |
| [指南：构建与打包](docs/guide/building.md) | 构建系统、版本管理 |
| [参考：工具目录](docs/reference/tools-catalog.md) | 全部 122 个工具列表 |
| [参考：客户端配置](docs/reference/client-config.md) | 各 AI 客户端的配置方式 |
| [参考：通信协议](docs/reference/protocol.md) | MCP Streamable HTTP 协议 |
| [参考：LSP 验证](docs/reference/lsp-client.md) | GDScript 语法验证流程 |
| [参考：C# 解决方案](docs/reference/csharp-solution.md) | 自动生成 .sln/.csproj |
| [参考：项目设置映射](docs/reference/project-settings-ext.md) | 显示/物理/渲染键映射 |
