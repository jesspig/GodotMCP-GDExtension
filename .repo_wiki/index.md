# Godot MCP — 项目知识库

> C++ **GDExtension** 单进程架构，通过 **MCP Streamable HTTP**（端口 9600）将 Godot 4.6+ 编辑器暴露给 AI 工具。使用 `godot-cpp 10.0.0-rc1`，X-macro 分文件注册，四层工具体系（语义专用 + 属性组 + 通用兜底 + 文档查询），Godot ClassDB 运行时驱动文档数据，rapidyaml（ryml）YAML 解析，内置 C++ 测试引擎。

## 项目快照

| 维度 | 当前状态 |
|------|---------|
| C++ 源码根 | `extensions/src/` |
| 注册方式 | **X-macro 分文件注册**（`register_itools.cpp` + `register/*.hpp`） |
| 工具总数 | **~171**（全部 X-macro 注册，无 codegen） |
| 工具体系 | **四层体系**：语义专用(~154) + 属性组(~2) + 通用兜底(2) + 文档(8) + 元工具(7) + 运行时(12) |
| 指令数据源 | **Godot ClassDB 运行时查询**（零维护） |
| 顶级分类 | 自动发现：`meta_tools`、`editor_tools`、`node_tools`、`runtime_tools` |
| 场景树工具 | `editor_tools/scene_tree/`（25 工具） |
| 运行时桥接工具 | `runtime_tools/bridge/`（6 工具）+ `lifecycle/`（6 工具） |
| SDK 层 | `extensions/src/sdk/`（`McpToolDefinition` + `McpToolRegistry`） |
| 测试框架 | C++ `TestEngine`（`/run-tests`）+ Python 编排器 |
| HTTP 端口 | `:9600`（env `GODOT_MCP_HTTP_PORT` 覆盖） |
| 桥接端口 | `:9601`（env `GODOT_MCP_BRIDGE_PORT` 覆盖） |
| Pinned deps | `godot-cpp 10.0.0-rc1`、`ryml v0.7.0` |
| 版本号 | 仅 `CMakeLists.txt:22` `PROJECT_VERSION` |
| Python | `>=3.14`（`.python-version` 锁定） |

## 快速导航

| 类别 | 文档 |
|------|------|
| 架构总览 | [overview/architecture.md](overview/architecture.md) |
| 线程模型 | [overview/threading-model.md](overview/threading-model.md) |
| **X-macro 注册体系** | [modules/x-macro-registration.md](modules/x-macro-registration.md) |
| 命令路由与调度 | [modules/command-routing.md](modules/command-routing.md) |
| 分类自动发现 | [modules/category-discovery.md](modules/category-discovery.md) |
| MCP 传输 | [modules/ipc-bridge.md](modules/ipc-bridge.md) |
| 运行时桥接 | [modules/runtime-bridge.md](modules/runtime-bridge.md) |
| 插件生命周期 | [modules/editor-plugin.md](modules/editor-plugin.md) |
| 元工具 | [modules/meta-tools.md](modules/meta-tools.md) |
| **通用兜底工具** | [modules/fallback-tools.md](modules/fallback-tools.md) |
| **文档查询工具** | [modules/doc-tools.md](modules/doc-tools.md) |
| 场景树工具 | [modules/scene-tree-tools.md](modules/scene-tree-tools.md) |
| 工作区工具 | [modules/workspace-tools.md](modules/workspace-tools.md) |
| 文件系统工具 | [modules/filesystem-tools.md](modules/filesystem-tools.md) |
| 项目设置工具 | `editor_tools/settings/`（4 个兜底工具，详见 [fallback-tools.md](modules/fallback-tools.md)） |
| 分组工具 | [modules/group-tools.md](modules/group-tools.md) |
| 信号工具 | [modules/signal-tools.md](modules/signal-tools.md) |
| 资源管理工具 | [modules/resource-tools.md](modules/resource-tools.md) |
| SDK 层 | [modules/sdk-layer.md](modules/sdk-layer.md) |
| HTTP 服务器 | [modules/http-server.md](modules/http-server.md) |
| UI 组件 | [modules/ui-components.md](modules/ui-components.md) |
| LSP 客户端 | [modules/lsp-client.md](modules/lsp-client.md) |
| 日志系统 | [modules/logging.md](modules/logging.md) |
| 输入映射 | [modules/input-map.md](modules/input-map.md) |
| 构建与打包 | [reference/build-and-package.md](reference/build-and-package.md) |
| 设计决策（ADR） | [design/decisions.md](design/decisions.md) |
| **竞品深度分析** | [design/competitive-analysis.md](design/competitive-analysis.md) |
| **V2 优化方案** | [design/v2-optimization-plan.md](design/v2-optimization-plan.md) |
| Phase 0 实施指南 | [design/phases/phase0-blocking-fixes.md](design/phases/phase0-blocking-fixes.md) |
| Phase 1 实施指南 | [design/phases/phase1-competitiveness.md](design/phases/phase1-competitiveness.md) |
| Phase 2 实施指南 | [design/phases/phase2-differentiation.md](design/phases/phase2-differentiation.md) |
| 测试框架 | [testing/overview.md](testing/overview.md) |
| 变更日志 | [log.md](log.md) |

## 已清理文档（已从磁盘删除）

| 文档 | 原因 |
|------|------|
| `modules/codegen.md` | codegen.py 已删除，X-macro 注册替代 |
| `modules/project-settings-ext.md` | 设置扩展已合并为 4 个兜底工具 |
| `modules/csharp-solution.md` | C# 解决方案工具已移除 |
| `modules/dock-ui.md` | `plugin/` 目录已删除 |
| `modules/editor-control-gdext.md` | 编辑器控制已合并到 meta_tools |
| `modules/settings-tools.md` | codegen 1688 工具体系已移除 |
| `modules/resource-property-tools.md` | codegen 资源属性工具体系已移除 |
| `reference/tools-catalog.md` | 旧工具快照，已被 X-macro 架构替代 |
| `testing/phase-system.md` | Python 阶段文件已清理 |
| `testing/file-verifier.md` | 已被 C++ godot_file_verifier 替代 |
| `testing/mcp-client.md` | 已删除 |

## Agent 上手指南

1. **从 `overview/architecture.md` 开始** — 理解单进程架构、数据流、目录布局
2. **阅读 `overview/threading-model.md`** — 理解纯主线程 `_process()` 驱动模型
3. **阅读 `modules/x-macro-registration.md`** — 理解 X-macro 注册体系
4. **阅读 `modules/command-routing.md`** — 理解 ITool 接口 + HandlerRegistry 调度
5. **阅读 `modules/runtime-bridge.md`** — 理解运行时桥接设计
6. **添加新工具时**：
   - 创建 `.hpp` 文件实现 `ITool` 接口
   - 在 `extensions/src/built_in/tools/register/` 下对应分类的 X-macro 文件加一行
   - 在 `extensions/src/built_in/register_itools.cpp` 加 `#include`
   - 不需要 codegen，不需要 `// @tool register`
7. **运行测试前**：见 `AGENTS.md`「测试」章节

## 给 Agent 的提醒

- **入口符号** `gdext_mcp_init`（`register_types.cpp:60`）
- **不要修改** `extensions/CMakeLists.txt:15` 的 `GODOTCPP_API_VERSION "4.6"` 与根 `CMakeLists.txt` 的 `compatibility_minimum = "4.6"` 之间的绑定
- **升级 godot-cpp / ryml 前必测** — 二者均为 FetchContent 拉取
- **不要用 `String::utf8("中文")`** — 全英文化后直接 `String("English")` 即可
- **DLL 文件锁** — Godot 编辑器持有 DLL，重建失败时先关闭编辑器
- **构建优化** — sccache/ccache、Unity jumbo build、lld-link 均已配置
- **构建命令** — 始终用 `uv run python build.py`（Python >=3.14）
