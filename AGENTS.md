# GodotMCP

Godot 4.6+ 编辑器 MCP 服务器，C++ GDExtension 单进程架构，纯主线程 `_process()` 驱动，无锁。

## 构建 & 测试

```bash
uv run python main.py build                   # Debug 构建 + 复制到 example/
uv run python main.py build --release         # Release
uv run python main.py build --clean-cache     # 清 build/ 缓存（保留 _deps/）
uv run python main.py build -j 16
uv run python main.py test                    # 完整流水线（自动启停 Godot）
uv run python main.py test --file 03_*.yaml   # 仅指定测试文件
```

**始终 `uv run python`**（uv 自动激活 `.venv`，Python >=3.14 锁定）。

## 依赖清理：Agent 不要自作主张

`build/_deps/` 是 FetchContent 下载的 godot-cpp 和 ryml 缓存，重下载因 GitHub 网络问题可能失败。

- 用户**明确要求**清理 → 请用户手动执行（`isatty()` + `input("yes/N")` 屏障，非交互终端自动拒绝）
- 用户**未提** → 不准碰
- `--clean-cache` / `--clean` 无害（只清构建缓存），直接执行

## 入口符号与生命周期

- 入口 `gdext_mcp_init`（`extensions/src/register_types.cpp:60`）
- 两级别：`LEVEL_SCENE` → `GameBridgeNode`（游戏进程 TCP 客户端），`LEVEL_EDITOR` → `McpEditorPlugin`（编辑器）
- `McpEditorPlugin::_enter_tree()` 注册 164 工具 → 启动 HttpServer(:9600)。`_process()` 每帧轮询 HTTP + 桥接

## 渐进式披露

`tools/list` 仅返回 9 个元工具（`is_meta()==true`）。其余 155 个通过发现链按需加载：`get_categories()` → `get_tools(category=...)` → `find_tool(query)` → `call_tool(name, args)`。

## 已知易错模式

### 编译与构建

- **`#include` 不能放 `namespace{}` 里**（Unity build 展开污染 `std::`）
- **`StreamPeerSocket` 不存在**。用 `StreamPeerTCP`。
- **`Dictionary::ptr()` 不存在**。用 `has()` + `operator[]`。
- **lld-link**：通过 `/lldlink` MSVC 标志启用，**不要同时设 `set(CMAKE_LINKER ...)`**。
- **`std::sort` 不能用于 `godot::Vector::Iterator`**（非 random-access）。用 `std::vector<T>`。
- **`int64_t`→`int` 截断**：Godot API `size()` 返回 `int64_t`，用 `int64_t` 遍历，传参 `static_cast<int>`。

### 正确性

- **`EditorProgress` → `Main::iteration()` 重入**：`save_scene()` 默认走预览路径。用 `ei->save_scene_as(path, false)`。
- **`HashMap` range-for 内 `erase()`**：迭代器失效。推入 `dead` 列表，循环外清理。
- **`save_scene` 后不要访问 `ctx.root`**：编辑器可能释放/重建根节点。
- **`TreeItem` 双重释放**：`Tree` 析构自动释放子节点；外部 `memdelete(TreeItem*)` 导致 double-free。
- **`is_file()` 在文件删除后恒 false**：在删除前捕获状态。
- **`try_read_body` 逐字节 `push_back`**：与主路径 `resize+copy_n` 不一致。统一用 `resize+copy_n`。

### 运行时桥接

- **消息用 `\n` 定界**，勿将整个 `read_buf_` 当单 JSON 解析。
- **缓冲区设 `BRIDGE_MAX_MESSAGE_SIZE`（64KB）上限**，超限清空。
- **`read_buf_` 解码后用 `read_offset_` 追踪**，处理后清空。

### 工具与属性

- **`set_node_property` 的 value 参数 schema** 不应声明 `"type": "object"`（否则 string 会被拒绝）。
- **`add_animation_node` 的 name 参数**：schema 中为 `node_name`，客户端可能传 `name`。需 `args_string(ctx.args, "name")` 后备。
- **`ToolResult::err("CODE", "msg")` 的 error 是 Dictionary**（`{code, message}`），非 String。

## 版本管理

- **单版本源**：根 `CMakeLists.txt:10` 的 `PROJECT_VERSION = "0.2.2"`
- `plugin.cfg` 和 `.gdextension` 由 `main.py build` 自动生成
- `pyproject.toml` 版本需手动同步

## 添加内置工具

创建 `.hpp`（继承 `ITool`）→ 在 `register/*.hpp` 加一行 `GODOT_MCP_TOOL(MyTool, is_destructive)` → 在 `register_itools.cpp` 加 `#include`。无需 codegen。

## 参考

- [项目知识库](.repo_wiki/index.md)（OpenCode 的 `instructions` 引用此文件，包含详细模块文档链接）
- [工具目录](docs/en/guide/tools-overview.md) — 所有工具参数和返回值
