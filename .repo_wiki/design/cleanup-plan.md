# 优化与清理计划

> 2026-06-02 全项目代码审查后制定的优化方案，按独立可验证的阶段组织。每个阶段可独立提交、编译验证。

## 审查范围

| 类别 | 文件数 | 说明 |
|------|:------:|------|
| C++ 源码 | 131 | `extensions/src/` 全部 `.hpp`/`.cpp` |
| Python 测试 | 10 | `tests/` 目录 |
| CI/构建 | 6 | `.github/workflows/` + `CMakeLists.txt` + `build.py` |
| 工具链 | 2 | `tools/codegen.py` + `tools/tool_schemas.json` |

---

## 阶段 1：死代码删除（纯删除，零风险）

### 1.1 删除整个文件

| 文件 | 原因 |
|------|------|
| `extensions/src/sdk/mcp_tool_adapter.hpp` | 零引用的死代码。原设计为 SDK→ITool 适配器，实际实现走 `CommandFn` 后备表路径 |
| `tests/test_phases/__init__.py` | 空壳。18 个 `phase_XX_*.py` 模块已删除 |
| `tests/mcp_client.py` | 仅被已死的 Python MCP fallback 路径（`test_orchestrator.py:267-318`）使用 |
| `tests/test_context.py` | 仅被已死的 Python MCP fallback 路径使用。`TestContext` 11 个字段中 8 个无人写入 |
| `tools/tool_schemas.json` | 全部 124 工具已迁移到 ITool 的 `input_schema()`。`load_schemas_from_json()` 跳过所有已注册工具 |

### 1.2 `handler_registry.hpp` + `.cpp` 删除项

| 项 | 原因 |
|------|------|
| `register_tool(const String &name, CommandFn fn)` | 零调用者。SDK 用 `register_custom_tool()` |
| `const CommandFn *find(const String &name)` | 零外部调用者。只查 CommandFn 表，语义错误 |
| `bool has(const String &name)` | 零外部调用者。只查 CommandFn 表，与 `execute()` 先查 ITool 的行为矛盾 |
| `int size()` | 零外部调用者。只计 CommandFn 表，不含 ITool |
| `void load_schemas_from_json(const String &json_text)` | 配套已删除的 `tool_schemas.json`。ITool 的 `input_schema()` 已完全替代 |

### 1.3 `editor_plugin.cpp` 删除项

| 项 | 行号 | 原因 |
|------|------|------|
| `load_tool_schemas()` 方法及调用 | 44-57 | 配套已删除的 `tool_schemas.json` |
| `kCandidatePaths` 向量 | 44 | 仅用于已删除的 `load_tool_schemas()` |
| 重复的 `register_itools(registry_)` 调用 | 66 | `register_all_tools()` 内部已调用，二次调用冗余 |

### 1.4 `tool_base.hpp` + `.cpp` 删除项

| 项 | 原因 |
|------|------|
| `ToolResult::is_ok(const Dictionary &r)` | 定义但零调用 |
| `ToolResult::is_err(const Dictionary &r)` | 定义但零调用 |

### 1.5 `cmd_utils.hpp` 删除项

| 项 | 原因 |
|------|------|
| `make_error()` | 零调用。已被 `ToolResult::err()` 替代 |
| `make_success()` | 零调用。已被 `ToolResult::ok()` 替代 |
| 头注释中 `is_ok() — short-circuit error returns` | 过时。`is_ok()` 不在此文件中 |

### 1.6 `mcp_tool_registry.hpp` + `.cpp` 删除项

| 项 | 原因 |
|------|------|
| `CustomTool::handler` 字段 | Mode A 不写入，Mode B 写入后从不读取。实际执行通过 `HandlerRegistry` 中的 `CommandFn` |

### 1.7 调试残留清理

| 文件 | 项 | 说明 |
|------|------|------|
| `test_http_handler.hpp` | 全部 `dbg`/`_h_*` 字典 | 12+ 调试字段无条件写入 HTTP 响应。简化为只保留核心逻辑 |
| `test_engine.cpp` | `_dbg_parsed_keys` / `_dbg_has_tests` | 调试输出泄漏到返回结果 |
| `test_engine.cpp:278` | `log_info("test_engine", "DEBUG tests count: ...")` | 调试日志残留 |

### 1.8 Python 测试文件清理

| 文件 | 删除内容 | 行号 |
|------|----------|------|
| `test_orchestrator.py` | `get_phases()` 函数 | 41-83 |
| `test_orchestrator.py` | 整个 `else` fallback 分支 | 267-318 |
| `test_orchestrator.py` | `backup_example()` 函数 | 135-149 |
| `test_orchestrator.py` | `from mcp_client import ...` | 23 |
| `test_orchestrator.py` | `from test_context import TestContext` | 24 |
| `test_orchestrator.py` | `[DEBUG]` print | 212-216 |
| `test_phases/base.py` | `PhaseRunner` 类 | 49-81 |
| `test_phases/base.py` | `ToolTest` 类 | 40-46 |
| `test_phases/base.py` | `disk_verified` / `disk_detail` 字段 | 12-13 |
| `report.py` | `add_phase()` 中 `end_time` 覆写 | 20 |
| `report.py` | `cleanup_status` 字段 | 13, 81 |
| `report.py` | `disk_verified` / `disk_detail` 显示 | 143 |
| `godot_manager.py` | `import signal` | 3 |

### 1.9 Wiki 文档同步

已同步删除以下过时引用：

- `overview/architecture.md`：删除 `mcp_tool_adapter.hpp` 行
- `extensions/gdext.md`：删除 `mcp_tool_adapter.hpp` 行，更新注册说明
- `modules/command-routing.md`：更新 HandlerRegistry API 描述
- `testing/overview.md`：删除旧 Python 阶段框架引用
- `design/unified-architecture-plan.md`：标注 `mcp_tool_adapter.hpp` 为未采用
- `design/decisions.md`：ADR-010 状态更新为"已接受"
- `index.md`：删除旧测试文档链接

---

## 阶段 2：Bug 修复

### 2.1 严重 Bug

| # | 文件 | 行号 | 问题 | 修复 |
|---|------|------|------|------|
| S1 | `mcp_tool_definition.cpp` | 40 | `has_method("execute")` 因 ClassDB 绑定始终为 true，基类实例会递归栈溢出 | 改为 `get_script_instance()` 检查 |
| S2 | `test_assertions.hpp` | 191 | `expect["error"]` 变量名错误，断言失败时错误信息为空 | 改为 `expect["error_contains"]` |
| S3 | `mcp_handler.cpp` | 386/390 | `pending_requests_` erase key 不匹配（`JSON::stringify(id)` 插入但 `Variant id` erase），导致已完成的请求永远不被清除 | 统一用 `JSON::stringify(id)` |

### 2.2 功能 Bug

| # | 文件 | 行号 | 问题 | 修复 |
|---|------|------|------|------|
| B1 | `mcp_tool_registry.cpp` | 247 | Mode B `is_meta` 参数无 `DEFVAL`，GDScript 调用必须传 7 个参数 | 添加 `DEFVAL(false)` |
| B2 | `test_engine.cpp` | 189-197 | `execute_chain` 完全忽略返回值，`before_all` 失败后测试级联失败 | 检查返回值，`before_all` 失败时中断后续测试 |
| B3 | `csharp_build.hpp` | 30-34 | 不使用 `ToolResult` 信封，错误返回裸字符串 | 改用 `ToolResult::ok/err` |
| B4 | `undo_tool.hpp` / `redo_tool.hpp` | 29/30 | "无操作"返回裸 dict（`success: false`），不经过 `ToolResult` | 改用 `ToolResult::err` |
| B5 | `get_node_collision_layer.hpp` | 25 | 检查 `success` 而非 `error`，与姊妹文件不一致 | 改为 `if (e.has("error"))` |
| B6 | `set_variable.hpp` | 30 | 使用裸 `set()` 无 undo 支持，而 `set_property` 有 undo | 改用 `undoable_set()` |

---

## 阶段 3：结构简化

| # | 改动 | 说明 |
|---|------|------|
| 3.1 | `editor_plugin.cpp` 直接调用 `register_itools(registry_)` | 删除 `register_all_tools()` 透传函数及其声明。`handler_registry.cpp:494-496` 和 `.hpp:90` 同步删除 |
| 3.2 | `tool_base.hpp` `registered_name()` 内联为 `name()` | 非虚方法，仅透传 `name()`，唯一调用点在 `handler_registry.cpp:61` |
| 3.3 | `CMakeLists.txt:65` 删除 "Legacy" 标签 | `cmd_utils.hpp/.cpp` 是活跃代码，"Legacy utilities" 标签误导 |
| 3.4 | `cmd_utils_json.cpp:277-287` 更新 `json_stringify_safe()` 注释 | 函数名和注释声称做非 ASCII 转义，实际只是 `JSON::stringify()` 透传。函数有活跃调用者需保留，但注释需更新 |
| 3.5 | `mcp_handler.hpp:56` 注释修正 | `Warning=3` → `Error=3`（RFC 5424 severity 3 是 Error） |
| 3.6 | `editor_plugin.cpp:94-99` fallback 路径加 `log_warn` | `has_method("add_control_to_bottom_panel")` 运行时检查的 else 分支应记录警告 |

---

## 阶段 4：DRY 提取

### 4.1 `cmd_utils.hpp` 新增 `walk_project_dir()`

**统一 4 处重复的目录遍历：**

| 当前位置 | 函数名 | 过滤 |
|----------|--------|------|
| `script_gd/list_gdscripts.hpp:12-23` | `list_gd_rec()` | `*.gd` |
| `script_cs/script_cs_helpers.hpp:25-36` | `list_cs_rec()` | `*.cs` |
| `project_settings/list_scenes.hpp:23-38` | `list_scenes_rec()` | `*.tscn`, `*.scn` |
| `search/search_helpers.hpp:40-57` | `walk_dir()` | 参数化扩展名 |

提取为参数化的 `walk_project_dir(root, extensions, skip_dirs) -> PackedStringArray`。

### 4.2 `cmd_utils.hpp` 新增 `collect_nodes_by()`

**统一 4 处重复的树遍历：**

| 当前位置 | 函数名 | 匹配条件 |
|----------|--------|----------|
| `find/find_nodes_by_name.hpp:10-14` | `collect_by_name()` | `n->get_name().contains(p)` |
| `find/find_nodes_by_type.hpp:10-14` | `collect_by_type()` | `n->is_class(cls)` |
| `find/find_nodes_by_group.hpp:10-14` | `collect_by_group()` | `n->is_in_group(group)` |
| `find/find_nodes_by_script.hpp:11-16` | `collect_by_script()` | script path check |

提取为 `collect_nodes_by(root, predicate, max_results) -> Array`，接受 lambda 谓词。

### 4.3 `testing/type_utils.hpp` 新增

- `normalize_type_hint(raw: String) -> String`：统一 `test_assertions.hpp` 和 `godot_file_verifier.hpp` 中的重复逻辑
- `constexpr double kDefaultTolerance = 0.0001`：消除 6 处魔法数字

---

## 验证策略

| 阶段 | 验证方式 |
|------|----------|
| 阶段 1 | `uv run python build.py` 编译通过 |
| 阶段 2 | 编译 + `uv run python tests/test_orchestrator.py` 测试通过 |
| 阶段 3 | 编译 + 测试通过 |
| 阶段 4 | 编译 + 测试通过 + 确认 124 工具数不变 |

每个阶段独立提交，提交信息遵循 Conventional Commits 规范。
