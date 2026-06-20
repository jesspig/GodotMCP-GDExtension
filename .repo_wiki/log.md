# 变更日志

> 仅追加的项目变更记录（最新在前）。同日期条目已合并。

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

## 2026-06-16 — Wiki 事实校正 + 废弃计划清理

- **工具总数**：171→152（修复运行时工具重复计数）
- **修复** 多文件中的版本号行号、目录树、时序图（POST-only）
- **新增** `modules/cmd-utils.md`；删除 `plans/` 目录（6 个文件）

## 2026-06-15 — 全量优化实施 + 协议升级 + 文档同步

- **MCP 协议升级**：2025-03-26→2026-07-28；移除 session（~200 行）、GET/DELETE 端点
- **安全修复**：6 个 P0 bug（CRLF 注入、TreeItem 双重释放、类型验证旁路等）
- **死代码删除**：`SearchEngine`、`refactor_casts.py`、LSP 客户端等
- **性能优化**：桥接延迟 -90%（50ms→5ms）、增量解析 O(n²)→O(n)、Release zip -80%
- **架构模板化**：新增 `dispatch_map.hpp`、`undo_helpers.hpp`、`args_get_typed.hpp`
- **文档**：新增 `tool-base.md`、`cmd-utils.md`；同步 6 个模块文档至最新代码

## 2026-06-14 — Wiki 全面校正 + ADR 压缩 + 废弃清理

- **源码验证**：5 路并行子代理审计，修复 23 个文件
- **重写** `test-engine.md`、`orchestrator.md`、`ui-components.md`、`input-map.md`、`ci-cd.md`、`client-config.md`
- **修复** 18 个模块文档的工具计数、API 签名、数据结构
- **ADR 压缩**：16 个 ADR 从 ~295 行压缩为摘要表 ~60 行
- **删除** `competitive-analysis.md`、`design/phases/`（3 文件）、`v2-optimization-plan.md`
- **清理** `roadmap.md`：移除已完成阶段

## 2026-06-13 — Wiki 数据修复 + 新增模块文档

- 修复 17 处过时数据；新增 `sdk-layer.md`、`http-server.md`、`ui-components.md`

## 2026-06-12 — V2 优化方案 + 竞品分析 + ADR-016

- 新增竞品分析、V2 优化方案、ADR-016~022

## 2026-06-11 — Wiki 全量事实校正 + 四层工具体系

- 重写线程模型、GDExt 组件图、元工具、编辑器插件文档
- ADR-015：三层→四层工具体系；X-macro 替代 codegen
- 新增 33 个 P1/P2 工具；删除 8 个已废弃 wiki 文件

## 2026-06-10 — ADR-015 + 运行时桥接文档

- 新增运行时桥接模块文档；架构图增加桥接组件

## 2026-06-08 — 市场分析 + ADR-014

- 新增 `AGENTS.md` 市场分析章节、ADR-014、设置/文件系统工具文档

## 2026-06-04 — 场景树工具 + 知识库补充

- 新增场景树 20 个工具文档、`signal-tools.md`、`resource-tools.md`

## 2026-06-02 — Code review + CommandFn→ITool 迁移

- 新增测试框架文档；重写命令路由、架构、GDExt 组件图

## 2026-06-01 — 统一工具架构重构计划

- 新增 `unified-architecture-plan.md`、ADR-010

## 2026-05-31 — 真相修复 + 测试框架文档

- 工具计数、构建命令修复；新增 `testing/` 模块文档

## 2026-05-30 — 单进程架构统一

- 双进程→单进程（C++ GDExtension only）；重写 6 个核心文档

## 2026-05-29 — HTTP 服务器 + MCP Streamable HTTP

- 新增 Streamable HTTP 传输；工具计数 115→122；ADR-010/011

## 2026-05-28 — 清理 Rust 遗留引用

- 删除 4 个 Rust 遗留页面；统一版本号

## 2026-05-27 — 知识库初始化

- 25 个 Wiki 页面创建；ADR-009

## 初始版本

- Rust `gdext` crate（v0.5）初始 Wiki：28 文件、7 目录
