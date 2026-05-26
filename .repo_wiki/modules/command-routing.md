# 命令路由

## 完整的调用链路

```
AI 客户端 call_tool("get_node_position", {"node_path": "Player"})
  │
  ▼
godot-mcp-server (Python) / handler.py
  ├─ match "get_server_version" → 直接返回（不转发）
  ├─ match "godot_editor_*" → 服务器端处理（不转发到 gdext）
  ├─ else → _forward_tool_call(name, args)
  │
  ▼ (WebSocket :9500)
godot_mcp_gdext / ws_server.rs → route_tool_call() → dispatch()
  │
  ├─ dispatch(): MetaCommands.can_handle("get_node_position") → 否
  ├─ dispatch(): 迭代 registry[0..16]
  │   ├─ NodeCommands.can_handle("get_node_position") → 否
  │   ├─ PropertyCommands.can_handle("get_node_position") → 是！
  │   │   │
  │   │   ▼
  │   │   handler.handle() → dispatcher.submit(move || cmd_*()).await
  │   │   │
  │   │   ▼
  │   │   (主线程 process_frame 泵)
  │   │   cmd_get_node_position() → EditorInterface → Node API
  │   │   │
  │   │   ▼ (返回值)
  │   │   WebSocket IPC 响应 (IpcResponse { status, data/error })
  │   │
  │   ▼
  godot-mcp-server → JSON-RPC 响应（TextContent）
  │
  ▼
AI 客户端 {"result": {"x": 100, "y": 200}}
```

## 路由实现（`ws_server.rs` `dispatch()`）

```rust
// 1. MetaCommands 单独处理（同步，无需 MainThreadDispatcher）
let meta = MetaCommands::new().with_engine_version(state.engine_version.clone());
if meta.can_handle(tool) {
    return meta.handle_meta_tool(tool);
}

// 2. 迭代所有已注册的 CommandHandler
for handler in registry {
    if handler.can_handle(tool) {
        return handler.handle(tool, args, dispatcher).await;
    }
}

// 3. 未知工具
Err(format!("Unknown tool: {}", tool))
```

**17 组 handler，全部在 `create_registry()` 中注册**：

| 索引 | 组名 | 工具数 | 同步/异步 |
|------|------|--------|----------|
| - | MetaCommands（dispatch 外独立） | 3 | 同步，无需 dispatcher |
| 0 | NodeCommands | 17 | 异步，通过 dispatcher |
| 1 | PropertyCommands | 19 | 异步 |
| 2 | CollisionCommands | 2 | 异步 |
| 3 | FindCommands | 4 | 异步 |
| 4 | ScriptHelpersCommands | 3 | 异步 |
| 5 | ProjectSettingsCommands | 7 | 异步 |
| 6 | SceneCommands | 15 | 异步 |
| 7 | ScriptGdCommands | 5 | 异步 |
| 8 | ScriptCsCommands | 6 | 异步 |
| 9 | SearchCommands | 3 | 异步 |
| 10 | UndoCommands | 2 | 异步 |
| 11 | Property3dCommands | 6 | 异步 |
| 12 | EditorControlCommands | 6 | 异步 |
| 13 | ProjectSettingsExtCommands | 10 | 异步 |
| 14 | PluginManagementCommands | 2 | 异步 |
| 15 | InputMapCommands | 4 | 异步 |

**注意**：`ws_server.rs` 的 `handle_request()` 首先检查 `method == "tool_call"`，提取 `ToolCallParams`，然后调用 `route_tool_call(&params.tool, &params.args, ...)`。如果 `method` 不是 `tool_call`，直接将 `method` 作为工具名、`params` 作为参数调用。

## 服务器端 vs gdext 工具

| 处理位置 | 工具 |
|---------|------|
| handler.py（Python 服务器端） | `get_server_version`, `godot_editor_open`, `godot_editor_close`, `godot_editor_restart` |
| gdext dispatch()（121 个） | 其余所有工具 |

## 新增工具流程

### 服务器侧（Python `server/`）

| 文件 | 修改 |
|------|------|
| `src/godot_mcp_server/registry.py` | 在 `_TOOLS` 列表中添加工具名、描述、JSON Schema |
| `src/godot_mcp_server/handler.py` | 如果是服务器端工具（`godot_editor_*`），添加处理分支 |

### gdext 侧（Rust `crates/gdext/`）

| 文件 | 修改 |
|------|------|
| `src/commands/xx.rs` | 实现 `cmd_your_tool()` 函数 |
| `src/commands/mod.rs` | 将 `YourToolHandler` 加入 `create_registry()` 的返回列表 |

**`ws_server.rs` 不需要修改**——`dispatch()` 自动遍历所有已注册的 handler。