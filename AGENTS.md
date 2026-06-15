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
- **老旧缓存**：`build.py` 自动检测 MSB4019/VCTargetsPath/编译器路径变更 → 自动 `--clean` 重试；SSL 错误自动 `CMAKE_TLS_VERIFY=0` 降级重试
- **优化**：sccache/ccache 自动检测；Unity jumbo build 默认开（batch_size=CPU 核数，最小 2）；lld-link 自动检测

## 测试

```bash
# 完整测试套件（自动启动/关闭 Godot）
pytest tests/test_orchestrator.py -v --asyncio-mode=auto

# 单个 YAML 测试（需要 Godot 运行中）
curl -X POST http://localhost:9600/run-tests \
  -H "Content-Type: application/x-yaml" \
  --data-binary @tests/yaml_tests/<name>.yaml
```

- **前置配置**：复制 `tests/.env.example` → `tests/.env`，设置 `GODOT_PATH` 指向 Godot 4.6 可执行文件

## 关键约束

- **版本号**只存在于根 `CMakeLists.txt:22`（`PROJECT_VERSION`）。`plugin.cfg` 与 `.gdextension` 由 CMake 自动生成。
- **`compatibility_minimum`** 从 `GODOTCPP_API_VERSION` 自动推导（`CMakeLists.txt:71`），二者始终同步，无需手动维护。
- **入口符号** `gdext_mcp_init`（`extensions/src/register_types.cpp:60`）。
- **Pinned deps**：`godot-cpp 10.0.0-rc1`、`ryml v0.7.0`（header-only）。升级前必须测试。
- 源码根 `extensions/src/`（不是仓库根 `src/`）。`register_types.cpp` 用 `LEVEL_SCENE`（游戏进程注册 `GameBridgeNode`）+ `LEVEL_EDITOR`（编辑器进程注册 `McpEditorPlugin`）。

## 架构速览

```
extensions/src/
  built_in/tools/         # 所有 ITool 工具实现（.hpp only，按 category 分目录）
  built_in/tools/register/ # X-macro 注册文件（register_meta/existing/fallback/docs.hpp）
  built_in/register_itools.cpp  # #include 汇总 + GODOT_MCP_TOOL 宏定义（L230）
  server/ipc/             # HTTP 服务器 + MCP 协议处理
  server/mcp/             # MCP JSON-RPC 2.0 处理器
  runtime/                # 运行时桥接（bridge.cpp + game_bridge.cpp）
  sdk/                    # GDScript SDK 类（McpToolDefinition/Registry）
  ui/                     # 编辑器 UI（dock, console, logger）
  testing/                # C++ 测试引擎
tests/                    # Python 编排器 + YAML 用例
```

- **双重注册**：`GDREGISTER` 注册 SDK 类 + EditorPlugin；`HandlerRegistry` 管理 `ITool` 主表 + SDK `CommandFn` 旁路表
- **端口**：HTTP 9600（env `GODOT_MCP_HTTP_PORT`），桥接 9601（`GODOT_MCP_BRIDGE_PORT`）
- **端点**：`/mcp`（JSON-RPC 2.0 + SSE），`/run-tests`（YAML 测试引擎）
- **轮询**：`McpEditorPlugin::_process()` 驱动 `HttpServer::poll()` + `RuntimeBridge::poll()`

## C++ 注意事项

- **Unity build 内 `#include` 不能放在 `namespace{}` 里**：`#include` 展开会污染外层命名空间，导致 VS 2026 内部头文件中的 `std::` 被解析为 `godot_mcp::std::` → 编译崩溃。所有 `#include` 必须在文件顶部（命名空间外）。
- **lld-link**：通过 MSVC `/lldlink` 标志启用。**不要同时设 `set(CMAKE_LINKER ...)`** — 这会绕过 `/lldlink` 导致链接失败。
- **MSVC UTF-8**：根 `CMakeLists.txt` 已加 `/utf-8 /bigobj`。**不要用 `String::utf8("中文")`**。
- **编辑器内部类**不在 godot-cpp 绑定中，用 `find_children("*", "ClassName")` + `call()` 动态调用。
- **`ITool::execute()` 类型验证**：`tool_base.cpp` 允许 `Variant::FLOAT` 通过 integer 类型检查。
- **底部面板**：`add_control_to_bottom_panel` 在 godot-cpp 10.0.0-rc1 未绑定，用 `call()` 兜底。

## 添加内置工具

创建 `.hpp` 文件（继承 `ITool`）→ 在对应 `register/*.hpp` 加一行 `GODOT_MCP_TOOL(MyTool, false)` → 在 `register_itools.cpp` 加 `#include`。无需 codegen。

```cpp
GODOT_MCP_TOOL(cls, is_destructive_val)  // 宏签名（register_itools.cpp:230）
```

`name()`、`category()`、`brief()`、`input_schema()` 等通过虚方法提供。

## 已知易错模式

- **`EditorProgress` → `Main::iteration()` → 重入**：`ei->save_scene()` 默认走预览路径 → 内部 `EditorProgress` 触发 `Main::iteration()` 递归 `_process()` → `http_server_.poll()` 重入。保存时用 `ei->save_scene_as(path, false)` 跳过预览。
- **`HashMap` range-for 内 `erase()`**：迭代器失效 → UB → 崩溃。需推入 `dead` 列表，循环外统一清理。
- **`save_scene` 后不要访问 `ctx.root`**：编辑器可能释放/重建根节点。
- **`is_file()` 在文件删除后恒 false**：需在删除前捕获状态。
- **`std::sort` 不能用于 `godot::Vector::Iterator`**（非 random-access）。用 `std::vector<T>` 替代。
- **`set_node_property` 的 value 参数 schema 不应声明 `"type": "object"`** — 否则 string 值会被类型检查拒绝。
- **`add_animation_node` 的 name 参数**：schema 中为 `node_name`，但客户端可能传 `name`。`execute_impl` 中需 `args_string(ctx.args, "name")` 后备。

## 文档

- `.repo_wiki/index.md`（通过 `opencode.json` 自动加载为 agent instructions）
- `docs/`：Rspress 站点（中/英双语），`pnpm run dev` 本地预览
