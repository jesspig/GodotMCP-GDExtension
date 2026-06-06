# 设计决策

> ADR 风格的关键架构决策记录。

## ADR-001: 单进程架构（GDExtension only）

**状态**：已接受（取代原双进程方案）
**日期**：2026
**背景**：原设计将系统拆分为 Python MCP 服务器（stdio）+ C++ GDExtension（WebSocket IPC）双进程。但双进程引入了不必要的复杂性：启动顺序管理、IPC 协议同步、延迟增加、部署困难。
**决策**：移除 Python 服务器，仅保留 C++ GDExtension，通过 MCP Streamable HTTP（端口 9600）直接暴露给 AI 客户端。
**后果**：

- 正面：消除进程间通信延迟
- 正面：单一部署单元（仅 `.dll`/`.so`/`.dylib`）
- 正面：无启动顺序依赖
- 正面：无端口冲突风险
- 负面：需要 GDExtension 自行实现完整的 MCP 协议栈（HTTP 解析、JSON-RPC 2.0、SSE）

## ADR-002: MCP Streamable HTTP 传输

**状态**：已接受
**背景**：MCP 规范定义了 Streamable HTTP 传输方式，支持 AI 客户端通过 HTTP POST/GET 直连 MCP 服务器。
**决策**：在 gdext 中实现 `HttpServer`（HTTP + SSE）和 `McpHandler`（JSON-RPC 2.0 会话管理），监听 `:9600`。
**后果**：

- 正面：符合 MCP 规范的标准化传输
- 正面：支持 SSE 服务器推送事件
- 正面：AI 客户端配置简单（仅一个 URL）
- 负面：需要在 C++ 中实现 HTTP 解析和会话管理

## ADR-003: `process_frame` 而非 `EditorPlugin::_process()` 用于驱动

**状态**：已接受
**背景**：`EditorPlugin::_process()` 在场景播放时停止触发，不适合需要持续轮询的 HTTP 服务器。
**决策**：使用 `SceneTree::process_frame` 信号来驱动 `HttpServer::poll()`，而不是重写 `_process()`。
**后果**：

- 正面：`process_frame` 在场景播放时继续触发，确保实时工具正常工作
- 正面：`McpEditorPlugin::_process()` 被有意留空——清楚表明不在此处运行逻辑
- 负面：增加了一层间接

## ADR-004: 使用 CMake 构建

**状态**：已接受
**背景**：godot-cpp 通过 FetchContent 拉取。
**决策**：使用 CMake 作为顶级构建系统，通过 `add_subdirectory(extensions/gdext)` 构建 C++ GDExtension。
**后果**：

- 正面：统一的构建入口——`cmake --build`
- 正面：CMake 处理跨平台任务（进程终止、文件操作、CPack 打包）
- 正面：`build.py` 是便捷包装，但 CMake 可直接使用

## ADR-005: C# Solution 直接生成（不启动第二个 Godot 进程）

**状态**：已接受
**背景**：Godot 的 `--generate-mono-solution` 标志启动一个新的编辑器进程，可能造成冲突。
**决策**：直接在 gdext 中生成 `.sln` 和 `.csproj` 文件，无需启动编辑器进程。
**后果**：

- 正面：无进程冲突
- 正面：速度更快（无进程开销）
- 负面：需要维护与 Godot 4.6 .NET SDK 格式兼容的模板
- 负面：`.sln` 和 `.csproj` 生成的逻辑需要跟踪 Godot .NET SDK 版本的变化

## ADR-006: 放弃跨线程日志方案，采用直接 GDExtension API

**状态**：已接受
**背景**：C++ 版本所有代码在主线程运行，没有跨线程日志需求。
**决策**：直接调用 `UtilityFunctions::print/push_warning/push_error`，28 行实现覆盖所有日志需求。
**后果**：

- 正面：日志即时输出，无延迟
- 正面：28 行代码 vs Rust 版本的 137 行

## ADR-007: `call_method` 使用 `Object::call()`（不通过 Deferred/Callable）

**状态**：已接受
**背景**：需要一个通用的方式来调用节点上的任何方法。
**决策**：使用 `node.call(method, &args)` 直接调用。
**后果**：

- 正面：支持任意方法签名
- 正面：立即执行（gdext 端已经运行在主线程上）
- 负面：如果方法预期参数与实际传递的类型不匹配，可能在运行时失败

## ADR-008: C++ GDExtension 重写（取代 Rust 实现）

**状态**：已接受
**日期**：2025-2026
**背景**：Rust 的 `gdext` crate（v0.5）存在严重的线程复杂性问题——需要 `MainThreadDispatcher`、MPSC 日志通道、tokio 运行时等复杂基础设施。约 50% 的代码用于处理跨线程通信而不是实际逻辑。
**决策**：使用 C++ 和 `godot-cpp 10.0.0-rc1` 重写 GDExtension。利用 Godot C++ API 在主线程直接运行，消除所有跨线程基础设施。
**后果**：

- 正面：消除整个 dispatcher 层（~300 行 Rust 删除）
- 正面：日志直接调用 `UtilityFunctions::print`（28 行 vs 137 行）
- 正面：JSON↔Variant 转换使用 Godot 原生 `Dictionary`/`JSON`（无 serde 依赖）
- 正面：构建系统简化——无 Corrosion，无 tokio
- 正面：编译速度显著提升
- 正面：Rust 遗留代码（`crates/`）已被完全移除
- 负面：C++ 缺乏 Rust 的所有权和生命周期保证

## ADR-009: C# 工具注册（`register_script_cs`）

**状态**：已接受（原决策已反转）
**背景**：C# 脚本工具（`script_cs.cpp`，6 个工具）最初实现时未在 `register_all_tools()` 中调用，等待 C# 支持经过完整测试。
**决策**：后来已在 `register_all_tools()` 中启用 `register_script_cs` 调用，C# 工具现全部活跃可用。
**后果**：

- 正面：C# 工具与所有其他工具一样通过 MCP 直接暴露
- 正面：无需区分"已注册但不可用"的工具组
- 负面：`dotnet` CLI 不在 PATH 时，`csharp_build` 调用会失败（通过 MCP 错误返回处理）

## ADR-010: 统一工具架构（ITool 接口 + 组合式能力声明 + 注释驱动自动注册）

**状态**：已接受（P1-P6 全部完成，124 个 ITool 已迁移）
**日期**：2026-06
**背景**：当前 119 个工具以自由函数（`CommandFn`）+ `tool_schemas.json` 两阶段注册，存在返回结构不统一、样板代码重复、新增工具需改 3 处、内置与 SDK 工具分叉存储等问题。
**决策**：实施统一工具架构重构：

1. **ITool 接口**：所有工具（内置/SDK）实现同一接口，`execute()` 为统一入口
2. **ToolResult 信封**：`{"success": true, "data": {...}}` / `{"success": false, "error": {"code": "...", "message": "..."}}`
3. **组合式能力声明**：`needs_scene()` / `needs_node()` 替代继承链
4. **可见性标记**：`is_meta()` 布尔值决定可见性（始终可见 vs 渐进式披露），非原计划的 `source()` 三值枚举
5. **注释驱动自动注册**：`// @tool register` → codegen.py 生成注册代码
6. **双表调度**：内置工具存入 `itool_table_`（ITool），SDK 自定义工具存入 `table_`（CommandFn 后备表），`execute()` 先查 ITool 再查 CommandFn
**后果**：

- 正面：消除 `tool_schemas.json` 两阶段注册问题
- 正面：消除约 60 处 `get_root()`/`resolve_node()` 样板代码
- 正面：新增工具只需创建 `.hpp` 文件，零处手动注册
- 正面：`input_schema()` 自描述，编译期类型安全
- 正面：YAML 驱动测试利用统一信封简化断言
- 负面：`mcp_tool_adapter.hpp`（SDK → ITool 适配器）未采用，为零引用死代码待删除
- 负面：`tool_schemas.json` 过渡期遗留待清理

完整方案见 [统一工具架构重构计划](unified-architecture-plan.md)。

## ADR-012: 场景树工具分类与 Undo/Redo 策略

**状态**：已接受
**日期**：2026-06-04
**背景**：需要实现场景树操作工具（增删节点、剪贴板、重设父级、场景实例等），涉及两个关键设计选择：分类归属和撤销策略。
**决策**：

1. **分类使用 `editor_tools/scene_tree`**：不对 category 做运行时映射，保持这个路径为顶级分类。`attach_script`/`detach_script` 也归入此分类而非独立 `script` 分类，以便 AI 在同一上下文中发现脚本相关操作。
2. **100% 使用 `EditorUndoRedoManager`**：所有修改操作通过 `EditorInterface::get_singleton()->get_editor_undo_redo()` 获取 `EditorUndoRedoManager`，不直接调用裸 `UndoRedo`。`create_action` 的三个参数（name、MERGE_DISABLE、context_node）模式。
3. **剪贴板用 `PackedScene` 实现**：因 `SceneTreeDock::node_clipboard`（`Vector<Node *>`）为非公开 API，GDExtension 不可访问。改用 `PackedScene::pack(dup)` + `static Ref<PackedScene>` 静态存储 + `instantiate(GEN_EDIT_STATE_INSTANCE)`。
4. **`set_scene_root` 不实现**：需要 `EditorNode::set_edited_scene()`，GDExtension 无该 API。替代工作流：`new_scene` → `add_node` → `save_scene`。
5. **跳过批重命名/多选编辑/扩展折叠/锁**：批重命名和多选编辑 UI 依赖强、AI 可逐节点操作替代；扩展/折叠是纯 UI 状态；锁定（`_edit_lock_` meta）用户明确排除。
**后果**：

- 正面：统一撤销栈与 Godot 官方 Ctrl+Z 完全兼容
- 正面：剪贴板序列化格式与 `.tscn` 一致
- 正面：脚本工具和场景工具在 AI 视野中自然关联
- 负面：剪贴板无法与 Godot 编辑器内部剪贴板互通（PackedScene vs Node*）
- 负面：20 个工具需仔细管理 `add_do_reference`/`add_undo_reference` 所有权

完整方案见 [场景树工具模块文档](../modules/scene-tree-tools.md)。

## 已废弃的决策

以下 ADR 随 Python 服务器移除而废弃：

- **ADR-旧1: 双进程架构（Server + GDExtension）** → 被 ADR-001（单进程）取代
- **ADR-旧2: WebSocket IPC** → 已移除，不再需要进程间通信
- **ADR-旧3: 静态注册所有工具 Schema（Python 侧权威）** → 已被 ITool + codegen 编译时注册取代
