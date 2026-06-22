# Godot MCP — 项目知识库

> C++ **GDExtension** 单进程架构，通过 **MCP Streamable HTTP**（端口 9600，MCP 2026-07-28 协议）将 Godot 4.6+ 编辑器暴露给 AI 工具。使用 `godot-cpp 10.0.0-rc1`，X-macro 分文件注册，四层工具体系（元工具 + 语义专用 + 通用兜底 + 文档查询），Godot ClassDB 运行时驱动文档数据，rapidyaml（ryml）YAML 解析，内置 C++ 测试引擎。**无 session，支持 GET（SSE 流）、POST、OPTIONS 三种 HTTP 方法**。

---

## 项目快照

| 维度 | 当前状态 |
|------|---------|
| C++ 源码根 | `extensions/src/` |
| 注册方式 | **X-macro 分文件注册**（`register_itools.cpp` + `register/*.hpp`） |
| 工具总数 | **152**（全部 X-macro 注册，无重复，无 codegen） |
| 工具体系 | **四层体系**：语义专用(137，含 14 运行时) + 通用兜底(2) + 文档(8) + 元工具(5) |
| 渐进式披露 | `tools/list` 仅返回 5 个元工具（`is_meta()==true`，~3KB JSON），其余通过发现链按需加载 |
| 发现链 | `get_categories()` → `get_tools(category)` → `get_tools(name, detail=true)` / `find_tool(query)` |
| 客户端自动配置 | **11 客户端**支持，项目级配置路径，通过底部面板一键生成 |
| Resources 层 | 3 个资源（scene-tree, project-settings, editor-info）+ 1 个 URI 模板（scene-node/{path}）[→ Phase 2: 扩展至 11+ 个](design/09-lld-resources-prompts.md) |
| 指令数据源 | **Godot ClassDB 运行时查询**（零维护） |
| 顶级分类 | 自动发现：`meta_tools`、`editor_tools`、`node_tools`、`runtime_tools` |
| 场景树工具 | `editor_tools/scene_tree/`（24 工具） |
| 运行时桥接工具 | `runtime_tools/bridge/`（8 工具）+ `lifecycle/`（6 工具） |
| SDK 层 | `extensions/src/sdk/`（`McpToolDefinition` + `McpToolRegistry`） |
| 测试框架 | C++ `TestEngine`（`/run-tests`）+ Python 编排器 |
| HTTP 端口 | `:9600`（env `GODOT_MCP_HTTP_PORT` 覆盖） |
| 桥接端口 | `:9601`（env `GODOT_MCP_BRIDGE_PORT` 覆盖） |
| Pinned deps | `godot-cpp 10.0.0-rc1`、`ryml v0.7.0` |
| 当前版本 | **0.2.2-dev1**（仅 `CMakeLists.txt` 的 `PROJECT_VERSION`） |
| Python | `>=3.14`（`.python-version` 锁定） |
| MCP 协议 | **Streamable HTTP 2026-07-28**（无 session，支持 GET SSE 流 + POST + OPTIONS） |

---

## 🏗️ 架构与概念

| 文档 | 说明 |
|------|------|
| [系统架构总览](overview/architecture.md) | 单进程架构、模块划分、数据流、部署拓扑 |
| [线程模型](overview/threading-model.md) | 纯主线程 `_process()` 驱动模型，无锁设计 |
| [工具体系架构](concepts/tool-system-architecture.md) | 4 层工具设计理念、渐进式披露、双重注册路径 |
| [运行时通信](concepts/runtime-communication.md) | 编辑器↔游戏进程 TCP 桥接：协议、异步化、生命周期 |
| [安全模型](concepts/security-model.md) | 5 层纵深防御：Token 认证、破坏性操作拦截、沙箱、非破坏编辑 |

---

## 🧩 核心模块

| 文档 | 说明 |
|------|------|
| [ITool 基类体系](modules/tool-base.md) | `ToolResult` / `ToolContext` / `ITool` 接口契约 |
| [命令路由与调度](modules/command-routing.md) | `tools/call` → `HandlerRegistry` → `ITool` 调用链路 |
| [命令注册中心](modules/handler-registry.md) | 搜索、分类树、双重分发、频率索引 |
| [X-macro 注册体系](modules/x-macro-registration.md) | 编译时宏注册机制，X-macro 文件组织 |
| [分类自动发现](modules/category-discovery.md) | `category()` 返回值 `/` 分割自动建树 |
| [MCP 传输层](modules/ipc-bridge.md) | HTTP Server、SSE 流、JSON-RPC 2.0 编解码 |
| [HTTP 服务器](modules/http-server.md) | `HttpServer` 实现、请求路由、CORS、限流 |
| [运行时桥接](modules/runtime-bridge.md) | `RuntimeBridgeServer` + `GameBridgeNode` TCP 桥接通信实现 |
| [SDK 层](modules/sdk-layer.md) | `McpToolDefinition` / `McpToolRegistry` 自定义工具 API |
| [插件生命周期](modules/editor-plugin.md) | `McpEditorPlugin` 初始化、工具注册、`_process()` 驱动 |
| [UI 组件](modules/ui-components.md) | 底部面板、确认对话框、控制台、日志器 |
| [日志系统](modules/logging.md) | 直接 GDExtension `UtilityFunctions::print` 日志 |
| [共享工具函数](modules/cmd-utils.md) | `SchemaBuilder`、`dispatch_map`、`undo_helpers` 等模板 |

---

## 🛠️ 工具分类

| 分类 | 文档 | 工具数 |
|------|------|:------:|
| 元工具 | [meta-tools.md](modules/meta-tools.md) | 5 |
| 场景树 | [scene-tree-tools.md](modules/scene-tree-tools.md) | 24 |
| 工作区/调试器 | [workspace-tools.md](modules/workspace-tools.md) | 13 |
| 脚本 | [script-tools.md](modules/script-tools.md) | 12 |
| 文件系统 | [filesystem-tools.md](modules/filesystem-tools.md) | 12 |
| 动画 | [animation-tools.md](modules/animation-tools.md) | 10 |
| 文档查询 | [doc-tools.md](modules/doc-tools.md) | 8 |
| 资源管理 | [resource-tools.md](modules/resource-tools.md) | 6 |
| 运行时工具 | [runtime-tools.md](modules/runtime-tools.md) | 14 |
| Control/UI | — | 4 |
| 碰撞形状 | — | 1 |
| 导出 | — | 4 |
| Shader | — | 5 |
| 音频 | — | 3 |
| 导航 | — | 3 |
| 3D 场景 | — | 3 |
| TileMap | — | 3 |
| 可视化 | — | 1 |
| 脚手架 | — | 1 |
| 设置 | [settings-tools.md](modules/settings-tools.md) | 4 |
| 输入映射 | [input-map.md](modules/input-map.md) | 4 |
| 信号 | [signal-tools.md](modules/signal-tools.md) | 4 |
| 分组 | [group-tools.md](modules/group-tools.md) | 4 |
| 插件管理 | [plugin-management.md](modules/plugin-management.md) | 2 |
| 通用兜底 | [fallback-tools.md](modules/fallback-tools.md) | 2 |

---

## 📐 设计文档

| 文档 | 说明 |
|------|------|
| [系统架构设计](design/00-architecture.md) | 四阶段演进架构、模块职责、技术选型、数据流 |
| [Bridge 异步化 LLD](design/01-lld-bridge-async.md) | `send_command_async` + SSE 推送，消除编辑器冻结 |
| [渐进式披露优化 LLD](design/02-lld-tools-list.md) | 元工具精简 + 缓存加速 + SSE 通知 |
| [run_editor_script LLD](design/03-lld-run-editor-script.md) | EditorScript 执行工具，组合 write_script 使用 |
| [undo/redo LLD](design/04-lld-undo-redo.md) | `EditorUndoRedoManager` 集成元工具 |
| [YAML 工作流引擎 LLD](design/05-lld-yaml-workflow.md) | PipelineRunner 复用，ExecuteWorkflowTool 包装 |
| [Shadow Scene Diff LLD](design/06-lld-shadow-scene.md) | 非破坏编辑：快照、diff、apply/rollback |
| [DAG 与并行开发方案](design/07-dag-and-parallel-plan.md) | 53 道工序任务拆分、5 人排期、4 里程碑 |
| [Resources & Prompts 增强 LLD](design/09-lld-resources-prompts.md) | 只读操作转 Resource、动态 Prompt、工作流指导 |

| [设计决策（ADR）](design/08-decisions.md) | 22 项已决策架构记录与演进关系 |
---

## 🧪 测试

| 文档 | 说明 |
|------|------|
| [测试框架概览](testing/overview.md) | 架构组件、运行方式、测试生命周期 |
| [C++ 测试引擎](testing/test-engine.md) | Pipeline 模型、断言系统、YAML 驱动 |
| [测试编排器](testing/orchestrator.md) | Python 编排器、Godot 生命周期管理 |

---

## 📋 参考

| 文档 | 说明 |
|------|------|
| [构建与打包](reference/build-and-package.md) | CMake 构建、Python 包装、addon 打包 |
| [CI/CD 流水线](reference/ci-cd.md) | Windows/Linux/macOS 三平台 CI |
| [客户端配置](reference/client-config.md) | 11 个 MCP 客户端配置说明 |
| [客户端 quirks](reference/client-quirks.md) | 各客户端已知行为差异 |

---

## 📖 规范

| 文档 | 说明 |
|------|------|
| [MCP 协议规范](specification/ipc-protocol.md) | JSON-RPC 2.0 端点、请求/响应格式 |
| [项目结构](specification/project-structure.md) | 目目录布局、文件命名、模块组织 |

---

## 🔧 GDExtension

| 文档 | 说明 |
|------|------|
| [GDExtension 组件图](extensions/gdext.md) | GDExtension 初始化、类注册、生命周期绑定 |

---

## Agent 上手指南

1. **从 `overview/architecture.md` 开始** — 理解单进程架构、数据流、目录布局
2. **阅读 `overview/threading-model.md`** — 理解纯主线程 `_process()` 驱动模型
3. **阅读 `design/00-architecture.md`** — 理解四阶段演进路线与模块职责
4. **阅读 `concepts/tool-system-architecture.md`** — 理解四层工具体系设计理念
5. **阅读 `concepts/runtime-communication.md`** — 理解编辑器↔游戏桥接模式
6. **阅读 `concepts/security-model.md`** — 理解纵深防御安全设计
7. **阅读 `modules/x-macro-registration.md`** — 理解 X-macro 注册体系
8. **阅读 `modules/command-routing.md`** — 理解 ITool 接口 + HandlerRegistry 调度
9. **阅读 `modules/handler-registry.md`** — 理解搜索、分类树与双重分发
10. **阅读 `modules/runtime-bridge.md`** — 理解运行时桥接实现

### 添加新工具

- 创建 `.hpp` 文件实现 `ITool` 接口
- 在 `extensions/src/built_in/tools/register/` 下对应分类的 X-macro 文件加一行
- 在 `extensions/src/built_in/register_itools.cpp` 加 `#include`
- 不需要 codegen，不需要 `// @tool register`
- 详细说明见 [X-macro 注册体系](modules/x-macro-registration.md)

### 运行测试

- `uv run python main.py test` — 完整流水线（自动启停 Godot）
- `uv run python main.py test --file 03_*.yaml` — 仅指定测试文件
- 详细说明见 [测试框架概览](testing/overview.md)

---

## 给 Agent 的提醒

- **入口符号** `gdext_mcp_init`（`register_types.cpp:60`）
- **不要修改** `extensions/CMakeLists.txt` 中 `GODOTCPP_API_VERSION "4.6"` 与根 `CMakeLists.txt` 的 `compatibility_minimum = "4.6"` 之间的绑定
- **升级 godot-cpp / ryml 前必测** — 二者均为 FetchContent 拉取
- **不要用 `String::utf8("中文")`** — 全英文化后直接 `String("English")` 即可
- **DLL 热重载** — `.gdextension` 设 `reloadable = true`（Godot 4.2+ 官方机制，`GDExtensionManager::reload_extension()` 自动检测文件变更并重载扩展）。`main.py build` 直接覆盖 DLL，编辑器在检测到变更后自动重载。Windows 下因 OS Loader 锁定 DLL，覆盖可能失败（视系统版本和配置而异），此时关闭编辑器重试。已知约束：仅限编辑器构建、修改 Godot base class 后需重启编辑器。
- **构建优化** — sccache/ccache、Unity jumbo build、lld-link 均已配置
- **构建命令** — 始终用 `uv run python main.py`（Python >=3.14）
- **不要误判渐进式披露** — `tools/list` 仅返回 5 个元工具（`is_meta()==true`，~3KB JSON），152 个工具通过发现链按需加载。首次对话的 token 开销与约 10 个工具的普通 MCP 服务器相当。验证方式：`get_info()` 返回的 `progressive_disclosure` 字段。（注意：`overview/architecture.md` 的 mermaid 图中的 `152 tool schemas` 指的是注册表总数，不是 `tools/list` 返回值；`tools/list` 的实现见 `mcp_handler.cpp` / `get_always_on_tools()`）。
