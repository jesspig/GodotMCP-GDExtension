# GodotMCP

Godot 4.6+ 编辑器 MCP 服务器（C++ GDExtension，纯主线程无锁，HTTP :9600）。

## Build

```bash
uv run python build.py                  # Debug + addons.zip
uv run python build.py --release        # Release + addons.zip
uv run python build.py --clean          # 清 CMake 缓存（保留 _deps/）
uv run python build.py --clean-all      # 完全清除 build/（含 _deps/）
uv run python build.py --no-zip         # 跳过 zip 快速迭代
uv run python build.py -j N             # 并行编译作业数
cmake --build build --target deep-clean # 仅清 addons/bin/ + _deps/
```

- **始终用 `uv run python`**（非 `py -3`）。`uv` 保证依赖一致且自动激活 .venv。
- **陈旧缓存自动重试**：`build.py:145-147` 检测 MSB4019/VCTargetsPath 等错误，自动 `--clean` 后重试。
- **FetchContent 缓存**：`godot-cpp 10.0.0-rc1` + `ryml v0.7.0` 在 `build/_deps/`。
- **DLL 文件锁**：`example/addons/godot_mcp/bin/godot_mcp_gdext.dll` 被 Godot 编辑器持有，重建失败时先关编辑器或禁用插件。
- **构建优化**：sccache/ccache 自动检测（CMakeLists.txt:29-35）；PCH（MSVC 约 30-50% 加速，`extensions/src/pch.hpp`）；Unity build 默认开启；lld-link 自动检测。

## 关键约束

- **版本号**只在根 `CMakeLists.txt:22`（`PROJECT_VERSION "0.2.0-dev3"`）。`plugin.cfg` 与 `.gdextension` 由 CMake 自动生成。
- **入口符号** `gdext_rust_init`（`register_types.cpp:45`）——遗留名，**不要改**。
- **`compatibility_minimum = "4.6"`** 与 `GODOTCPP_API_VERSION "4.6"`（`extensions/CMakeLists.txt:15`）必须同步。
- **Pinned deps**: `godot-cpp 10.0.0-rc1`、`ryml v0.7.0`。升级前必须测试。

## 架构

```
AI 客户端 ── Streamable HTTP :9600 ──► godot_mcp_gdext (C++ GDExtension)
  POST /mcp, JSON-RPC 2.0 + SSE       McpEditorPlugin::_on_process_frame() 驱动 HttpServer::poll()
                                       工具由 codegen 自动注册（@tool register + YAML 数据库）
```

- **源码根**：`extensions/src/`（不是仓库根的 `src/`）。`editor_plugin.cpp:48` 调 `register_itools()`，由 codegen 生成的 `build/generated/generated_registration.cpp` 注入。
- **端口**：默认 9600，env `GODOT_MCP_HTTP_PORT` 覆盖（含范围校验）。
- **端点**：`/mcp`（JSON-RPC 2.0 + SSE），`/run-tests`（YAML 测试）。
- **双重注册**：`GDREGISTER` 注册 SDK 类 + EditorPlugin；`HandlerRegistry` 管理 `ITool` 主表与 SDK `CommandFn` 旁路表。

## 添加内置工具

**CMake 自动编译**所有 `extensions/src/built_in/tools/**/*.cpp`。**codegen 自动注册**扫描 `// @tool register` 注释 + YAML 数据库。

```cpp
// extensions/src/built_in/tools/<category>/my_tool.hpp
// @tool register
class MyTool : public ITool {
    void set_registry(HandlerRegistry *reg) override { reg_ = reg; }
    String name() const override { return "my_tool"; }
    String category() const override { return "node_tools"; }
    String brief() const override { ... }
    String description() const override { ... }  // 纯虚函数，必须实现
    Dictionary input_schema() const override { ... }
    bool is_meta() const override { return false; }
    bool needs_scene() const override { return false; }
    bool needs_node() const override { return false; }
    Dictionary execute_impl(const ToolContext &ctx) override { ... }
};
```

- **`description()` 是纯虚函数**，所有 `ITool` 子类必须实现。简单场景返回 `brief()`。
- **`// @tool register` 注释**是 codegen 识别的唯一标记，缺少则工具不注册。
- **顶级分类**硬编码于 `handler_registry.cpp:183-192` 的 `top_level_meta()`。新顶级分类**必须**同步加 meta。
- **YAML 属性数据库**（`node_props/db/*.yaml` ~283 节点，`node_resource/db/*.yaml` ~419 资源）→ 模板 `node_property_tool.hpp`。用 `tools/collect_node_props.py --godot /path/to/godot` 重新生成。
- **现有分类目录**：`meta/`、`group/`、`signal/`、`node_tools/general/`、`node_props/`、`node_resource/`、`editor_tools/scene_tree/`、`editor_tools/workspace/`。
- **场景树修改**：必须使用 `EditorUndoRedoManager`（通过 `EditorInterface::get_singleton()->get_editor_undo_redo()`），不用裸 `UndoRedo`。剪贴板通过 `PackedScene::pack()` / `instantiate()`。
- **写入文件**：不能直接写 `.tscn`，必须经 EditorInterface API 或写后调用 `notify_file_changed()`。

## C++ 注意事项

- **MSVC UTF-8**：根 CMakeLists.txt:43 已加 `/utf-8 /bigobj`。非 ASCII 字符串字面量**必须**用 `String::utf8("中文")`，否则 MSVC 按 GBK 解释。
- **编辑器内部类**（`EditorDebuggerNode`、`EditorLog` 等）不在 godot-cpp 绑定中。用 `find_children("*", "ClassName", true, false)` 遍历场景树 + `call()` 动态调用。
- **常用 helper**（`cmd_utils.hpp`）：
  - `resolve_node()`：接受 `""`/`"."`/`"/"`/`"/root"`/根节点名/`"Root/Child"`
  - `undoable_set()`：修改节点属性优先用（立即应用 + 注册撤销）
  - `args_string/int/float/bool`：从 Dictionary 安全取参数
  - `variant_to_json` / `json_to_variant`：Variant ↔ JSON 递归转换
- **底部面板**：`add_control_to_bottom_panel` 在 godot-cpp 10.0.0-rc1 未绑定，用 `call()` 兜底（`editor_plugin.cpp:74-79`）。

## 测试

YAML 驱动，C++ `TestEngine` 在编辑器进程内执行，Python 编排器管理 Godot 生命周期。

```bash
cp tests/.env.example tests/.env        # 编辑 GODOT_PATH
pip install -r tests/requirements.txt
curl -X POST http://localhost:9600/run-tests \
  -H "Content-Type: application/x-yaml" \
  --data-binary @tests/yaml_tests/<name>.yaml  # 单文件测试
uv run python tests/test_orchestrator.py       # 完整套件
```

- **测试文件**：`tests/yaml_tests/*.yaml`（18 文件）。
- **清理**：双源追踪（EditorFileSystem 快照 + 工具返回值取交集），只删测试自建文件。
- **报告**：`tests/output/`（gitignored），JSON + Markdown。
- **带内 UI**：编辑器底部 "Tests" 面板（`TestRunnerDock`）可手工运行 YAML。
- **测试依赖**：`pytest`、`pytest-asyncio`、`httpx`、`python-dotenv`、`PyYAML`、`mcp`。

## 文档站点

```bash
pnpm install
pnpm run dev    # Rspress 开发服务器
pnpm run build  # 构建 docs/
```

- `docs/` — Rspress 站点（中/英双语，`i18n.json` 驱动）。
- `.repo_wiki/` — 项目知识库（架构、ADR、变更日志）。优先查这里获取设计细节。
- `package.json` 中 `"type": "module"`。

## CI（Ubuntu-only）

```bash
cmake -B build -S .
cmake --build build --config Debug
```

- Release 构建见 `.github/workflows/release.yml`，平台矩阵（ubuntu/macos/windows）生成 addons.zip 并发布 GitHub Release。
- godot-cpp 缓存 key: `godot-cpp-${{ runner.os }}-10.0.0-rc1`。

## Codegen

```bash
uv run python tools/codegen.py \
  --source-dir extensions/src/built_in/tools \
  --node-props-db extensions/src/built_in/tools/node_props/db \
  --resource-props-db extensions/src/built_in/tools/node_resource/db \
  --output build/generated/generated_registration.cpp
```

- 扫描 `.hpp` 中的 `// @tool register`，正则 `^class\s+(\w+)\s*[:]` 提取类名。
- **UTF-8 BOM 会导致 codegen 无法识别 `// @tool register`**。用 `Set-Content`（PowerShell）创建的文件带 BOM，必须用 `$PSDefaultParameterValues['Out-File:Encoding']='utf8'` 或 Python 写入。
- YAML 数据库可选：无 PyYAML 时跳过属性工具生成。
- CMake 自动在 `extensions/CMakeLists.txt:101-116` 中驱动 codegen 作为自定义命令。

## 项目知识库

- [`.repo_wiki/index.md`](.repo_wiki/index.md)
