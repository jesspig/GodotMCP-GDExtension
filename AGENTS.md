# GodotMCP

Godot 4.6+ 编辑器 MCP 服务器（C++ GDExtension，纯主线程无锁，Streamable HTTP :9600）。

## Build

```bash
uv run python build.py                  # Debug + addons 复制到 example/
uv run python build.py --release        # Release
uv run python build.py --no-zip         # 跳过 zip 快速迭代
uv run python build.py --clean          # 清 build/（保留 _deps/）
uv run python build.py --clean-all      # 删整个 build/（含 _deps/）
```

- **始终用 `uv run python`**（uv 自动激活 `.venv`，依赖锁在 `pyproject.toml`）
- **Python `>=3.14`**（`.python-version` 锁定）
- **DLL 文件锁**：`example/addons/godot_mcp/bin/godot_mcp_gdext.dll` 被 Godot 编辑器持有，重建失败先关编辑器
- **老旧缓存**：`build.py` 自动检测编译器/路径变更 → 自动 `--clean` 重试；SSL 错误自动 `CMAKE_TLS_VERIFY=0` 降级
- **优化**：sccache/ccache 自动检测；Unity jumbo build（batch_size=CPU 核数，上限 12）；lld-link 自动检测

## Test

```bash
pytest tests/test_orchestrator.py -v --asyncio-mode=auto   # 完整套件（自动启停 Godot）
curl -X POST http://localhost:9600/run-tests \              # 单 YAML（需 Godot 运行中）
  -H "Content-Type: application/x-yaml" \
  --data-binary @tests/yaml_tests/<name>.yaml
```

前置：复制 `tests/.env.example` → `tests/.env`，设 `GODOT_PATH` 指向 Godot 4.6。

## 关键约束

- **版本号**仅 `CMakeLists.txt:16` `PROJECT_VERSION`。`plugin.cfg` 和 `.gdextension` 由 CMake 自动生成。
- **`compatibility_minimum`** 从 `GODOTCPP_API_VERSION` 自动推导（`extensions/CMakeLists.txt:15`），二者始终同步。
- **入口符号** `gdext_mcp_init`（`extensions/src/register_types.cpp:60`）。
- **Pinned deps**：`godot-cpp 10.0.0-rc1`（`extensions/CMakeLists.txt:12`）、`ryml v0.7.0`。升级前必测。
- 源码根 `extensions/src/`（不是仓库根）。`register_types.cpp` 用 `LEVEL_SCENE`（游戏进程 `GameBridgeNode`）+ `LEVEL_EDITOR`（编辑器进程 `McpEditorPlugin`）。

## 架构速览

```
extensions/src/
  built_in/tools/         # 所有 ITool 工具实现（.hpp only，按 category 分目录）
  built_in/tools/register/ # X-macro 注册文件（register_meta/existing/fallback/docs.hpp）
  built_in/register_itools.cpp  # #include 汇总 + GODOT_MCP_TOOL 宏定义
  built_in/cmd_utils/     # dispatch_map.hpp / undo_helpers.hpp / args_get_typed.hpp
  server/ipc/             # HTTP 服务器（Streamable HTTP，仅 POST+OPTIONS）
  server/mcp/             # MCP JSON-RPC 2.0 处理器（无 session / 2026-07-28）
  runtime/                # 运行时桥接
  sdk/                    # GDScript SDK 类
  ui/                     # 编辑器 UI（dock, console, logger）
  testing/                # C++ 测试引擎
```

- **MCP 协议**：Streamable HTTP，支持 `2026-07-28` / `2025-11-25` / `2025-03-26`。**无 GET 端点，无 session**。
- **端口**：HTTP 9600（env `GODOT_MCP_HTTP_PORT`），桥接 9601（`GODOT_MCP_BRIDGE_PORT`）
- **端点**：`POST /mcp`（JSON-RPC 2.0），`/run-tests`（YAML 测试引擎）
- **轮询**：`McpEditorPlugin::_process()` 驱动 `HttpServer::poll()` + `RuntimeBridge::poll()`
- **双重注册**：`GDREGISTER` 注册 SDK 类 + EditorPlugin；`HandlerRegistry` 管理 `ITool` 主表 + SDK `CommandFn` 旁路表

## 添加内置工具

创建 `.hpp`（继承 `ITool`）→ 在对应 `register/*.hpp` 加一行 `GODOT_MCP_TOOL(MyTool, false)` → 在 `register_itools.cpp` 加 `#include`。无需 codegen。

```cpp
GODOT_MCP_TOOL(cls, is_destructive_val)  // 宏签名（register_itools.cpp:213）
```

`name()`、`category()`、`brief()`、`input_schema()` 等通过虚方法提供。新工具有 `cmd_utils/` 下的共享模板可用。

## C++ 注意事项

- **`#include` 不能放在 `namespace{}` 里**（Unity build 下展开会污染 `std::` → 编译崩溃）。所有 `#include` 必须在文件顶部、命名空间外。
- **lld-link**：通过 MSVC `/lldlink` 标志启用。**不要同时设 `set(CMAKE_LINKER ...)`**。
- **MSVC UTF-8**：`extensions/CMakeLists.txt:163` 已加 `/utf-8 /bigobj`。**不要用 `String::utf8("中文")`**。
- **`Dictionary::ptr()` 不存在**于 godot-cpp 10.0.0-rc1。用 `has()` + `operator[]` 代替。
- **编辑器内部类**不在 godot-cpp 绑定中，用 `find_children("*", "ClassName")` + `call()` 动态调用。
- **`add_control_to_bottom_panel`** 已在 godot-cpp 10.0.0-rc1 绑定（`editor_plugin.hpp:128`），直接调用。

## 已知易错模式

- **`EditorProgress` → `Main::iteration()` → 重入**：`ei->save_scene()` 默认走预览路径触发递归 `_process()`。用 `ei->save_scene_as(path, false)` 跳过预览。
- **`HashMap` range-for 内 `erase()`**：迭代器失效。推入 `dead` 列表，循环外统一清理。
- **`save_scene` 后不要访问 `ctx.root`**：编辑器可能释放/重建根节点。
- **`is_file()` 在文件删除后恒 false**：需在删除前捕获状态。
- **`std::sort` 不能用于 `godot::Vector::Iterator`**（非 random-access）。用 `std::vector<T>`。
- **`set_node_property` 的 value 参数 schema** 不应声明 `"type": "object"` — 否则 string 值会被拒绝。
- **`add_animation_node` 的 name 参数**：schema 中为 `node_name`，但客户端可能传 `name`。需 `args_string(ctx.args, "name")` 后备。
- **`TreeItem` 双重释放**：`Tree` 析构时自动释放子节点；外部 `memdelete(TreeItem*)` 导致 double-free。

## 帮助工具

`extensions/src/built_in/cmd_utils/` 包含模板化共享工具：

- `dispatch_map.hpp` — 编译期构造的 String→String 查表，替代 if/else 链。用法：`static const auto map = make_dispatch_map<...>(...); const auto *v = map.find(key);`
- `undo_helpers.hpp` — `commit_add_child_undo(ur, action, parent, child, root)` 统一 undo 模式。
- `args_get_typed.hpp` — 类型安全参数读取，支持 String/Dictionary/Array/Vector2/3/Color/bool/integral/floating_point。

## 项目知识库

- [项目 Wiki](.repo_wiki/index.md)
