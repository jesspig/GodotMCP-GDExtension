# Godot MCP — 项目知识库

> Python/Cython **MCP 服务器** + Rust GDExtension **cdylib** 双进程架构，通过 **125 条命令**将 Godot 4.6+ 编辑器暴露给 AI 工具。

## 快速导航

| 章节 | 说明 |
|------|------|
| [架构总览](overview/architecture.md) | 双进程架构、三 crate 拆分、数据流图 |
| [线程模型](overview/threading-model.md) | **接触 gdext 前必读**。tokio↔主线程分离、dispatcher、日志泵 |

## 逐 crate / 模块文档

| 文件 | 说明 |
|------|------|
| [core](crates/core.md) | `crates/core/src/` 共享协议类型 (IpcRequest, IpcResponse, ToolCallParams, ToolManifest) |
| [server（Python）](crates/server.md) | `server/` 目录：Python/Cython MCP 服务器，编译为独立 exe |
| [gdext](crates/gdext.md) | `crates/gdext/src/` GDExtension cdylib：编辑器插件、WebSocket 服务端、125 个命令处理器 |

## 模块文档

| 模块 | 说明 |
|------|------|
| [命令路由](modules/command-routing.md) | 从 MCP `call_tool` 到 `cmd_*` 调用的完整链路；17 组全部在 create_registry() 中注册 |
| [Scene / Node / Property 命令模式](modules/scene-commands.md) | 节点/属性/场景工具模式、`j2v`/`v2j` JSON↔Variant 转换规则、`resolve_node`、`try_load` |
| [IPC 桥接](modules/ipc-bridge.md) | gdext 侧 `IpcWebSocketServer` + server 侧 `GodotBridge` WebSocket 通信 |
| [Dispatcher](modules/dispatcher.md) | `MainThreadDispatcher`: tokio 工作线程 → 主线程闭包执行 |
| [日志](modules/logging.md) | mpsc 日志通道 + `process_frame` 泵（因为 Godot 宏拒绝非主线程访问） |
| [编辑器插件](modules/editor-plugin.md) | `McpEditorPlugin` 生命周期；`process()` 为何故意空置 |
| [Dock UI](modules/dock-ui.md) | 右侧 Dock VBox 面板，4 个子面板，当前与计划状态 |
| [Editor Control（gdext）](modules/editor-control-gdext.md) | gdext 侧的编辑器控制：play/stop/refresh/get_editor_info |
| [ProjectSettings 扩展](modules/project-settings-ext.md) | 聚合设置工具：显示、物理、渲染、项目信息、层名称 |
| [输入映射](modules/input-map.md) | InputMap 命令：列出/添加/设置/移除输入动作 |
| [插件管理](modules/plugin-management.md) | 列出/启用/禁用编辑器插件 |
| [LSP 验证客户端](modules/lsp-client.md) | 通过 Godot LSP 服务器实现 GDScript 语法验证 |
| [C# 解决方案生成](modules/csharp-solution.md) | 直接在 gdext 中生成 .sln + .csproj，无需启动第二个 Godot 进程 |

## 参考文档

| 文档 | 说明 |
|------|------|
| [工具目录](reference/tools-catalog.md) | 全部 125 个工具的 JSON Schema、参数、返回值 |
| [客户端配置](reference/client-config.md) | 12 种 AI 客户端 × stdio 配置模板（仅 stdio 可用） |
| [客户端 quirks](reference/client-quirks.md) | 各客户端配置怪癖速查表 |
| [构建与打包](reference/build-and-package.md) | `build.py` 标志、CI 门禁顺序、热重载、文件锁恢复 |
| [CI/CD 流水线](reference/ci-cd.md) | GitHub Actions 工作流：CI 门禁 + 跨平台 Release 发布 |

## 规范文档

| 文档 | 说明 |
|------|------|
| [IPC 协议](specification/ipc-protocol.md) | IpcRequest/IpcResponse/IpcNotification/ToolCallParams 线路格式 |
| [Cargo Workspace](specification/workspace.md) | 各 crate Cargo.toml、锁定版本、addon 清单 |

## 设计文档

| 文档 | 说明 |
|------|------|
| [设计决策](design/decisions.md) | ADR 风格的架构选择记录 |
| [变更日志](log.md) | 仅追加的项目变更记录 |