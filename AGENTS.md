# GodotMCP — 智能体指令

MCP 服务器，通过 **125 条命令**（121 条 gdext + 4 条服务端处理）将 Godot 4.6+ 编辑器暴露给 AI 工具。

## 架构

```
AI 客户端 ── stdio ──► godot-mcp-server.exe ── WebSocket :9500 ──► godot_mcp_gdext.dll
                       (Python/Cython)                        (crates/gdext, cdylib)

crates/core ── 共享协议类型
server/ ── Python 服务器，通过 Cython --embed 编译为独立 exe
          registry.py 是工具 schema 的唯一权威来源
```

- **stdio 是唯一的 MCP 传输方式**。
- **添加工具**：在 `server/src/godot_mcp_server/registry.py` 注册 schema → 在 `crates/gdext/src/ipc/ws_server.rs::route_tool_call` 添加路由分支。

## 处理器路由链

`crates/gdext/src/commands/mod.rs::create_registry()` 注册 17 组 handler。`dispatch()`（`ws_server.rs`）先独立处理 MetaCommands（3 条，同步，无需 MainThreadDispatcher），再迭代 registry。

## 主线程 / tokio 分割（最大陷阱）

工具路由在 tokio 工作线程上运行。**几乎所有 Godot API 在非主线程调用都会 panic。**

1. **`MainThreadDispatcher`**：工作线程调用 `dispatcher.submit(move || cmd_*(...))` → 返回 `oneshot` future。主线程通过 `process_pending()` 排空 `VecDeque`。**所有 `cmd_*` 函数都通过此机制调用。**
2. **`logging` 模块**：工作线程调用 `log_info/log_warn/log_error` → mpsc 通道 + `eprintln!`。主线程 `drain_to_console()` → `godot_print!`。**永远不要在工作线程调用 `godot_print!`。**

两个队列都通过 `Callable::from_fn` 在 `SceneTree::process_frame` 上排空（不是 `EditorPlugin::_process()`——后者持有 `bind_mut`，在重入 `EditorInterface` 调用时会死锁）。

## gdext API 约束

- `Dictionary::get(key)` 返回 `Option<Variant>`（单参数，无默认值）。用 `.map(|v| v.to::<T>()).unwrap_or(default)` 解包。
- `ProjectSettings::get_setting(name)` 接受 `impl AsArg<GString>`——传 `&str` 或 `GString`，不要传 `&StringName`。
- 保存资源优先用 `ResourceSaver::singleton().save_ex(resource).path(path).done()`，或先 `resource.set_path()`。
- 用 `godot::tools::try_load::<T>(path)`（返回 `Result`）而非 `ResourceLoader::singleton().load() + cast`。
- `commands/mod.rs` 的 `resolve_node(&root, path)` 接受 `""`、`"."`、`"/"`、`"/root"`、根节点名称或 `"RootName/Child"`（自动剥离前缀）。所有节点查找都应使用此函数。
- JSON↔Variant：`j2v`/`v2j` 处理 `Vector2/3/4`、`Color`、`Rect2`、`Quaternion`、`Resource`。始终使用它们。
- 场景文件操作必须使用 `EditorInterface`——编辑器看不到直接的 `.tscn` 文件写入。
- 写入文件后，调用 `EditorInterface::singleton().get_resource_filesystem().update_file()` 让编辑器感知变更。
- 创建子目录：`DirAccess::open("res://")` → `make_dir_recursive()`。
- `ensure_parent_dir()`（`commands/mod.rs`）：**必须在主线程调用**，用于创建 `res://` 路径的父目录。

## 编辑器模式限制

- `get_variable`/`set_variable`：编辑器模式下仅支持 `@export`。非导出变量使用 `PlaceHolderScriptInstance`。
- `validate_gdscript`：需要编辑器设置 → 网络 → Language Server → 启用。会创建临时 tokio 运行时。
- `add_circle_collision`/`add_rectangle_collision`：检测已有的 `CollisionShape2D`；`mode` 字段指示操作路径。
- `rename_scene`：目标已打开但不是活动标签页时会报错。
- `find_nodes_by_name`：大小写敏感的子串匹配（`contains`），不是 glob。
- `get/set_node_collision_layer/mask`：非 CollisionObject2D/3D 节点会报错。
- `set_node_texture`：以 `Texture2D` 加载（不是通用 `Resource`）。默认属性 `"texture"`。
- `set_project_setting`/`set_main_scene`：会调用 `ProjectSettings::save()`；失败时 `warning` 字段有提示。

## 构建

```
py -3 build.py                        # debug + addons.zip (CMake + Corrosion)
py -3 build.py --release              # release + addons.zip
py -3 build.py --clean                # cargo clean + 清空 addons/bin/
py -3 build.py --no-zip               # 跳过打包（快速迭代）
py -3 build.py --no-server            # 仅重建 dll（编辑器中快速迭代）
```

CMake 构建流程：Cython `--embed` 编译 `server/entry.pyx` → patch `PYTHONHOME` → 编译为独立 `.exe`。需要 `server/.venv`（含 Cython）。

**注意事项**：
- Windows 上 `python`/`python3` 是 Microsoft Store 存根——使用 `py -3`。
- 重建后需重启 MCP 客户端——它持有旧的 `godot-mcp-server.exe`。CMake 会自动 `taskkill`/`pkill`；手动操作则先 `taskkill`/`pkill`。
- Godot 编辑器加载插件时 `godot_mcp_gdext.dll` 被锁定。重建前需关闭编辑器或禁用插件。
- 锁定依赖：`godot = "=0.5"`。升级需充分测试。
- `Cargo.lock` 已提交（二进制 crate）。不要随意重新生成。
- `rust-toolchain.toml`：`channel = "stable"`，组件 `rustfmt` + `clippy`。
- `plugin.cfg` 版本通过 CMake 从 `Cargo.toml [workspace.package].version` 自动同步。只需改 cargo 版本，不要改 plugin.cfg。

## 测试

- `cargo test --workspace` — 离线测试（core + gdext），无需 Godot。
- `cd server && pytest` — Python 服务器测试（需要 `.venv`，`pytest-asyncio`）。
- **CI 门禁**（Ubuntu）：`cargo fmt --check --all` → `cargo clippy --workspace -- -D warnings` → `cmake -B build -S .` → `cmake --build build --config Debug` → `cargo test --workspace`。

## 服务端工具（4 条）

`get_server_version`、`godot_editor_open/close/restart` 在 Python 服务器中直接处理（不转发到 gdext）。

- `GODOT_PATH` 环境变量必须通过 MCP 客户端 `env` 配置设置（stdio 服务器不继承 shell 环境变量）。
- 关闭：`taskkill /F /IM`（Windows）或 `pkill -f`（Unix）。
- 重启在终止后等待 500ms。

## 关键模式

- 所有 `cmd_*` 都是**自由函数**——闭包无法在 `dispatcher.submit()` 跨越时捕获 `&self`。Handler 通过 `pipe()` 转换 `Result<Value, String>`。
- `godot-mcp-server` 接受 `--godot-port`（默认 `9500`）。
- GDExtension 初始化级别为 `InitLevel::Editor`（`lib.rs`）。
- Tokio 运行时：`new_multi_thread().worker_threads(2)`（`editor_plugin.rs`）。
- `j2v` 自动将 `res://`/`user://` 字符串通过 `try_load` 作为 `Resource` 加载。
- 服务器到 Godot 的重连：指数退避，最多 5 次尝试，1s–30s。
- `csharp_create_solution` 直接在 Python 中生成 `.sln` + `.csproj`——**无需启动第二个 Godot 进程**（避免端口 9500 冲突）。

## 项目知识库

- [项目 Wiki](.repo_wiki/index.md)
