# 变更日志

> 仅追加的项目变更记录（最新在前）。同日期条目已合并。

## 2026-06-26 — Phase 4 增量更新：影子场景 + 重放 + Pipeline 重构

- **工具计数校正**：152→164（元工具 5→9，脚本 12→13，新增影子场景/重放/差异共 7 工具）
- **影子场景引擎**：新增 `scene_diff/` 模块（SceneShadow/SceneDiff/ScenePatcher/SceneSnapshot），4 个 ITool（stage_scene_change, preview_change, apply_changes, discard_changes）
- **录制/重放系统**：新增 `replay/` 模块（OperationRecorder/OperationReplay），2 个 ITool（record_operations, replay_operations）
- **元工具扩展**：新增 undo/redo/get_undo_history/execute_workflow（共 9 个元工具）
- **Pipeline 重构**：`testing/` → `pipeline/` 目录迁移，`PipelineRunner` 拆分为 `PipelineRunnerBase` + `TestRunner` + `WorkflowRunner` 三层继承
- **Resources 扩容**：3→10 个运行时资源（新增 console/breakpoints/performance/filesystem/signals/groups/classes）
- **Prompt 系统**：9 个动态 Prompt（新增 shadow_edit/export_project 等）
- **Workflow 引擎**：新增 `execute_workflow` 元工具 + WorkflowRunner + WorkflowParser
- **脚本工具**：新增 `run_editor_script`（执行 EditorScript 子类）
- **Runtime 桥接**：多实例支持完善，新增 `list_game_instances` 工具
- **设计文档清理**：删除 10 份已实现的 Phase 1~4 设计文档（`01-lld-bridge-async` ~ `06-lld-shadow-scene`、`07-dag-and-parallel-plan`、`09-lld-resources-prompts` + `00-architecture.md` + `08-decisions.md`），代码和模块文档为当前权威参考

### Lint 自检
- 修复 overview/architecture.md 目录树不完整（缺 scene_diff/replay/pipeline）
- 校准 10 个文件的 30+ 处数值统计（工具计数 152→164、元工具 5→9、语义工具 137→145、脚本 12→13、Resources 3→10）
- 修正 testing/test-engine.md 已废弃注释
- 修复 concepts/tool-system-architecture.md、design/*.md、modules/*.md、extensions/gdext.md 等的过时数值和路径
- 修复 specification/project-structure.md 中的 PipelineRunner→PipelineRunnerBase 引用
- 修复 extensions/gdext.md 中 GODOT_MCP_TOOL 签名（7 参→2 参）
- 修复 design/00-architecture.md 中 testing/→pipeline/ 路径

### [20:30] update | Wiki 维护：修复目录树重复、URI 模板计数、目录结构遗漏

- **架构目录修复**：`overview/architecture.md` 删除 178-187 行重复的 scene_diff/replay 目录块；修正 pipeline/ 描述（"替代旧 testing/" → "TestRunner/WorkflowRunner"）；更新 testing/ 目录内容（移除已迁移的 yaml_parser/type_utils）
- **URI 模板计数校准**：`index.md` Resources 层计数 1→2 个 URI 模板（`scene-node/{path}` + `class/{name}`）
- **目录结构补齐**：`specification/project-structure.md` mermaid 图添加 scene_diff/replay/pipeline，文本树补齐 4 个遗漏模块
- **AGENTS.md 关联**：追加 `## 项目知识库` 章节，链接 `.repo_wiki/index.md` 和 `log.md`
- **index.md 修复**：合并重复的"影子场景工具"描述行（原 2 行→1 行）；修正 Pipeline 引擎描述（"替代旧 testing/" → "testing/ 为薄门面"）
- **数值扫描验证**：工具总数 164、元工具 9、语义工具 145、文档 8、通用兜底 2、Resources 10+2、Prompts 9 — 全部与源码一致

### Lint 自检
- 修复 `index.md` 中指向已删除 `design/00-architecture.md` 的断链（Agent 上手指南第 3 步），重定向至 `overview/architecture.md`
- 校准 `architecture.md` 目录树中 testing/ 文件清单（yaml_parser/type_utils 已确认迁移至 pipeline/）
- 修复 `specification/project-structure.md` mermaid 图遗漏 scene_diff/replay/pipeline 目录
- 验证 47 个 Wiki 页面间 Markdown 链接无断链

## 2026-06-20 — 竞品分析 + 设计文档体系 + Wiki 全面更新

- **竞品分析**：覆盖 Godot/Unity/Unreal 三引擎 MCP 生态共 ~46 个项目，完成能力矩阵对比与锐评
- **产品规划**：4 阶段迭代路线图（53 道工序 DAG、5 人排期、4 里程碑）
- **设计文档**：新增 8 份设计文档（`00-architecture.md` 至 `06-lld-shadow-scene.md`、`07-dag-and-parallel-plan.md`、`08-decisions.md`）
- **ADR**：新增 ADR-017~022，演进关系图更新；ADR-022 标记已废弃
- **文档审计清理**：
  - 删除 `07-lld-sdk-ecosystem.md`（编译时 SDK 方案废弃）
  - 重命名 `03-lld-execute-gdscript.md` → `03-lld-run-editor-script.md`
  - 统一 `00-architecture.md` 中 `tools/list` 方向为渐进式披露
  - 修复 ADR 关系图标签、排期表任务编号、架构图工具计数（x145→x143）
  - 修复 `01-lld-bridge-async.md` 文件路径引用
  - 移除 `00-architecture.md` 中 3 处编译时 SDK 引用
- **新增 `concepts/` 目录**：3 篇概念文档（工具系统、运行时通信、安全模型）
- **补齐 Mermaid 图**：`plugin-management.md`、`scene-commands.md`
- **Index 重构**：按架构/模块/工具/设计/测试/参考/规范 7 区重组织

## 2026-06-18 — 文档审计 + 发布 v0.2.1 + 工具计数校正

- **文档审计**：`docs/en/` 与 `docs/zh/` 全部 48 个文件同步修复；工具计数 149+→153
- **重写** 工具目录（`tools-catalog.md`）、架构文档、SDK API 文档（`sdk-api.md`）
- **发布** v0.2.1 更新日志，含 MCP 协议升级、架构优化、153 工具、构建系统模块化
- **工具计数校正**：152→153（`wait_for_bridge`）；元工具 7→8（`list_settings`）；运行时 12→13
- **修复** 工作区工具数 31→13；文档全面对齐代码；删除已完成的 `plans/` 目录
- **修复** `client-quirks.md`：`validate_gd_script`/`validate_csharp_script` 合并

## 历史摘要

### 2026-06-16
- 工具计数校正：171→152（修复运行时工具重复计数）
- 废弃 `plans/` 目录清理（6 个文件）
- 多文件版本号行号、目录树、时序图修复

### 2026-06-15
- MCP 协议升级 2025-03-26→2026-07-28，移除 session（~200 行）
- 6 个 P0 安全漏洞修复（CRLF 注入、TreeItem 双重释放等）
- 死代码删除：SearchEngine、refactor_casts.py、LSP 客户端
- 性能优化：桥接延迟 -90%、增量解析 O(n²)→O(n)

### 2026-06-14
- 5 路并行子代理审计，修复 23 个文件
- 模块文档重写：test-engine/orchestrator/ui-components/input-map/ci-cd/client-config
- ADR 16→摘要表压缩（~295→60 行）
- 废弃文件清理：competitive-analysis、design/phases、v2-optimization-plan

### 2026-06-13
- 修复 17 处过时数据
- 新增模块文档：sdk-layer、http-server、ui-components

### 2026-06-12
- 新增竞品分析（~46 个项目矩阵对比）
- 新增 V2 优化方案、ADR-016~022

### 2026-06-11
- 全量事实校正：线程模型、GDExt 组件图、元工具、编辑器插件
- ADR-015：三层→四层工具体系，X-macro 替代 codegen
- 新增 33 个 P1/P2 工具，删除 8 个废弃 wiki 文件

### 2026-06-10
- 新增运行时桥接模块文档、架构图桥接组件

### 2026-06-08
- 市场分析、ADR-014、设置/文件系统工具文档

### 2026-06-04
- 场景树 20 个工具文档、signal-tools/resource-tools

### 2026-06-02
- 测试框架文档、命令路由重写、CommandFn→ITool 迁移

### 2026-06-01
- 统一工具架构重构计划、ADR-010

### 2026-05-31
- 工具计数/构建命令修复、testing/ 模块文档

### 2026-05-30
- 双进程→单进程架构统一（C++ GDExtension only）
- 6 个核心文档重写

### 2026-05-29
- Streamable HTTP 传输、工具计数 115→122、ADR-010/011

### 2026-05-28
- 4 个 Rust 遗留页面清理、版本号统一

### 2026-05-27
- 25 个 Wiki 页面创建、ADR-009

### 初始版本
- Rust gdext crate v0.5 初始 Wiki（28 文件、7 目录）
