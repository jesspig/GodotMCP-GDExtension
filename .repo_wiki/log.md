# 变更日志

> 仅追加的项目变更记录（最新在前）。

## 2026-06-18 — 工具计数校正 + 废弃计划清理 + client-quirks 修复

- **修复** 工具总数：`152` → `153`（新增 `wait_for_bridge` 工具未计入）
- **修复** 元工具计数：`7` → `8`（`list_settings` 有 `is_meta=true`）
- **修复** 运行时工具计数：`12` → `13`（`bridge` 组 6→7，漏计 `wait_for_bridge`）
- **重写** `modules/workspace-tools.md`：`31 个工具` → `13 个实际工具`（18 个"子操作"非独立 ITool，改为复合工具参数值标注）
- **修复** `overview/architecture.md`、`extensions/gdext.md`：工作区 31→13、桥接 6→7、工具总数 152→153
- **修复** `modules/x-macro-registration.md`：register_existing.hpp 154→136
- **修复** `modules/runtime-tools.md`：补全 `wait_for_bridge` 文档及注册行号
- **删除** `plans/pipeline-refactor-plan.md`、`plans/pipeline-refactor-dag.md`（P→S→S 重构已在 commit b5a52e7 完成，原执行计划不再具备指导意义。新架构已完整记录在 `testing/test-engine.md`）
- **修复** `reference/client-quirks.md`：`validate_gd_script`/`validate_csharp_script` 合并为 `validate_script`

## 2026-06-16 — Wiki 事实校正 + 废弃计划清理

- **修复** 工具总数：`171` → `152`（运行时 12 个工具已含在 register_existing 的 135 中，不应单独计数）
- **修复** 工具分层描述：`语义专用(~145) + 通用兜底(2) + 文档(8) + 元工具(7) + 运行时(12)` → `语义专用(135，含 12 运行时) + 通用兜底(2) + 文档(8) + 元工具(7)`
- **修复** `index.md` 版本号行号：`CMakeLists.txt:22` → `:16`
- **修复** `overview/architecture.md`：工具数 `~171` → `152`；`/utf-8` 位置 `根 CMakeLists.txt:43` → `extensions/CMakeLists.txt:163`；移除不存在的 `lsp/` 目录；新增 `cmd_utils/` 7 文件和 `client_config_registry.hpp`；`plugin/` 3→2；`mcp_handler` 注释 "会话管理"→"处理器（无 session）"
- **修复** `overview/architecture.md` 时序图：移除已废弃的 `initialize`/`GET /mcp`/`Mcp-Session-Id` 流程，改为 POST-only `tools/list` + `tools/call`
- **修复** `AGENTS.md`：MSVC UTF-8 位置 `根 CMakeLists.txt` → `extensions/CMakeLists.txt:163`
- **修复** `modules/plugin-management.md`：工具数 3→2（`enable_plugin`+`disable_plugin` 合并为 `set_plugin_enabled`）
- **新增** `modules/cmd-utils.md` 4 个缺失文件文档：`error_codes.hpp`、`memdelete_guard.hpp`、`schema_builder.hpp`、`tracked_settings.hpp`
- **新增** `index.md` 导航：`modules/plugin-management.md`
- **删除** `plans/` 目录（6 个文件）：已完成优化计划的执行文档，与已删除的 `design/phases/` 和 `design/v2-optimization-plan.md` 同类

## 2026-06-15 — 全量优化实施 + 协议升级 2026-07-28

- **协议升级**：MCP Streamable HTTP 从 `2025-03-26` 升级至 `2026-07-28`；移除 `GET /mcp` 端点、`DELETE /mcp` 端点、`Mcp-Session-Id` 头、`initialize/initialized` 握手机制；改为纯 `POST+OPTIONS` 通信，SSE 仅内联在 POST 响应中推送
- **Session 移除**：`mcp_handler` 中删除全部 ~200 行 session 管理代码（`Session` 结构体、`create_session`、`validate_session`、`cleanup_expired_sessions`、`generate_uuid`）；事件队列从 session 索引改为全局单队列
- **Header 校验**：新增 `Mcp-Method`/`Mcp-Name` HTTP 头解析（`http_parser.cpp`） + 请求 body 一致性校验（`handle_post`）
- **P0 bug 修复**：6 个安全/崩溃 bug（CRLF 注入、TreeItem 双重释放、类型验证旁路、ctx.root 悬挂指针、register 泄漏）
- **死代码删除**：移除 `SearchEngine`（170 行）、`refactor_casts.py`（223 行）、`rebuild_metadata_indices`
- **架构模板化**：新增 `dispatch_map.hpp`、`undo_helpers.hpp`、`args_get_typed.hpp` 三个共享模板；描述默认实现消除 21 处 boilerplate；DispatchMap 替代 6 个工具的 if/else 链
- **性能优化**：`bridge.cpp` step 50ms→5ms（-90% 响应延迟）；增量解析 O(n²)→O(n)；`BUFFER_LIMIT` 64KB→1MB；`walk_project_dir` 改用 `std::filesystem`（10x 加速）；CPack STRIP ON（Release zip -80%）；unity batch_size 32→12
- **知识库同步**：更新 `ipc-bridge.md`、`http-server.md`、`cmd-utils.md`、`runtime-bridge.md`、`editor-plugin.md`、`ipc-protocol.md` 以反映当前代码状态

## 2026-06-15 — 新增 tool-base.md + cmd-utils.md 模块文档

- **新增** `modules/tool-base.md`：ITool 基类体系完整文档，含 ToolResult/ToolContext/ITool 类图、`execute()` 模板方法流程图、类型验证映射表
- **新增** `modules/cmd-utils.md`：共享工具函数文档，含场景辅助、JSON↔Variant 转换序列图、undoable_set 流程、树遍历流程图
- **更新** `index.md`：新增两个模块的导航链接

## 2026-06-14 — Wiki 全面校正 + ADR 压缩 + 废弃文档清理

### 源码深度验证（5 路并行子代理审计，修复 23 个文件）

- **重写** `testing/test-engine.md`：移除虚构的 HTTP 响应格式、不存在的 `TestRunnerDock`、虚构的数据结构；修正 `project_godot`→`project_settings`；按实际源码重写全部流程
- **重写** `testing/orchestrator.md`：移除虚构的 `McpSession`/`PhaseRunner.run_all()`/`backup_example()` 等已废弃架构描述，按 `test_orchestrator.py` 实际实现重写
- **修复** `testing/overview.md`：移除不存在的 `smoke_test.py` 引用，更新文件结构
- **重写** `modules/ui-components.md`：修正 McpDock 为右侧面板（非底部）、`add_control_to_bottom_panel` 直接调用（非 `call()` 兜底）、McpLogger 为纯 C++ 类
- **修复** `modules/lsp-client.md`：标记为死代码（编译但零调用者），修正端口号 6005 和超时 5000ms
- **重写** `modules/input-map.md`：修正全部 API 签名（`input_list_actions` 无参数、事件类型 `key`/`mb`/`jb`/`ja`、deadzone 默认 0.2）
- **修复** `modules/meta-tools.md`：元工具 7→8（`list_settings` 有 `is_meta=true`）；修正 `get_info` 返回结构；`call_tool` 参数名 `tool_name`/`arguments`；搜索排序 freq 优先
- **修复** `modules/runtime-bridge.md`：单客户端（非 4）、缓冲区 64KB（非 8KB）、`connect()` 无参数；移除 4 个已修复的"已知问题"
- **修复** `modules/editor-plugin.md`：`start()` 3 参数返回 `Error`；`load_config()` ProjectSettings 优先；补全 17 个成员变量
- **修复** `overview/threading-model.md`：`start()` 签名修正
- **修复** `specification/ipc-protocol.md`：`tools/list` 返回 always-on 工具（非全部）；修正 SSE 事件为 `notifications/tools/list_changed`；未知工具返回 `kInternalError`（非 `kInvalidParams`）；补全 3 个缺失方法
- **修复** `modules/ipc-bridge.md`：`tools/list` 描述修正；会话最大 16 个、TTL 3600s；连接最大 32 个
- **修复** `modules/http-server.md`：补全会话级限制（max sessions 16, TTL 3600s）
- **重写** `reference/ci-cd.md`：`ci.yml` 不存在，完全重写为 `release.yml`+`docs.yml` 两个工作流的准确描述
- **修复** `reference/build-and-package.md`：移除虚构的 CI 门禁段；修正 GLOB 描述为 header-only
- **重写** `reference/client-config.md`：修正 opencode 格式（`"mcp"`+`"type":"remote"`，非 `"mcpServers"`+`"streamable-http"`）；新增 `client_config_registry.hpp` 11 种客户端参考
- **修复** `specification/project-structure.md`：移除不存在的 `yaml_tests/`；修正 CI 工作流名称；新增 `client_config_registry.hpp`（后重命名）
- **修复** `modules/fallback-tools.md`：移除不存在的 Layer 2（`NodePropertyGetTool`/`NodePropertySetTool`）
- **修复** `modules/scene-tree-tools.md`：`attach_script` 支持 C#
- **修复** `modules/sdk-layer.md`：`call("execute",args)` 行号 73→49
- **修复** `extensions/gdext.md`：CMakeLists.txt 行号全面修正（+5 漂移）；补全 `ui/` 目录；GLOB 描述修正
- **修复** `modules/x-macro-registration.md`：元工具注释补充 `list_settings`
- **修复** `modules/workspace-tools.md`：性能监视器实现细节从截图段移至正确位置

### 工具计数与分类校正

- **修复** 工具计数：元工具 6→8（新增 `generate_client_config` + `list_settings`）；场景树 20→24；工作区 24→31；动画 5→10；导出 2→4；着色器 3→5；输入映射 1→4
- **修复** `overview/architecture.md` 目录树：补齐缺失分类（audio/navigation/3d_scene），修正所有工具计数
- **修复** `extensions/gdext.md` 文件树：移除已删除的 `pch.hpp`/`node_tools`/`node_props` 条目；修正 `screenshot_utils.hpp` 条目（文件实际存在，保留引用），修正工具计数
- **修复** `modules/filesystem-tools.md`：移除不存在的 `create_gd_script`/`create_csharp_script`/`filesystem_utils.hpp`，新增 `save_resource_as`，计数 14→12
- **修复** `modules/input-map.md`：工具名 `list_input_actions`→`input_list_actions`、`set_input_action_events`→`add_input_event_binding`
- **修复** `modules/workspace-tools.md`：新增 5 个调试器工具（get_locals/debugger_step_out/list_breakpoints/set_breakpoint/remove_breakpoint）+ 2 个截图工具（capture_viewport/capture_game_viewport）
- **修复** `modules/scene-tree-tools.md`：Batch 4 计数 4→5（toggle_placeholder 归类修正）
- **修复** `reference/client-quirks.md`：移除已删除工具引用（get_variable/set_variable/csharp_build/add_circle_collision）

### ADR 压缩 + 废弃文档清理

- **压缩** `design/decisions.md`：全部 16 个 ADR 从详细叙事（~295 行）压缩为摘要表（~60 行），保留 ID/日期/决策/要点
- **确认** ADR-016 9 项子决策全部实现（预编译分发/GameBridge 安全/底部面板 UI/24 个新工具/WSL2/CORS+Session/安全增强/客户端模板/限流）
- **压缩** ADR-014/015/016 为摘要表（~645→~120 行）
- **删除** `design/competitive-analysis.md`（市场分析已过时）
- **删除** `design/phases/` 3 个实施指南文件（625 行）
- **删除** `design/v2-optimization-plan.md`（已完成）
- **清理** `design/roadmap.md`：移除全部已完成的 Phase 1/2/3 和 V2 优化阶段
- **修复** `design/decisions.md`：ADR-015 Phase 2/3 标记为 ✅ 完成；ADR-014 移除已删除 roadmap.md 链接
- **更新** `index.md`：新增导航链接，移除废弃文档引用

## 2026-06-13 — Wiki 数据修复 + 新增模块文档

- **修复** 17 处过时数据：工具数 `~149` → `~171`、`register_types.cpp:56` → `:60`、`register_itools.cpp:201` → `:229`
- **修复** `GODOT_MCP_TOOL` 宏签名：补齐第 7 参数 `is_destructive_val`
- **新增** `modules/sdk-layer.md`、`modules/http-server.md`、`modules/ui-components.md`
- **更新** `index.md`：添加 SDK/HTTP/UI 导航链接

## 2026-06-12 — V2 优化方案 + 竞品深度分析 + ADR-016

- **新增** `design/competitive-analysis.md`：20+ Godot MCP 竞品技术分析、SWOT 评估、能力矩阵对比
- **新增** `design/v2-optimization-plan.md`（已删除）、`design/phases/` 3 个实施指南（已删除）
- **新增** `design/decisions.md` ADR-016~022
- **更新** `index.md`

## 2026-06-11 — Wiki 全量事实校正 + ADR-015 修订 + 清理 8 个废弃文档

- **重写** `overview/threading-model.md`、`extensions/gdext.md`、`modules/meta-tools.md`、`modules/editor-plugin.md`
- **修订** `design/decisions.md` ADR-015：三层→四层工具体系；X-macro 替代 codegen；YAML→DocTools 迁移
- **新增** 33 个 P1/P2 工具 + mcp_handler Resources/Prompts 集成
- **删除** 8 个已废弃 wiki 文件：`codegen.md`、`dock-ui.md`、`project-settings-ext.md`、`editor-control-gdext.md`、`csharp-solution.md`、`tools-catalog.md`、`settings-tools.md`、`resource-property-tools.md`
- **修复** 10 个活跃文件中的残余 codegen 引用

## 2026-06-10 — ADR-015 设计 + Wiki 运行时桥接同步

- **新增** `design/decisions.md` ADR-015：搜索引擎 + 自动 Undo + SDK 平权 + 三层工具体系
- **新增** `modules/runtime-bridge.md`：GameBridgeNode + RuntimeBridge 完整文档
- **更新** `overview/architecture.md`：架构图增加桥接组件；线程模型改为 `_process()` 驱动
- **更新** `design/decisions.md`：ADR-003 标记已替换；新增 ADR-011；ADR-014 P0 完成

## 2026-06-08 — 市场分析 + ADR-014 + 设置/文件系统工具文档

- **新增** `AGENTS.md` 市场分析章节：4 阵营 20+ 竞品、P0/P1/P2 缺口分析
- **新增** `design/decisions.md` ADR-014：功能优化路线图
- **新增** `modules/settings-tools.md`、`modules/filesystem-tools.md`

## 2026-06-04 — 场景树工具 + 知识库补充

- **新增** `modules/scene-tree-tools.md`：20 个场景树操作工具
- **新增** `design/decisions.md` ADR-012：Undo/Redo 策略
- **新增** `modules/signal-tools.md`、`modules/resource-tools.md`、`modules/resource-property-tools.md`
- **删除** 3 个已实现设计文档

## 2026-06-02 — Code review + CommandFn→ITool 迁移 + 测试框架文档

- **新增** `design/cleanup-plan.md`、`testing/test-engine.md`
- **重写** `modules/command-routing.md`、`overview/architecture.md`、`extensions/gdext.md`、`testing/overview.md`、`AGENTS.md`

## 2026-06-01 — 统一工具架构重构计划

- **新增** `design/unified-architecture-plan.md`、ADR-010

## 2026-05-31 — 真相修复 + 测试框架文档

- **修复** 工具计数、构建命令；**新增** `testing/` 模块文档

## 2026-05-30 — 代码对齐 + 单进程架构统一

- 双进程 → 单进程（C++ GDExtension only）；**重写** 6 个核心文档；**删除** Python 服务器遗留文档

## 2026-05-29 — HTTP 服务器、工具计数修正、架构文档重构

- **新增** MCP Streamable HTTP 传输；**修正** 工具计数 115→122；**新增** ADR-010/011

## 2026-05-28 — 全面清理 Rust 遗留引用

- **删除** 4 个 Rust 遗留页面；**新增** 2 个文档；统一版本号

## 2026-05-27 — 知识库与 C++ 实现对齐

- 25 个 Wiki 页面更新/编写；**新增** ADR-009

## 初始版本

- 使用 Rust `gdext` crate（v0.5）创建初始 Wiki 文档（28 文件、7 目录）
