# GodotMCP

MCP 服务器，通过 C++ GDExtension 将 Godot 4.6+ 编辑器暴露给 AI 工具。

## Build

```bash
uv run python build.py                # Debug + addons.zip
uv run python build.py --release      # Release + addons.zip
uv run python build.py --clean        # 清空 CMake 缓存（保留 _deps/）
uv run python build.py --clean-all    # 完全清除构建目录
uv run python build.py --no-zip       # 跳过 zip，快速迭代
uv run python build.py -j N           # 并行编译作业数
cmake --build build --target deep-clean  # 仅清除 addons/bin/ + _deps/
```

- **Windows 必须用 `uv run python`**，裸 `python` 或 `py -3` 都可能挂（Microsoft Store stub）。
- 自动检测 Ninja + MSVC cl.exe（通过 vswhere），Unity Build（batch_size 8）已启用。
- `godot-cpp` / `ryml` 通过 FetchContent 拉取，缓存在 `build/_deps/`。`--clean` 保留缓存，`--clean-all` 或 `deep-clean` 清除。
- CI（`.github/workflows/ci.yml`）：Ubuntu 下 `cmake -B build -S .` → `cmake --build build`，缓存 godot-cpp。

## Architecture

```
AI 客户端 ── Streamable HTTP :9600 ──► godot_mcp_gdext
  (POST /mcp, JSON-RPC 2.0 + SSE)      (C++ GDExtension, 纯主线程)
```

- **纯主线程无锁**：`EditorPlugin::_on_process_frame()` 驱动 `HttpServer::poll()`。
- **入口符号**：`gdext_rust_init`（`register_types.cpp`，遗留名——不要改 `.gdextension`）。
- **入口点**：`register_types.cpp` → `McpEditorPlugin`。仅在 `MODULE_INITIALIZATION_LEVEL_EDITOR` 初始化。
- **端口**：默认 9600，环境变量 `GODOT_MCP_HTTP_PORT` 覆盖。

### 工具系统（ITool + codegen）

所有内置工具通过 `// @tool register` 注释 + `tools/codegen.py` 自动注册。**无需**改 CMakeLists.txt 或 handler_registry.cpp。

```cpp
// @tool register
class MyTool : public ITool {
    String name() const override { return "my_tool"; }
    String category() const override { return "my_group"; }  // 原始分类
    String brief() const override { return "..."; }
    String description() const override { return "..."; }
    Dictionary input_schema() const override { ... }
    bool is_meta() const override { return false; }
    bool needs_scene() const override { return false; }
    bool needs_node() const override { return false; }
    Dictionary execute_impl(const ToolContext &ctx) override { ... }
};
```

- **源码位置**：`extensions/src/built_in/tools/<category>/*.hpp`（124 个 ITool，17 组）
- **codegen 生成**：`build/extensions/generated/generated_registration.cpp`（CMake 自动驱动 `tools/codegen.py`）
- **两轴分类**：`source()`（meta/built_in/custom）决定可见性，`category()` 决定分组
- **渐进式披露**：meta 工具始终可见；非 meta 工具通过 `list_tool_categories` → `list_tools` 二级发现
- **Category remap**：17 个原始 category → 6 个顶级（node/scene/editor/script/settings/other），见 `handler_registry.cpp:256`
- **统一调度**：`HandlerRegistry::execute()` 先查 `itool_table_`，再查 `table_`（CommandFn 后备，仅 SDK 自定义工具使用）
- **editor_plugin.cpp 二次调用**：`register_all_tools()` 内部已含 `register_itools()`，但 `editor_plugin.cpp:66` 又单独调了一次 `register_itools(registry_)`（幂等，无害）

### 工具分组

| 原始 category | 映射后 | 数 | 目录 |
|--------------|--------|:--:|------|
| `meta` | `other/meta` | 5 | `tools/meta/` |
| `node` | `node/operation` | 21 | `tools/node/` |
| `property/2d` | `node/property/2d` | 21 | `tools/property/` |
| `property/3d` | `node/property/3d` | 6 | `tools/property_3d/` |
| `collision` | `node/collision` | 2 | `tools/collision/` |
| `find` | `node/find` | 4 | `tools/find/` |
| `scene` | `scene` | 16 | `tools/scene/` |
| `editor_control` | `editor/general` | 7 | `tools/editor_control/` |
| `search` | `editor/search` | 3 | `tools/search/` |
| `script_gd` | `script/gdscript` | 5 | `tools/script_gd/` |
| `script/csharp` | `script/csharp` | 6 | `tools/script_cs/` |
| `script_helpers` | `script/helpers` | 3 | `tools/script_helpers/` |
| `settings/core` | `settings/project` | 7 | `tools/project_settings/` |
| `settings/extended` | `settings/extended` | 10 | `tools/project_settings_ext/` |
| `input_map` | `settings/input_map` | 4 | `tools/input_map/` |
| `plugin_management` | `other/plugin_management` | 2 | `tools/plugin_management/` |
| `undo` | `other/undo` | 2 | `tools/undo/` |
| **总计** | | **124** | |

## C++ 注意事项

- **MSVC UTF-8**：含非 ASCII 字符的字符串必须用 `String::utf8("中文")`，否则 MSVC 按 GBK 解释会乱码。CMake 已加 `/utf-8` 编译选项。
- **EditorInterface 写文件**：编辑器看不到直接 `.tscn` 写入，必须用 EditorInterface API。写文件后调用 `notify_file_changed()`（`cmd_utils.hpp:92`）。
- **`resolve_node()`**（`cmd_utils.hpp:48`）：接受 `""`、`"."`、`"/"`、`"/root"`、根节点名、`"RootName/Child"`。
- **`undoable_set()`**（`cmd_utils.hpp:81`）——"立即应用 + 注册撤销"惯用模式。
- **`variant_to_json()` / `json_to_variant()`**——Godot Variant ↔ JSON 递归转换。
- **Args 解析辅助函数**：`args_string()` / `args_int()` / `args_float()` / `args_bool()`（`cmd_utils.hpp:122-140`）。
- **`tool_schemas.json`**：`tools/tool_schemas.json` 在构建时自动复制到 `example/addons/godot_mcp/`，运行时通过 `load_tool_schemas()` 加载。
- **版本号只在根 `CMakeLists.txt:22` 维护**。`plugin.cfg` 和 `.gdextension` 由 CMake 自动生成。
- **Pinned deps**：`godot-cpp` 10.0.0-rc1，`ryml` v0.7.0（FetchContent）。升级需测试。
- **Godot LSP 客户端**：`lsp/client.cpp`，用于 GDScript 校验（validate_gdscript 工具）。

## 测试

- **YAML 驱动**：`POST /run-tests`（`Content-Type: application/x-yaml`）或 C++ `TestRunnerDock` 底部面板。
  ```bash
  curl -X POST http://localhost:9600/run-tests \
    -H "Content-Type: application/x-yaml" \
    --data-binary @tests/yaml_tests/<name>.test.yaml
  ```
- **Python 编排器**：`uv run python tests/test_orchestrator.py`（管理 Godot 生命周期）。
- **清理策略**：双源追踪（EditorFileSystem 快照差分 + 工具返回值路径追踪），只删交集，保护用户文件。
- **旧 Python 阶段测试**在 `tests/test_phases/`，过渡期共存。

## SDK 层（GDScript/C# 自定义工具）

- `McpToolDefinition`（RefCounted）——GDScript 可继承，定义工具元数据 + 执行逻辑。
- `McpToolRegistry`（单例）——注册/反注册自定义工具。内部调用 `HandlerRegistry::register_custom_tool()`（走 `CommandFn` 后备表）。
- SDK 工具注册名自动加 `custom_` 前缀。

## 文档站点

```bash
pnpm install
pnpm run dev    # Rspress 开发服务器
pnpm run build  # 构建 docs/
```

- `docs/` — Rspress 站点（中/英双语）。
- `.repo_wiki/` — 项目知识库（架构、模块、参考）。

## 客户端配置

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

## 开发配置

- Python ≥3.14，依赖 `pyyaml`。
- `uv` 管理 Python 环境（通过 `uv.lock` + `pyproject.toml`）。
- Node.js 依赖通过 `pnpm` 管理（仅文档站点需要）。

## 项目知识库

- [项目 Wiki](.repo_wiki/index.md)
