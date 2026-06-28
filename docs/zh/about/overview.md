# 概述

GodotMCP 是一个 **C++ GDExtension** 插件，适用于 Godot 4.6+，通过 **Model Context Protocol (MCP)** 将编辑器暴露给 AI 编码助手。它完全运行在 Godot 编辑器进程的主线程中 —— 无独立服务器进程、无多线程、无锁。

## 功能介绍

GodotMCP 充当 AI 工具（Claude Code、Cline、Continue、Cursor、opencode、Roo Code 等）与 Godot 编辑器之间的桥梁。AI 助手可以：

- **创建和管理场景** — 添加、删除、重命名、复制、重新父级节点
- **编辑脚本** — 读写 GDScript 和 C# 文件
- **管理文件** — 创建、删除、移动、复制项目文件
- **控制编辑器** — 运行/停止项目、设置断点、截取视口
- **查询运行时** — 检查游戏场景、读写属性、调用方法
- **查询 Godot 文档** — 通过 ClassDB 搜索类、方法、属性
- **以及 150+ 其他操作**，覆盖每个编辑器领域

## 核心特性

| 特性 | 说明 |
|------|------|
| **164 个内置工具** | 涵盖场景树、脚本、文件系统、动画、着色器等 |
| **纯 C++ GDExtension** | 无需外部依赖，编辑器进程内原生运行 |
| **纯主线程** | 无多线程、无锁、无同步原语 |
| **Streamable HTTP** | MCP 2026-07-28 协议，端口 9600，支持 GET（SSE 流）、POST、OPTIONS |
| **运行时桥接** | TCP 桥接（端口 9601）查询和控制运行中的游戏 |
| **X-macro 注册** | 无需代码生成，创建一个 `.hpp` 文件即可添加工具 |
| **SDK 层** | GDScript 和 C# API，支持创建自定义 MCP 工具 |
| **内置测试引擎** | 基于 YAML 的流水线测试框架 |
| **撤销/重做支持** | 工具集成 Godot 的 EditorUndoRedoManager |
| **ClassDB 驱动** | 文档工具查询 Godot 运行时类数据库，零维护 |
| **11 个客户端配置生成器** | 为 opencode、Cursor、Claude Code 等生成配置 |

## 架构一览

```
AI 客户端 (Claude Code / Cline / Cursor / opencode …)
    │  Streamable HTTP :9600
    ▼
Godot 编辑器进程
    ├── McpEditorPlugin — 插件生命周期 + _process() 轮询
    ├── HttpServer — HTTP/1.1 + SSE
    ├── McpHandler — JSON-RPC 2.0 → MCP
    ├── HandlerRegistry — 164 个 ITool 调度 + 搜索引擎
    ├── SDK → McpToolRegistry — 自定义工具注册
    ├── RuntimeBridge — TCP :9601 → 游戏进程
    └── TestEngine — YAML 流水线测试
`

## 数据速览

| 指标 | 值 |
|------|-----|
| 内置工具 | 164 |
| 注册文件 | 4 个 X-macro `.hpp` 文件 |
| SDK 类 | 2 个（McpToolDefinition + McpToolRegistry） |
| 测试 YAML 文件 | 26 |
| HTTP 端口 | 9600（可配置） |
| 桥接端口 | 9601（可配置） |
| Godot 版本 | 4.6+ |
| godot-cpp | 10.0.0-rc1 |