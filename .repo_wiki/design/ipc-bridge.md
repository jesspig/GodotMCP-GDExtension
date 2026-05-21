# IPC 桥接细节

## 相关页面

- [架构概览](../overview/architecture.md) — 桥接在架构中的位置
- [IPC 与 MCP 协议](../specification/protocol.md) — 消息格式定义
- [工具清单与热切换](tools.md) — 工具调用的桥接路由
- [当前实现状态](../implementation/current-status.md) — 已完成与待实现的对照

---

## 当前实现（Phase 1 已完成）

### GDExtension 侧：WebSocket 服务端

**文件**：`crates/gdext/src/ipc/ws_server.rs` (150 行)

```rust
pub struct IpcWebSocketServer {
    port: u16,
    state: Arc<PluginState>,         // engine_version + plugin_version
    shutdown: Arc<Notify>,
}

impl IpcWebSocketServer {
    pub fn new(port: u16, state: Arc<PluginState>, shutdown: Arc<Notify>) -> Self;

    pub async fn run(&mut self) -> anyhow::Result<()> {
        // 绑定 127.0.0.1:{port}
        // tokio::select! between accept 和 shutdown.notified()
        // 每个连接：accept_async → 发送 godot_ready 通知 → handle_request 循环
    }

    async fn handle_connection(ws, state);
    fn handle_request(request: &IpcRequest, state: &PluginState) -> IpcResponse;
}
```

**当前支持的方法**（`handle_request` 硬编码匹配）：

| 方法 | 响应 |
|------|------|
| `ping` | `{ "message": "pong" }` |
| `get_engine_version` | `{ "engine_version": "4.x.y" }` |
| `get_plugin_version` | `{ "plugin_version": "0.1.0" }` |
| 其他 | Error code: -2, "Unknown method: ..." |

**连接时通知**（每个 WebSocket 客户端连接时发送一次）：

```json
{
  "type": "notification",
  "event": "godot_ready",
  "data": {
    "engine_version": "4.5.1",
    "plugin_version": "0.1.0",
    "protocol_version": "1.0"
  }
}
```

### MCP Server 侧：WebSocket 客户端

**文件**：`crates/server/src/bridge.rs` (72 行)

```rust
pub struct GodotBridge {
    writer: Arc<Mutex<SplitSink<...>>>,
    pending: Arc<DashMap<String, oneshot::Sender<Value>>>,
}

impl GodotBridge {
    pub async fn connect(port: u16) -> anyhow::Result<Self>;
    pub async fn call(&self, method: &str, params: Value) -> anyhow::Result<Value>;
}
```

**机制**：
1. `connect(port)` → 连接 `ws://127.0.0.1:{port}`
2. 派生读取任务：解析 `IpcResponse` → 通过 UUID 匹配 `oneshot` sender
3. `call(method, params)` → `IpcRequest(UUID + method + params)` → 存储 oneshot sender → 发送 JSON → 等待响应

**错误恢复**（`handler.rs` 中）：
- 桥接方法返回错误时，将内部 bridge 置为 `None`（`self.bridge.lock().await.take()`）
- 下次工具调用时重新懒连接

---

## 计划中的扩展（Phase 2+）

### 命令路由（待实现）

当前 3 个方法（ping/get_engine_version/get_plugin_version）通过 `match` 硬编码处理。未来 48 个工具将通过 `CommandHandler` trait 路由：

```rust
// 计划：crates/gdext/src/commands/mod.rs
pub trait CommandHandler: Send + Sync {
    fn can_handle(&self, method: &str) -> bool;
    fn execute(&self, params: &Value) -> Result<Value, String>;
    fn is_enabled(&self) -> bool { true }
    fn group_name(&self) -> &str { "unknown" }
}

pub fn create_registry() -> Vec<Box<dyn CommandHandler + Send + Sync>> {
    vec![
        Box::new(scene::SceneCommands::new()),
        Box::new(asset::AssetCommands::new()),
        Box::new(script::ScriptCommands::new()),
        Box::new(editor::EditorCommands::new()),
        Box::new(project::ProjectCommands::new()),
        Box::new(debug::DebugCommands::new()),
    ]
}

fn handle_request(request: &IpcRequest, handlers: &[Box<dyn CommandHandler>]) -> IpcResponse {
    for handler in handlers {
        if handler.can_handle(&request.method) {
            if !handler.is_enabled() {
                return IpcResponse {
                    id: request.id.clone(),
                    result: IpcResult::Error {
                        code: -3,
                        message: format!("Tool group '{}' is disabled", handler.group_name()),
                    },
                };
            }
            match handler.execute(&request.params) {
                Ok(data) => return IpcResponse {
                    id: request.id.clone(),
                    result: IpcResult::Success { data },
                },
                Err(msg) => return IpcResponse {
                    id: request.id.clone(),
                    result: IpcResult::Error { code: -1, message: msg },
                },
            }
        }
    }
    IpcResponse {
        id: request.id.clone(),
        result: IpcResult::Error { code: -2, message: format!("Unknown method: {}", request.method) },
    }
}
```

### 连接管理（待实现）

多客户端连接管理：连接 ID 追踪、广播通知（`tool_list_updated`）、连接状态上报。

### 心跳与重连（待实现）

| 机制 | 间隔 | 说明 |
|------|------|------|
| ping/pong | 10s | 保持连接活性，检测断开 |
| 自动重连 | 1s, 5s, 30s (指数退避) | Server 侧检测到断开后自动重连 |
| 连接超时 | 30s | 首次连接超时阈值 |
| 工具超时 | 30s | 每个工具调用的最大等待时间 |

当前无心跳机制——连接断开由 WebSocket 底层检测，设计中的 10s ping/pong 尚未实现。