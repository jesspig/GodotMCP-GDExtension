# 当前实现状态

> 本文档是代码库的真实反映。所有引用均为实际存在的文件路径。

## Phase 2a 完成度

| 步骤 | 描述 | 状态 |
|------|------|------|
| 2a.1 | 协议升级：引入 `ToolCallParams` | ✅ 完成 |
| 2a.2 | 主线程调度器 | ✅ 完成 |
| 2a.3 | `CommandHandler` trait | ✅ 完成 |
| 2a.4 | `MetaCommands` 迁移 | ✅ 完成 |
| 2a.5 | Server `ToolRegistry` + 动态 `list_tools` | ✅ 完成 |
| 2a.6 | `tool_list_updated` 单向推送 | ✅ 完成 |
| 2a.7 | Dock UI 主容器 + 状态栏 | ✅ 完成 |
| 2a.8 | Dock UI 工具管理面板（交互完整） | ✅ 完成 |
| 2a.9 | Dock UI 客户端集成面板（骨架） | ✅ 完成 |
| 2a.10 | Dock UI 高级设置面板（骨架） | ✅ 完成 |
| 2a.11 | 文档同步 | ✅ 完成 |

## 已实现组件

### crates/core (`crates/core/src/`)

| 文件 | 内容 |
|------|------|
| `protocol.rs` | `IpcRequest`, `IpcResponse`, `IpcResult`, `IpcNotification`, `ToolCallParams` |
| `tool_manifest.rs` | `ToolManifest`, `ToolCategory`, `ToolInfo`, `ToolListUpdate`, `ToolState` |

### crates/gdext (`crates/gdext/src/`)

| 文件 | 内容 |
|------|------|
| `lib.rs` | `#[gdextension]` 入口，声明 `commands`, `dispatcher`, `dock`, `editor_plugin`, `ipc` |
| `editor_plugin.rs` | `McpEditorPlugin`：tokio runtime + WS server + dispatcher + dock 管理 |
| `dispatcher.rs` | `MainThreadDispatcher`：tokio worker → Godot 主线程调度 |
| `commands/mod.rs` | `CommandHandler` trait + `create_registry()` |
| `commands/meta.rs` | `MetaCommands`：ping, get_engine_version, get_plugin_version |
| `ipc/ws_server.rs` | `IpcWebSocketServer`：TCP :9500，CommandHandler 路由，broadcast 通道 |
| `ipc/plugin_state.rs` | `PluginState { engine_version, plugin_version }` |
| `dock/mod.rs` | 模块导出 |
| `dock/main_dock.rs` | 主容器创建函数 |
| `dock/status_bar.rs` | 状态栏：指示灯 + 标签 + 按钮 |
| `dock/tool_manager.rs` | 工具管理面板：CheckBox 列表 + toggle 广播 |
| `dock/integration.rs` | 客户端集成面板骨架（12 客户端列表，按钮禁用） |
| `dock/settings.rs` | 高级设置面板骨架（端口输入框 + 操作按钮禁用） |

### crates/server (`crates/server/src/`)

| 文件 | 内容 |
|------|------|
| `main.rs` | CLI (clap `--godot-port`)，stdio transport |
| `handler.rs` | `GodotMcpHandler`：`ServerHandler` impl，动态 `list_tools` 从 ToolRegistry 读取 |
| `bridge.rs` | `GodotBridge`：WebSocket 客户端，oneshot 应答，通知监听（`tool_list_updated`） |
| `tool_registry.rs` | `ToolRegistry`：工具启用/禁用状态管理，`update_from_notification` |

## 测试覆盖

| Crate | 测试数 | 覆盖内容 |
|-------|--------|----------|
| `godot-mcp-core` | 15 | 协议序列化往返 + ToolCallParams + ToolManifest |
| `godot-mcp-gdext` | 12 | MetaCommands + MainThreadDispatcher |
| `godot-mcp-server` | 20 | ToolRegistry + GodotMcpHandler（含 disabled/unknown 区分） |
| **总计** | **47** | |

## 已知缺失（Phase 2b+ 待实现）

| 缺失项 | 影响 |
|--------|------|
| 30+ 核心工具（Scene/Script/Editor/Project） | AI 客户端只能调用 4 个元工具 |
| Debug Tools 6 个 | 运行时调试不可用 |
| 客户端集成实际配置写入 | UI 存在但按钮禁用 |
| 高级设置保存 | UI 存在但按钮禁用 |
| Streamable HTTP 传输 | 仅支持 stdio 客户端 |
| 心跳机制 | 连接断开依赖 WebSocket 底层检测 |

## 构建与验证

```bash
cargo check --workspace               # 类型检查
cargo test --workspace                 # 运行 47 个测试
cargo clippy --workspace -- -D warnings  # 零警告
cargo build -p godot-mcp-gdext        # 构建 GDExtension (debug)
cargo build -p godot-mcp-server       # 构建 Server (debug)
python package_addons.py              # 一键构建 + 打包 addons.zip
```
