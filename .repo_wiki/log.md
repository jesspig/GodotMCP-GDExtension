# 变更日志

> 仅追加的项目变更记录（最新在前）。

## 2026-06-12 — V2 优化方案 + 竞品深度分析 + ADR-016~022

**竞品深度分析与 V2 优化方案制定**

- **新增** `design/competitive-analysis.md`：20+ Godot MCP 竞品技术分析、SWOT 评估、能力矩阵对比
- **新增** `design/v2-optimization-plan.md`：V2 优化总方案（P0/P1/P2 三级优先级，Phase 0-2 时间线）
- **新增** `design/phases/phase0-blocking-fixes.md`：P0 阻断性修复实施指南（Release 流水线、CI、GameBridge 安全）
- **新增** `design/phases/phase1-competitiveness.md`：P1 竞争力提升实施指南（编辑器 UI、22-30 个新工具、WSL2、CORS/Session）
- **新增** `design/phases/phase2-differentiation.md`：P2 差异化优势实施指南（安全增强、客户端模板、限流、CI 测试）
- **新增** `design/decisions.md` ADR-016~022：预编译分发、网络绑定安全、编辑器 UI、工具覆盖面补全、WSL2 兼容、安全模型增强、用户引导与分发
- **更新** `index.md`：导航新增竞品分析 + V2 方案 + Phase 指南

## 2026-06-11 — Wiki 全量事实校正 + ADR-015 修订

**Wiki 全量事实校正：codegen 引用清理 + 线程模型修正 + 工具计数同步**

- **重写** `overview/threading-model.md`：`process_frame` 信号 → `_process(delta)` 虚函数驱动
- **重写** `extensions/gdext.md`：移除所有 codegen 引用 → X-macro 分文件注册体系
- **重写** `modules/meta-tools.md`：5→6 工具（新增 `find_tool` 搜索引擎）
- **重写** `modules/editor-plugin.md`：移除 TestRunnerDock 代码；更新生命周期图
- **更新** `reference/build-and-package.md`、`specification/project-structure.md`：移除 codegen
- **更新** `overview/architecture.md`：workspace 工具计数 31→29
- **更新** `index.md`、`AGENTS.md`

**ADR-015 修订：四层工具体系 + X-macro 注册 + Godot 内置文档驱动**

- **修订** `design/decisions.md` ADR-015：三层→四层工具体系；X-macro 替代 codegen；YAML→DocTools 迁移
- **新增** 33 个 P1/P2 工具 + mcp_handler Resources/Prompts 集成
- **更新** `design/roadmap.md`、`index.md`、`AGENTS.md`

**Wiki 清理：删除 8 个废弃文档 + 修复 10 个残余 codegen 引用**

- **删除** 8 个已废弃 wiki 文件：`codegen.md`、`dock-ui.md`、`project-settings-ext.md`、`editor-control-gdext.md`、`csharp-solution.md`、`tools-catalog.md`、`settings-tools.md`、`resource-property-tools.md`
- **修复** 10 个活跃文件中的残余 `// @tool register` / `codegen` / `generated_registration.cpp` 引用：`scene-tree-tools.md`、`filesystem-tools.md`、`group-tools.md`、`signal-tools.md`、`resource-tools.md`、`input-map.md`、`plugin-management.md`（2→3 工具）、`test-engine.md`（移除 TestRunnerDock UI 章节）、`testing/overview.md`（移除 Dock 入口）、`specification/ipc-protocol.md`（resources/prompts 已实现）
- **更新** `index.md`：已废弃文档表改为"已清理文档"清单（含磁盘删除确认）；settings-tools 导航改为 fallback-tools 链接

## 2026-06-10 — ADR-015 设计 + Wiki 运行时桥接同步

**下一代工具架构设计（ADR-015）+ 架构分析**

- **新增** `design/decisions.md` ADR-015：搜索引擎 + 自动 Undo + SDK 平权 + 三层工具体系，10 项子决策 + 四阶段路线图
- **更新** `design/roadmap.md`：新增 Phase 4 追踪清单
- **分析** 全项目深潜（40+ 源码文件 + 竞品 + Godot UndoRedo 文档）

**Wiki 知识库同步（运行时桥接、架构修正、ADR 更新）**

- **新增** `modules/runtime-bridge.md`：GameBridgeNode + RuntimeBridge 完整文档
- **更新** `overview/architecture.md`：架构图增加桥接组件；线程模型改为 `_process()` 驱动；顶级分类新增 `runtime_tools`
- **更新** `extensions/gdext.md`、`modules/editor-plugin.md`：反映运行时桥接
- **更新** `design/decisions.md`：ADR-003 标记已替换；新增 ADR-011；ADR-014 P0 完成
- **更新** `index.md`、`AGENTS.md`

## 2026-06-08 — 市场分析 + ADR-014 + 设置/文件系统工具文档

**市场分析 + ADR-014 优化路线图**

- **新增** `AGENTS.md` 市场分析章节：4 阵营 20+ 竞品、P0/P1/P2 缺口分析
- **新增** `design/decisions.md` ADR-014：功能优化路线图

**项目设置工具集 + 文件系统工具文档**

- **新增** `modules/settings-tools.md`：1688 个专属工具 + 4 个兜底工具文档
- **新增** `modules/filesystem-tools.md`：14 个文件系统工具文档
- **重写** `modules/codegen.md`：新增 settings-db 输入
- **更新** `index.md`、`overview/architecture.md`、`reference/tools-catalog.md`、`reference/build-and-package.md`、`modules/editor-plugin.md`、`modules/project-settings-ext.md`、`AGENTS.md`

## 2026-06-04 — 场景树工具 + 知识库补充

**场景树工具方案 + ADR-012**

- **新增** `modules/scene-tree-tools.md`：20 个场景树操作工具（4 批）
- **新增** `design/decisions.md` ADR-012：Undo/Redo 策略

**知识库补充 + 清理已实现设计文档**

- **新增** `modules/signal-tools.md`、`modules/resource-tools.md`、`modules/resource-property-tools.md`
- **重写** `index.md`、`overview/architecture.md`、`extensions/gdext.md`、`modules/command-routing.md`、`modules/dock-ui.md`、`specification/project-structure.md`
- **修复** `modules/editor-plugin.md`、`reference/build-and-package.md`、`reference/client-quirks.md`
- **删除** 3 个已实现设计文档

## 2026-06-02 — Code review + CommandFn→ITool 迁移 + 测试框架文档

- **新增** `design/cleanup-plan.md`、`testing/test-engine.md`
- **重写** `modules/command-routing.md`、`overview/architecture.md`、`extensions/gdext.md`、`testing/overview.md`、`AGENTS.md`
- **更新** `reference/tools-catalog.md`、`reference/build-and-package.md`、`modules/ipc-bridge.md`、`modules/editor-plugin.md`、`design/unified-architecture-plan.md`、`design/decisions.md`

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
