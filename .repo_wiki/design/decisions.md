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

## ADR-003: `_process()` 重写用于驱动（原 `process_frame` 信号已替换）

**状态**：已替换（2026-06，`feature/runtime` 分支改为 `_process()` 重写）
**背景**：`EditorPlugin::_process()` 在场景播放时停止触发。原决策使用 `SceneTree::process_frame` 信号，但在引入 `RuntimeBridge`（需 `ei->is_playing_scene()` 检测游戏启停）后，`_process()` 重写可同时承载 HTTP 轮询 + 桥接连接管理。
**决策**：用 `_process(delta)` 重写替代 `process_frame` 信号。`_process()` 中三合一驱动：`http_server_.poll()` + `_try_bridge_connect()` + `runtime_bridge_.poll()`。游戏运行时 `_try_bridge_connect()` 通过 `ei->is_playing_scene()` 自动感知，不影响桥接功能。
**后果**：

- 正面：消除 `process_frame` 信号的一层间接
- 正面：`_process()` 自然承载 HTTP 轮询 + 桥接连接 + 桥接轮询三者
- 正面：`_exit_tree()` 中无需特殊清理（`_process()` 跟随 EditorPlugin 生命周期自动停止）
- 负面：`_process()` 在场景播放时不触发，但桥接检测不依赖此机制（`is_playing_scene()` 状态检查）

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

## ADR-011: 运行时桥接设计（GameBridgeNode TCP + RuntimeBridge 客户端）

**状态**：已接受（2026-06，`feature/runtime` 分支实现）
**背景**：需要让 AI 客户端能查询和控制运行中的游戏（获取场景树、读写属性、调用方法、截图、模拟输入等）。
**决策**：双组件架构——游戏进程内 `GameBridgeNode`（Node）作为 TCP 服务端（:9601），编辑器进程内 `RuntimeBridge` 作为 TCP 客户端。JSON-RPC 风格通信（非标准 JSON-RPC，自定义 `{cmd, params, id}` 格式）。编辑器通过 `ei->is_playing_scene()` 感知游戏启停。
**后果**：

- 正面：纯 GDExtension 实现，无外部依赖
- 正面：一个 DLL 双端复用，`Engine::is_editor_hint()` 区分行为
- 正面：TCP 同步通道简单可靠，无 WebSocket 握手开销
- 负面：`send_command()` 忙等待最长 5 秒，阻塞 `_process()` 帧循环
- 负面：TCP 无加密/认证，仅限 localhost 使用
- 负面：`EditorDebuggerPlugin` 方案更符合 Godot 架构，但不在 godot-cpp 绑定中，需 `find_children()` + `call()` 动态调用

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

## ADR-013: 移除预编译头（PCH），仅保留 Unity Build

**状态**：已接受
**日期**：2026-06-08
**背景**：启用 Unity Build（batch_size = CPU 核心数）后，源文件从 ~16 个 TU 合并为 ~2 个 unity batch，godot-cpp 头文件仅需解析 2 次。PCH 在此之上的边际收益仅为节省 1 次解析，却带来 ~100MB `.pch` 文件、编译器 flag 变更时全量重编、PCH 头文件修改触发全量重建等开销。同时，PCH 原本承担注入 `using namespace godot;` 的作用，现由 `codegen.py` 在 `generated_registration.cpp` 中直接生成该声明，PCH 已无必要。
**决策**：移除 PCH（`GODOTMCP_USE_PCH` 选项、`target_precompile_headers` 调用及相关 CMake 逻辑）。`pch.hpp` 保留作为参考，不再参与编译。
**后果**：

- 正面：构建系统简化，消除 PCH 管理复杂度
- 正面：消除 ~100MB `.pch` 文件磁盘占用
- 正面：无 PCH 失效导致的意外全量重编
- 正面：不改变增量构建性能——Unity 已提供主要优化，ccache 覆盖重复构建
- 负面：极端场景（非 unity 构建、无 ccache）下编译略慢于有 PCH 时
- 参考：`codegen.py` 生成的 `using namespace godot;` 确保工具头文件中非限定 `String`/`Dictionary` 等 godot 类型正确解析

## ADR-014: 功能优化路线图（P0/P1/P2 三阶段）

**状态**：全部已完成（2026-06-10 实施完毕）
**日期**：2026-06-08（原始），2026-06-10（P0 完成），2026-06-10（P1/P2 完成）
**背景**：市场分析显示，市面 20+ Godot MCP 项目中，本项目（~11,790 工具）在架构上（C++ GDExtension 进程内、纯主线程无锁、Codegen 自动注册）具有唯一不可替代优势，但功能覆盖存在显著缺口。竞品普遍具备的游戏运行时桥接、编辑器/游戏截图、脚本读写编辑等核心能力，本项目缺失。

**决策**：按 P0/P1/P2 三级优先级实施功能补全，每阶段完成后进入下一阶段。

### Phase 1 (P0) — 入场券 ✅（已完成）

**目标**：消除所有"竞品都有，缺失即硬伤"的功能缺口。

#### 1.1 游戏运行时桥接 ✅

**实现路径**：GDExtension TCP 双组件架构：

| 子能力 | 实现方式 | 状态 |
|--------|---------|------|
| 输入注入（键盘/鼠标/Action） | GameBridgeNode TCP :9601 + `simulate_input` 命令 | ✅ |
| 运行时属性读取 | `get_property` 命令 | ✅ |
| 运行时属性设置 | `set_property` 命令 + `json_to_variant` 转换 | ✅ |
| 运行时场景树获取 | `get_scene_tree` 命令 + `node_to_dict()` 递归 | ✅ |
| 运行时方法调用 | `call_method` 命令 + 参数转换 | ✅ |
| 游戏截图 | `screenshot` 命令（PNG/JPG，Base64） | ✅ |
| 暂停控制 | `set_pause` 命令 | ✅ |

实际实现使用 **纯 TCP :9601**（非 EditorDebuggerPlugin），仅限 localhost。

#### 1.2 编辑器/游戏截图 ✅

- 编辑器截图：`meta_screenshot` 工具（通过 EditorInterface API）
- 游戏截图：`capture_game_screenshot` 工具（通过运行时桥接）
- 返回格式：MCP ImageContent

#### 1.3 脚本读写编辑 ✅

| 工具 | 实现方式 | 状态 |
|------|---------|------|
| `read_gd_script` | `FileAccess::open()` 读取 `.gd` 文件 | ✅ |
| `write_gd_script` | `FileAccess::open()` 写入 | ✅ |
| `patch_gd_script` | 精确替换指定行/段 | ✅ |
| `validate_gd_script` | LSP 客户端验证 | ✅ |
| `attach_script` | `node.set_script(ResourceLoader::load(path))` | ✅ |
| `detach_script` | `node.set_script(Variant())` | ✅ |
| `list_gd_scripts` / `grep_scripts` / `glob_scripts` | 目录遍历 | ✅ |
| 对应 C# 工具（7 个） | 同上，`.cs` 扩展 | ✅ |

### Phase 2 (P1) — 竞争力 ✅（已完成）

**目标**：补齐显著提升竞争力的功能模块。

#### 2.1 动画系统工具

| 工具 | 实现方式 |
|------|---------|
| `create_animation_player` | 创建 AnimationPlayer 节点 + AnimationLibrary |
| `create_animation_clip` | 在 AnimationLibrary 中创建 Animation |
| `add_animation_track` | 添加 value/position/rotation/scale 轨道 |
| `set_keyframe` | 插入/修改/删除关键帧 |
| `get_animation_info` | 查询动画列表、轨道结构、关键帧数据 |

#### 2.2 UI/Control 工具

| 工具 | 实现方式 |
|------|---------|
| `create_control` | 创建 Control 子类节点（Button/Label/Panel 等） |
| `set_layout_preset` | 设置锚点预设（full_rect/center 等） |
| `set_theme_override` | 批量设置主题覆盖（颜色/字体/字号） |
| `create_stylebox` | 创建 StyleBoxFlat 并应用到节点 |

#### 2.3 TileMap 操作

| 工具 | 实现方式 |
|------|---------|
| `set_tilemap_cells` | 批量放置瓦片到指定网格坐标 |
| `erase_tilemap_cells` | 批量擦除指定坐标的瓦片 |
| `get_tilemap_info` | 查询 TileMapLayer 元信息 |

#### 2.4 碰撞形状一键创建

| 工具 | 实现方式 |
|------|---------|
| `create_collision_shape` | 一步创建 CollisionShape2D/3D + 配置形状（RectangleShape2D/CircleShape2D/CapsuleShape2D 等 9 种） |

### Phase 3 (P2) — 差异化 ✅（已完成）

**目标**：实现竞品没有或只有少数竞品有的差异化功能。

#### 3.1 断点调试集成

参考 Sods2 的实现，通过 `EditorDebuggerPlugin` 与调试器通信：

| 工具 | 实现方式 |
|------|---------|
| `set_breakpoint` | 通过 `EditorDebuggerNode` 设置断点 |
| `remove_breakpoint` | 移除断点 |
| `list_breakpoints` | 列出所有活跃断点 |
| `get_stack_trace` | 获取当前堆栈跟踪 |
| `get_locals` | 获取局部变量 |
| `step_over/into/out` | 单步执行 |
| `continue` | 继续执行 |

**注意**：`EditorDebuggerNode` 是编辑器内部类，需通过 `find_children()` + `call()` 动态调用。

#### 3.2 MCP Resources + Prompts ✅

| 资源 URI | 内容 |
|----------|------|
| `godot://scene/current` → `scene-tree` | 当前打开的场景信息 |
| `godot://project/info` → `project-settings` | 项目元数据 |
| `editor-info` | 编辑器版本与项目信息（新增） |
| `scene-node/{path}` | 场景节点详情（Resource Template，新增） |

| Prompt 模板 | 用途 |
|-------------|------|
| `create_scene` | 创建新场景（替代 setup_scene_structure） |
| `create_node` | 创建新节点 |
| `fix_error` | 修复编辑器错误（替代 debug_physics） |
| `explain_node` | 解释节点类型和用途 |
| `code_review` | 代码审查 |

#### 3.3 项目可视化器

参考 tomyud1 的方案：启动一个 HTTP 服务（如 `:6510`）提供浏览器端力导向图，展示脚本关系、场景图、依赖关系。

#### 3.4 Godot 文档查询

| 工具 | 实现方式 |
|------|---------|
| `get_class_info` | 调用 Godot 的 `ClassDB` API |
| `search_docs` | 使用 `--doctool` 生成的文档缓存 |
| `get_best_practices` | 预置最佳实践知识库 |

#### 3.5 项目脚手架

| 工具 | 实现方式 |
|------|---------|
| `create_project` | 调用 Godot CLI 创建新项目 |

#### 3.6 导出/插件/输入映射管理

| 工具 | 实现方式 |
|------|---------|
| `get_export_presets` / `run_export` | 读取/调用导出预设 |
| `list_plugins` / `set_plugin_enabled` | 管理编辑器插件 |
| `list_input_actions` / `add_input_action` | 管理输入映射 |

#### 3.7 Shader 工具

| 工具 | 实现方式 |
|------|---------|
| `create_shader` | 创建 `.gdshader` 文件 |
| `read_shader` | 读取 shader 源码 |
| `apply_shader_preset` | 应用预设效果（dissolve/outline/hologram 等，参考 alexmeckes 的 11 种预设） |

### 实施原则

1. **每阶段完成后才能进入下一阶段**：P0 未完成前不开始 P1，以此类推
2. **优先利用 GDExtension 优势**：做 Node.js 方案做不到的事（底层 API 调用、实时性能数据、编辑器 UI 自动化）
3. **保持 codegen 体系**：新工具应尽量利用 `// @tool register` + codegen 自动注册
4. **工具发现优化**：~11,790 工具中大部分是属性 get/set 噪音，考虑按使用频率分级或合并为通用工具
5. **每项功能完成后**：更新 `.repo_wiki/log.md` 并补充对应模块文档
6. **测试先行**：每项新工具必须附带 YAML 测试用例

**详细追踪清单见 [roadmap.md](roadmap.md)**。

## ADR-015: 下一代工具架构（搜索引擎 + 自动 Undo + SDK 平权 + 四层工具体系 + 注册体系重构）

**状态**：已实施（Phase 0 + Phase 2 核心完成，Phase 1/3 待后续）
**日期**：2026-06-11（修订），2026-06-11（Phase 2 核心实施）
**前置 ADR**：ADR-010（统一 ITool 接口）、ADR-012（Undo/Redo 策略）、ADR-014（P0/P1/P2 路线图）

**背景**：ADR-014 功能优化路线图完成后，项目在核心功能覆盖上已追平竞品。但在架构层面仍有以下待解决问题：

1. **工具发现困难**：~11,791 工具中无搜索能力，AI 只能通过分类逐层浏览或精准名称调用
2. **Undo/Redo 覆盖不全**：节点属性 set 和场景树工具有 undo，但资源属性/setting 工具无 undo
3. **SDK 工具降级**：GDScript/C# 自定义工具走 CommandFn 后备表，无法享受 ITool 的前置检查（`needs_scene`/`needs_node`）和自动 Undo
4. **工具描述未国际化**：中英混用，AI 客户端对非英文工具理解准确度下降
5. **输入输出结构半统一**：新旧两种返回格式兼容代码存在（`McpHandler::456`），Schema 靠手工校验
6. **无通用兜底工具**：引擎升级新增属性时，YAML 数据库未更新前无法操作
7. **MCP Resources/Prompts 空实现**：协议已声明支持但返回空数据
8. **注册方式脆弱**：`// @tool register` 注释扫描依赖 Python 文本解析，UTF-8 BOM 会导致漏扫，无编译期检查
9. **指令数据源重复**：YAML 数据库（5,575 项）与 Godot 内置 `DocTools` 文档系统功能重叠，需手动同步

**决策**：实施下一代工具架构重构，分为 12 项子决策。

---

### 决策 1：ToolSearchEngine — 内置搜索引擎

在 `HandlerRegistry` 中嵌入完整的搜索引擎服务：

- **索引构建**：`register_tool()` / `unregister_custom_tool()` 时自动维护倒排索引
- **索引维度**：工具名（小写）| brief | description | category | 别名（YAML `aliases` 字段）| 调用频次
- **搜索模式**：
  - `exact`：工具名/别名精确匹配（最高优先级）
  - `prefix`：工具名前缀匹配（高优先级）
  - `fuzzy`：Levenshtein 距离 ≤ max(2, len/3)（工具名）
  - `fulltext`：子串匹配 brief/description（兜底）
  - `auto`：自动选择最佳模式（默认）
- **排序算法**：匹配类型权重 × 调用频次权重，支持 `category` 过滤 + `limit`/`offset` 分页
- **集成 completion/complete**：对 `node`/`class`/`property`/`tool` 参数提供自动补全

**新元工具**：`find_tool`（`meta_tools` 分类）

```json
{
  "query": "string [required]",
  "mode": "string? [auto|exact|prefix|fuzzy|fulltext]",
  "category": "string?",
  "limit": "integer? [1-100, default 20]",
  "offset": "integer? [default 0]"
}
```

---

### 决策 2：ToolOutput 统一信封增强

在 ADR-010 的 `ToolResult` 基础上升级为 `ToolOutput`：

- **成功**：`{"success": true, "data": {...}, "meta": {"undoable": bool, "destructive": bool, "duration_ms": int}}`
- **失败**：`{"success": false, "error": {"code": "NODE_NOT_FOUND", "message": "...", "recoverable": bool, "details": {...}}}`
- **确认**：`{"success": false, "error": {"code": "CONFIRMATION_REQUIRED", "message": "...", "destructive": true, "confirm_token": "..."}}`（破坏性操作二次确认）
- `McpHandler::handle_tools_call()` 统一识别三种类型，`CONFIRMATION_REQUIRED` 返回特殊错误码

---

### 决策 3：自动 Input Schema 校验

`ITool::execute()` 模板方法在执行 `execute_impl()` 前自动校验入参：

- 检查 `input_schema()["required"]` 中所有字段是否存在
- 检测类型不匹配（string → int 自动转换，复杂类型提示）
- 失败返回 `ToolOutput::err("MISSING_REQUIRED", ..., recoverable=true)`
- AI 客户端通过 `recoverable=true` 标记可自动重试

---

### 决策 4：全英文化

所有内置工具的 `brief()` / `description()` / `category_description()` 统一使用英文。分类标签保持英文。

---

### 决策 5：自动 Undo 包装

在 `HandlerRegistry::execute()` 中对声明 `supports_undo() == true` + `needs_scene() == true` 的工具自动应用 Undo/Redo 包装：

- **ITool 新增能力声明**：
  - `supports_undo() → bool`：此操作可否撤销
  - `is_destructive() → bool`：此操作是否不可逆（如文件删除）
- **自动包装逻辑**：
  - 对已知属性修改（参数含 `node` + `property` + `value`），自动记录修改前值
  - 执行后自动创建 `EditorUndoRedoManager` action
  - 对批量操作（复合工具），整批注册为一个 action
- **声明矩阵**：

| 工具类型 | supports_undo | is_destructive |
|---------|:------------:|:-------------:|
| scene_tree CRUD | ✅ | ❌ |
| NodePropertySetTool | ✅ | ❌ |
| NodeResourceSetTool | ✅ | ❌ |
| SettingSetTool | ✅ | ❌ |
| 复合工具 | ✅ | ❌ |
| 文件创建 | ❌ | ❌ |
| 文件删除 | ❌ | ✅ |
| 运行时桥接 | ❌ | ❌ |

---

### 决策 6：SDK 工具平权（IToolAdapter）

移除当前 SDK 工具走 `CommandFn` 后备表的降级路径，改用 `IToolAdapter` 包装后注册到 `itool_table_` 主表：

```cpp
class IToolAdapter : public ITool {
    // 通过构造函数参数指定能力声明（needs_scene/needs_node/supports_undo）
    // execute_impl() 委托给原始 CommandFn / Callable
};
```

**效果**：
- SDK 工具自动享受 `needs_scene`/`needs_node` 前置检查
- SDK 工具自动享受 Undo 包装
- SDK 工具自动享受 Schema 校验
- SDK 工具与内置工具的**唯一差异**：名称自动加 `custom_` 前缀
- SDK 工具可通过 `McpToolDefinition` 扩展属性声明（如 `set_needs_scene(true)`）

---

### 决策 7：四层工具体系（修订）

工具分类为四层，互补共存，**100% 全覆盖**：

| 层 | 定位 | 示例 | 数量 |
|----|------|------|:----:|
| **Layer 0: 通用兜底** | 按属性名读写任意属性，保证全覆盖 | `get_node_property`, `set_node_property` | 4 |
| **Layer 1: 语义专用** | 最常用操作的快捷方式，类型安全 schema | `set_node_position_2d`, `set_control_text` | ~80 |
| **Layer 2: 属性组** | 按 Godot `ADD_GROUP` 边界批量操作 | `configure_visibility`, `configure_layout` | ~126 |
| **Layer 3: 文档** | 查询 Godot 内置文档，替代 YAML 指令数据 | `get_class_doc`, `get_property_doc` | 7 |

**Layer 0 — 通用兜底工具**（`node_tools/fallback` + `editor_tools/settings/fallback`）：

| 工具 | 参数 | 功能 | Undo |
|------|------|------|:----:|
| `get_node_property` | `node_path`, `property` | 按名称读任意节点属性 | ❌ |
| `set_node_property` | `node_path`, `property`, `value` | 按名称写任意节点属性（带类型验证） | ✅ |
| `get_setting` | `path` | 读任意项目设置 | ❌ |
| `set_setting` | `path`, `value` | 写任意项目设置 | ✅ |

**Layer 1 — 语义专用工具**（按操作语义组织，跨类型兼容）：

| 领域 | 工具示例 | 数量 |
|------|---------|:----:|
| Transform | `get/set_node_position_2d/3d`, `get/set_node_rotation_2d/3d`, `get/set_node_scale_2d/3d` | 12 |
| Visibility | `get/set_node_visible`, `get/set_node_modulate`, `get/set_node_self_modulate` | 6 |
| Control | `get/set_control_text`, `get/set_control_anchor_preset`, `get/set_control_size` | 10 |
| Physics | `get/set_collision_layer`, `get/set_collision_mask`, `get/set_rigidbody_mass` | 6 |
| Audio | `get/set_audio_stream`, `get/set_audio_volume`, `get/set_audio_playing` | 6 |
| Animation | `get/set_animation_current`, `get/set_animation_speed`, `get/set_animation_autoplay` | 6 |
| Visual | `get/set_sprite_texture`, `get/set_material_override`, `get/set_shader_parameter` | 6 |
| 3D | `get/set_3d_mesh`, `get/set_3d_light_color`, `get/set_3d_light_energy` | 6 |
| Script | `attach_script`, `detach_script`, `get/set_script_variable` | 4 |
| Camera | `get/set_camera_current`, `get/set_camera_zoom` | 4 |
| TileMap | `get/set_tilemap_cell`, `get/set_tilemap_layer_enabled` | 4 |
| Other | `get/set_node_z_index`, `get/set_node_process_mode`, `get/set_environment_background` | 10 |
| **小计** | | **~80** |

**Layer 2 — 属性组工具**（按 Godot 源码 `ADD_GROUP` 边界组织）：

| 基类 | 组工具 | 覆盖子类数 |
|------|--------|:---------:|
| Node | `configure_process`, `configure_physics_interpolation`, `configure_auto_translate` | ~200+ |
| CanvasItem | `configure_visibility`, `configure_ordering`, `configure_texture`, `configure_material` | ~150+ |
| Node2D | `configure_transform_2d` | ~80+ |
| Node3D | `configure_transform_3d`, `configure_visibility_3d` | ~50+ |
| Control | `configure_layout`, `configure_control_transform`, `configure_container_sizing`, `configure_mouse`, `configure_focus`, `configure_theme`, `configure_input`, `configure_accessibility`, `configure_tooltip` | ~60+ |
| Sprite2D | `configure_sprite_offset`, `configure_sprite_animation`, `configure_sprite_region` | 1 |
| Camera2D | `configure_camera_limits`, `configure_camera_smoothing`, `configure_camera_drag` | 1 |
| GPUParticles2D | `configure_particles_time`, `configure_particles_collision`, `configure_particles_drawing`, `configure_particles_trails` | 1 |
| CPUParticles2D | `configure_cpu_particles_time`, `configure_cpu_particles_drawing`, `configure_cpu_particles_emission`, `configure_cpu_particles_direction`, `configure_cpu_particles_gravity`, `configure_cpu_particles_velocity`, `configure_cpu_particles_damping`, `configure_cpu_particles_angle`, `configure_cpu_particles_scale`, `configure_cpu_particles_color` | 1 |
| Light2D | `configure_light_range`, `configure_light_shadow` | 2 |
| Polygon2D | `configure_polygon_texture`, `configure_polygon_skeleton`, `configure_polygon_invert`, `configure_polygon_data` | 1 |
| Line2D | `configure_line_fill`, `configure_line_capping`, `configure_line_border` | 1 |
| Parallax2D | `configure_parallax_repeat`, `configure_parallax_limit`, `configure_parallax_override` | 1 |
| TileMapLayer | `configure_tilemap_rendering`, `configure_tilemap_physics`, `configure_tilemap_navigation` | 1 |
| CollisionObject | `configure_collision`, `configure_collision_input` | ~10 |
| Area | `configure_area_gravity`, `configure_area_damping`, `configure_area_audio` | 2 |
| RigidBody | `configure_rigidbody_mass`, `configure_rigidbody_deactivation`, `configure_rigidbody_solver`, `configure_rigidbody_linear`, `configure_rigidbody_angular`, `configure_rigidbody_forces` | 2 |
| CharacterBody | `configure_characterbody_floor`, `configure_characterbody_platform`, `configure_characterbody_collision` | 2 |
| RayCast | `configure_raycast_collide_with`, `configure_raycast_debug` | 2 |
| Joint | `configure_joint_angular_limit`, `configure_joint_motor` | ~8 |
| Light3D | `configure_light3d_light`, `configure_light3d_shadow`, `configure_light3d_distance_fade`, `configure_light3d_directional_shadow`, `configure_light3d_omni`, `configure_light3d_spot` | 3 |
| VisualInstance3D | `configure_visual_sorting`, `configure_visual_geometry`, `configure_visual_gi`, `configure_visual_visibility_range` | ~15 |
| Camera3D | `configure_camera3d_projection`, `configure_camera3d_attributes` | 1 |
| AudioStreamPlayer3D | `configure_audio3d_emission`, `configure_audio3d_attenuation`, `configure_audio3d_doppler` | 1 |
| Decal | `configure_decal_textures`, `configure_decal_parameters`, `configure_decal_fade`, `configure_decal_cull_mask` | 1 |
| GPUParticles3D | `configure_particles3d_time`, `configure_particles3d_collision`, `configure_particles3d_drawing`, `configure_particles3d_trails`, `configure_particles3d_process_material`, `configure_particles3d_draw_passes` | 1 |
| CPUParticles3D | `configure_cpu_particles3d_*`（同 2D 版 10 个组） | 1 |
| 资源 | `configure_material`, `configure_texture`, `configure_stylebox`, `configure_animation`, `configure_curve`, `configure_gradient`, `configure_environment`, `configure_navigation_mesh`, `configure_particle_material`, `configure_tileset`, `configure_theme`, `configure_shader` | ~15 |
| **小计** | | **~126** |

**Layer 3 — 文档工具**（数据源：Godot 内置 `DocTools`）：

| 工具 | 参数 | 返回 | 数据来源 |
|------|------|------|---------|
| `get_class_doc` | `class_name` | 类的完整文档（描述、继承链、属性、方法、信号、常量、枚举、教程） | `EditorHelp::get_doc_data().class_list[]` |
| `get_property_doc` | `class_name`, `property` | 属性元数据（类型、描述、默认值、枚举值、范围、setter/getter） | `ClassDoc.properties[]` |
| `get_method_doc` | `class_name`, `method` | 方法文档（签名、参数描述、返回值、错误码） | `ClassDoc.methods[]` |
| `get_enum_doc` | `class_name`, `enum_name` | 枚举所有值及描述 | `ClassDoc.enums[]` + `constants[]` |
| `search_docs` | `query`, `categories` | 增强搜索（类/方法/属性/信号，含描述文本） | `ei->call("search_help")` + `ClassDoc` |
| `get_class_list` | `filter` | 按继承链或分类列出类 | `ClassDB.get_class_list()` |
| `get_inheritance_chain` | `class_name` | 类的完整继承链 | `ClassDB.get_parent_class()` |

---

### 决策 8：分类系统 YAML 驱动（取代硬编码）

当前 `top_level_meta()` 硬编码 4 个顶级分类，改为通过首个注册工具的 `category_meta()` 自动发现：

```cpp
class ITool {
    // 新增：分类元数据，只在该分类的第一个工具注册时生效
    virtual Dictionary category_meta() const { return {}; }
    // 返回: {"label": "...", "description": "..."}
};
```

系统自动根据工具的 `category()` 构建分类树，无需手动维护顶级分类列表。现有的 4 个顶级分类由对应分类下第一个注册的工具携带 `category_meta()` 维持。

---

### 决策 9：MCP Resources + Prompts 补齐

**Resources**：
- `godot://scene/current` → 当前场景结构快照
- `godot://project/config` → 项目配置摘要

**Prompts**：
- `create_player_controller` → 创建玩家控制器引导
- `debug_performance` → 性能调试引导
- `setup_collision_shapes` → 碰撞体设置引导

---

### 决策 10：`completion/complete` 增强

利用 `ToolSearchEngine` 提供上下文感知的自动补全：

| 参数名 | 补全来源 |
|--------|---------|
| `tool` / `tool_name` | ToolSearchEngine |
| `category` | 当前已注册分类路径 |
| `node` / `node_path` | 当前场景节点路径列表 |
| `class` / `class_name` | Godot ClassDB |
| `property` | 节点类型的可用属性 |
| `path` / `res_path` | 文件系统 `res://` 路径 |

---

### 决策 11：X-macro 分文件注册（取代 `// @tool register` + codegen）

**背景**：当前注册方式依赖 Python `codegen.py` 扫描 `.hpp` 文件中的 `// @tool register` 注释字符串，存在 UTF-8 BOM 漏扫、无编译期检查、无 IDE 支持（跳转定义/重构）等问题。

**决策**：改用 X-macro 分文件注册模式：

```cpp
// extensions/src/built_in/tools/register/register_transform.hpp
GODOT_MCP_TOOL(SetNodePosition2D,   "set_node_position_2d",   "node_tools/transform", true,  true)
GODOT_MCP_TOOL(GetNodePosition2D,   "get_node_position_2d",   "node_tools/transform", true,  false)
```

```cpp
// extensions/src/built_in/register_itools.cpp
#define GODOT_MCP_TOOL(cls, name, cat, need_scene, need_node) \
    { auto tool = std::make_unique<cls>(); reg.register_tool(std::move(tool)); }

void register_itools(HandlerRegistry &reg) {
    #include "built_in/tools/register/register_meta.hpp"
    #include "built_in/tools/register/register_transform.hpp"
    // ... 每个分类一个注册文件
}
```

**文件结构**：

```
extensions/src/built_in/
├── register_itools.cpp              ← 新增：#include 所有 X-macro 注册文件
├── tools/
│   ├── register/                    ← 新增：X-macro 注册文件目录
│   │   ├── register_meta.hpp
│   │   ├── register_transform.hpp
│   │   ├── register_visibility.hpp
│   │   ├── register_control.hpp
│   │   ├── register_physics.hpp
│   │   ├── register_audio.hpp
│   │   ├── register_animation.hpp
│   │   ├── register_3d.hpp
│   │   ├── register_script.hpp
│   │   ├── register_camera.hpp
│   │   ├── register_tilemap.hpp
│   │   ├── register_groups.hpp       ← Layer 2 属性组工具
│   │   ├── register_fallback.hpp     ← Layer 0 通用兜底
│   │   ├── register_docs.hpp         ← Layer 3 文档工具
│   │   └── register_existing.hpp     ← 保留的 141 个现有工具
│   ├── node_properties/             ← 新增：Layer 1 语义工具
│   │   ├── transform_tools.hpp
│   │   ├── visibility_tools.hpp
│   │   ├── control_tools.hpp
│   │   ├── physics_tools.hpp
│   │   ├── audio_tools.hpp
│   │   ├── animation_tools.hpp
│   │   ├── script_tools.hpp
│   │   ├── camera_tools.hpp
│   │   ├── tilemap_tools.hpp
│   │   ├── threed_tools.hpp
│   │   ├── fallback_tools.hpp
│   │   └── doc_tools.hpp
│   ├── group_tools/                 ← 新增：Layer 2 属性组工具
│   │   ├── configure_visibility.hpp
│   │   ├── configure_transform.hpp
│   │   ├── configure_layout.hpp
│   │   └── ...
│   ├── node_props/db/               ← 保留，仅文档用途
│   ├── node_resource/db/            ← 保留，仅文档用途
│   ├── meta/                        ← 保留现有
│   ├── editor_tools/                ← 保留现有
│   └── runtime_tools/               ← 保留现有
```

**变更清单**：
- 删除 `tools/codegen.py`（445 行）
- 删除 `extensions/CMakeLists.txt` 中 codegen 的 `add_custom_command`
- 删除 `build/generated/generated_registration.cpp`（不再生成）
- 删除所有 `.hpp` 文件中的 `// @tool register` 注释
- 新增 `register_itools.cpp` + ~15 个 X-macro 注册文件
- 现有 141 个工具类保留，只需在 `register_existing.hpp` 加 X-macro 行

---

### 决策 12：指令数据源迁移（YAML → Godot 内置文档）

**背景**：当前 YAML 数据库（`node_props/db/*.yaml` + `node_resource/db/*.yaml` + `settings/db/*.yaml`）存储了 5,575 个属性/设置的元数据，通过 codegen 生成 11,650 个 get/set 工具。Godot 引擎内置了完整的文档系统（`EditorHelp::get_doc_data()` → `DocTools` → `class_list: HashMap<String, ClassDoc>`），包含更丰富的信息（描述文本、枚举值、教程链接、废弃标记等），且永远与引擎版本同步。

**决策**：

| 用途 | 旧方案 | 新方案 |
|------|--------|--------|
| 属性元数据 | YAML 数据库 | `EditorHelp::get_doc_data()` |
| 类型信息 | YAML `type_name` | `PropertyDoc.type` + `hint` + `hint_string` |
| 枚举值 | 无 | `PropertyDoc.enumeration` + `EnumDoc` |
| 描述文本 | 无 | `PropertyDoc.description` |
| 教程链接 | 无 | `ClassDoc.tutorials[]` |
| 工具注册 | YAML → codegen → 11,650 个工具 | Layer 0 通用兜底 + Layer 3 文档查询 |

**YAML 数据库的去留**：
- 保留 `db/` 目录作为文档参考（不参与构建）
- **不再**从 YAML 生成工具注册代码
- **不再**参与 CMake 构建流程

**`set_node_property` 的类型验证**：利用文档系统在运行时验证/转换值：

```cpp
Dictionary set_node_property_impl(const ToolContext &ctx) {
    String prop = args_string(ctx.args, "property");
    Variant value = ctx.args["value"];

    // 查询文档系统获取属性元数据
    DocData::PropertyDoc prop_doc = get_property_doc(ctx.node->get_class(), prop);

    // 根据元数据验证/转换值
    if (prop_doc.enumeration != "") {
        value = resolve_enum(prop_doc.enumeration, value); // "Disabled" → 0
    }
    if (prop_doc.type == "float" && value.get_type() == Variant::INT) {
        value = (float)(int)value;
    }

    // 通过 EditorUndoRedoManager 应用
    undo_redo->create_action("MCP: Set " + prop, UndoRedo::MERGE_ENDS, ctx.root);
    undo_redo->add_do_property(ctx.node, prop, value);
    undo_redo->add_undo_property(ctx.node, prop, ctx.node->get(prop));
    undo_redo->commit_action();

    return {{"property", prop}, {"value", variant_to_json(value)}};
}
```

---

### 实施路线图

```
Phase 0 — 基础架构 ✅
  - ToolSearchEngine + find_tool 元工具
  - ToolOutput 统一信封 + Schema 校验
  - 全工具描述英文化
  [ ] completion/complete 集成 ToolSearchEngine

Phase 1 — Undo/Redo + SDK 平权 ✅
  - supports_undo/is_destructive 能力声明
  - HandlerRegistry 自动 Undo 包装
  - IToolAdapter + SDK 注册路径改造（SDK 工具已走 itool_table_）

Phase 2 — ★ 四层工具体系 + 注册重构（核心）🟡 部分完成
  ✅ X-macro 分文件注册替代 // @tool register + codegen
  ✅ 删除 codegen.py + CMake codegen 步骤
  ✅ Layer 0: 通用兜底工具 (2 个)
  ✅ Layer 3: 文档工具 (8 个) — 基于 Godot ClassDB 运行时 API
  ✅ 分类系统自动发现取代 top_level_meta() 硬编码
  [ ] Layer 1: 语义专用工具 (~80 个)
  [ ] Layer 2: 属性组工具 (~126 个)

Phase 3 — MCP 差异化
  [ ] MCP Resources: godot://scene/current, godot://project/config
  [ ] MCP Prompts: create_player_controller, debug_performance, setup_collision_shapes
  [ ] 集成测试 + 文档更新
```

### 后果

**正面**：
- 搜索能力覆盖全部工具，AI 从"知道确切名字才能用"变为"描述意图即可发现"
- Undo/Redo 覆盖全部可逆操作，用户 Ctrl+Z 行为与 Godot 原生一致
- SDK 工具与内置工具完全平权，降低用户自定义工具的门槛
- **四层工具体系覆盖所有场景**：语义快捷 → 属性组批量 → 通用兜底 → 文档查询
- **100% 全覆盖**：Layer 0 保证任何属性/设置都可访问
- **零维护指令数据**：Godot 内置文档永远与引擎版本同步
- 全英文减少 AI 非英语语言模型的歧义
- Resources/Prompts 充分使用 MCP 协议能力
- **编译时安全注册**：X-macro 由编译器处理，无 BOM 漏扫风险
- **工具数减少 97%**：~11,791 → ~359，降低客户端 token 消耗

**负面**：
- ToolSearchEngine 增加 ~500 行 C++ 代码
- 自动 Undo 包装对非常规操作（调用方法而非设值）需手动处理
- SDK 工具必须显式声明 `needs_scene`/`needs_node` 等能力，否则默认为 false
- 全英文化对中文用户的学习曲线略有增加（但 AI 客户端无此问题）
- 属性组工具的参数 Schema 比单属性工具更复杂
- 现有 141 个工具需迁移注册方式（删除注释 + 加 X-macro 行）

**保留**：
- 单进程 GDExtension 架构不变（ADR-001）
- ITool 接口 + HandlerRegistry 调度不变（ADR-010）
- 纯主线程无锁模型不变
- 现有 141 个手写工具类不变（仅改注册方式）

## 已废弃的决策

以下 ADR 随 Python 服务器移除而废弃：

- **ADR-旧1: 双进程架构（Server + GDExtension）** → 被 ADR-001（单进程）取代
- **ADR-旧2: WebSocket IPC** → 已移除，不再需要进程间通信
- **ADR-旧3: 静态注册所有工具 Schema（Python 侧权威）** → 已被 ITool + codegen 编译时注册取代
