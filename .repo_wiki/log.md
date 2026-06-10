# 变更日志

> 仅追加的项目变更记录（最新在前）。

## 2026-06-10 — Wiki 知识库同步（运行时桥接、架构修正、ADR 更新）

- **新增** `modules/runtime-bridge.md`：GameBridgeNode TCP :9601 + RuntimeBridge 客户端完整文档（架构图、数据流、7 个命令、状态机、已知问题）
- **更新** `overview/architecture.md`：架构图增加 GameBridgeNode/RuntimeBridge 组件；线程模型改为 `_process()` 驱动；目录树增加 `runtime/` 和 `runtime_tools/`；顶级分类改为 4 个（新增 `runtime_tools`）；新增"运行时桥接"章节
- **更新** `extensions/gdext.md`：文件结构增加 `runtime/` 子目录；工具分类表新增 `runtime_tools`；目录树展开编辑器工具子分类
- **更新** `modules/editor-plugin.md`：`_process()` 替换 `process_frame` 信号；Processing 状态增加桥接连接/轮询；`_exit_tree()` 增加 `runtime_bridge_.disconnect()`
- **更新** `design/decisions.md`：ADR-003 标记为已替换（`process_frame` → `_process()`）；新增 ADR-011 运行时桥接设计；ADR-014 标记 P0 阶段已完成（Phase 1 各项标记 ✅）
- **更新** `index.md`：快照表增加桥接端口、脚本工具、运行时工具计数；导航表新增运行时桥接；上手指南增加第 4 步
- **更新** `AGENTS.md`：末尾追加 `项目知识库` 章节，链接至 `.repo_wiki/index.md`

## 2026-06-08 — 市场分析 + ADR-014 优化路线图 + AGENTS.md 同步

- **新增** `AGENTS.md`「市场分析与优化路线图」章节：竞品生态全景（4 阵营 20+ 项目）、功能缺口分析（P0/P1/P2 三级）、核心竞争力定位、三阶段优化路线图
- **新增** `design/decisions.md` ADR-014：功能优化路线图（P0/P1/P2 三阶段），含每项功能的实现路径、架构决策、参考竞品、实施原则
- **更新** `index.md`：导航链接新增 ADR-014 优化路线图；Agent 上手指南新增第 5 步"了解优化方向"
- **更新** `AGENTS.md`：同步 wiki 链接指向 ADR-014

## 2026-06-08 — 项目设置工具集 + 文件系统工具文档 + 知识库同步

- **新增** `modules/settings-tools.md`：1688 个专属 get/set 工具 + 4 个兜底工具完整文档（架构图、YAML 数据库、特性标签变体、设计决策）
- **新增** `modules/filesystem-tools.md`：14 个文件系统工具文档
- **更新** `index.md`：工具计数 67→11734，新增 settings/filesystem 导航链接
- **更新** `overview/architecture.md`：目录树新增 filesystem/、settings/、collect_settings.py
- **重写** `modules/codegen.md`：新增 settings-db 输入 + YAML 格式示例 + collect_settings.py
- **更新** `reference/tools-catalog.md`：同步工具计数 84+566+838+1688=11734
- **更新** `reference/build-and-package.md`：移除 PCH 引用（ADR-013 已移除）
- **更新** `modules/editor-plugin.md`：移除 TestRunnerDock 引用（已删除）
- **更新** `modules/project-settings-ext.md`：标注已被 settings-tools 替代
- **更新** `AGENTS.md`：追加 wiki 链接

## 2026-06-04 — 场景树工具方案 + ADR-012 + 锚定摘要

- **新增** `modules/scene-tree-tools.md`：20 个场景树操作工具完整文档（4 批，Undo/Redo 模式、剪贴板策略、跳过列表）
- **新增** `design/decisions.md` ADR-012：场景树工具分类、Undo/Redo 策略、`EditorUndoRedoManager` 统一使用、禁止 `set_scene_root`
- **更新** `index.md`：添加 `modules/scene-tree-tools.md` 导航链接
- **更新** `AGENTS.md`：添加场景树工具分类说明（`EditorUndoRedoManager`、`PackedScene` 剪贴板）

## 2026-06-04 — 知识库补充 + 清理已实现设计文档

- **新增** `modules/signal-tools.md`（4 个信号工具文档）
- **新增** `modules/resource-tools.md`（6 个资源管理工具文档）
- **新增** `modules/resource-property-tools.md`（419 个资源类型 YAML 属性系统文档）
- **重写** `index.md`：列出当前 ITool + 283 节点属性 + 419 资源属性工具；标记已废弃测试文档
- **重写** `overview/architecture.md`：目录树从 `extensions/gdext/` → `extensions/src/`；双重注册路径图
- **重写** `extensions/gdext.md`：文件树 + 19 `.hpp` `// @tool register` 列表 + `cmd_utils.hpp` 函数表
- **重写** `modules/command-routing.md`：17 组 → 2 顶级分类；`top_level_meta()`；ITool 接口契约
- **重写** `modules/dock-ui.md`：纠正"未实现"错误
- **重写** `specification/project-structure.md`：新目录树 + CMake 构建流程图
- **修复** `modules/editor-plugin.md`：删除已不存在的 `tool_schemas.json`、`register_all_tools()`、`load_tool_schemas()`；修正 `add_control_to_bottom_panel` 为 `call()` 兜底
- **修复** `reference/build-and-package.md` 与 `reference/client-quirks.md` 中 `py -3` → `uv run python`
- **删除** 3 个已实现设计文档：`design/unified-architecture-plan.md`、`design/cleanup-plan.md`、`design/node-property-system.md`
- **更新** `index.md`：移除上述删除文档的导航链接

## 2026-06-02 — Code review + CommandFn→ITool 迁移 + 测试框架文档

- **新增** `design/cleanup-plan.md`：全项目代码审查后的优化清理方案（4 阶段，全部已完成）
- **新增** `testing/test-engine.md`：C++ 进程内测试引擎完整设计
- **重写** `modules/command-routing.md`：从旧 CommandFn 系统更新为 ITool + codegen 统一调度
- **重写** `overview/architecture.md`：架构图、数据流、目录布局全部更新为 ITool/`extensions/src/`
- **重写** `extensions/gdext.md`：路径从 `extensions/gdext/` 更正为 `extensions/src/`
- **重写** `testing/overview.md`：明确 C++ YAML 引擎为唯一测试路径
- **重写** `AGENTS.md`：清理旧 P1-P6 规划信息；修正 `source()` → `is_meta()`；删除 `tool_schemas.json` 引用
- **更新** `reference/tools-catalog.md`：Meta 工具 3→5，工具数 129→124
- **更新** `reference/build-and-package.md`：版本号 `0.1.5`→`0.2.0-dev2`
- **更新** `modules/ipc-bridge.md`、`modules/editor-plugin.md`：反映当前实现
- **更新** `design/unified-architecture-plan.md`：标注 `mcp_tool_adapter.hpp` 未采用
- **更新** `design/decisions.md`：ADR-010 状态"已接受"

## 2026-06-01 — 统一工具架构重构计划

- **新增** `design/unified-architecture-plan.md`：ITool 接口、ToolResult 统一信封、组合式能力声明、两轴分类、注释驱动自动注册、SDK 桥接
- **新增** `design/decisions.md` 中 ADR-010：统一工具架构决策记录

## 2026-05-31 — 真相修复 + 测试框架文档

- **修复** `register_script_cs` 状态：C# 工具已激活（17 组全部在 `register_all_tools()` 中调用）
- **修复** 工具计数：129 模式定义 ≈ 122 实际可用
- **修复** 构建命令：`py -3` → `uv run python`
- **新增** `testing/` 模块文档（overview / orchestrator / mcp-client / phase-system / file-verifier）

## 2026-05-30 — 代码对齐 + 单进程架构统一

- **删除** `modules/server.md`（Python/Cython MCP 服务器文档）
- **删除** `reference/editor-control.md`
- **重写** `overview/architecture.md`、`overview/threading-model.md`、`modules/ipc-bridge.md`、`specification/ipc-protocol.md`：双进程 → 单进程（C++ GDExtension only，Streamable HTTP :9600）
- **重写** `reference/client-config.md`：stdio → streamable-http 配置
- **重写** `design/decisions.md`：更新 ADR 反映单进程架构
- **修复** 多处代码引用与实际不一致的问题（`editor-plugin.md`、`ipc-bridge.md`、`threading-model.md`、`project-structure.md`、`extensions/gdext.md`、`overview/architecture.md`）
- **更新** 10+ 模块文件移除 WebSocket/ws_server/Python 服务器引用
- **补充** `reference/build-and-package.md`：`deep-clean` target + stale cache 自动恢复

## 2026-05-29 — HTTP 服务器、工具计数修正、架构文档重构

- **新增**：MCP Streamable HTTP 传输（`HttpServer` :9600 + `McpHandler` JSON-RPC 2.0）
- **修正**：工具计数 115→122、`IpcResponse.code` 类型 int→str、ProjectSettingsCommands 工具数 3→7
- **新增** ADR-010（MCP Streamable HTTP）、ADR-011（`register_script_cs` 未注册）
- **更新** 架构总览、线程模型、IPC 协议、CI/CD、项目结构

## 2026-05-28 — 全面清理 Rust 遗留引用

- **删除** 4 个 Rust 遗留页面（`crates/core.md`、`crates/gdext.md`、`modules/dispatcher.md`、`specification/workspace.md`）
- **新增** `modules/server.md`、`specification/project-structure.md`
- **全量更新** 所有页面统一版本号 `0.1.5-dev2`
- **清理** 所有页面中的 Rust 语言/架构对比引用

## 2026-05-27 — 知识库与 C++ 实现对齐

- 25 个 Wiki 页面更新/编写（架构总览、线程模型、模块文档、引用文档、规范、设计决策）
- 过时 Rust 页面标记为"仅遗留"
- 新增 ADR-009（C++ GDExtension 重写决策）

## 初始版本

- 使用 Rust `gdext` crate（v0.5）创建初始 Wiki 文档（28 文件、7 目录）
