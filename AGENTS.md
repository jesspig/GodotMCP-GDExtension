# GodotMCP

Godot 4.6+ 编辑器 MCP 服务器（C++ GDExtension，纯主线程无锁，Streamable HTTP :9600）。

## 构建

```bash
uv run python main.py build                   # Debug 构建 + 复制到 example/
uv run python main.py build --release         # Release
uv run python main.py build --zip             # 构建后额外打包 addons.zip
uv run python main.py build --clean-cache     # 清 build/ 缓存（保留 _deps/）
uv run python main.py build --clean-deps      # ⚠ 仅清 _deps/，严重风险见下方
uv run python main.py build --clean-all       # ⚠ 删整个 build/（含 _deps/），严重风险见下方
uv run python main.py build --clean           # 同 --clean-cache（向后兼容）
uv run python main.py build -j 16             # 并行 16 作业
uv run python main.py build --compiler clang-cl --generator ninja --cache sccache   # Windows CI 配方
uv run python main.py package                 # 打包 addons.zip（可用 --libs-dir 指定预编译库目录）
uv run python main.py test                    # 完整测试流水线（自动启停 Godot）
uv run python main.py test --file 03_*.yaml   # 仅指定测试文件
uv run python main.py test --headless         # 仅运行 headless 兼容测试
cmake --build build --target deep-clean       # ⚠ 已守卫（需 env GODOTMCP_ALLOW_DEEP_CLEAN=yes）
```

> ⚠ **依赖清理禁令**：以下操作均会删除 `_deps/`（FetchContent 下载的依赖缓存）。
> 由于 GitHub 网络连通性问题（SSL 吊销检查、超时、连接重置），**重下载经常失败**。
> **Agent 绝对禁止擅自执行**以下任何操作：
>
> - `python main.py build --clean-deps`
> - `python main.py build --clean-all`
> - `cmake --build build --target deep-clean`（direct cmake bypass）
>
> 即使你认为有必要，也必须使用 **Question 工具**先向用户解释网络风险，待用户明确同意后方可执行。
>
> 代码防护：
>
> - `--clean-deps` / `--clean-all`（Python）：三重防护 — `isatty()` 检测（非交互终端自动拒绝，含 agent 提示 + 3 秒延缓 + 退出码 42）+ `input("yes/N")` 交互确认 + `EOFError`/`KeyboardInterrupt` 兜底。见 `_reject_agent()`。
> - `deep-clean`（cmake）：同样有 `isatty()` 风格守卫 — 未设 `GODOTMCP_ALLOW_DEEP_CLEAN=yes` 则 FATAL_ERROR 退出，message 明确阻止 agent 绕过。
> - agent 不可绕过以上任何防护。
>
> `--clean-cache`（或 `--clean`）无害，只管构建缓存不动 `_deps/`，无需用户确认。

- **始终 `uv run python`**（uv 自动激活 `.venv`，依赖锁在 `pyproject.toml` / `uv.lock`）
- **Python `>=3.14`**（`.python-version` 锁定）
- **DLL 文件锁**：`example/addons/godot_mcp/bin/godot_mcp_gdext.dll` 被 Godot 编辑器持有，重建失败先关编辑器
- **老旧缓存**：`main.py build` 自动检测编译器/路径变更 → 自动 `--clean` 重试；SSL 错误自动 `CMAKE_TLS_VERIFY=0` 降级

## 测试

```bash
uv run python main.py test                          # 完整流水线（自动启停 Godot）
curl -X POST http://localhost:9600/run-tests \       # 单 YAML（需 Godot 运行中）
  -H "Content-Type: application/x-yaml" \
  --data-binary @tests/yaml_tests/<name>.yaml
```

前置：复制 `tests/.env.example` → `tests/.env`，设 `GODOT_PATH` 指向 Godot 4.6。

测试框架：C++ `TestEngine`（`/run-tests` 端点）+ Python 编排器（`tests/test_orchestrator.py`）。YAML 测试文件位于 `tests/yaml_tests/`，输出报告到 `tests/output/`。

## 关键约束

- **版本号**仅根 `CMakeLists.txt` 中的 `PROJECT_VERSION`。`plugin.cfg` 和 `.gdextension` 由 `main.py build` 自动生成（`scripts/_addon.py:generate_addon_configs()`）。
- **`compatibility_minimum`** 从 `GODOTCPP_API_VERSION`（`extensions/CMakeLists.txt`）自动推导，二者始终同步。
- **入口符号** `gdext_mcp_init`（`extensions/src/register_types.cpp:60`）。
- **Pinned deps**：`godot-cpp 10.0.0-rc1`、`ryml v0.7.0`（均在 `extensions/CMakeLists.txt` via FetchContent）。升级前必测。
- **源码根 `extensions/src/`**。两生命周期入口：`LEVEL_SCENE`（游戏进程 `GameBridgeNode`）+ `LEVEL_EDITOR`（编辑器进程 `McpEditorPlugin`）。
- **端口配置**：HTTP `:9600`（`GODOT_MCP_HTTP_PORT`，ProjectSettings `godot_mcp/http_port`），桥接 `:9601`（`GODOT_MCP_BRIDGE_PORT`）。Host 通过 `GODOT_MCP_HTTP_HOST` 配置。
- **启动时序**：`McpEditorPlugin::_enter_tree()` → 注册工具 → 启动 HTTP Server。轮询在 `_process()` 中处理 HTTP + 运行时桥接。
- **`McpToolRegistry`** 注册为 Engine 单例，C# 通过 `Engine.GetSingleton("McpToolRegistry")` 访问。
- **`GDREGISTER_CLASS`** 注册 SDK 类 + EditorPlugin；`HandlerRegistry` 管理 `ITool` 主表 + SDK `CommandFn` 旁路表。
- **CI 工具链**：Windows `clang-cl+ninja+sccache`，Linux `gcc+ninja+sccache`，macOS `appleclang+ninja+sccache`。
- **文档站**：Rspress + Mermaid，`docs/` → `doc_build/`。`pnpm run build` 构建。
- **`.opencode/opencode.json`** 引用 `.repo_wiki/index.md` 作为 instruction，后者包含详细模块文档链接。

## 添加内置工具

创建 `.hpp`（继承 `ITool`）→ 在对应 `register/*.hpp` 加一行 `GODOT_MCP_TOOL(MyTool, false)` → 在 `register_itools.cpp` 加 `#include`。无需 codegen。

```cpp
GODOT_MCP_TOOL(cls, is_destructive_val)
```

`name()`、`category()`、`brief()`、`input_schema()` 等通过虚方法提供。新工具可用 `SchemaBuilder` + `cmd_utils/` 下的共享模板。

## 已知易错模式

### 编译与构建

- **`#include` 不能放在 `namespace{}` 里**（Unity build 下展开会污染 `std::` → 编译崩溃）。
- **`StreamPeerSocket` 不存在**于 godot-cpp 10.0.0-rc1。用 `StreamPeerTCP` 代替。
- **`Dictionary::ptr()` 不存在**于 godot-cpp 10.0.0-rc1。用 `has()` + `operator[]`。
- **lld-link**：通过 MSVC `/lldlink` 标志启用，**不要同时设 `set(CMAKE_LINKER ...)`**。
- **MSVC UTF-8**：`extensions/CMakeLists.txt` 已加 `/utf-8 /bigobj`。源码全英文，不要嵌入中文字符串字面量。
- **`std::sort` 不能用于 `godot::Vector::Iterator`**（非 random-access）。用 `std::vector<T>`。

### 正确性

- **`EditorProgress` → `Main::iteration()` → 重入**：`ei->save_scene()` 默认走预览路径触发递归 `_process()`。用 `ei->save_scene_as(path, false)` 跳过预览。
- **`HashMap` range-for 内 `erase()`**：迭代器失效。推入 `dead` 列表，循环外统一清理。
- **`save_scene` 后不要访问 `ctx.root`**：编辑器可能释放/重建根节点。
- **`is_file()` 在文件删除后恒 false**：需在删除前捕获状态。
- **`TreeItem` 双重释放**：`Tree` 析构时自动释放子节点；外部 `memdelete(TreeItem*)` 导致 double-free。
- **`int64_t`→`int` 截断**：Godot API `size()` 返回 `int64_t`，大量循环计数器用 `int`。用 `int64_t` 遍历，仅传参时 `static_cast<int>`。
- **`try_read_body` 逐字节 `push_back`**（`http_parser.cpp:149`）与主路径 `resize+copy_n` 不一致，大 body 下 O(n) 分配。统一用 `resize+copy_n`。

### 运行时桥接（game_bridge.cpp）

- **`StreamPeerSocket` 不存在** → 用 `StreamPeerTCP`。
- **消息用 `\n` 定界**，勿将整个 `read_text_` 当单 JSON 解析（否则流水线消息丢失、损坏消息永久挂起）。
- **`read_buf_` 解码后用 `read_offset_` 追踪，处理后清空 `read_buf_`**，保持剩余 `read_text_` 给下帧。
- **缓冲区设 `MAX_MESSAGE_SIZE`（64KB）上限**，超限清空防内存泄。

### 属性工具

- **`set_node_property` 的 value 参数 schema** 不应声明 `"type": "object"` — 否则 string 值会被拒绝。
- **`add_animation_node` 的 name 参数**：schema 中为 `node_name`，但客户端可能传 `name`。需 `args_string(ctx.args, "name")` 后备。
- **`ToolResult::err("CODE", "msg")` 的 error 是 Dictionary**（`{code, message}`），非 String。测试 `error_contains` 匹配的是 `"CODE: msg"` 格式，同时支持 code 和 msg 子串匹配。

## 项目知识库

- [项目 Wiki](.repo_wiki/index.md)

## 参考

- [工具目录](docs/en/reference/tools-catalog.md) — 所有工具参数和返回值
