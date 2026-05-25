# GodotMCP — 智能体指令

MCP 服务器，通过 **125 条命令**（121 条 gdext + 4 条服务端处理）将 Godot 4.6+ 编辑器暴露给 AI 工具。

## 架构

```
AI 客户端 ── stdio ──► godot-mcp-server.exe ── WebSocket :9500 ──► godot_mcp_gdext.dll
                        (crates/server, bin)                           (crates/gdext, cdylib)
                                    │                                            │
                                    └──── crates/core ── 共享协议类型 ──────────┘
```

- **stdio 是唯一的 MCP 传输方式**。`transport-streamable-http-server` 在依赖中但未接线。
- **添加工具需要两侧同步**：在 `crates/server/src/tool_registry.rs::register_defaults` 注册 schema，同时在 `crates/gdext/src/ipc/ws_server.rs::route_tool_call` 添加路由分支。服务端工具只需注册 + `handler.rs` 匹配分支。
- **测试中硬编码的计数**：`tool_registry.rs` 和 `handler.rs` 均断言 `total == 125`。增删工具时两处都要更新。
- **`dispatch()`**（`ws_server.rs:249`）先独立处理 MetaCommands（3 条，同步，无需 MainThreadDispatcher），然后迭代 registry 中的 17 个 handler 组。`dispatch()` 不单独维护工具列表——新工具只需在对应 handler 和 registry 中注册。

## 硬性规定

- **必须结合联网搜索确认正确的 API**：在使用任何 Godot、gdext 或 Rust API 之前，必须先进行联网搜索确认 API 的正确用法、参数和行为。
- **禁止访问包源码**：禁止直接查看或依赖第三方包的源码来推断 API 用法，必须通过官方文档、教程或搜索结果来确认。

## 处理器路由链

```
server handler → get_server_version + EditorControl（4 条，服务端）
gdext route_tool_call →
  MetaCommands(3, 同步) → NodeCommands(21) → PropertyCommands(21)
  → CollisionCommands(2) → FindCommands(4) → ScriptHelpersCommands(3)
  → ProjectSettingsCommands(3) → SceneCommands(15) → ScriptGdCommands(5)
  → ScriptCsCommands(6) → SearchCommands(3) → UndoCommands(2)
  → Property3dCommands(6) → EditorControlCommands(6) → ProjectSettingsExtCommands(2)
  → PluginManagementCommands(2) → InputMapCommands(4)
```

一共 17 组 handler，全部注册在 `crates/gdext/src/commands/mod.rs::create_registry()`。`dispatch()` 在迭代 registry 前先特殊处理 MetaCommands。

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
- `commands/mod.rs` 中的 `resolve_node(&root, path)` 接受 `""`、`"."`、`"/"`、`"/root"`、根节点名称或 `"RootName/Child"`（自动剥离前缀）。所有节点查找都应使用此函数。
- JSON↔Variant：`j2v`/`v2j` 处理 `Vector2/3/4`、`Color`、`Rect2`、`Quaternion`、`Resource`。始终使用它们。
- 场景文件操作必须使用 `EditorInterface`——编辑器看不到直接的 `.tscn` 文件写入。
- 写入文件后，调用 `EditorInterface::singleton().get_resource_filesystem().update_file()` 让编辑器感知变更。
- 创建子目录：`DirAccess::open("res://")` → `make_dir_recursive()`。
- `ensure_parent_dir()`（`commands/mod.rs:86`）：**必须在主线程调用**，用于创建 `res://` 路径的父目录。

## 编辑器模式限制

- `get_variable`/`set_variable`：编辑器模式下仅支持 `@export`。非导出变量使用 `PlaceHolderScriptInstance`。
- `validate_gdscript`：需要编辑器设置 → 网络 → Language Server → 启用。会创建临时 tokio 运行时。
- `add_circle_collision`/`add_rectangle_collision`：检测已有的 `CollisionShape2D`；`mode` 字段指示操作路径。
- `rename_scene`：目标已打开但不是活动标签页时会报错。
- `find_nodes_by_name`：大小写敏感的子串匹配（`contains`），不是 glob。
- `get/set_node_collision_layer/mask`：非 CollisionObject2D/3D 节点会报错。
- `set_node_texture`：以 `Texture2D` 加载（不是通用 `Resource`）。默认属性 `"texture"`。
- `set_project_setting`/`set_main_scene`：会调用 `ProjectSettings::save()`；失败时 `warning` 字段有提示。

## 构建系统（CMake + Corrosion）

```
py -3 build.py                        # debug + addons.zip
py -3 build.py --release              # release + addons.zip
py -3 build.py --clean                # cargo clean + 清空 addons/bin/
py -3 build.py --no-zip               # 跳过打包（快速迭代）
py -3 build.py --no-server            # 仅重建 dll
```

**CI 门禁**（`.github/workflows/ci.yml`，Ubuntu）：`cargo fmt --check --all` → `cargo clippy --workspace -- -D warnings` → `cmake -B build -S .` → `cmake --build build --config Debug` → `cargo test --workspace`（58 个测试，全部离线，无需 Godot）。

**Windows 上**：`python`/`python3` 是 Microsoft Store 存根——使用 `py -3`。

**注意事项**：
- 重建后需重启 MCP 客户端——它持有旧的 `godot-mcp-server.exe`。CMake 会自动 `taskkill`/`pkill`；手动操作则先 `taskkill`/`pkill`。
- Godot 编辑器加载插件时 `godot_mcp_gdext.dll` 被锁定。重建前需关闭编辑器或禁用插件。
- 锁定依赖：`godot = "=0.5"`、`rmcp = "=1.7"`。未经测试不要升级。
- `Cargo.lock` 已提交（二进制 crate）。不要随意重新生成。
- `rust-toolchain.toml` 指定 `channel = "stable"`，组件 `rustfmt` + `clippy`。
- `plugin.cfg` 版本通过 CMake 从 `Cargo.toml [workspace.package].version` 自动同步。只需改 cargo 版本，不要改 plugin.cfg。
- CMake 将构建的 DLL 复制到 `example/addons/godot_mcp/bin/`。`example/*` 已被 gitignore（例外：`project.godot`、`icon.svg`、`.gitignore`）。DLL 不被版本控制跟踪。

## C# 解决方案生成

`csharp_create_solution` 直接在 Rust 中生成 `.sln` + `.csproj`——**无需启动第二个 Godot 进程**（避免端口 9500 冲突）。

- SDK 版本来自 `Engine::get_version_info()` → `Godot.NET.Sdk/X.Y.Z[-pre.N]`（SemVer 2.0：`"beta2"` → `"beta.2"`）
- 项目名来自 `dotnet/project/assembly_name` → `application/config/name` → 项目文件夹名
- `.csproj`：UTF-8 **无** BOM；`.sln`：UTF-8 **有** BOM
- `csharp_build` 调用 `dotnet build`——编辑器持有程序集文件锁时无法运行

## 编辑器控制工具（服务端）

4 条工具在 `server/handler.rs` 中处理——不会到达 gdext：`get_server_version`、`godot_editor_open/close/restart`。关闭编辑器会断开 WebSocket，所以响应来自服务器进程。

- 必须设置 `GODOT_PATH` 环境变量。必须在 MCP 客户端 `env` 配置中设置（stdio 服务器不继承 shell 环境变量）。
- 关闭：`taskkill /F /IM`（Windows）或 `pkill -f`（Unix）。
- 重启在终止后等待 500ms。
- `get_server_version` 返回 crate 版本，纯 server 端处理，不需要 Godot。

## 关键模式

- 所有 `cmd_*` 都是**自由函数**——闭包无法在 `dispatcher.submit()` 跨越时捕获 `&self`。Handler 通过 `pipe()` 转换 `Result<Value, String>`。
- `FileAccess::file_exists()` 检查文件；`DirAccess::dir_exists_absolute()` 检查目录。
- `godot-mcp-server` 接受 `--godot-port`（默认 `9500`）。通过 MCP 客户端 `args` 传递。
- GDExtension 初始化级别为 `InitLevel::Editor`（`lib.rs`）。
- Tokio 运行时：`new_multi_thread().worker_threads(2)`（`editor_plugin.rs`）。
- `j2v` 自动将 `res://`/`user://` 字符串通过 `try_load` 作为 `Resource` 加载。
- 服务器到 Godot 的重连：指数退避，5 次尝试，1s–30s。
- MetaCommands（ping、get_engine_version、get_plugin_version）**不通过 MainThreadDispatcher**——`dispatch()` 在 tokio 线程上直接调用 `handle_meta_tool()`（同步，无需 Godot API）。

## 项目知识库

- [项目 Wiki](.repo_wiki/index.md)
