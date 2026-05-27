# GodotMCP — 智能体指令

MCP 服务器，通过 **125 条命令**（121 条 gdext + 4 条服务端处理）将 Godot 4.6+ 编辑器暴露给 AI 工具。

## 架构（两进程）

```
AI 客户端 ── stdio ──► godot-mcp-server.exe ── WebSocket :9500 ──► godot_mcp_gdext.dll
                       (Python/Cython)                           (extensions/gdext/, C++)

server/ ── Python 服务器，通过 Cython --embed 编译为独立 exe
           registry.py 是工具 schema 的唯一权威来源
extensions/gdext/ ── C++ GDExtension（当前活跃实现）
crates/gdext/ ── Rust GDExtension（遗留，仅用于测试——看代码时优先看 C++ 版本）
crates/core/ ── Rust 共享协议类型（遗留，仅用于测试）
```

- **stdio 是唯一的 MCP 传输方式**。
- **添加工具**：在 `server/src/godot_mcp_server/registry.py` 注册 schema → 在 `extensions/gdext/src/commands/handler_registry.cpp` 添加 `register_<group>()` 函数。

## GDExtension 实现关键区别

C++ 重写消除了 Rust gdext 绑定的复杂性：

| 方面 | Rust（遗留） | C++（当前） |
|------|-------------|------------|
| 线程 | tokio 工作线程 + `MainThreadDispatcher` + process_frame 泵 | **纯主线程**——`EditorPlugin::_on_process_frame()` 调用 `ws_server_.poll()` |
| 日志 | MPSC 通道 + `eprintln!` + `drain_to_console()` | 直接 `UtilityFunctions::print/push_warning/push_error` |
| WebSocket | tokio-tungstenite | Godot 内置 `TCPServer` + `WebSocketPeer` |
| JSON↔Variant | `j2v`/`v2j` 自由函数 | Godot 原生 `Dictionary`/`JSON` |
| 工具路由 | `CommandHandler` trait + `dispatch()` | `HandlerRegistry` 中的 `CommandFn` 函数指针 |
| 依赖 | `godot = "=0.5"` crate | `godot-cpp 10.0.0-rc1`（FetchContent） |

## 构建

```
py -3 build.py                        # debug + addons.zip（CMake + Cython --embed）
py -3 build.py --release              # release + addons.zip
py -3 build.py --clean                # cargo clean + 清空 addons/bin/
py -3 build.py --no-zip               # 跳过打包（快速迭代）
py -3 build.py --no-server            # 仅重建 gdext dll（编辑器中快速迭代）
```

CMake 构建流：`extensions/gdext/` 的 C++ GDExtension + 通过 Cython `--embed` 编译 `server/entry.py` → 补丁 `PYTHONHOME` → 独立 `.exe`。需要 `server/.venv`（含 Cython）。

**注意事项**：
- Windows 上 `python`/`python3` 是 Microsoft Store 存根——使用 `py -3`。
- 重建后需重启 MCP 客户端——它持有旧的 `godot-mcp-server.exe`。CMake 会自动 `taskkill`/`pkill`；手动操作则先 `taskkill`/`pkill`。
- Godot 编辑器加载插件时 `godot_mcp_gdext.dll` 被锁定。重建前需关闭编辑器或禁用插件。
- GDExtension 入口符号：`gdext_rust_init`（`register_types.cpp` 中的 C++ 代码的遗留名称——**不要修改** `.gdextension` 文件）。
- 版本在 `CMakeLists.txt` 中设置：`set(PROJECT_VERSION "0.1.5-dev.1")`。仅在此处更改版本，**不**在 `plugin.cfg` 中。
- godot-cpp 固定为 `10.0.0-rc1`（`extensions/gdext/CMakeLists.txt` 中的 FetchContent）。升级需充分测试。

## 工具注册约定

125 个工具分布在 16 个 C++ 处理器组中（与 Rust 的 17 个组重叠——部分合并）。注册流程：

1. **Python 端**：在 `registry.py` 的 `_TOOLS` 列表中添加工具名称、描述和 JSON schema。
2. **C++ 端**：在 `extensions/gdext/src/commands/` 中创建 `cmd_<group>.cpp` → 实现 `register_<group>(HandlerRegistry &)` 自由函数 → 在 `handler_registry.cpp` 中添加声明和调用。

## 端口配置

默认端口 `9500`，可通过以下方式覆盖：
- `--godot-port` CLI 参数（传给服务器 exe）
- `GODOT_MCP_PORT` 环境变量（在编辑器中读取——对基于 Rust 的遗留 `crates/gdext/` 无影响）

## 服务端工具（4 条，Python 中处理）

`get_server_version`、`godot_editor_open/close/restart` 在 Python 服务器中直接处理，不转发到 gdext。

- `GODOT_PATH` 环境变量必须通过 MCP 客户端 `env` 配置设置（stdio 服务器不继承 shell 环境变量）。
- 关闭：`taskkill /F /IM`（Windows）或 `pkill -f`（Unix）。
- 重启在终止后等待 500ms。

## 测试

```
cargo test --workspace      # Rust 离线测试（core + 遗留 gdext），不需要 Godot
cd server && pytest         # Python 服务器测试（需要 .venv，pytest-asyncio）
```

- **CI 门禁**（Ubuntu）：`cargo fmt --check --all` → `cargo clippy --workspace -- -D warnings` → `cmake -B build -S .` → `cmake --build build --config Debug` → `cargo test --workspace`。
- Rust 测试覆盖了协议类型和 Rust gdext 处理器。暂没有 C++ 测试。

## gdext API 注意事项（C++ 版本）

- 所有处理器接收 `Dictionary args` 并返回 `Dictionary`。失败时返回包含 `"error"` 字符串键的 Dictionary。
- 场景文件操作必须使用 `EditorInterface`——编辑器看不到直接的 `.tscn` 文件写入。
- 写入文件后，调用 `EditorInterface::get_singleton()->get_resource_filesystem()->update_file()` 让编辑器感知变更。
- 创建子目录：`DirAccess::open("res://")` → `make_dir_recursive()`。
- `resolve_node()` 在 `cmd_utils.hpp` 中——接受 `""`、`"."`、`"/"`、`"/root"`、根节点名称或 `"RootName/Child"`。
- 工具限制见 README.md 的表和 `.repo_wiki`。

## 项目知识库

- [项目 Wiki](.repo_wiki/index.md) — 架构、线程模型、工具目录、协议规范
