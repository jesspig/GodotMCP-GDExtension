# 命令路由

## 完整的调用链路

```
AI 客户端 call_tool("get_node_position", {"node_path": "Player"})
  │
  ▼
godot-mcp-server / handler.rs
  ├─ match "godot_editor_*" → 服务器端处理（不转发到 gdext）
  ├─ else → forward_tool_call(name, args)
  │
  ▼ (WebSocket :9500)
godot_mcp_gdext / ws_server.rs → route_tool_call()
  │
  ├─ can_handle("get_node_position") → PropertyCommands → 否
  ├─ can_handle("get_node_position") → NodeCommands → 否
  ├─ can_handle("get_node_position") → SceneCommands → 否
  ├─ ... (17 个组依次检查)
  ├─ can_handle("get_node_position") → NodeConvenience → 是！
  │   │
  │   ▼
  │   dispatcher.submit(move || cmd_get_node_position(args)).await
  │   │
  │   ▼
  │   (主线程 process_frame 泵)
  │   cmd_get_node_position() → EditorInterface → Node API
  │   │
  │   ▼ (返回值)
  │   WebSocket IPC 响应
  │
  ▼
godot-mcp-server → JSON-RPC 响应
  │
  ▼
AI 客户端 {"result": {"x": 100, "y": 200}}
```

## 路由链（`ws_server.rs`）

```rust
// 检查顺序（17 组）
MetaCommands::can_handle(name)
    || NodeCommands::can_handle(name)
    || PropertyCommands::can_handle(name)
    || CollisionCommands::can_handle(name)
    || FindCommands::can_handle(name)
    || ScriptHelpersCommands::can_handle(name)
    || ProjectSettingsCommands::can_handle(name)
    || SceneCommands::can_handle(name)
    || ScriptGdCommands::can_handle(name)
    || ScriptCsCommands::can_handle(name)
    || SearchCommands::can_handle(name)
    || UndoCommands::can_handle(name)
    || Property3dCommands::can_handle(name)
    || NodeConvenience::can_handle(name)
    || SceneInfo::can_handle(name)
```

**注意**: `NodeConvenience`（4 个工具）和 `SceneInfo`（1 个工具）在 `route_tool_call` 中是额外路由分支，它们不是**独立**的 `CommandHandler`。

## 新增工具清单

添加新工具需要修改以下位置：

### 服务器侧（`crates/server/`）

| 文件 | 修改 |
|------|------|
| `src/tool_registry.rs` | 在 `register_defaults()` 中添加工具的 JSON Schema |
| `src/handler.rs` | 如果是服务器端工具（`godot_editor_*`），添加处理分支 |
| 测试 | `tool_registry.rs` 的 `total == 125` 断言需要更新 |
| 测试 | `handler.rs` 的 `total == 125` 断言也需要更新 |

### gdext 侧（`crates/gdext/`）

| 文件 | 修改 |
|------|------|
| `src/commands/xx.rs` | 实现 `cmd_your_tool()` 函数 + `YourToolHandler::can_handle()` |
| `src/commands/mod.rs` | 将 `YourToolHandler` 加入 `create_registry()` 的返回列表（若为新的组） |
| `src/ipc/ws_server.rs` | 在 `route_tool_call()` 链中添加 `YourToolHandler::can_handle()` 分支 |

### 现有组 vs 新组

- **已有组**：如果新工具属于已有命令组（如 `MetaCommands`），只需在组内添加 `can_handle` 和 `handle` 方法，并在 `route_tool_call` 中注册（如果该组的 handler 尚未注册路由）。
- **新组**：如果创建新命令组，需要在 `commands/mod.rs` 的 `create_registry()` **和** `ws_server.rs` 的 `route_tool_call()` 中注册。

## 测试

- `handler.rs` 断言 `total == 125`（服务器侧工具数）
- `tool_registry.rs` 断言 `total == 125`（注册表工具数）
- 离线测试（无 Godot）：不能测试真实工具调用，但可以测试 schema 注册和工具列表查询

## 注意

- 新增工具后**两者计数必须更新**，否则测试会失败
- `create_registry()` 和 `route_tool_call()` 的分组**必须保持同步**，否则 gdext 端会拒绝路由而返回错误