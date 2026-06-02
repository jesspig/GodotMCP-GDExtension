# Godot MCP — 项目知识库

> C++ **GDExtension** 单进程架构，通过 **MCP Streamable HTTP**（端口 9600）将 Godot 4.6+ 编辑器暴露给 AI 工具。使用 godot-cpp 10.0.0-rc1，124 个 ITool 子类，rapidyaml（ryml）YAML 解析，内置 C++ 测试引擎。

## 快速导航

| 章节 | 说明 |
|------|------|
| [架构总览](overview/architecture.md) | 单进程架构、完整数据流、目录布局 |
| [线程模型](overview/threading-model.md) | 纯主线程模型，HTTP 服务器 poll |

## 核心组件

| 文件 | 说明 |
|------|------|
| [GDExtension C++](extensions/gdext.md) | **唯一实现**：`extensions/src/` C++ GDExtension，124 个 ITool 子类，codegen 自动注册，MCP Streamable HTTP 传输 |

## 模块文档

| 模块 | 说明 |
|------|------|
| [命令路由](modules/command-routing.md) | 从 MCP `call_tool` 到 `HandlerRegistry::execute()` 的完整链路；ITool + 自定义工具统一调度 |
| [Scene / Node / Property 命令模式](modules/scene-commands.md) | 节点/属性/场景工具模式；C++ 的 `Dictionary`/`JSON` 原生操作 |
| [MCP Streamable HTTP](modules/ipc-bridge.md) | `HttpServer`（:9600）+ `McpHandler`（JSON-RPC 2.0 会话管理 + SSE） |
| [编辑器插件](modules/editor-plugin.md) | C++ `McpEditorPlugin` 生命周期（`_enter_tree` → `process_frame` → `_exit_tree`） |
| [日志](modules/logging.md) | 直接 `UtilityFunctions::print/push_warning/push_error`，28 行实现 |
| [Editor Control（gdext）](modules/editor-control-gdext.md) | gdext 侧的编辑器控制：play/stop/refresh/get_editor_info |
| [ProjectSettings 扩展](modules/project-settings-ext.md) | 聚合设置工具：显示、物理、渲染、项目信息、层名称 |
| [输入映射](modules/input-map.md) | InputMap 命令：列出/添加/设置/移除输入动作 |
| [插件管理](modules/plugin-management.md) | 列出/启用/禁用编辑器插件 |
| [LSP 验证客户端](modules/lsp-client.md) | 通过 Godot LSP 服务器实现 GDScript 语法验证（`StreamPeerTCP`） |
| [C# 解决方案生成](modules/csharp-solution.md) | 直接在 gdext 中生成 `.sln` + `.csproj`，无需启动第二个 Godot 进程 |
| [Dock UI](modules/dock-ui.md) | 右侧 Dock 面板状态（当前未实现，计划中） |

## 参考文档

| 文档 | 说明 |
|------|------|
| [工具目录](reference/tools-catalog.md) | 全部工具的 JSON Schema、参数、返回值 |
| [客户端配置](reference/client-config.md) | Streamable HTTP 配置模板（opencode / Claude Code / Cursor / JetBrains） |
| [客户端 quirks](reference/client-quirks.md) | 各客户端配置怪癖速查表 |
| [CI/CD 流水线](reference/ci-cd.md) | GitHub Actions 工作流：CI 门禁 + 跨平台 Release 发布 |
| [构建与打包](reference/build-and-package.md) | `build.py` 标志、CMake 流程、文件锁限制 |

## 规范文档

| 文档 | 说明 |
|------|------|
| [IPC 协议](specification/ipc-protocol.md) | MCP Streamable HTTP 协议（JSON-RPC 2.0 + SSE） |
| [项目结构](specification/project-structure.md) | 完整目录布局、构建与测试命令 |

## 测试框架

| 文档 | 说明 |
|------|------|
| [测试框架总览](testing/overview.md) | 双轨架构：C++ 进程内引擎 + Python 编排器 |
| [C++ 测试引擎](testing/test-engine.md) | TestEngine + YAML 配置 + 磁盘校验 + 双源清理 |
| [编排器](testing/orchestrator.md) | 生命周期管理：备份 → 启动 → 连接 → 运行 → 恢复 → 报告 |
| [自定义 MCP 客户端](testing/mcp-client.md) | 绕过 MCP SDK 的 httpx 客户端（解决 float ID 校验问题） |
| [阶段系统](testing/phase-system.md) | 旧阶段执行系统（将被 YAML 测试替代） |
| [磁盘文件验证](testing/file-verifier.md) | 纯 Python tscn 解析器（旧，将被 C++ 引擎替代） |

## 设计文档

| 文档 | 说明 |
|------|------|
| [统一工具架构](design/unified-architecture-plan.md) | ITool 接口 + 组合式能力声明 + 注释驱动自动注册 |
| [设计决策](design/decisions.md) | ADR 风格的架构选择记录 |
| [变更日志](log.md) | 仅追加的项目变更记录
