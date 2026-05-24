# Godot MCP — 项目知识库

> Rust-only **Model Context Protocol** bridge 让 AI 客户端驱动 Godot 4.6+ 编辑器。双进程、三 crate 架构。

## 快速导航

| 章节 | 说明 |
|------|------|
| [架构总览](overview/architecture.md) | 双进程架构、三 crate 拆分、数据流图 |
| [线程模型](overview/threading-model.md) | **接触 gdext 前必读**。tokio↔主线程分离、dispatcher、日志泵 |
| [构建与打包](reference/build-and-package.md) | `build.py` 命令、CI 门禁、热重载技巧、文件锁恢复 |

## 逐 crate 文档

| Crate | 文件 | 说明 |
|-------|------|------|
| [core](crates/core.md) | `crates/core/src/` | 共享协议类型 (IpcRequest, IpcResponse, ToolCallParams, ToolManifest) |
| [server](crates/server.md) | `crates/server/src/` | MCP 服务端二进制：rmcp stdio 传输、工具注册表、WebSocket 桥接 |
| [gdext](crates/gdext.md) | `crates/gdext/src/` | GDExtension cdylib：编辑器插件、WebSocket 服务端、99 个命令处理器 |

## 模块文档

| 模块 | 说明 |
|------|------|
| [命令路由](modules/command-routing.md) | 从 MCP `call_tool` 到 `cmd_*` 调用的完整链路；新增工具须同时注册两侧 |
| [Scene 命令](modules/scene-commands.md) | 节点/属性/场景工具模式、`j2v`/`v2j` JSON↔Variant 转换规则、`resolve_node`、`try_load` |
| [IPC 桥接](modules/ipc-bridge.md) | gdext 侧 `IpcWebSocketServer` + server 侧 `GodotBridge` WebSocket 通信 |
| [Dispatcher](modules/dispatcher.md) | `MainThreadDispatcher`: tokio 工作线程 → 主线程闭包执行 |
| [日志](modules/logging.md) | mpsc 日志通道 + `process_frame` 泵（因为 Godot 宏拒绝非主线程访问） |
| [编辑器插件](modules/editor-plugin.md) | `McpEditorPlugin` 生命周期；`process()` 为何故意空置 |
| [Dock UI](modules/dock-ui.md) | 右侧 Dock VBox 面板，4 个子面板，当前与计划状态 |

## 参考文档

| 文档 | 说明 |
|------|------|
| [工具目录](reference/tools-catalog.md) | 全部 99 个工具的 JSON Schema、参数、返回值 |
| [客户端配置](reference/client-config.md) | 12 种 AI 客户端 × stdio 配置模板（仅 stdio 可用） |
| [客户端 quirks](reference/client-quirks.md) | 各客户端配置怪癖速查表 |
| [构建与打包](reference/build-and-package.md) | `build.py` 标志、CI 门禁顺序、热重载、文件锁恢复 |

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

## 过时文档预警

- **`README.md`** 写着 "35 tools" — 实际是 **99**。不信任该数字。
- 本章首页（本页）内容来源于对当前源码的直接分析。
