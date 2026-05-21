# 当前实现状态

> 本文档是代码库的真实反映。所有引用均为实际存在的文件路径。

## Phase 1 完成度

| 步骤 | 描述 | 状态 |
|------|------|------|
| 1 | Cargo workspace + `crates/core` 协议类型 | ✅ 完成 |
| 2 | `rust-toolchain.toml` | ❌ **缺失** |
| 3 | `crates/gdext` — EditorPlugin + WS server | ✅ 完成 |
| 4 | `crates/server` — CLI + stdio + 4 工具 | ✅ 完成 |
| 5 | WebSocket IPC 桥接 | ✅ 完成 |
| 6 | End-to-end ping/pong | ✅ 完成 |

## 已实现组件

### crates/core (`crates/core/src/`)

| 文件 | 行数 | 内容 |
|------|------|------|
| `lib.rs` | 5 | `pub mod protocol; pub mod tool_manifest;` |
| `protocol.rs` | 33 | `IpcRequest`, `IpcResponse`, `IpcResult`(带 `#[serde(tag = "status")]`), `IpcNotification` |
| `tool_manifest.rs` | 38 | `ToolManifest`, `ToolCategory`, `ToolInfo`, `ToolListUpdate`, `ToolState` |

依赖：`serde`, `serde_json`, `uuid`(v4 + serde)

### crates/gdext (`crates/gdext/src/`)

| 文件 | 行数 | 内容 |
|------|------|------|
| `lib.rs` | 14 | `#[gdextension]` 入口，`InitLevel::Editor` |
| `editor_plugin.rs` | 92 | `McpEditorPlugin`：enter_tree 构建 tokio runtime(2 worker)，启动 WS server；exit_tree 发 shutdown 信号 |
| `ipc/mod.rs` | 2 | 声明 `ws_server` 和 `plugin_state` 子模块 |
| `ipc/plugin_state.rs` | 5 | ` PluginState { engine_version, plugin_version }` |
| `ipc/ws_server.rs` | 150 | `IpcWebSocketServer`：TCP :9500，accept 循环，连接时发 `godot_ready` 通知，3 个 RPC 方法 |

**未声明的空目录**：`commands/`（无 `mod.rs`，不在 `lib.rs` 模块树中）

### crates/server (`crates/server/src/`)

| 文件 | 行数 | 内容 |
|------|------|------|
| `main.rs` | 27 | CLI (clap `--godot-port`)，stdio transport (rmcp `serve`) |
| `handler.rs` | 145 | `GodotMcpHandler`：`ServerHandler` impl，4 个工具，懒连接 bridge |
| `bridge.rs` | 72 | `GodotBridge`：WebSocket 客户端，oneshot 应答，`Arc<Mutex<Option<...>>>` 懒连接 |

**未声明的空目录**：`transports/`（无 `mod.rs`）

## 工具实现现状

当前仅 4 个 MCP 工具，全部在 `handler.rs` 中：

| 工具 | 类型 | 实现方式 |
|------|------|---------|
| `ping` | IPC | 通过 bridge 调用 GDExtension |
| `get_engine_version` | IPC | 通过 bridge 调用 GDExtension |
| `get_plugin_version` | IPC | 通过 bridge 调用 GDExtension |
| `get_server_version` | 本地 | 直接返回 `CARGO_PKG_VERSION` |

GDExtension 侧对应 3 个方法在 `ws_server.rs` 的 `handle_request` 中以硬编码 `match` 实现。

计划中的 48 个工具（6 分类）**全部未实现**。详见 [工具清单与热切换](../design/tools.md)。

## 已知缺失

| 缺失项 | 影响 |
|--------|------|
| `rust-toolchain.toml` | 本地工具链版本不确定 |
| 测试 | 无法自动化验证 |
| `Cargo.lock` 被 gitignore | 二进制 crate 应提交 lockfile |
| Dock UI 面板 | 用户无法可视化控制插件 |
| Streamable HTTP 传输 | 仅支持 stdio 客户端 |
| 心跳机制 | 连接断开依赖 WebSocket 底层检测 |
| 工具热切换 | 所有工具硬编码，无法运行时调整 |

## 构建与验证

```bash
cargo check --workspace               # 类型检查
cargo build -p godot-mcp-gdext        # 构建 GDExtension (debug)
cargo build -p godot-mcp-server       # 构建 Server (debug)
python package_addons.py              # 一键构建 + 打包 addons.zip
```

目前无测试套件。`cargo test` 会通过（无测试函数 = 零失败），但没有任何有意义的用例。