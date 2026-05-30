# GodotMCP — 智能体指令

MCP 服务器，通过 C++ GDExtension 将 Godot 4.6+ 编辑器暴露给 AI 工具。

## 架构（单进程）

```
AI 客户端 ── Streamable HTTP :9600 ──► godot_mcp_gdext.dll（C++ GDExtension）
                                           │
                                           ├── MCP Session 管理
                                           ├── JSON-RPC 2.0 处理
                                           └── HandlerRegistry（CommandFn）

extensions/gdext/ ── C++ GDExtension（唯一的代码库）
```

- **Streamable HTTP** 是唯一的 MCP 传输方式（端口 9600）。
- **添加工具**：在 `extensions/gdext/src/commands/` 中创建 `cmd_<group>.cpp` → 实现 `register_<group>(HandlerRegistry &)` 自由函数 → 在 `handler_registry.cpp` 中添加声明并在 `register_all_tools()` 中调用。

## GDExtension 实现

C++ GDExtension 使用 godot-cpp 10.0.0-rc1 构建：

| 方面 | C++ 实现 |
|------|----------|
| 线程 | **纯主线程**——`EditorPlugin::_on_process_frame()` 调用 `http_server_.poll()` |
| 日志 | 直接 `UtilityFunctions::print/push_warning/push_error` |
| HTTP 服务器 | Godot 内置 `TCPServer` + 手动 HTTP/1.1 解析 |
| JSON↔Variant | Godot 原生 `Dictionary`/`JSON` |
| 工具路由 | `HandlerRegistry` 中的 `CommandFn` 函数指针 |
| MCP 协议 | `McpHandler` 处理 JSON-RPC 2.0、会话管理、SSE 流 |
| CORS | OPTIONS 预检 + `Access-Control-Allow-Origin: *` |
| LSP 验证 | `lsp/client.cpp` 通过 `StreamPeerTCP` 连接 Godot 内置 LSP 服务器 |
| 依赖 | `godot-cpp 10.0.0-rc1`（FetchContent） |

## 构建

```
py -3 build.py                        # debug + addons.zip
py -3 build.py --release              # release + addons.zip
py -3 build.py --clean                # 清空 build/ 缓存（保留 _deps，即 godot-cpp 缓存）
py -3 build.py --no-zip               # 跳过打包（快速迭代）
```

CMake 构建流：`extensions/gdext/` 的 C++ GDExtension → 输出 `godot_mcp_gdext.dll/.so/.dylib`。

`build.py` 特性：

- `--clean` **仅**清空 CMake 缓存文件，**保留** `_deps/`（godot-cpp FetchContent 缓存），避免重新下载。
- **自动 stale cache 恢复**：检测到 `MSB4019`、`VCTargetsPath`、`CMAKE_C_COMPILER` 等错误时，自动清空缓存重试。
- **VCTargetsPath 自动检测**：通过 `vswhere` 查找 Visual Studio 安装路径，自动设置 `VCTargetsPath`（解决 .NET SDK preview 干扰）。

**注意事项**：

- Godot 编辑器加载插件时 `godot_mcp_gdext.dll` 被锁定。重建前需关闭编辑器或禁用插件。
- GDExtension 入口符号：`gdext_rust_init`（`register_types.cpp` 中的 C++ 代码的遗留名称——**不要修改** `.gdextension` 文件）。
- 版本在 `CMakeLists.txt` 中设置：`set(PROJECT_VERSION "...")`。仅在此处更改版本，**不**在 `plugin.cfg` 中（后者由 CMake 生成）。
- godot-cpp 固定为 `10.0.0-rc1`（`extensions/gdext/CMakeLists.txt` 中的 FetchContent）。升级需充分测试。

## 工具注册约定

125 个工具分布在 16 个已注册的 C++ 处理器组中（`handler_registry.cpp` 中的 `register_all_tools`）。注册流程：

1. 在 `extensions/gdext/src/commands/` 中创建 `cmd_<group>.cpp` → 实现 `register_<group>(HandlerRegistry &)` 自由函数 → 在 `handler_registry.cpp` 中添加声明并在 `register_all_tools()` 中调用。

## 端口配置

| 端口 | 用途 | 环境变量 |
|------|------|----------|
| 9600 | MCP Streamable HTTP（AI 客户端连接） | `GODOT_MCP_HTTP_PORT` |

## 项目配置

- **pyproject.toml** 在项目根目录。仅保留构建工具依赖。
- **Lint**：`ruff`（line-length=88, select=E/W/F/I/B/C/N, ignore=E501/B008），`mypy --strict`。

## 测试

- 暂没有 C++ 测试。
- **CI 门禁**（Ubuntu）：`cmake -B build -S .` → `cmake --build build --config Debug`。
- **Release**：push tag `v*` 触发跨平台构建（Ubuntu/MacOS/Windows），产出 `addons.zip`（含三个平台的 gdext .dll/.so/.dylib）。

## gdext API 注意事项（C++ 版本）

- 所有处理器接收 `Dictionary args` 并返回 `Dictionary`。失败时返回包含 `"error"` 字符串键的 Dictionary。
- 场景文件操作必须使用 `EditorInterface`——编辑器看不到直接的 `.tscn` 文件写入。
- 写入文件后，调用 `EditorInterface::get_singleton()->get_resource_filesystem()->update_file()` 让编辑器感知变更。
- 创建子目录：`DirAccess::open("res://")` → `make_dir_recursive()`。
- `resolve_node()` 在 `cmd_utils.hpp` 中——接受 `""`、`"."`、`"/"`、`"/root"`、根节点名称或 `"RootName/Child"`。
- `.gdextension` 文件由 CMake 生成输出到 `example/addons/godot_mcp/`。`entry_symbol = "gdext_rust_init"`，`compatibility_minimum = "4.6"`，`reloadable = true`。
- 工具限制见 README.md 的表和 `.repo_wiki`。

## MCP 客户端配置

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

## 项目知识库

- [项目 Wiki](.repo_wiki/index.md) — 架构、线程模型、工具目录、协议规范
