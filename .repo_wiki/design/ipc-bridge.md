# IPC 桥接细节

## 相关页面

- [架构概览](../overview/architecture.md) — 桥接在架构中的位置
- [IPC 与 MCP 协议](../specification/protocol.md) — 消息格式定义
- [工具清单与热切换](tools.md) — 工具调用的桥接路由

---

## GDExtension 侧：WebSocket 服务端

```rust
// crates/gdext/src/ipc/ws_server.rs
use tokio::net::TcpListener;
use tokio_tungstenite::accept_async;
use futures_util::{SinkExt, StreamExt};
use std::sync::Arc;
use tokio::sync::Notify;
use parking_lot::RwLock;

use godot_mcp_core::protocol::*;
use crate::commands::CommandHandler;

pub struct IpcWebSocketServer {
    port: u16,
    handlers: Arc<Vec<Box<dyn CommandHandler + Send + Sync>>>,
    shutdown_signal: Arc<Notify>,
    active_connections: Arc<RwLock<Vec<ConnectionHandle>>>,
}

struct ConnectionHandle {
    id: String,
    sender: futures_util::stream::SplitSink<WebSocketStream<tokio::net::TcpStream>, Message>,
}

impl IpcWebSocketServer {
    pub fn new(port: u16) -> Self {
        Self {
            port,
            handlers: Arc::new(commands::create_registry()),
            shutdown_signal: Arc::new(Notify::new()),
            active_connections: Arc::new(RwLock::new(Vec::new())),
        }
    }

    pub async fn run(self) {
        let listener = TcpListener::bind(("127.0.0.1", self.port)).await.unwrap();
        
        loop {
            tokio::select! {
                result = listener.accept() => {
                    match result {
                        Ok((stream, addr)) => {
                            match accept_async(stream).await {
                                Ok(ws) => {
                                    let handlers = self.handlers.clone();
                                    let conns = self.active_connections.clone();
                                    let (tx, mut rx) = ws.split();
                                    
                                    let conn_id = uuid::Uuid::new_v4().to_string();
                                    conns.write().push(ConnectionHandle {
                                        id: conn_id.clone(),
                                        sender: tx,
                                    });
                                    
                                    let ready = IpcNotification {
                                        msg_type: "notification".into(),
                                        event: "godot_ready".into(),
                                        data: serde_json::json!({
                                            "project_path": "res://",
                                            "godot_version": "4.3+",
                                            "protocol_version": "1.0",
                                        }),
                                    };
                                    
                                    tokio::spawn(async move {
                                        while let Some(Ok(msg)) = rx.next().await {
                                            if let Message::Text(text) = msg {
                                                if let Ok(request) = serde_json::from_str::<IpcRequest>(&text) {
                                                    let response = handle_request(&request, &handlers);
                                                    // send response...
                                                }
                                            }
                                        }
                                        conns.write().retain(|c| c.id != conn_id);
                                    });
                                }
                                Err(e) => { /* log */ }
                            }
                        }
                        Err(e) => { /* log */ }
                    }
                }
                _ = self.shutdown_signal.notified() => break,
            }
        }
    }
}
```

## MCP Server 侧：WebSocket 客户端

```rust
// crates/server/src/bridge.rs
use tokio_tungstenite::connect_async;
use futures_util::{SinkExt, StreamExt};
use dashmap::DashMap;
use tokio::sync::oneshot;

pub struct GodotBridge {
    writer: Arc<Mutex<SplitSink<WebSocketStream, Message>>>,
    pending: Arc<DashMap<String, oneshot::Sender<serde_json::Value>>>,
}

impl GodotBridge {
    pub async fn connect(port: u16) -> anyhow::Result<Self> {
        let url = format!("ws://127.0.0.1:{}", port);
        let (ws, _) = connect_async(&url).await?;
        let (write, read) = ws.split();
        
        let writer = Arc::new(Mutex::new(write));
        let pending: Arc<DashMap<String, oneshot::Sender<serde_json::Value>>> = Arc::new(DashMap::new());
        
        let p = pending.clone();
        tokio::spawn(async move {
            let mut read = read;
            while let Some(Ok(msg)) = read.next().await {
                if let Message::Text(text) = msg {
                    if let Ok(response) = serde_json::from_str::<IpcResponse>(&text) {
                        if let Some((_, tx)) = p.remove(&response.id) {
                            match response.result {
                                IpcResult::Success { data } => { let _ = tx.send(data); }
                                IpcResult::Error { message, .. } => {
                                    let _ = tx.send(serde_json::json!({"error": message}));
                                }
                            }
                        }
                    }
                }
            }
        });
        
        Ok(Self { writer, pending })
    }

    pub async fn call(&self, method: &str, params: Value) -> anyhow::Result<Value> {
        let id = uuid::Uuid::new_v4().to_string();
        let request = IpcRequest {
            id: id.clone(),
            method: method.into(),
            params,
        };
        
        let (tx, rx) = oneshot::channel();
        self.pending.insert(id.clone(), tx);
        
        let msg = serde_json::to_string(&request)?;
        self.writer.lock().await.send(Message::Text(msg.into())).await?;
        
        let result = rx.await?;
        Ok(result)
    }
}
```

## 命令路由

```rust
// crates/gdext/src/handler.rs
pub trait CommandHandler: Send + Sync {
    fn can_handle(&self, method: &str) -> bool;
    fn execute(&self, params: &Value) -> Result<Value, String>;
}

// crates/gdext/src/commands/mod.rs
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
        result: IpcResult::Error {
            code: -2,
            message: format!("Unknown tool: {}", request.params.get("tool").unwrap_or(&Value::Null)),
        },
    }
}
```

## 心跳与重连

| 机制 | 间隔 | 说明 |
|------|------|------|
| ping/pong | 10s | 保持连接活性，检测断开 |
| 自动重连 | 1s, 5s, 30s (指数退避) | Server 侧检测到断开后自动重连 |
| 连接超时 | 30s | 首次连接超时阈值 |
| 工具超时 | 30s | 每个工具调用的最大等待时间 |

## 连接状态管理

```rust
// 在 ws_server.rs 中
struct ConnectionManager {
    connections: Vec<ConnectionInfo>,
}

struct ConnectionInfo {
    id: String,
    address: SocketAddr,
    connected_at: SystemTime,
    last_seen: SystemTime,
}

impl ConnectionManager {
    fn get_connections(&self) -> Vec<ConnectionInfo> { ... }
    fn remove_connection(&mut self, id: &str) { ... }
    fn broadcast(&self, notification: &IpcNotification) { ... }
}
```