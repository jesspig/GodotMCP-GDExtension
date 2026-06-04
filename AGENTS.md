# GodotMCP

MCP 服务器，通过 C++ GDExtension 将 Godot 4.6+ 编辑器暴露给 AI 工具。纯主线程无锁，HTTP 端口 9600。

## Build

```bash
uv run python build.py                # Debug + addons.zip
uv run python build.py --release      # Release + addons.zip
uv run python build.py --no-zip       # 跳过 zip，快速迭代
uv run python build.py --clean        # 清空 CMake 缓存（保留 _deps/）
uv run python build.py --clean-all    # 完全清除 build/（含 _deps/）
uv run python build.py -j N           # 并行编译作业数
cmake --build build --target deep-clean  # 仅清 addons/bin/ + _deps/
```

- **所有平台使用 `uv run python`**（而非裸 `python`/`py -3`）：`uv` 确保依赖一致且自动激活虚拟环境。Windows 裸 `py -3` 在 Microsoft Store stub 下会挂。`build.py` 自动检测 `vswhere` + Ninja + MSVC `cl.exe`。
- **FetchContent 缓存**：`godot-cpp 10.0.0-rc1` 与 `ryml v0.7.0` 在 `build/_deps/`。`--clean` 保留，`--clean-all` / `deep-clean` 清除。
- **DLL 文件锁**：Godot 编辑器持有 `addons/bin/godot_mcp_gdext.dll`，重建失败时先关闭编辑器或禁用插件。
- **CI**: Ubuntu `cmake -B build -S .` → `cmake --build build` + godot-cpp 缓存。

## 关键约束

- **版本号**只在根 `CMakeLists.txt:22`（`PROJECT_VERSION`）。`plugin.cfg` 与 `.gdextension` 由 CMake 自动生成。
- **入口符号** `gdext_rust_init`（`register_types.cpp:45`）——遗留名，兼容 `.gdextension` 配置，**不要改**。
- **`compatibility_minimum = "4.6"`** 与 `GODOTCPP_API_VERSION "4.6"`（`extensions/CMakeLists.txt:15`）必须同步。
- **Pinned deps**: `godot-cpp 10.0.0-rc1`、`ryml v0.7.0`。升级前必须测试。

## 架构

```
AI 客户端 ── Streamable HTTP :9600 ──► godot_mcp_gdext (C++ GDExtension, 纯主线程)
  POST /mcp, JSON-RPC 2.0 + SSE       EditorPlugin::_on_process_frame() 驱动 HttpServer::poll()
                                       工具由 codegen 自动注册（@tool register + YAML 数据库）
```

- **源码根**：`extensions/src/`（不是仓库根的 `src/`）。`editor_plugin.cpp:48` 调 `register_itools()`，由 codegen 生成的 `build/extensions/generated/generated_registration.cpp` 注入。
- **端口**：默认 9600，env `GODOT_MCP_HTTP_PORT` 覆盖（含范围校验）。
- **端点**：`/mcp`（JSON-RPC 2.0 + SSE），`/run-tests`（YAML 测试）。
- **.opencode/opencode.json** 已预配置 MCP server。
- **双重注册**：GDREGISTER 注册 SDK 类 + EditorPlugin，`HandlerRegistry` 管理 ITool 主表与 SDK `CommandFn` 旁路表。

## 添加内置工具

**无需**改 `CMakeLists.txt` 或 `handler_registry.cpp`。CMake `GLOB` 自动编译 `extensions/src/built_in/tools/**/*.cpp`，`tools/codegen.py` 扫描 `// @tool register` 注释生成注册代码。

```cpp
// extensions/src/built_in/tools/<category>/my_tool.hpp
// @tool register
class MyTool : public ITool {
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }
    String name() const override { return "my_tool"; }
    String category() const override { return "node_tools"; }
    String brief() const override { ... }
    String description() const override { ... }
    Dictionary input_schema() const override { ... }
    bool is_meta() const override { return false; }       // true → tools/list 始终可见
    bool needs_scene() const override { return false; }   // 注入 ctx.root
    bool needs_node() const override { return false; }    // 注入 ctx.node
    Dictionary execute_impl(const ToolContext &ctx) override { ... }
};
```

- **顶级分类**硬编码于 `handler_registry.cpp:182-190` 的 `top_level_meta()`。新顶级需同步加 meta。
- **节点属性工具**：`node_props/db/*.yaml`（~283 节点类型）→ `node_property_tool.hpp` 模板。`tools/collect_node_props.py --godot /path/to/godot` 重新生成。
- **资源属性工具**：`node_resource/db/*.yaml`（~419 资源类型）→ `node_resource_tool.hpp` 模板。
- **现有分类目录**：`meta/`、`group/`、`signal/`、`node_tools/general/`、`node_props/`、`node_resource/`。
- **场景树工具**：`editor_tools/scene_tree/`（20 个工具，4 批）。详见 `.repo_wiki/modules/scene-tree-tools.md`。全部场景树修改操作使用 `EditorUndoRedoManager`（通过 `EditorInterface::get_singleton()->get_editor_undo_redo()` 获取），不直接调用裸 `UndoRedo`。剪贴板通过 `PackedScene::pack()` / `instantiate()` 实现。
- **`tool_base.hpp:48-55`**：`ITool` 接口契约。

## SDK 层（GDScript / C# 自定义工具）

- `McpToolDefinition`（`sdk/mcp_tool_definition.hpp`）——GDScript 可继承。
- `McpToolRegistry` 单例：Mode A `register_definition(this)` 或 Mode B `register_tool(name, ...)`（`mcp_tool_registry.cpp:113-160`）。**注册名自动加 `custom_` 前缀**。
- 两条路径调 `HandlerRegistry::register_custom_tool()` → `CommandFn` 旁路表。

## C++ 注意事项

- **MSVC UTF-8**：`extensions/CMakeLists.txt:152` 已加 `/utf-8 /bigobj`。**含非 ASCII 的字符串字面量必须用 `String::utf8("中文")`**，否则 MSVC 按 GBK 解释。
- **常用 helper**（`cmd_utils.hpp`）：
  - `resolve_node()`：接受 `""`/`"."`/`"/"`/`"/root"`/根节点名/`"Root/Child"`。
  - `undoable_set()`：修改节点属性时优先用（立即应用 + 注册撤销）。
  - `notify_file_changed()`：直接写 `.tscn`/`.gd` 后必须调用。
  - `args_string/int/float/bool`：从 `Dictionary` 安全取参数 + 默认值。
  - `variant_to_json` / `json_to_variant`：Godot Variant ↔ JSON 递归转换。
  - `ensure_parent_dir()` / `globalize_path()` / `relative_path()`：res:// 路径辅助。
- **写文件**：不能直接 `.tscn` 写盘，必须经 EditorInterface API 或写后 `notify_file_changed()`。
- **LSP 客户端**：`lsp/client.cpp` 通过 `StreamPeerTCP` 调 Godot LSP 服务（`validate_gdscript` 工具使用）。
- **底部面板**：`add_control_to_bottom_panel` 在 godot-cpp 10.0.0-rc1 未绑定，`editor_plugin.cpp:74-79` 用 `call()` 兜底。

## 测试

**YAML 驱动**，C++ `TestEngine`（`testing/test_engine.cpp`）在编辑器进程内执行测试，Python 编排器管理 Godot 生命周期。

```bash
# 准备（仅一次）
cp tests/.env.example tests/.env
# 编辑 tests/.env 设置 GODOT_PATH

# 安装测试依赖
pip install -r tests/requirements.txt

# 单文件测试（需 Godot 在 :9600 运行）
curl -X POST http://localhost:9600/run-tests \
  -H "Content-Type: application/x-yaml" \
  --data-binary @tests/yaml_tests/<name>.yaml

# 完整套件
uv run python tests/test_orchestrator.py
# 或: pytest tests/test_orchestrator.py -v --asyncio-mode=auto
```

- **测试文件**：`tests/yaml_tests/*.yaml`（18 文件）。
- **清理**：双源追踪（EditorFileSystem 快照 + 工具返回值）取交集，只删测试自建文件。
- **报告**：`tests/output/`（gitignored），JSON + Markdown。
- **带内 UI**：编辑器底部 "Tests" 面板（`TestRunnerDock`）可手工运行 YAML。

## 文档站点

```bash
pnpm install
pnpm run dev    # Rspress 开发服务器
pnpm run build  # 构建 docs/
```

- `docs/`——Rspress 站点（中/英双语，`i18n.json` 驱动）。
- `.repo_wiki/`——项目知识库（架构、ADR、变更日志）。优先查这里获取设计细节。

## 项目知识库

- [`.repo_wiki/index.md`](.repo_wiki/index.md)
