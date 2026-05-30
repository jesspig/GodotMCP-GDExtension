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

## 端口

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

- **所有处理器**：接收 `Dictionary args`，返回 `Dictionary`。失败时返回含 `"error"` 键的 Dictionary。
- **场景文件写入**：必须通过 `EditorInterface`——编辑器看不到直接 `.tscn` 文件写入。写入后调用 `EditorInterface::get_singleton()->get_resource_filesystem()->update_file()`。
- **挂载脚本**：必须按顺序调用 `fs->update_file(path)` → `ResourceLoader::load(path)` → `gdscript->set_source_code(src)` → `gdscript->reload()`，否则 `@export` 变量不会注册。
- **EditorNode 不可通过 Engine 单例获取**（GDExtension 限制）。需遍历 `SceneTree.get_root()` 的子节点搜索 `"EditorNode"`。
- **路径解析**：`cmd_utils.hpp:resolve_node()` 接受 `""`、`"."`、`"/"`、`"/root"`、根节点名称或 `"RootName/Child"`。
- **undoable_set()**：在 `cmd_utils.hpp` 中——立即应用属性变更并注册撤销步骤。
- **create_node 的撤销**：必须在 `EditorUndoRedoManager` 中加上 `add_do_reference(node)` 防止 Godot 在撤销前垃圾回收新节点。
- **工具 schemas** 需要先在 `tools/tool_schemas.json` 中定义 schema JSON，构建时 CMake 复制到输出目录。如果 schema 缺失，工具仍可被调用但 MCP 客户端无参数提示。

## 项目规范

- `pyproject.toml` 仅用于 `build.py` 的 lint（ruff + mypy --strict）。C++ 代码没有 lint/测试。
- 没有 C++ 测试框架。
- `AGENTS.md` 是唯一指令文件（无 CLAUDE.md、.cursorrules 等）。
- 项目 Wiki：[.repo_wiki/index.md](.repo_wiki/index.md)。
