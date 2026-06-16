# 任务详细规格 — 每个节点的 WHY/WHAT/WHERE/DONE/DON'T

---

## Batch 0 任务规格

### B1: 移除 RELEASE option，统一 CMAKE_BUILD_TYPE

**WHY**: 自定义 `RELEASE` option 与 CMake 内建 `CMAKE_BUILD_TYPE` 功能重叠，导致 PGO 条件不一致。用户传 `-DCMAKE_BUILD_TYPE=Release` 时 PGO 不生效。

**WHAT**:
1. 删除根 `CMakeLists.txt:7` 的 `option(RELEASE ...)`
2. 将 `ext/CMakeLists.txt:191,195` 的 `MSVC AND RELEASE` 改为 `MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release"`
3. 更新 `build.py:221` 传 `-DCMAKE_BUILD_TYPE=Release` 而非 `-DRELEASE=ON`

**WHERE**:
- `CMakeLists.txt` L7
- `extensions/CMakeLists.txt` L191, L195
- `build.py` L221

**DONE**: `cmake -DCMAKE_BUILD_TYPE=Release ...` 后 PGO 条件正确生效

**DON'T**: 不要修改其他 CMake 选项

---

### B2: add_compile_options → target_compile_options

**WHY**: `add_compile_options(/wd4244 /wd4267)` 是目录级命令，污染 godot-cpp 和 ryml 的编译。应限定到 `godot_mcp_gdext` target。

**WHAT**:
1. 删除 `ext/CMakeLists.txt:207` 的 `add_compile_options(/wd4244 /wd4267)`
2. 添加 `target_compile_options(godot_mcp_gdext PRIVATE /wd4244 /wd4267)` 到 MSVC 条件块

**WHERE**: `extensions/CMakeLists.txt` L207 → 移到 L177 附近（MSVC target_compile_options 段）

**DONE**: godot-cpp 和 ryml 编译时不再被抑制警告

**DON'T**: 不要删除警告抑制本身（Unity build 仍需要）

---

### B3: CMAKE_POSITION_INDEPENDENT_CODE 限定到 ryml

**WHY**: 全局 `CMAKE_POSITION_INDEPENDENT_CODE ON` 影响 godot-cpp（它可能已自行处理 PIC）。应只设给需要的 target。

**WHAT**:
1. 删除 `ext/CMakeLists.txt:44` 的 `set(CMAKE_POSITION_INDEPENDENT_CODE ON)`
2. 在 `FetchContent_MakeAvailable(rapidyaml)` 之后添加 `set_target_properties(ryml::ryml PROPERTIES POSITION_INDEPENDENT_CODE ON)`

**WHERE**: `extensions/CMakeLists.txt` L44 → 移到 rapidyaml FetchContent 之后

**DONE**: ryml 仍编译为 PIC，godot-cpp 不受影响

**DON'T**: 不要修改 godot-cpp 的编译属性

---

### B4: rapidyaml GIT → URL+SHA256

**WHY**: GIT 拉取无 hash 校验（供应链风险），速度慢，依赖 git 可执行文件。与 godot-cpp 的 URL+SHA256 方式不一致。

**WHAT**:
1. 将 `FetchContent_Declare(rapidyaml GIT_REPOSITORY ...)` 改为 `URL` + `URL_HASH`
2. URL 使用 GitHub release tarball: `https://github.com/biojppm/rapidyaml/releases/download/v0.7.0/rapidyaml-0.7.0.tar.gz`
3. 计算 SHA256 并填入 `URL_HASH`
4. 处理 c4core 子模块问题（可能需要 `FETCHCONTENT_SUBDIRECTORIES` 或预打包）

**WHERE**: `extensions/CMakeLists.txt` L34-41

**DONE**: `cmake --build` 成功拉取 rapidyaml，无 GIT 依赖

**DON'T**: 不要升级 rapidyaml 版本（保持 v0.7.0）

**Context**: 注释说 "Must use GIT so the ext/c4core submodule is fetched"。需要验证 tarball 是否包含 c4core，或改用 `GIT_REPOSITORY` + `GIT_SUBMODULES "ext/c4core"` 限定子模块范围。

---

### B5: 删除冗余 PREFIX 设置

**WHY**: CMake 对 SHARED library 的默认 PREFIX 在 Windows 上是 `""`，在 UNIX 上是 `"lib"`。当前代码设置的正是默认值。

**WHAT**:
1. 删除 `ext/CMakeLists.txt:157` 的 `PREFIX ""`
2. 删除 `ext/CMakeLists.txt:160-162` 的 `if(UNIX) set_target_properties(... PREFIX "lib")`

**WHERE**: `extensions/CMakeLists.txt` L157, L160-162

**DONE**: 编译产物名称不变（仍为 `godot_mcp_gdext.dll` / `libgodot_mcp_gdext.so`）

**DON'T**: 不要修改其他 target properties

---

### B6: file(WRITE) → configure_file

**WHY**: `file(WRITE ...)` 每次 configure 都重写文件，触发 Godot 文件监视器。`configure_file` 仅在内容变化时更新。

**WHAT**:
1. 创建 `plugin.cfg.in` 和 `godot_mcp_gdext.gdextension.in` 模板文件
2. 将 `CMakeLists.txt:68-92` 的 `file(WRITE ...)` 替换为 `configure_file(... @ONLY)`
3. 模板中使用 `@PROJECT_VERSION@` 等 CMake 变量

**WHERE**: `CMakeLists.txt` L68-92, 新建模板文件

**DONE**: 重新 configure 不触发文件变更（如果版本/路径未变）

**DON'T**: 不要改变生成的 plugin.cfg 和 .gdextension 的内容格式

---

### B7: 清理死配置

**WHY**: `_CRT_SECURE_NO_WARNINGS` 源码中无 CRT 调用；`GODOT_MCP_PLUGIN_VERSION` 条件永远为 false；Unity batch size 文档错误。

**WHAT**:
1. 删除 `ext/CMakeLists.txt:119-121` 的 `_CRT_SECURE_NO_WARNINGS`
2. 删除 `ext/CMakeLists.txt:110-115` 的 `GODOT_MCP_PLUGIN_VERSION` 条件（直接用 `${PROJECT_VERSION}`）
3. 修正 `ext/CMakeLists.txt:127-128` 的 Unity batch size 文档

**WHERE**: `extensions/CMakeLists.txt` L110-121, L127-128

**DONE**: CMake configure 无警告，文档准确

**DON'T**: 不要删除其他配置选项

---

### B9: build.py 修复

**WHY**: VS 发现逻辑重复；bat 临时文件并发冲突；-j 参数验证应报错而非静默降级。

**WHAT**:
1. 统一 VS 发现逻辑（`_find_msvc_cl` 使用 `vswhere`）
2. bat 临时文件加入 PID: `_vs_env_{os.getpid()}.bat`
3. `-j < 1` 改为报错退出

**WHERE**: `build.py` L40-58, L84-100, L125-126, L264-272

**DONE**: `python build.py -j 0` 报错退出；并发构建不冲突

**DON'T**: 不要改变构建产物的输出路径

---

### X1: McpConsole TreeItem 未删除

**WHY**: `logged_entries_` pop_front 后对应的 TreeItem 未从 Tree 中删除，导致索引错位，详情面板显示错误日志。

**WHAT**:
1. 在 `mcp_console.cpp:320` 获取 `oldest` 后，添加 `root->get_tree()->call_deferred("free_item", oldest)` 或直接使用 Tree 的 API 删除
2. 注意：AGENTS.md 记录 "Tree 析构时自动释放子 TreeItem"，但这里需要手动删除单个 item

**WHERE**: `extensions/src/ui/mcp_console.cpp` L314-326

**DONE**: 日志超过 kMaxVisible 时，旧条目正确从 Tree 移除，点击条目显示正确详情

**DON'T**: 不要使用 `memdelete(TreeItem*)` — 会导致 double-free

**Context**: Godot 的 Tree 控件管理子 TreeItem 的生命周期。删除 TreeItem 应使用 `tree->clear()` 或让 Tree 自行管理。可能需要用 `oldest->free_children()` + 从 parent 移除的方式。需要查阅 godot-cpp 10.0.0-rc1 的 Tree API。

---

### X2: read_response() 阻塞主线程

**WHY**: `OS::delay_msec(5)` 在 while 循环中最多阻塞 2000ms，冻结编辑器 UI。

**WHAT**:
1. 将 `read_response()` 改为非阻塞模式：检查数据是否可用，不可用则立即返回 "pending" 状态
2. 在 `RuntimeBridge::poll()` 中分帧处理：每帧检查一次数据，累积到超时后返回结果
3. 引入状态机：IDLE → WAITING → READY → TIMEOUT

**WHERE**: `extensions/src/runtime/bridge.cpp` L190-210, `extensions/src/runtime/bridge.hpp`

**DONE**: 运行时桥接请求不冻结编辑器 UI

**DON'T**: 不要引入多线程（违反架构约束）

**Context**: 这是一个较大的重构。`read_response()` 的调用方需要同步修改为异步模式。可能需要引入 callback 或 future 模式。如果改动范围过大，可以先将 timeout 减小到 100ms 作为临时缓解。

---

### X3: UTF-8 TCP 分片死锁

**WHY**: 多字节 UTF-8 字符被 TCP 分片时，`parse_utf8` 失败后 `read_buf_` 未清理，后续调用持续失败。

**WHAT**:
1. 在 `parse_utf8` 失败时，保留不完整的字节（可能是分片的后半部分未到）
2. 记录失败位置，下次 `read_clients()` 时尝试将新数据拼接到旧数据后重新解析
3. 设置最大重试次数，超过后丢弃数据并记录警告

**WHERE**: `extensions/src/runtime/game_bridge.cpp` L181-191

**DONE**: TCP 分片场景下 UTF-8 文本正确解析

**DON'T**: 不要改变 TCP 读取逻辑的其他部分

---

### X4: Auth token 长度泄漏

**WHY**: 常量时间比较之前先做长度检查，泄漏 token 长度信息。

**WHAT**:
1. 删除 `http_server.cpp:254` 的长度检查
2. 将 `auth` 补齐到与 `auth_token_` 相同长度后再做常量时间比较
3. 或直接使用固定长度的比较（比较 `max(auth.length(), token.length())` 个字节）

**WHERE**: `extensions/src/server/ipc/http_server.cpp` L250-265

**DONE**: 不同长度的 token 响应时间无显著差异

**DON'T**: 不要降低认证性能

---

### I1: MemdeleteGuard\<T\> RAII 模板

**WHY**: 多处工具手动 `memdelete` 回滚链（如 `create_mesh_instance_3d.hpp` 6 处），容易遗漏导致内存泄漏。

**WHAT**:
1. 创建 `cmd_utils/memdelete_guard.hpp`
2. 实现模板：
```cpp
template <typename T>
struct MemdeleteGuard {
    T *ptr = nullptr;
    explicit MemdeleteGuard(T *p) : ptr(p) {}
    ~MemdeleteGuard() { if (ptr) memdelete(ptr); }
    void dismiss() { ptr = nullptr; }
    T *get() const { return ptr; }
    T *operator->() const { return ptr; }
    MemdeleteGuard(const MemdeleteGuard &) = delete;
    MemdeleteGuard &operator=(const MemdeleteGuard &) = delete;
};
```

**WHERE**: `extensions/src/built_in/cmd_utils/memdelete_guard.hpp` (新建)

**DONE**: 编译通过，模板可被工具文件 include

**DON'T**: 不要用于 Godot Wrapped 子类（仅用于 memnew 分配的纯 C++ 对象或 Node）

---

### I2: SchemaBuilder 辅助函数

**WHY**: 171 个工具各写 15-25 行 schema 构建样板代码，总计 ~3000 行重复。

**WHAT**:
1. 创建 `cmd_utils/schema_builder.hpp`
2. 实现流式 API：
```cpp
// 用法示例：
Dictionary build_input_schema() const override {
    return SchemaBuilder()
        .prop("node_path", "string", "Node path in scene tree")
        .prop("count", "integer", "Number of items", 1)  // default value
        .prop("enabled", "boolean", "Enable feature", false)
        .required({"node_path"})
        .build();
}
```
3. 支持类型：string, integer, number, boolean, array, object
4. 支持 enum、default value、description

**WHERE**: `extensions/src/built_in/cmd_utils/schema_builder.hpp` (新建)

**DONE**: 编译通过，SchemaBuilder().prop(...).required(...).build() 返回正确的 Dictionary

**DON'T**: 不要引入外部依赖

---

### I3: 空 schema 默认实现

**WHY**: 无参数工具的 `build_input_schema()` 有 3 种不同写法，每个都重复 6 行。

**WHAT**:
1. 在 `ITool` 基类 (`tool_base.hpp`) 中将 `build_input_schema()` 改为虚方法并提供默认实现
2. 默认实现返回 `{"type": "object", "properties": {}}`
3. 无参数工具可删除自己的 `build_input_schema()` override

**WHERE**: `extensions/src/built_in/tool_base.hpp`

**DONE**: 无参数工具不 override `build_input_schema()` 时返回正确空 schema

**DON'T**: 不要改变有参数工具的 schema 行为

---

### I4: 统一错误码常量

**WHY**: `"MISSING_ARG"` / `"MISSING_PARAM"` / `"BAD_PARAM"` 混用，客户端难以统一处理。

**WHAT**:
1. 创建 `cmd_utils/error_codes.hpp`
2. 定义常量：
```cpp
namespace error_codes {
    constexpr const char *MISSING_REQUIRED_PARAM = "MISSING_REQUIRED_PARAM";
    constexpr const char *INVALID_PARAM_TYPE = "INVALID_PARAM_TYPE";
    constexpr const char *NODE_NOT_FOUND = "NODE_NOT_FOUND";
    constexpr const char *SCENE_NOT_OPEN = "SCENE_NOT_OPEN";
    constexpr const char *INTERNAL_ERROR = "INTERNAL_ERROR";
    // ...
}
```
3. 后续批次逐步替换各工具中的硬编码字符串

**WHERE**: `extensions/src/built_in/cmd_utils/error_codes.hpp` (新建)

**DONE**: 编译通过，常量可被工具文件 include

**DON'T**: 不要在此批次替换所有工具中的错误码（后续批次做）

---

### T4: 删除冗余 debugger 快捷工具

**WHY**: `debugger_break/continue/step_over/step_into/step_out` 5 个工具是 `debugger_control` 的冗余快捷方式，逻辑完全一致。

**WHAT**:
1. 删除 5 个 .hpp 文件
2. 从 `register/register_meta.hpp`（或对应 register 文件）中删除 5 行 `GODOT_MCP_TOOL(DebuggerBreakTool, ...)`
3. 从 `register_itools.cpp` 中删除 5 行 `#include`
4. 确认 `debugger_control` 工具已覆盖所有功能

**WHERE**:
- `tools/editor_tools/workspace/debugger_break.hpp` (删除)
- `tools/editor_tools/workspace/debugger_continue.hpp` (删除)
- `tools/editor_tools/workspace/debugger_step_over.hpp` (删除)
- `tools/editor_tools/workspace/debugger_step_into.hpp` (删除)
- `tools/editor_tools/workspace/debugger_step_out.hpp` (删除)
- `register/*.hpp` (删除 5 行 X-macro)
- `register_itools.cpp` (删除 5 行 #include)

**DONE**: 编译通过，工具总数减少 5，`debugger_control` 仍可执行所有 5 个操作

**DON'T**: 不要删除 `debugger_control.hpp`

---

### T5: 删除冗余 workspace 快捷工具

**WHY**: `set_workspace_2d/3d/script/assetlib` 4 个工具是 `set_workspace` 的冗余快捷方式。

**WHAT**:
1. 删除 4 个 .hpp 文件
2. 从 register 文件删除 4 行 X-macro
3. 从 `register_itools.cpp` 删除 4 行 #include

**WHERE**:
- `tools/editor_tools/workspace/set_workspace_2d.hpp` (删除)
- `tools/editor_tools/workspace/set_workspace_3d.hpp` (删除)
- `tools/editor_tools/workspace/set_workspace_script.hpp` (删除)
- `tools/editor_tools/workspace/set_workspace_assetlib.hpp` (删除)
- `register/*.hpp`, `register_itools.cpp`

**DONE**: 编译通过，工具总数减少 4，`set_workspace` 仍可切换所有 4 个工作区

**DON'T**: 不要删除 `set_workspace.hpp`

---

### T7: 修正 is_destructive 标记

**WHY**: `CopyFileTool=true`（应为 false，复制不破坏原文件）；`WriteCsharpScriptTool=false` / `PatchCsharpScriptTool=false`（应为 true，与 GDScript 版本一致）。

**WHAT**:
1. `register_existing.hpp:82`: `GODOT_MCP_TOOL(CopyFileTool, true)` → `GODOT_MCP_TOOL(CopyFileTool, false)`
2. `register_existing.hpp:110`: `GODOT_MCP_TOOL(WriteCsharpScriptTool, false)` → `GODOT_MCP_TOOL(WriteCsharpScriptTool, true)`
3. `register_existing.hpp:111`: `GODOT_MCP_TOOL(PatchCsharpScriptTool, false)` → `GODOT_MCP_TOOL(PatchCsharpScriptTool, true)`

**WHERE**: `extensions/src/built_in/tools/register/register_existing.hpp` L82, L110, L111

**DONE**: 编译通过，`get_tools` 返回的 `is_destructive` 字段正确

**DON'T**: 不要修改其他工具的 is_destructive 标记

---

## Batch 1 任务规格

### B8: 提升警告级别

**WHY**: MSVC `/W3` 偏低；GCC/Clang 缺少 `-Wextra` 等重要警告。

**WHAT**:
1. MSVC: `/W3` → `/W4`
2. GCC/Clang: `-Wall -Wno-unused-parameter` → `-Wall -Wextra -Wno-unused-parameter`
3. 编译并修复新出现的警告（可能需要添加显式类型转换、消除未使用变量等）

**WHERE**: `extensions/CMakeLists.txt` L177, L179

**DONE**: 编译无新 warning

**DON'T**: 不要添加 `-Werror`（先收集警告，后续再考虑）

**Context**: 提升警告级别后可能出现大量 C4244/C4267 警告。如果数量过多，可以先添加 per-target 的 `/wd4244 /wd4267`（已在 B2 中设置），后续逐步修复。

---

### B10: register_itools.cpp SKIP_UNITY_BUILD

**WHY**: `register_itools.cpp` 包含 ~150 个头文件，Unity build 中与其他 .cpp 合并可能导致符号冲突。

**WHAT**:
1. 在 `ext/CMakeLists.txt` 中添加：
```cmake
set_source_files_properties(src/built_in/register_itools.cpp PROPERTIES SKIP_UNITY_BUILD ON)
```

**WHERE**: `extensions/CMakeLists.txt`（target 定义之后）

**DONE**: Unity build 编译通过，register_itools.cpp 独立编译

**DON'T**: 不要对其他文件设置 SKIP_UNITY_BUILD

---

### T6: 合并 args_get / args_get_typed

**WHY**: `args_get<T>` 和 `args_get_typed<T>` 功能几乎完全重叠，唯一区别是后者额外支持 Vector2/Vector3/Color。

**WHAT**:
1. 将 `args_get_typed.hpp` 的所有特化合并到 `cmd_utils.hpp`（或反过来）
2. 删除冗余版本
3. 更新所有 include 路径

**WHERE**:
- `extensions/src/built_in/cmd_utils.hpp` L244-267
- `extensions/src/built_in/cmd_utils/args_get_typed.hpp`

**DONE**: 编译通过，所有使用 `args_get` 和 `args_get_typed` 的工具正常工作

**DON'T**: 不要改变函数签名

---

### U3: 测试编排器进程泄漏修复

**WHY**: `test_orchestrator.py:208` 抛出异常时 `manager.stop()` 不执行，Godot 进程泄漏。

**WHAT**:
1. 在 `run_test_session()` 中使用 `try/finally` 确保 `manager.stop()` 始终执行
2. 或实现 `GodotManager` 的 `__aenter__`/`__aexit__`

**WHERE**: `tests/test_orchestrator.py` L122-213

**DONE**: 异常场景下 Godot 进程被正确终止

**DON'T**: 不要改变测试逻辑

---

### U4: httpx client 复用

**WHY**: `_check_mcp_ready()` 每 0.5s 创建/销毁 httpx client，60s 超时内最多 120 个实例。

**WHAT**:
1. 在 `_wait_for_mcp()` 中创建一个共享 `httpx.AsyncClient`
2. 传递给 `_check_mcp_ready()` 使用
3. 循环结束后关闭 client

**WHERE**: `tests/godot_manager.py` L78-103

**DONE**: 轮询期间只创建 1 个 httpx client

**DON'T**: 不要改变超时逻辑

---

## Batch 2-7 任务规格

> 以下任务规格采用精简格式。详细程度同上，但省略重复的模式说明。

### X5 + T1: 所有权修复 + MemdeleteGuard 应用

**WHY**: `add_child` 后 `memdelete` 违反所有权规则；手动回滚链容易遗漏。

**WHAT**:
1. (X5) 审查所有 `add_child` 后的 `memdelete` 路径，修复所有权违规
2. (T1) 将手动 `memdelete` 回滚链替换为 `MemdeleteGuard<T>`
3. 重点文件：`create_mesh_instance_3d.hpp`（6 处）、`create_collision_shape.hpp`、`create_audio_player.hpp` 等

**WHERE**: `tools/**/*.hpp` (~20 文件)

**DONE**: 编译通过，无手动 memdelete 回滚链

**DON'T**: 不要修改工具的逻辑行为

---

### A1: ToolInfo 去重

**WHY**: `ToolInfo` 完整复制 `ITool` 元数据，两套数据需同步但无保证机制。

**WHAT**:
1. 方案 A（推荐）：`ToolInfo` 只存 `ITool*` 指针，查询时通过指针访问 `ITool` 虚方法
2. 方案 B：删除 `ToolInfo`，直接在 `itool_table_` 上查询
3. 更新 `register_tool()`、`get_tools()`、`search_tools()`、`get_categories()` 等

**WHERE**: `server/registry/handler_registry.hpp` L22-37, `handler_registry.cpp` L48-60, L252-263, L396, L419

**DONE**: 编译通过，工具搜索/分类功能正常

**DON'T**: 不要改变 MCP 协议的响应格式

---

### A2: pending_requests_ 修复

**WHY**: `pending_requests_` 键值相同（应用 HashSet），且永远为空（同步添加/删除），restart 等待机制失效。

**WHAT**:
1. 评估 pending restart 功能是否仍需要
2. 如需要：改为异步模式（添加工具执行完成回调）
3. 如不需要：删除 `pending_requests_`、`has_pending_requests()`、`pending_request_count()` 及相关 restart 逻辑

**WHERE**: `server/mcp/mcp_handler.hpp` L91, `mcp_handler.cpp` L259,274, `editor_plugin.cpp` L85-88

**DONE**: 编译通过，无死代码

**DON'T**: 不要改变 MCP 请求处理逻辑

---

### A3: 场景树递归深度限制

**WHY**: `_build_scene_tree_node` 无深度限制，大场景生成巨大 JSON 响应。

**WHAT**:
1. 添加 `max_depth` 参数（默认 10）
2. 添加 `max_children_per_level` 参数（默认 100）
3. 超限时在响应中标注 `"truncated": true`

**WHERE**: `server/mcp/mcp_handler.cpp` L524-540

**DONE**: 1000+ 节点的场景树响应 < 1MB

**DON'T**: 不要改变正常场景树的响应格式

---

### A4 + M5: 死代码清理

**WHY**: `ToolExecutor::logger_` 从未使用；`finalize_registration()` 空壳；`ToolResult::ok_with_meta()` 等零调用。

**WHAT**:
1. (A4) 删除 `tool_executor.hpp:40` 的 `logger_` 成员和构造函数参数
2. (A4) 删除 `handler_registry.cpp:65-69` 的 `finalize_registration()` 空壳（或简化为 inline）
3. (M5) 删除 `tool_base.hpp:42-45` 的 `ok_with_meta()`、`ok_with_confirm()`、`err_with_recoverable()`

**WHERE**: `tool_executor.hpp/cpp`, `handler_registry.cpp`, `tool_base.hpp`

**DONE**: 编译通过，无引用这些死代码的地方

**DON'T**: 不要删除 `ToolResult::ok()` 和 `ToolResult::err()`

---

### A5 + A6: 参数清理 + 错误码统一

**WHY**: `auth_token` / `permission_policy` 参数从未有效使用；错误码类型混用（int vs string）。

**WHAT**:
1. (A5) 删除 `ToolExecutor::execute()` 的 `auth_token` 和 `permission_policy` 参数
2. (A5) 删除 `check_permission()` 方法
3. (A6) 统一 `RuntimeBridge::make_response()` 的错误码为字符串类型

**WHERE**: `tool_executor.hpp/cpp`, `bridge.cpp`

**DONE**: 编译通过，API 更简洁

**DON'T**: 不要改变成功响应的格式

---

### P1-P8: 性能优化

> 详见各节点的 "WHAT" 描述，此处仅列关键点。

| ID | 核心改动 | 关键约束 |
|----|----------|----------|
| P1 | 预构建搜索索引（name+brief+description → token map） | 索引在 `register_tool()` 时增量更新 |
| P2 | `send_response()` 用 `PackedByteArray` 或 `vformat()` | 不改变 HTTP 响应格式 |
| P3 | `format_params_for_log()` 预分配 String buffer | 不改变日志格式 |
| P4 | 所有已知大小的 `std::vector` 添加 `reserve()` | 纯增量改动 |
| P5 | McpLogger 保持持久化 `Ref<FileAccess>` | 轮转时重新打开 |
| P6 | 跟踪字节偏移替代 `read_text_.utf8().size()` | 不改变协议格式 |
| P7 | 59 个 monitor 用 `DispatchMap` 替代线性扫描 | 编译期构造 |
| P8 | `get_console_errors/warnings` 使用 `ConsoleCache` | 与 `get_console_output` 共享缓存 |

---

### T2: SchemaBuilder 大规模应用

**WHY**: 171 个工具的 schema 样板代码可缩减 70%+。

**WHAT**:
1. 逐目录替换手动 schema 构建为 `SchemaBuilder` 调用
2. 无参数工具删除 `build_input_schema()` override（使用 I3 的默认实现）
3. 验证每个工具的 schema 输出与替换前一致

**WHERE**: `tools/**/*.hpp` (171 文件)

**DONE**: 编译通过，`get_tools` 返回的 schema 与替换前完全一致

**DON'T**: 不要改变任何工具的 schema 语义（类型、required、description）

**Context**: 这是最大规模的改动（171 文件）。建议按目录分批替换，每批编译验证。

---

### T3: GDScript/C# 工具对合并

**WHY**: 10 个文件中 5 对工具逻辑完全相同，仅扩展名和语言标识不同。

**WHAT**:
1. 创建参数化模板 `ScriptToolBase<LANG>` 或工厂函数
2. 每对工具合并为 1 个文件，通过模板参数区分
3. 更新 register 和 include

**WHERE**: `tools/**/scripts/read_gd_script.hpp` + `read_csharp_script.hpp` → 合并，write/patch/validate/list 同理

**DONE**: 编译通过，GDScript 和 C# 工具功能不变，文件数从 10 减到 5

**DON'T**: 不要改变工具的 name() 和 category()

---

### M4: set_shader_uniform undo 支持

**WHY**: 直接修改 shader 参数但无 Undo 注册，与其他场景修改工具不一致。

**WHAT**:
1. 在 `set_shader_uniform.hpp:160` 之前，通过 `EditorUndoRedoManager` 注册 undo 步骤
2. 保存旧值，undo 时恢复

**WHERE**: `tools/editor_tools/shader/set_shader_uniform.hpp` L155-165

**DONE**: Ctrl+Z 可撤销 shader uniform 修改

**DON'T**: 不要改变成功响应的格式

---

### M6-M10: 工具合并/模板化

| ID | 合并目标 | 节省文件数 |
|----|----------|------------|
| M6 | 5 个 performance 子集工具 → 用 `get_performance_monitors` + filter 参数 | -5 |
| M7 | 2 个 console 查询工具 → 用 `get_console_output` + type filter | -2 |
| M8 | 3 个 debugger 状态查询工具 → 用 `get_debugger_state` + fields 参数 | -3 |
| M9 | 6 个 resource 工具提取 `ResourceToolBase` | 0（重构不减少） |
| M10 | 3 个 toggle 工具模板化 | 0（重构不减少） |

**DONE**: 编译通过，功能等价

---

### M11: Misc P3 清理

**WHY**: 零散小项收尾。

**WHAT**:
1. `debugger_step_out.hpp`: 修正 action 字段为 `"debug_next"` 而非 `"step_out"`，或在 brief 中说明
2. `get_info.hpp:74`: `error_count` 从硬编码 0 改为实际值或移除
3. 检查并修复 `#pragma` 顺序（如果 M3 有遗漏）
4. 其他 P3 小项

**WHERE**: 多文件（单行改动）

**DONE**: 编译通过

**DON'T**: 不要引入新功能
