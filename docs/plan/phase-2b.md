# Phase 2b — Scene Management

> **Status**: 🛠 Partially shipped
> **Merge commit**: `12fb1431` (feature/scene_manager → develop)
> **Tag**: `v0.1.0` at `d1ee1fb3`

## Why this phase exists

Phase 2a gave us `CommandHandler` routing, `MainThreadDispatcher`, and a Dock UI skeleton — but only 4 meta tools. Phase 2b fills the tool surface: scene manipulation, script editing, editor control, and project config. The goal is that an AI agent can do most of what a human developer does inside the Godot editor, without ever touching `.tscn` files directly.

## Sub-phases

| # | Sub-phase | Tools | Status |
|---|-----------|-------|--------|
| 2b.1 | Scene Management | 10 + 21 utility | ✅ Shipped |
| 2b.2 | Script Management | 13 | ⏳ Not started |
| 2b.3 | Editor Control | 7 | ⏳ Not started |
| 2b.4 | Project Management | 6 | ⏳ Not started |
| 2b.5 | Server registry sync | — | ✅ Shipped |
| 2b.6 | e2e tests | 5 representative tools | ⏳ Not started |
| 2b.7 | Documentation sync | — | ⏳ Not started |

---

## 2b.1 — Scene Management (✅ Shipped)

10 core scene tools + 21 utility tools (scene-file ops, editor tabs, advanced node ops). Total 31 tools in `handle_scene_tool`.

**Shipped tools**:

- Read: `get_scene_tree`, `get_node_path`, `get_property_list`, `get_property`
- Node write: `create_node`, `delete_node`, `rename_node`, `set_property`, `duplicate_node`, `move_node`
- Script attach: `attach_script`, `detach_script`
- Search: `find_nodes`
- Scene file: `create_scene`, `delete_scene`, `rename_scene`, `branch_to_scene`, `scene_to_branch`, `instantiate_scene`
- Advanced: `reset_parent`, `set_as_root`, `batch_set_property`
- Editor tabs: `open_scene`, `close_scene`, `save_scene`, `save_scene_as`, `save_all_scenes`, `reload_scene`, `get_open_scenes`, `get_open_scene_roots`, `mark_scene_unsaved`

**Shipped infrastructure**:

- Cross-thread logging: `log_info`/`log_warn`/`log_error` → `mpsc` channel → `drain_to_console()` via pump; `eprintln!` mirror
- Main-thread pump: `Callable::from_fn` on `SceneTree::process_frame` — solves `bind_mut` re-entrancy (gdext issue #338)
- `j2v`/`v2j` JSON↔Variant helpers (Vector2/3/4, Color, Rect2, Quaternion, Resource)
- `resolve_node(root, path)` for root aliases (`""`, `"."`, `"/"`, root name)
- Server registry expanded from 4→35 tools; `package_addons.py` rewritten with flags
- Wiki restructure (14 pages), bilingual README, AGENTS.md, License

---

## 2b.2 — Script Management (⏳ Not started)

### Design decisions (confirmed)

1. **GDScript 和 C# 拆为两套独立工具**，命名全部带语言前缀（如 `create_gdscript` / `create_csharp_script`），AI 一眼区分目标语言
2. **内置 Godot 4.6 官方 default 模板**—— `ScriptLanguage::make_template()` 和 `get_built_in_templates()` 是 C++ virtual methods，不通过 ClassDB 暴露，gdext 无法调用。我们在代码中硬编码与 Godot 编辑器一致的模板字符串，用 `_BASE_` / `_CLASS_` / `_TS_` 占位符替换
3. **C# 编译**：gdext 端直接 spawn `dotnet build`（与 Godot 编辑器本质一致）
4. **C# solution 创建**：spawn `godot --headless --build-solutions --path <project_root> --quit`
5. **validate_gdscript 走 LSP**：连接 Godot 内置 GDScript Language Server (TCP `127.0.0.1:<port>`)，通过 `textDocument/didOpen` → `publishDiagnostics` 获取完整诊断（含行号、列号、severity、message），弥补 `Script::reload()` 只返回 `Error` 枚举码的不足
6. **LSP 端口**：运行时读 `EditorSettings::get_setting("network/language_server/remote_port")`，默认 6005
7. **LSP 连接策略**：首发用短连接（每次 validate 新建 TCP → init → didOpen → 收 diagnostics → shutdown），性能不达标时升级为长连接（归入 Phase 6）
8. **`regex` 依赖**：新增 `regex = "1"` 到 `crates/gdext/Cargo.toml`
9. **共享 helpers 提取到 `mod.rs`**：`v2j`/`j2v`/`resolve_node`/`s` 从 `scene.rs` 提取到 `commands/mod.rs`，`scene.rs` 和新模块都 `use super::*`

### 工具清单（13 工具）

#### A. GDScript 子组（6 工具）

| 工具 | 必需参数 | 可选参数 | 返回 | 核心 API |
|------|---------|---------|------|---------|
| `create_gdscript` | `path`, `base_class` | `class_name`, `template`, `overwrite` | `{path, created, bytes, language}` | 内置 GDScript 模板 + `FileAccess` 写入 + `EditorFileSystem::update_file` |
| `read_gdscript` | `path` | `from_editor` | `{path, source, length, language}` | `FileAccess::get_file_as_string` 或 `Script::get_source_code` |
| `edit_gdscript` | `path`, `source` | — | `{path, bytes, reloaded, reload_error?}` | `FileAccess` 覆写 + `update_file` + `Script::reload()` |
| `validate_gdscript` | `path` | — | `{path, diagnostics: [{line, column, severity, message, source}]}` | 短连接 LSP TCP: `initialize` → `didOpen` → `publishDiagnostics` → `shutdown` |
| `list_gdscripts` | — | `root`, `include_addons`, `max_results` | `{scripts: [{path, size}], count, truncated}` | `DirAccess::get_files_at` / `get_directories_at` 递归 `.gd` |
| `eval_gdscript_expression` | `expression` | `node_path`, `const_only` | `{expression, result, type}` 或 `{expression, error, phase}` | `Expression::new_gd().parse + execute_ex().const_calls_only(true)` |

#### B. C# 子组（5 工具）

| 工具 | 必需参数 | 可选参数 | 返回 | 核心 API |
|------|---------|---------|------|---------|
| `create_csharp_script` | `path`, `base_class` | `overwrite` | `{path, created, bytes, language}` | 内置 C# 模板 + `FileAccess` 写入 + `update_file`；前置 `.csproj` 存在检查 |
| `read_csharp_script` | `path` | — | `{path, source, length, language}` | `FileAccess::get_file_as_string` |
| `edit_csharp_script` | `path`, `source` | — | `{path, bytes, updated_file}` | `FileAccess` 覆写 + `update_file`（不 reload） |
| `list_csharp_scripts` | — | `root`, `include_addons`, `max_results` | `{scripts: [{path, size}], count, truncated}` | `DirAccess` 递归 `.cs` |
| `csharp_build` | — | `configuration` | `{exit_code, success, stdout, stderr, errors: [{file, line, column, code, message}]}` | `std::process::Command("dotnet").arg("build")`，cwd = `ProjectSettings::globalize_path("res://")`，正则解析 MSBuild 输出 |
| `csharp_create_solution` | — | — | `{exit_code, success, stdout, stderr}` | `std::process::Command(godot_exe).args("--headless", "--build-solutions", "--path", project_root, "--quit")`。失败直接报错 |

> **注意**：`csharp_build` 和 `csharp_create_solution` 虽然通过 `std::process::Command` spawn 子进程，仍放在 `dispatcher.submit()` 闭包内执行，因为 `ProjectSettings::globalize_path("res://")` 和 `OS::get_executable_path()` 必须在主线程调用。子进程 spawn 本身不阻塞主线程太久，但 `output()` 是同步等待的 —— 后续 Phase 6 可改为异步。

#### C. 通用搜索（2 工具）

| 工具 | 必需参数 | 可选参数 | 返回 | 核心 API |
|------|---------|---------|------|---------|
| `find_in_file` | `path`, `pattern` | `literal`, `case_sensitive`, `max_matches` | `{path, matches: [{line, col, text}], count}` | `FileAccess::get_file_as_string` + `regex` |
| `search_project` | `pattern` | `literal`, `extensions`, `root`, `case_sensitive`, `max_matches`, `max_files` | `{matches: [{path, line, col, text}], count, files_scanned, truncated}` | 递归 `DirAccess` + `match_lines` |

### 内置模板

**GDScript**（占位符替换 `_BASE_` / `_CLASS_` / `_TS_`）：

```gdscript
extends _BASE_

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
_TS_pass

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
_TS_pass
```

**C#**（占位符替换 `_BASE_` / `_CLASS_` / `_BINDINGS_NAMESPACE_` / `_TS_`）：

```csharp
using _BINDINGS_NAMESPACE_;
using System;

public partial class _CLASS_ : _BASE_
{
    public override void _Ready()
    {
_TS_base._Ready();
    }

    public override void _Process(double delta)
    {
_TS_base._Process(delta);
    }
}
```

### LSP 子系统

新增 `crates/gdext/src/lsp/` 模块：

```
crates/gdext/src/lsp/
  mod.rs          # pub mod client; pub mod protocol;
                  # LspClient::validate(path) -> Vec<Diagnostic>
  client.rs       # TCP 客户端: connect → initialize → didOpen → publishDiagnostics → shutdown
  protocol.rs     # 最小 LSP types: InitializeParams, DidOpenParams, Diagnostic, PublishDiagnosticsParams
```

**实现流程**（`validate_gdscript` 被调用时）：

1. `dispatcher.submit()` 闭包内，主线程读 `EditorSettings::get_setting("network/language_server/remote_port")` 拿端口
2. 在闭包内 `tokio::runtime::Handle::current().block_on()` 执行 LSP 流程：
   - `TcpStream::connect("127.0.0.1:<port>")`
   - 发 `initialize` 请求（带 `processId`, `rootUri`, `capabilities`）
   - 收 `InitializeResult` 响应（如果首个响应是 `publishDiagnostics` 则重发 `initialize`，规避 Godot LSP first-connect bug #78764）
   - 发 `initialized` 通知
   - 发 `textDocument/didOpen` 通知（带 source code, languageId = "gdscript"）
   - 在 3s deadline 内收 `textDocument/publishDiagnostics` 通知
   - 发 `textDocument/didClose` + `shutdown` + `exit`，关闭连接
3. `Vec<Diagnostic>` 转 JSON 返回

**Fallback**：LSP 连不上 → `{"error": "GDScript LSP not available, ensure the editor is running with Language Server enabled (default port 6005)"}`

### 文件清单

| 文件 | 操作 | 改动 |
|------|------|------|
| `crates/gdext/Cargo.toml` | edit | `regex = "1"` |
| `crates/gdext/src/commands/mod.rs` | edit | `pub mod script_gd; pub mod script_cs; pub mod search;` + `create_registry` 加 3 个 handler + 提取 `pub fn s/v2j/j2v/resolve_node` |
| `crates/gdext/src/commands/scene.rs` | edit | 删除 4 个 helper 定义，改 `use super::*;` |
| `crates/gdext/src/commands/script_gd.rs` | new | 6 GDScript 工具 + `GDSCRIPT_TEMPLATE` 常量 |
| `crates/gdext/src/commands/script_cs.rs` | new | 5 C# 工具 + `CSHARP_TEMPLATE` 常量 + `std::process::Command` spawn |
| `crates/gdext/src/commands/search.rs` | new | 2 通用搜索 + `walk_dir` / `match_lines` helpers |
| `crates/gdext/src/lsp/mod.rs` | new | `pub mod client; pub mod protocol;` + `LspClient::validate()` |
| `crates/gdext/src/lsp/client.rs` | new | TCP 客户端：connect → initialize → didOpen → publishDiagnostics → shutdown |
| `crates/gdext/src/lsp/protocol.rs` | new | 最小 LSP types |
| `crates/gdext/src/lib.rs` | edit | `pub mod lsp;` |
| `crates/gdext/src/ipc/ws_server.rs` | edit | `route_tool_call` 加 3 个 handler 分支 |
| `crates/server/src/tool_registry.rs` | edit | 13 个 schema + 测试 35→48 |
| `crates/server/src/handler.rs` | edit | 测试 35→48 |

### 已知限制

1. **`validate_gdscript` 依赖 LSP 可用**——如果编辑器 Language Server 被关闭或端口被占用，验证不可用。返回明确错误信息，AI agent 可以降级为直接使用 `edit_gdscript` + 人工检查
2. **C# 验证 = build**——不做独立的 `validate_csharp_script`。`csharp_build` 的 stderr 已包含完整编译诊断
3. **`csharp_create_solution` 需要 Godot .NET 版本**——如果用户用的是非 .NET Godot，spawn 会失败，直接报错
4. **`search_project` 在主线程上可能卡帧**——递归遍历 + 文件读取全部在 `dispatcher.submit()` 闭包内。缓解：`max_files` + `max_matches` 上限。Phase 6 可考虑迁移 `FileAccess` 到 worker thread
5. **短连接 LSP 性能**——每次 validate 约 200-500ms（TCP 建立 + init + parse），高频调用场景不友好。升级为长连接归入 Phase 6

### Done means

- [ ] 13 个新工具全部可编译 + test 绿
- [ ] 工具注册数 48
- [ ] GDScript 工具链：创建 → 读取 → 编辑 → 验证（含语法错误诊断返回行号/列号） → 列表 → eval 全部通过
- [ ] C# 工具链：创建 solution → 创建脚本 → 读取 → 编辑 → build（含编译错误解析）全部通过
- [ ] 通用搜索：`find_in_file` + `search_project`（正则 + 字面量）通过
- [ ] LSP 连不上时返回明确错误信息而非 panic
- [ ] `cargo fmt --check --all && cargo clippy --workspace -- -D warnings && cargo build --workspace && cargo test --workspace` 全绿
- [ ] `.repo_wiki/reference/tools-catalog.md` 新增 13 条
- [ ] `.repo_wiki/log.md` 追加 entry
- [ ] 此处 2b.2 状态标记 ✅

---

## 2b.3 — Editor Control (⏳ Not started)

| Tool | Description |
|------|-------------|
| `play` | 运行当前场景 |
| `pause` | 暂停运行中的场景 |
| `stop` | 停止运行中的场景 |
| `get_console` | 获取编辑器控制台输出 |
| `clear_console` | 清空编辑器控制台 |
| `refresh_project` | 刷新项目文件系统 |
| `execute_menu_item` | 执行编辑器菜单项 |

## 2b.4 — Project Management (⏳ Not started)

| Tool | Description |
|------|-------------|
| `get_project_settings` | 读取项目设置 |
| `update_project_settings` | 更新项目设置 |
| `get_input_map` | 获取输入映射 |
| `configure_input_map` | 配置输入映射 |
| `list_scenes` | 列出项目所有场景文件 |
| `run_tests` | 运行测试 |

## 2b.6 — e2e tests (⏳ Not started)

5 个代表性工具的端到端测试：mock WS 服务端 + 真实 server 进程。

## 2b.7 — Documentation sync (⏳ Not started)

每个工具的参数与响应示例。
