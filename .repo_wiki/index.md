# Godot MCP — 项目知识库

> Python/Cython **MCP 服务器** + C++ **GDExtension** 双进程架构，通过 **125 条命令**将 Godot 4.6+ 编辑器暴露给 AI 工具。支持 Legacy stdio/WebSocket 和 MCP Streamable HTTP 双传输。

## 快速导航

| 章节 | 说明 |
|------|------|
| [架构总览](overview/architecture.md) | 双传输架构、完整数据流、目录布局 |
| [线程模型](overview/threading-model.md) | 纯主线程模型，双服务器 poll |

## 核心组件

| 文件 | 说明 |
|------|------|
| [GDExtension C++](extensions/gdext.md) | **唯一实现**：`extensions/gdext/` C++ GDExtension，使用 godot-cpp 10.0.0-rc1，17 个处理器文件（16 组活跃注册 115 个工具），WsServer + HttpServer 双传输 |
| [MCP 服务器 (Python)](modules/server.md) | `server/` 目录：Python/Cython MCP 服务器，编译为独立 exe，125 个工具 Schema |

## 模块文档

| 模块 | 说明 |
|------|------|
| [命令路由](modules/command-routing.md) | 从 MCP `call_tool` 到 `cmd_*` 调用的完整链路；C++ `HandlerRegistry`；17 组处理器状态 |
| [Scene / Node / Property 命令模式](modules/scene-commands.md) | 节点/属性/场景工具模式；C++ 的 `Dictionary`/`JSON` 原生操作 |
| [IPC 桥接](modules/ipc-bridge.md) | 路径 A: gdext `WsServer` + server `GodotBridge`（WebSocket）；路径 B: `HttpServer` + `McpHandler`（MCP Streamable HTTP） |
| [编辑器插件](modules/editor-plugin.md) | C++ `McpEditorPlugin` 生命周期（`_enter_tree` → 双服务器 `process_frame` → `_exit_tree`） |
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
| [工具目录](reference/tools-catalog.md) | 全部 125 个工具的 JSON Schema、参数、返回值 |
| [客户端配置](reference/client-config.md) | stdio 配置模板（VS Code / Claude Code / Cursor / JetBrains） |
| [客户端 quirks](reference/client-quirks.md) | 各客户端配置怪癖速查表 |
| [编辑器控制（服务器端）](reference/editor-control.md) | 4 个 server 端工具：`get_server_version`、`godot_editor_open/close/restart` |
| [CI/CD 流水线](reference/ci-cd.md) | GitHub Actions 工作流：CI 门禁 + 跨平台 Release 发布 |
| [构建与打包](reference/build-and-package.md) | `build.py` 标志、CMake 流程、文件锁限制 |

## 规范文档

| 文档 | 说明 |
|------|------|
| [IPC 协议](specification/ipc-protocol.md) | 两条路径：WebSocket IPC 格式 + MCP Streamable HTTP 协议 |
| [项目结构](specification/project-structure.md) | 完整目录布局、构建与测试命令 |

## 设计文档

| 文档 | 说明 |
|------|------|
| [设计决策](design/decisions.md) | ADR 风格的架构选择记录（含 C++ 重写、HTTP 传输、C# 未注册状态等） |
| [变更日志](log.md) | 仅追加的项目变更记录
