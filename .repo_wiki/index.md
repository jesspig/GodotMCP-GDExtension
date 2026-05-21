# Godot MCP — Repo Wiki

> Godot 编辑器的 Model Context Protocol 桥接方案。纯 Rust 实现：GDExtension 插件 + 独立 MCP Server 二进制。

## 实现文档

| 页面 | 描述 |
|------|------|
| [当前实现状态](implementation/current-status.md) | Phase 1 完成度、已实现组件、已知缺失 |
| [GDExtension 实现](implementation/gdext-skeleton.md) | EditorPlugin、WS Server、IPC 模块详细文档 |
| [MCP Server 实现](implementation/server-skeleton.md) | CLI、ServerHandler、GodotBridge 详细文档 |

## 概览

| 页面 | 描述 |
|------|------|
| [架构概览](overview/architecture.md) | 整体架构（含实现状态标注）、三协议通信、组件拓扑 |

## 技术规范

| 页面 | 描述 |
|------|------|
| [IPC 与 MCP 协议](specification/protocol.md) | IPC 消息格式、MCP 传输层、协议版本（含当前支持的方法） |
| [Cargo Workspace 结构](specification/workspace.md) | 三层 crate 设计：core / gdext / server（含实际 vs 计划目录树） |

## 设计文档

| 页面 | 描述 |
|------|------|
| [Dock UI 面板](design/dock-ui.md) | 独立 Dock 设计、状态栏、客户端列表、工具管理（计划） |
| [工具清单与热切换](design/tools.md) | 48 工具按类别分组、热切换机制（计划） |
| [IPC 桥接细节](design/ipc-bridge.md) | WebSocket 通信、命令路由（含当前实现与计划） |

## 使用指南

| 页面 | 描述 |
|------|------|
| [客户端配置](guide/client-config.md) | 12 个 AI 客户端自动配置方案 |

## 项目规划

| 页面 | 描述 |
|------|------|
| [分阶段实施计划](planning/phases.md) | Phase 1-4 时间线与里程碑 |
| [变更日志](log.md) | 项目演变记录 |

## 决策记录

| 决策 | 结论 | 依据 |
|------|------|------|
| 语言 | Rust 全栈 | `godot-rust/gdext` + `rmcp`，单语言零运行时依赖 |
| GDExtension 库 | `godot-rust/gdext` v0.5+ | 4.6K stars，120 贡献者，EditorPlugin 原生支持 |
| MCP SDK | `rmcp` v1.7+ | 官方 Rust SDK，支持 stdio + Streamable HTTP |
| IPC | WebSocket (tokio-tungstenite) | 跨平台、全双工、Rust 生态成熟 |
| 传输协议 | stdio + Streamable HTTP | MCP 2025 标准，覆盖所有主流客户端 |
| UI 面板 | 独立 Dock (add_control_to_dock) | FileSystem 风格，不占用底部面板空间 |
| 工具管理 | 热切换（IPC 实时同步） | 面板开关 → 通知 Server → Service 动态重载 |
| 插件形式 | GDExtension EditorPlugin | 原生 C++ 级别 API 访问，非 GDScript 桥接 |

## 产出物

- **`godot-mcp-server`** — 独立二进制文件，AI 客户端直接启动，零运行时依赖
- **`addons.zip`** — Godot 编辑器插件包，解压到 `addons/` 目录即可使用

## 外部参考

- [godot-rust/gdext](https://github.com/godot-rust/gdext) — Rust bindings for Godot 4
- [modelcontextprotocol/rust-sdk](https://github.com/modelcontextprotocol/rust-sdk) — 官方 Rust MCP SDK (rmcp)
- [hi-godot/godot-ai](https://github.com/hi-godot/godot-ai) — 现有 Godot MCP 实现（GDScript + Python）
- [CoplayDev/unity-mcp](https://github.com/CoplayDev/unity-mcp) — Unity MCP 社区版（方案对标）
- [modelcontextprotocol/python-sdk](https://github.com/modelcontextprotocol/python-sdk) — Python MCP SDK