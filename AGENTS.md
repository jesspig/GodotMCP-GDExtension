# GodotMCP — 智能体指令

MCP 服务器，通过 **125 条命令**（121 条 gdext + 4 条服务端处理）将 Godot 4.6+ 编辑器暴露给 AI 工具。

## 架构（两进程）

```
AI 客户端 ── stdio ──► godot-mcp-server.exe ── WebSocket :9500 ──► godot_mcp_gdext.dll
                        (Python/Cython)                           (extensions/gdext/, C++)

server/ ── Python 服务器，通过 Cython --embed 编译为独立 exe
           registry.py 是工具 schema 的唯一权威来源
extensions/gdext/ ── C++ GDExtension
```

- **stdio 是唯一的 MCP 传输方式**。
- **添加工具**：在 `server/src/godot_mcp_server/registry.py` 注册 schema → 在 `extensions/gdext/src/commands/handler_registry.cpp` 添加 `register_<group>()` 函数。

## GDExtension 实现

C++ GDExtension 使用 godot-cpp 10.0.0-rc1 构建：

| 方面 | C++ 实现 |
|------|----------|
| 线程 | **纯主线程**——`EditorPlugin::_on_process_frame()` 调用 `ws_server_.poll()` |
| 日志 | 直接 `UtilityFunctions::print/push_warning/push_error` |
| WebSocket | Godot 内置 `TCPServer` + `WebSocketPeer` |
| JSON↔Variant | Godot 原生 `Dictionary`/`JSON` |
| 工具路由 | `HandlerRegistry` 中的 `CommandFn` 函数指针 |
| LSP 验证 | `lsp/client.cpp` 通过 `StreamPeerTCP` 连接 Godot 内置 LSP 服务器 |
| 依赖 | `godot-cpp 10.0.0-rc1`（FetchContent） |

## 构建

```
py -3 build.py                        # debug + addons.zip（CMake + Cython --embed）
py -3 build.py --release              # release + addons.zip
py -3 build.py --clean                # 清空 build/ 缓存（保留 _deps，即 godot-cpp 缓存）
py -3 build.py --no-zip               # 跳过打包（快速迭代）
py -3 build.py --no-server            # 仅重建 gdext dll（编辑器中快速迭代）
```

CMake 构建流：`extensions/gdext/` 的 C++ GDExtension + 通过 Cython `--embed` 编译 `server/entry.py` → `server/tools/patch_entry_c.py` 补丁 `PYTHONHOME` → 编译为独立 `.exe`。需要项目根目录下的 `.venv`（含 Cython）。

`build.py` 特性：

- `--clean` **仅**清空 CMake 缓存文件，**保留** `_deps/`（godot-cpp FetchContent 缓存），避免重新下载。
- **自动 stale cache 恢复**：检测到 `MSB4019`、`VCTargetsPath`、`CMAKE_C_COMPILER` 等错误时，自动清空缓存重试。
- **VCTargetsPath 自动检测**：通过 `vswhere` 查找 Visual Studio 安装路径，自动设置 `VCTargetsPath`（解决 .NET SDK preview 干扰）。

**注意事项**：

- Windows 上 `python`/`python3` 是 Microsoft Store 存根——使用 `py -3`。
- 重建后需重启 MCP 客户端——它持有旧的 `godot-mcp-server.exe`。CMake 会自动 `taskkill`/`pkill`；手动操作则先 `taskkill`/`pkill`。
- Godot 编辑器加载插件时 `godot_mcp_gdext.dll` 被锁定。重建前需关闭编辑器或禁用插件。
- GDExtension 入口符号：`gdext_rust_init`（`register_types.cpp` 中的 C++ 代码的遗留名称——**不要修改** `.gdextension` 文件）。
- 版本在 `CMakeLists.txt` 中设置：`set(PROJECT_VERSION "...")`。仅在此处更改版本，**不**在 `plugin.cfg` 中（后者由 CMake 生成）。
- godot-cpp 固定为 `10.0.0-rc1`（`extensions/gdext/CMakeLists.txt` 中的 FetchContent）。升级需充分测试。

## 工具注册约定

125 个工具分布在 16 个已注册的 C++ 处理器组中（`handler_registry.cpp` 中的 `register_all_tools`）。注册流程：

1. **Python 端**：在 `registry.py` 的 `_TOOLS` 列表中添加工具名称、描述和 JSON schema。
2. **C++ 端**：在 `extensions/gdext/src/commands/` 中创建 `cmd_<group>.cpp` → 实现 `register_<group>(HandlerRegistry &)` 自由函数 → 在 `handler_registry.cpp` 中添加声明并在 `register_all_tools()` 中调用。

## 端口配置

默认端口 `9500`，可通过以下方式覆盖：

- `--godot-port` CLI 参数（传给服务器 exe）
- `GODOT_MCP_PORT` 环境变量（在编辑器中读取）

## 服务端工具（4 条，Python 中处理）

`get_server_version`、`godot_editor_open/close/restart` 在 Python 服务器中直接处理，不转发到 gdext。

- `GODOT_PATH` 环境变量必须通过 MCP 客户端 `env` 配置设置（stdio 服务器不继承 shell 环境变量），用于 `godot_editor_open/restart`。
- 关闭：`taskkill /F /IM`（Windows）或 `pkill -f`（Unix）。
- 重启在终止后等待 500ms。

## 项目配置

- **pyproject.toml** 在项目根目录（不是 `server/`）。`server/` 下无包配置。
- **Python**：`requires-python = ">=3.13"`；CI 使用 Python 3.14。
- **测试**：`pytest`（从根目录运行，`testpaths = ["server/tests"]`）。pytest 配置：`asyncio_mode = auto`。
- **依赖管理**：CI 使用 `uv`（`uv venv .venv` + `uv pip install -e ".[dev]"`）。本地开发也建议用 `uv` 创建 `.venv`，CMake 会自动检测并使用 `.venv/Scripts/python.exe`。
- **MCP 依赖**：`mcp>=1.27`、`websockets>=14`、`cython>=3.0`（dev）。
- **Lint**：`ruff`（line-length=88, select=E/W/F/I/B/C/N, ignore=E501/B008），`mypy --strict`。

## 测试

```
pytest                      # Python 服务器测试（需要 .venv，pytest-asyncio）
```

- **CI 门禁**（Ubuntu）：`cmake -B build -S .` → `cmake --build build --config Debug` → `pytest`。
- **Release**：push tag `v*` 触发跨平台构建（Ubuntu/MacOS/Windows），产出自带 `addons.zip`（含三个平台的 gdext .dll/.so/.dylib）和三个平台的 server 二进制。
- 暂没有 C++ 测试。

## gdext API 注意事项（C++ 版本）

- 所有处理器接收 `Dictionary args` 并返回 `Dictionary`。失败时返回包含 `"error"` 字符串键的 Dictionary。
- 场景文件操作必须使用 `EditorInterface`——编辑器看不到直接的 `.tscn` 文件写入。
- 写入文件后，调用 `EditorInterface::get_singleton()->get_resource_filesystem()->update_file()` 让编辑器感知变更。
- 创建子目录：`DirAccess::open("res://")` → `make_dir_recursive()`。
- `resolve_node()` 在 `cmd_utils.hpp` 中——接受 `""`、`"."`、`"/"`、`"/root"`、根节点名称或 `"RootName/Child"`。
- `.gdextension` 文件由 CMake 生成输出到 `example/addons/godot_mcp/`。`entry_symbol = "gdext_rust_init"`，`compatibility_minimum = "4.6"`，`reloadable = true`。
- 工具限制见 README.md 的表和 `.repo_wiki`。

## 项目知识库

- [项目 Wiki](.repo_wiki/index.md) — 架构、线程模型、工具目录、协议规范
