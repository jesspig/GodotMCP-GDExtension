# GodotMCP

MCP 服务器，通过 C++ GDExtension 将 Godot 4.6+ 编辑器暴露给 AI 工具。支持渐进式工具披露和 GDScript/C# 自定义工具扩展。

## 架构

```
AI 客户端 ── Streamable HTTP :9600 ──► godot_mcp_gdext.dll/.so/.dylib
                                            │
                                            ├── server/
                                            │   ├── HttpServer (TCPServer + HTTP/1.1)
                                            │   ├── McpHandler (JSON-RPC 2.0 + SSE)
                                            │   └── HandlerRegistry (CommandFn + ToolInfo)
                                            │
                                            ├── sdk/
                                            │   ├── McpToolDefinition (RefCounted, GDScript 可继承)
                                            │   └── McpToolRegistry (单例, 工具注册中心)
                                            │
                                            ├── built_in/
                                            │   ├── cmd_info.cpp (godot_info)
                                            │   ├── cmd_meta_tools.cpp (4 个渐进式发现工具)
                                            │   └── cmd_*.cpp (其余内置工具)
                                            │
                                            └── McpEditorPlugin (EditorPlugin, 组装者)
```

- **纯主线程**——无锁。`EditorPlugin::_on_process_frame()` 驱动 `HttpServer::poll()`。
- `LspClient` 通过 `StreamPeerTCP` 连接 Godot 内置 LSP（端口 6005）做 GDScript 验证。
- C++17，godot-cpp 10.0.0-rc1（FetchContent 固定）。
- 入口符号：`gdext_rust_init`（遗留名——不要修改 `.gdextension` 文件）。

## 渐进式工具披露

`tools/list` 只返回 5 个 always-on 工具（~500 tokens），其余工具通过 meta-tools 按需发现：

| Always-on 工具 | 用途 |
|---|---|
| `godot_info` | 连接状态 + 引擎/插件/项目/编辑器信息 |
| `list_tool_categories` | 列出所有工具分类及工具数 |
| `list_tools` | 列出分类下工具（name + brief） |
| `get_tool_schema` | 获取单工具完整 schema |
| `call_tool` | 兜底调用任意工具（不推荐） |

**典型工作流**：`godot_info` → `list_tool_categories` → `list_tools("scene")` → `get_tool_schema("create_scene")` → 原生调用

## 自定义工具（SDK）

GDScript/C# 开发者可通过 SDK 注册自定义 MCP 工具。自定义工具自动添加 `custom_` 前缀。

### 方式 A：继承 McpToolDefinition（推荐）

```gdscript
@tool
class_name MyTool
extends McpToolDefinition

func _init():
    tool_name = "my_action"          # → 最终注册为 custom_my_action
    category = "my_game"
    brief = "执行自定义操作"
    description = "详细描述..."
    input_schema = { "type": "object", "properties": { ... }, "required": [...] }

func execute(args: Dictionary) -> Dictionary:
    # 工具逻辑
    return { "success": true }
```

注册：`MyTool.new().register_tool()`

### 方式 B：Callable 直接注册

```gdscript
McpToolRegistry.get_singleton().register_tool(
    "my_action", "my_game", "简短描述", "详细描述",
    { "type": "object", "properties": { ... } },
    Callable(self, "_on_execute")
)
```

### 动态刷新

注册/注销工具后自动发送 `notifications/tools/list_changed` 通知。支持此通知的客户端会自动刷新工具列表。

## 构建

```bash
uv run python build.py                  # debug + addons.zip
uv run python build.py --release        # release + addons.zip
uv run python build.py --clean          # 清空 CMake 缓存（保留 _deps/）
uv run python build.py --clean-all      # 完全清除构建目录（包括 FetchContent _deps/）
uv run python build.py -j N             # 并行编译作业数
cmake --build build --target deep-clean # 仅清除 addons/bin/ + _deps/（FetchContent 缓存）
```

- **Windows 关键**：必须用 `uv run python` 而非裸 `python`——Microsoft Store 的 python 路由桩会静默卡死。
- Ninja 自动检测（Windows via vswhere + cl.exe），Unity Build（batch_size 8）已启用。
- 检测到 `MSB4019`/`VCTargetsPath` 等 stale cache 错误时自动清空缓存重试。
- **DLL 被 Godot 编辑器锁定时**：关闭编辑器或在项目设置中禁用插件后再重建。
- **版本**在根 `CMakeLists.txt:22` 设置（`set(PROJECT_VERSION "...")`）。`plugin.cfg` 和 `.gdextension` 由 CMake 自动生成——不要手动改。
- **Python 3.14+** 必需（`pyproject.toml`）。
- **CI 门禁**（Ubuntu）：`cmake -B build -S . && cmake --build build --config Debug`。
- **Release**：push tag `v*` → 跨平台构建（Ubuntu/MacOS/Windows），产出 `addons.zip`。

## 测试

```bash
uv run python tests/smoke_test.py         # 导入检查（不需要 Godot）
uv run python tests/test_orchestrator.py  # 完整集成测试（需要 Godot 运行）
```

- 18 个阶段文件（`tests/test_phases/phase_01_connect.py` → `phase_18_cleanup.py`），每个导出 `TOOL_TESTS: list[ToolTest]`。编排器顺序执行各阶段并生成 JSON + Markdown 报告。
- **必须先创建 `tests/.env`**，设置 `GODOT_PATH` 指向 Godot 4.6 可执行文件（参考 `tests/.env.example`）。
- 测试管理 Godot 编辑器生命周期（启动/停止），自动备份/恢复 `example/` 中的 `icon.svg`、`.gitignore`、`project.godot`。
- 使用自定义 `McpSession`（`tests/mcp_client.py`），**非 MCP SDK**——因为 GodotMCP 的 JSON-RPC ID 序列化为 float，与 pydantic 校验不兼容。
- 依赖管理：`uv add`，定义在 `tests/requirements.txt`（mcp, pytest, pytest-asyncio, python-dotenv, httpx）。

## 内置工具注册

内置工具在 `extensions/src/built_in/` 下，通过 `register_all_tools()` 注册：

1. `extensions/src/built_in/` 下创建 `cmd_<group>.cpp`
2. 实现 `register_<group>(HandlerRegistry &)` 自由函数
3. `server/registry/handler_registry.cpp` 添加前向声明 + 在 `register_all_tools()` 中调用

工具 schemas 从 `tools/tool_schemas.json` 加载（每个工具包含 `category` + `brief` 字段）。

## 端口

| 端口 | 用途 | 环境变量 |
|------|------|----------|
| 9600 | MCP Streamable HTTP | `GODOT_MCP_HTTP_PORT` |
| 6005 | Godot LSP（内置，仅用于验证） | — |

## C++ GDExtension 注意事项

- 所有处理器接收 `Dictionary args` 并返回 `Dictionary`。失败时返回含 `"error"` 键的 Dictionary。
- 场景文件操作必须使用 `EditorInterface`——编辑器看不到直接的 `.tscn` 文件写入。
- 写入文件后调用 `EditorInterface::get_singleton()->get_resource_filesystem()->update_file()`。
- 创建子目录：`DirAccess::open("res://")` → `make_dir_recursive()`。
- `resolve_node()` 在 `cmd_utils.hpp` 中——接受 `""`、`"."`、`"/"`、`"/root"`、根节点名称或 `"RootName/Child"`。
- `variant_to_json()` / `json_to_variant()`——Godot Variant ↔ JSON 递归转换。
- `undoable_set()`——"立即应用 + 注册撤销" 惯用模式。

## 文档站点

- Rspress 站点在 `docs/`，支持中/英双语（`rspress.config.ts`）。
- 开发：`pnpm dev`，构建：`pnpm build`，输出到 `doc_build/`。

## 客户端配置

```json
{
  "mcpServers": {
    "godot": { "type": "streamable-http", "url": "http://localhost:9600/mcp" }
  }
}
```

OpenCode 本地配置在 `.opencode/opencode.json`（已预设指向 `127.0.0.1:9600`）。
