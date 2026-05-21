# 架构概览

> 图例：✅ = 已实现 | 📋 = 计划中 | ❌ = 未实现

## 整体拓扑

```mermaid
graph TD
    subgraph AI["AI 客户端层"]
        CC["OpenCode / Claude Code<br/>✅ stdio 可用"]
        Other["Cursor / Copilot / Gemini CLI / ...<br/>❌ HTTP 待实现"]
    end

    subgraph RustServer["godot-mcp-server（独立二进制）✅"]
        CLI["CLI 解析 (clap) ✅"]
        StdioSrv["stdio Transport ✅<br/>(rmcp transport-io)"]
        HttpSrv["Streamable HTTP 📋<br/>(rmcp + axum) 未实现"]
        Handler["GodotMcpHandler ✅<br/>(ServerHandler 手动实现)"]
        Bridge["GodotBridge ✅<br/>(WebSocket 客户端)"]

        CLI --> StdioSrv
        CLI -.-> HttpSrv
        StdioSrv --> Handler
        Handler --> Bridge
    end

    subgraph GodotProc["Godot Editor 进程"]
        GDExt["godot-mcp-gdext ✅<br/>(Rust cdylib)"]
        EditorPlugin["McpEditorPlugin ✅<br/>#[class(tool, base=EditorPlugin)]"]
        DockUI["Dock 面板 📋<br/>(add_control_to_dock) 未实现"]
        WSServer["WebSocket 服务端 ✅<br/>(tokio-tungstenite, 3 个方法)"]
        CmdRouter["命令路由 📋<br/>(CommandHandler trait) 未实现"]

        ToolImpl["工具实现模块 📋<br/>scene / asset / script / editor / project / debug<br/>全部未实现"]

        GDExt --> EditorPlugin
        EditorPlugin -.-> DockUI
        EditorPlugin --> WSServer
        WSServer -.-> CmdRouter
        CmdRouter -.-> ToolImpl
    end

    CC -->|"stdio ✅"| StdioSrv
    Other -.->|"HTTP 📋"| HttpSrv

    Bridge <-->|"ws://127.0.0.1:9500<br/>JSON-RPC 2.0 ✅"| WSServer
```

## 数据流

### stdio 模式（已实现 — 由 AI 客户端直接启动）

```
AI Client (e.g. OpenCode)
    |
    | MCP Protocol (stdio JSON-RPC 2.0)
    ↓
godot-mcp-server (子进程)
    |
    | WebSocket (JSON-RPC 2.0)
    ↓
GDExtension WebSocket Server (:9500)
    |
    | EditorInterface / ClassDB API (3 个方法)
    ↓
Godot Editor
```

### Streamable HTTP 模式（计划，未实现）

```
AI Client (e.g. Cursor / Codex)
    |
    | MCP Protocol (HTTP POST /mcp)
    ↓
godot-mcp-server (HTTP 守护进程, axum :8900)
    |
    | WebSocket (JSON-RPC 2.0)
    ↓
GDExtension WebSocket Server
    |
    | EditorInterface / ClassDB API
    ↓
Godot Editor
```

## 实现状态总览

| 组件 | 状态 | 详情 |
|------|------|------|
| Cargo Workspace (3 crates) | ✅ | core / gdext / server |
| core: 协议类型 | ✅ | IpcRequest, IpcResponse, IpcNotification, ToolManifest |
| gdext: EditorPlugin | ✅ | InitLevel::Editor, tokio worker_threads=2 |
| gdext: WS Server (:9500) | ✅ | 3 个方法: ping, get_engine_version, get_plugin_version |
| gdext: Dock UI | ❌ | 独立 Dock 面板未实现 |
| gdext: CommandHandler | ❌ | 命令路由 trait 未实现 |
| gdext: 48 工具 | ❌ | commands/ 目录为空 |
| server: CLI + stdio | ✅ | clap --godot-port, rmcp transport-io |
| server: 4 个 MCP 工具 | ✅ | ping, get_engine_version, get_plugin_version, get_server_version |
| server: GodotBridge | ✅ | WebSocket 客户端, oneshot 应答, 懒连接 + 错误恢复 |
| server: Streamable HTTP | ❌ | transports/ 目录为空 |
| server: 48 工具 | ❌ | tools/ 目录不存在 |
| CI/CD | ✅ | GitHub Actions, 标签触发, 3 平台构建 + 打包 |
| 构建脚本 | ✅ | package_addons.py 一键构建打包 |

## 核心设计原则

1. **单语言全栈**：Rust 贯穿引擎端（GDExtension）和服务端（MCP Server），无 Python/Node.js 运行时依赖
2. **共享类型**：`core` crate 定义 IPC 协议和工具清单，`gdext` 和 `server` 共享同一套类型系统
3. **解耦传输**：MCP Server 不直接操作 Godot API，所有操作通过 WebSocket 委托给 GDExtension
4. **热加载**：GDExtension 支持 `reloadable = true`，修改 Rust 代码后无需重启 Godot 编辑器

---

## 与相关页面的关系

| 页面 | 关系 |
|------|------|
| [Cargo Workspace 结构](../specification/workspace.md) | 代码组织方式 |
| [IPC 与 MCP 协议](../specification/protocol.md) | 通信协议细节 |
| [当前实现状态](../implementation/current-status.md) | 已完成与待实现的详细对照 |
| [Dock UI 面板](../design/dock-ui.md) | 编辑器插件 UI 设计 |
| [IPC 桥接细节](../design/ipc-bridge.md) | 桥接实现 |

## 现有方案对比

| 维度 | hi-godot/godot-ai | CoplayDev/unity-mcp | **本方案** |
|------|-------------------|---------------------|------------|
| 引擎端 | GDScript EditorPlugin | C# EditorPlugin | **Rust GDExtension** |
| 服务端 | Python FastMCP | Python uvx | **Rust rmcp** |
| 运行时依赖 | Python 3.10+, uv | Python 3.10+, uv | **无（静态编译）** |
| 传输协议 | Streamable HTTP | HTTP + stdio | **stdio ✅ + Streamable HTTP 📋** |
| 配置启动 | `uvx godot-ai` | `uvx mcp-for-unity` | **`godot-mcp-server`** |
| UI 面板 | 底部面板 + 配置页 | 完整管理窗口 | **独立 Dock 面板 📋** |
| 工具热切换 | 不支持 | 支持 (manage_tools) | **面板 CheckBox → IPC 实时同步 📋** |